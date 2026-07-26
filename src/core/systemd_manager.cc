/* systemd_manager.cc */

#include "systemd_manager.h"
#include <sstream>

namespace Mounter {

bool SystemdManager::create_and_enable(const SystemdMountConfig& /*config*/)
{
  // Phase 4: implement via mounter-helper
  return false;
}

bool SystemdManager::disable_and_remove(const std::string& /*mount_point*/)
{
  // Phase 4: implement via mounter-helper
  return false;
}

std::string SystemdManager::generate_mount_unit(const SystemdMountConfig& config)
{
  // Escape the mount point path for systemd unit naming:
  // /mnt/nas-media → mnt-nas-media
  auto escape_path = [](const std::string& path) -> std::string {
    std::string result;
    for (char c : path) {
      if (c == '/') {
        if (!result.empty()) result += '-';
      } else if (c == '-') {
        result += "\\x2d";
      } else {
        result += c;
      }
    }
    // Remove leading dash
    if (!result.empty() && result[0] == '-') result.erase(0, 1);
    return result;
  };

  auto escaped = escape_path(config.mount_point);

  std::ostringstream unit;
  unit << "[Unit]\n"
       << "Description=Mount SMB share: " << config.server << "/" << config.share << "\n"
       << "After=network-online.target\n"
       << "Wants=network-online.target\n"
       << "\n"
       << "[Mount]\n"
       << "What=//" << config.server << "/" << config.share << "\n"
       << "Where=" << config.mount_point << "\n"
       << "Type=cifs\n"
       << "Options=credentials=" << config.credentials_file
       << ",vers=" << config.smb_version
       << ",iocharset=utf8";
  if (!config.extra_options.empty()) {
    unit << "," << config.extra_options;
  }
  unit << "\n"
       << "TimeoutSec=30\n"
       << "\n"
       << "[Install]\n"
       << "WantedBy=multi-user.target\n";

  return unit.str();
}

std::string SystemdManager::generate_automount_unit(const SystemdMountConfig& config)
{
  auto escape_path = [](const std::string& path) -> std::string {
    std::string result;
    for (char c : path) {
      if (c == '/') {
        if (!result.empty()) result += '-';
      } else if (c == '-') {
        result += "\\x2d";
      } else {
        result += c;
      }
    }
    if (!result.empty() && result[0] == '-') result.erase(0, 1);
    return result;
  };

  auto escaped = escape_path(config.mount_point);

  std::ostringstream unit;
  unit << "[Unit]\n"
       << "Description=Automount SMB share: " << config.server << "/" << config.share << "\n"
       << "\n"
       << "[Automount]\n"
       << "Where=" << config.mount_point << "\n"
       << "TimeoutIdleSec=600\n"  // unmount after 10 min idle
       << "\n"
       << "[Install]\n"
       << "WantedBy=multi-user.target\n";

  return unit.str();
}

} // namespace Mounter
