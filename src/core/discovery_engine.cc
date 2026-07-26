/* discovery_engine.cc — nmap + smbclient based SMB discovery */

#include "discovery_engine.h"

#include <glibmm/main.h>
#include <giomm/subprocess.h>

#include <sstream>
#include <regex>
#include <netdb.h>
#include <arpa/inet.h>
#include <cstring>
#include <fstream>

namespace Mounter {

DiscoveryEngine::DiscoveryEngine() = default;

DiscoveryEngine::~DiscoveryEngine()
{
  cancel();
  if (worker_.joinable()) {
    worker_.join();
  }
}

void DiscoveryEngine::cancel()
{
  cancelled_.store(true, std::memory_order_release);
}

// ── Public API ──────────────────────────────────────────────

void DiscoveryEngine::scan_async(const std::string& subnet,
                                 DiscoveryProgressCallback progress_cb,
                                 DiscoveryResultCallback result_cb)
{
  cancelled_.store(false, std::memory_order_release);

  if (worker_.joinable()) {
    worker_.join();
  }

  worker_ = std::thread([this, subnet, progress_cb, result_cb]() {
    // Phase 1: scan for SMB hosts via nmap
    if (progress_cb && !cancelled_.load(std::memory_order_acquire)) {
      Glib::signal_idle().connect_once([progress_cb]() {
        progress_cb("Scanning " + DiscoveryEngine::detect_subnet() + " for SMB hosts...");
      });
    }

    auto hosts_ips = scan_hosts(subnet);

    if (cancelled_.load(std::memory_order_acquire)) return;

    // Phase 2: enumerate shares on each host
    std::vector<DiscoveredHost> hosts;
    for (size_t i = 0; i < hosts_ips.size(); ++i) {
      if (cancelled_.load(std::memory_order_acquire)) break;

      const auto& ip = hosts_ips[i];

      if (progress_cb) {
        auto msg = "Enumerating shares on " + ip + " (" +
                   std::to_string(i + 1) + "/" +
                   std::to_string(hosts_ips.size()) + ")...";
        Glib::signal_idle().connect_once([progress_cb, msg]() {
          progress_cb(msg);
        });
      }

      DiscoveredHost host;
      host.ip_address = ip;
      host.hostname   = resolve_hostname(ip);
      host.reachable  = true;
      host.shares     = list_shares(ip);
      hosts.push_back(std::move(host));
    }

    // Phase 3: deliver results on main thread
    if (result_cb && !cancelled_.load(std::memory_order_acquire)) {
      Glib::signal_idle().connect_once([result_cb, hosts]() {
        result_cb(hosts);
      });
    }
  });
}

// ── Subnet detection ────────────────────────────────────────

std::string DiscoveryEngine::detect_subnet()
{
  // Read the default route from /proc/net/route to find the primary interface subnet
  std::ifstream route_file("/proc/net/route");
  if (!route_file.is_open()) return "192.168.1.0/24";

  std::string line;
  std::string iface;
  // Skip header
  std::getline(route_file, line);

  while (std::getline(route_file, line)) {
    std::istringstream iss(line);
    std::string dest, gateway;
    iss >> iface >> dest >> gateway;

    // Default route has destination 00000000
    if (dest == "00000000" && gateway != "00000000") {
      // Found default route — now find the interface's IP and netmask
      // Use a simpler approach: parse `ip -4 addr show <iface>`
      break;
    }
  }

  // Use `ip route` to find the default subnet
  try {
    auto proc = Gio::Subprocess::create(
      std::vector<std::string>{"sh", "-c",
        "ip -4 route show default 2>/dev/null | head -1 | awk '{print $3}'"},
      Gio::Subprocess::Flags::STDOUT_PIPE | Gio::Subprocess::Flags::STDERR_SILENCE);

    auto [stdout_bytes, stderr_bytes] = proc->communicate(nullptr, nullptr);

    if (stdout_bytes) {
      gsize size;
      const auto* data = static_cast<const char*>(stdout_bytes->get_data(size));
      std::string result(data, size);
      // Trim whitespace
      while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
        result.pop_back();

      if (!result.empty()) {
        // Append /24 as a reasonable default
        return result + "/24";
      }
    }
  } catch (...) {}

  return "192.168.1.0/24";
}

// ── Host scanning (nmap) ────────────────────────────────────

std::vector<std::string> DiscoveryEngine::scan_hosts(const std::string& subnet)
{
  std::vector<std::string> hosts;

  try {
    // nmap -p 445 --open -oX - <subnet>
    // -p 445: only scan the SMB port
    // --open: only show hosts with port 445 open
    // -oX -: output XML to stdout
    auto proc = Gio::Subprocess::create(
      std::vector<std::string>{"nmap", "-p", "445", "--open", "-oX", "-", subnet},
      Gio::Subprocess::Flags::STDOUT_PIPE | Gio::Subprocess::Flags::STDERR_SILENCE);

    auto [stdout_bytes, stderr_bytes] = proc->communicate(nullptr, nullptr);

    if (!stdout_bytes) return hosts;

    gsize size;
    const auto* data = static_cast<const char*>(stdout_bytes->get_data(size));
    std::string xml_output(data, size);

    // Simple XML parsing: extract IP addresses from <address addr="..." addrtype="ipv4"/>
    std::regex addr_regex(R"(<address\s+addr=\"([^\"]+)\"\s+addrtype=\"ipv4\")");
    auto begin = std::sregex_iterator(xml_output.begin(), xml_output.end(), addr_regex);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
      std::string ip = (*it)[1].str();
      hosts.push_back(ip);
    }
  } catch (...) {}

