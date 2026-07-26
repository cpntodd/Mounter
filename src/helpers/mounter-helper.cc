/* mounter-helper.cc — Privileged operations helper
 *
 * This binary is invoked via pkexec to perform operations that require
 * root privileges:
 *   - Creating mount points (mkdir -p)
 *   - Mounting SMB shares (mount -t cifs)
 *   - Unmounting shares (umount)
 *   - Writing credential files
 *   - Managing systemd mount units
 *
 * Communication: reads a JSON command from stdin, writes a JSON
 * response to stdout. Exit code 0 on success, non-zero on failure.
 *
 * Usage (via pkexec):
 *   pkexec /usr/libexec/mounter/mounter-helper mount
 *   → reads JSON with mount params from stdin
 *   → performs mount
 *   → writes JSON result to stdout
 *
 * Commands:
 *   mount     - Mount a CIFS share
 *   umount    - Unmount a CIFS share
 *   write-cred - Write a credentials file
 *   write-unit - Write a systemd unit file
 *   enable-unit - systemctl enable
 *   disable-unit - systemctl disable
 *   daemon-reload - systemctl daemon-reload
 */

#include <iostream>
#include "i18n.h"
#include <string>
#include <string_view>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fstream>
#include <sstream>

namespace {

// ── Simple JSON helpers (no library dependency in the helper) ──
// The GUI sends structured JSON, but for robustness we parse
// simple key-value pairs from a minimal JSON object.

std::string json_get_string(const std::string& json, const std::string& key)
{
  // Minimal JSON parser: finds "key":"value" and returns value
  std::string search = "\"" + key + "\":\"";
  auto pos = json.find(search);
  if (pos == std::string::npos) return {};

  pos += search.size();
  auto end = json.find("\"", pos);
  if (end == std::string::npos) return {};

  return json.substr(pos, end - pos);
}

// ── Run a command and return exit code ──────────────────────
int run_command(const std::string& cmd)
{
  return std::system(cmd.c_str());
}

// ── Write a file as root (already privileged) ────────────────
bool write_file(const std::string& path, const std::string& content,
                mode_t mode = 0644)
{
  std::ofstream file(path);
  if (!file.is_open()) return false;
  file << content;
  file.close();
  chmod(path.c_str(), mode);
  return true;
}

bool ensure_directory(const std::string& path, mode_t /*mode*/ = 0755)
{
  // Simple recursive mkdir. The mode parameter is not used directly
  // because we delegate to the mkdir command which handles permissions
  // via the system umask.
  std::string cmd = "mkdir -p \"" + path + "\"";
  return run_command(cmd) == 0;
}

// ── Command handlers ────────────────────────────────────────

int cmd_mount(const std::string& json)
{
  auto server     = json_get_string(json, "server");
  auto share      = json_get_string(json, "share");
  auto mountpoint = json_get_string(json, "mount_point");
  auto username   = json_get_string(json, "username");
  auto password   = json_get_string(json, "password");
  auto domain     = json_get_string(json, "domain");
  auto version    = json_get_string(json, "smb_version");
  auto options    = json_get_string(json, "extra_options");

  if (server.empty() || share.empty() || mountpoint.empty()) {
    std::cout << R"JSON({"success":false,"error":"Missing required fields (server, share, mount_point)"})JSON" << std::endl;
    return 1;
  }

  // Defaults
  if (version.empty()) version = "3.1.1";

  // Ensure mount point exists
  if (!ensure_directory(mountpoint)) {
    std::cout << R"JSON({"success":false,"error":"Failed to create mount point directory"})JSON" << std::endl;
    return 1;
  }

  // Build mount command
  std::ostringstream cmd;
  cmd << "mount -t cifs \"//" << server << "/" << share << "\""
      << " \"" << mountpoint << "\""
      << " -o vers=" << version;

  if (!username.empty()) {
    cmd << ",username=" << username;
  }
  if (!password.empty()) {
    cmd << ",password=" << password;
  }
  if (!domain.empty()) {
    cmd << ",domain=" << domain;
  }
  if (!options.empty()) {
    cmd << "," << options;
  }

  // Default to current user's uid/gid for local access
  cmd << ",uid=" << getuid() << ",gid=" << getgid();
  cmd << ",iocharset=utf8";

  int ret = run_command(cmd.str());
  if (ret == 0) {
    std::cout << R"JSON({"success":true})JSON" << std::endl;
    return 0;
  } else {
    std::cout << R"JSON({"success":false,"error":"mount.cifs failed with exit code )JSON"
              << ret << R"JSON("})JSON" << std::endl;
    return ret;
  }
}

