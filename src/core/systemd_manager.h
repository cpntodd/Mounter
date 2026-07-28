/* systemd_manager.h — systemd .mount/.automount unit management */

#pragma once

#include <string>
#include <functional>

namespace Mounter {

struct SystemdMountConfig {
  std::string server;
  std::string share;
  std::string mount_point;
  std::string credentials_file;
  std::string smb_version = "3.1.1";
  std::string extra_options;
  uid_t       uid = 0;
  gid_t       gid = 0;
};

using SystemdCallback = std::function<void(bool success, const std::string& error)>;

class SystemdManager
{
public:
  SystemdManager() = default;

  /// Create and enable a .mount + .automount unit pair via mounter-helper.
  void create_and_enable_async(const SystemdMountConfig& config,
                               SystemdCallback callback);

  /// Generate the unit file content for a .mount unit.
  static std::string generate_mount_unit(const SystemdMountConfig& config);

  /// Generate the unit file content for a .automount unit.
  static std::string generate_automount_unit(const SystemdMountConfig& config);

  /// Escape a filesystem path for systemd unit naming.
  static std::string escape_path(const std::string& path);
};

} // namespace Mounter