  return hosts;
}

// ── Share enumeration (smbclient) ───────────────────────────

std::vector<DiscoveredShare> DiscoveryEngine::list_shares(const std::string& host)
{
  std::vector<DiscoveredShare> shares;

  try {
    // First try guest access: smbclient -L //host -N -g
    auto proc = Gio::Subprocess::create(
      std::vector<std::string>{"smbclient", "-L", "//" + host, "-N", "-g"},
      Gio::Subprocess::Flags::STDOUT_PIPE | Gio::Subprocess::Flags::STDERR_SILENCE);

    auto [stdout_bytes, stderr_bytes] = proc->communicate(nullptr, nullptr);

    if (stdout_bytes) {
      gsize size;
      const auto* data = static_cast<const char*>(stdout_bytes->get_data(size));
      std::string output(data, size);

      // Parse grepable smbclient output:
      // Lines like: Disk|share_name|Comment
      // Skip lines starting with "Anonymous login successful" etc.
      std::istringstream iss(output);
      std::string line;
      while (std::getline(iss, line)) {
        // smbclient -g format: type|name|comment
        auto first_pipe = line.find('|');
        if (first_pipe == std::string::npos) continue;

        auto type = line.substr(0, first_pipe);
        if (type != "Disk") continue; // Only care about file shares

        auto rest = line.substr(first_pipe + 1);
        auto second_pipe = rest.find('|');

        std::string share_name = (second_pipe != std::string::npos)
          ? rest.substr(0, second_pipe)
          : rest;

        // Skip system shares
        if (share_name.empty() || share_name == "IPC$" ||
            share_name.find('$') == share_name.size() - 1) continue;

        std::string comment = (second_pipe != std::string::npos)
          ? rest.substr(second_pipe + 1)
          : "";

        DiscoveredShare share;
        share.name      = share_name;
        share.comment   = comment;
        share.guest_ok  = true;
        share.auth_required = false;
        shares.push_back(std::move(share));
      }
    }

    // If no shares found via guest, the share might require auth
    // We mark them as auth_required so the UI can prompt
  } catch (...) {}

  return shares;
}

// ── Hostname resolution ─────────────────────────────────────

std::string DiscoveryEngine::resolve_hostname(const std::string& ip)
{
  struct sockaddr_in sa;
  std::memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  inet_pton(AF_INET, ip.c_str(), &sa.sin_addr);

  char hostname[NI_MAXHOST];
  int ret = getnameinfo(
    reinterpret_cast<struct sockaddr*>(&sa), sizeof(sa),
    hostname, sizeof(hostname),
    nullptr, 0,
    NI_NAMEREQD);

  if (ret == 0) {
    std::string name{hostname};
    // Strip .local suffix for display
    auto dot = name.find('.');
    if (dot != std::string::npos) {
      return name.substr(0, dot);
    }
    return name;
  }

  return ""; // no reverse DNS
}

} // namespace Mounter