int cmd_umount(const std::string& json)
{
  auto mountpoint = json_get_string(json, "mount_point");
  if (mountpoint.empty()) {
    std::cout << R"JSON({"success":false,"error":"Missing mount_point"})JSON" << std::endl;
    return 1;
  }

  // Try normal unmount first
  std::string cmd = "umount \"" + mountpoint + "\"";
  int ret = run_command(cmd);

  if (ret != 0) {
    // Try lazy unmount as fallback (for stale mounts after suspend)
    cmd = "umount -l \"" + mountpoint + "\"";
    ret = run_command(cmd);
  }

  if (ret == 0) {
    std::cout << R"JSON({"success":true})JSON" << std::endl;
    return 0;
  } else {
    std::cout << R"JSON({"success":false,"error":"umount failed"})JSON" << std::endl;
    return ret;
  }
}

int cmd_write_cred(const std::string& json)
{
  auto path     = json_get_string(json, "path");
  auto username = json_get_string(json, "username");
  auto password = json_get_string(json, "password");
  auto domain   = json_get_string(json, "domain");

  if (path.empty()) {
    std::cout << R"JSON({"success":false,"error":"Missing path for credentials file"})JSON" << std::endl;
    return 1;
  }

  // Ensure the parent directory exists
  auto slash = path.rfind('/');
  if (slash != std::string::npos) {
    ensure_directory(path.substr(0, slash), 0700);
  }

  std::ostringstream content;
  content << "username=" << username << "\n";
  content << "password=" << password << "\n";
  if (!domain.empty()) {
    content << "domain=" << domain << "\n";
  }

  if (write_file(path, content.str(), 0600)) {
    std::cout << R"JSON({"success":true})JSON" << std::endl;
    return 0;
  } else {
    std::cout << R"JSON({"success":false,"error":"Failed to write credentials file"})JSON" << std::endl;
    return 1;
  }
}

int cmd_write_unit(const std::string& json)
{
  auto path    = json_get_string(json, "path");
  auto content = json_get_string(json, "content");

  if (path.empty() || content.empty()) {
    std::cout << R"JSON({"success":false,"error":"Missing path or content for unit file"})JSON" << std::endl;
    return 1;
  }

  if (write_file(path, content, 0644)) {
    std::cout << R"JSON({"success":true})JSON" << std::endl;
    return 0;
  } else {
    std::cout << R"JSON({"success":false,"error":"Failed to write unit file"})JSON" << std::endl;
    return 1;
  }
}

} // anonymous namespace

int main(int argc, char* argv[])
{
  // Must be run as root (pkexec ensures this, but double-check)
  if (geteuid() != 0) {
    std::cerr << "mounter-helper must be run as root (use pkexec)" << std::endl;
    return 1;
  }

  if (argc < 2) {
    std::cerr << "Usage: mounter-helper <command>\n"
              << "Commands: mount, umount, write-cred, write-unit\n"
              << "Reads JSON from stdin, writes JSON result to stdout."
              << std::endl;
    return 1;
  }

  std::string command{argv[1]};

  // Read all of stdin into a string
  std::ostringstream input_buf;
  input_buf << std::cin.rdbuf();
  std::string json_input = input_buf.str();

  if (command == "mount") {
    return cmd_mount(json_input);
  } else if (command == "umount") {
    return cmd_umount(json_input);
  } else if (command == "write-cred") {
    return cmd_write_cred(json_input);
  } else if (command == "write-unit") {
    return cmd_write_unit(json_input);
  } else {
    std::cerr << "Unknown command: " << command << std::endl;
    return 1;
  }
}
