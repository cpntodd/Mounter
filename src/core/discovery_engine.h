/* discovery_engine.h — Network SMB discovery
 *
 * Uses nmap for port scanning and smbclient for share enumeration.
 * All operations run asynchronously and deliver results on the main thread.
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <memory>

namespace Mounter {

struct DiscoveredHost {
  std::string ip_address;
  std::string hostname;
  bool        reachable = false;
  bool        is_smb_server = false;  // confirmed SMB protocol response
  std::vector<struct DiscoveredShare> shares;
};

struct DiscoveredShare {
  std::string name;
  std::string comment;
  bool        guest_ok = false;
  bool        auth_required = false;
};

using DiscoveryProgressCallback = std::function<void(const std::string& status)>;
using DiscoveryResultCallback  = std::function<void(const std::vector<DiscoveredHost>& hosts)>;

class DiscoveryEngine
{
public:
  DiscoveryEngine();
  ~DiscoveryEngine();

  DiscoveryEngine(const DiscoveryEngine&) = delete;
  DiscoveryEngine& operator=(const DiscoveryEngine&) = delete;

  /// Scan a subnet for SMB hosts and enumerate their shares.
  /// @param subnet           CIDR notation, e.g. "192.168.1.0/24"
  /// @param progress_cb      Called with status messages during scan
  /// @param result_cb        Called on main thread with discovered hosts
  void scan_async(const std::string& subnet,
                  DiscoveryProgressCallback progress_cb,
                  DiscoveryResultCallback result_cb);

  /// Cancel an in-progress scan.
  void cancel();

  /// Detect the likely local subnet (best guess from routing table).
  static std::string detect_subnet();

private:
  /// Run nmap to find hosts with port 445 open.
  std::vector<std::string> scan_hosts(const std::string& subnet);

  /// Run smbclient to list shares on a host. Sets out_is_smb to true if
  /// the host responded to the SMB protocol handshake.
  std::vector<DiscoveredShare> list_shares(const std::string& host, bool& out_is_smb);

  /// Resolve hostname from IP (reverse DNS lookup).
  static std::string resolve_hostname(const std::string& ip);

  std::thread        worker_;
  std::atomic<bool>  cancelled_{false};
};

} // namespace Mounter
