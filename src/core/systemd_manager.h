/* systemd_manager.h — systemd .mount/.automount unit management
 *
 * Generates, enables, and disables systemd mount and automount units
 * for persistent SMB shares.
 */

#pragma once

#include <string>

namespace Mounter {

struct SystemdMountConfig {
  std::string server;
  std::string share;
  std::string mount_point;
  std::string credentials_file;
  std::string smb_version = "3.1.1";
  std::string extra_options;
};

class SystemdManager
{
public:
  SystemdManager() = default;

  /// Create and enable a .mount + .automount unit pair.
  /// Returns true on success.
  bool create_and_enable(const SystemdMountConfig& config);

  /// Disable and remove units for a given mount point.
  bool disable_and_remove(const std::string& mount_point);

  /// Generate the unit file content for a .mount unit.
  static std::string generate_mount_unit(const SystemdMountConfig& config);

  /// Generate the unit file content for a .automount unit.
  static std::string generate_automount_unit(const SystemdMountConfig& config);
};

} // namespace Mounter
