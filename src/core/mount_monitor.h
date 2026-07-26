/* mount_monitor.h — Polls /proc/mounts for CIFS entries
 *
 * Periodically checks /proc/mounts for active CIFS mount points
 * and emits signals when mounts appear or disappear.
 */

#pragma once

#include <gtkmm.h>
#include <sigc++/sigc++.h>
#include <string>
#include <vector>

namespace Mounter {

struct MountInfo {
  std::string server;
  std::string share;
  std::string mount_point;
  std::string options;

  bool operator==(const MountInfo& other) const {
    return server == other.server
        && share == other.share
        && mount_point == other.mount_point
        && options == other.options;
  }
};

class MountMonitor
{
public:
  MountMonitor();

  /// Start polling every `interval_ms` milliseconds.
  void start(unsigned int interval_ms = 2000);

  /// Stop polling.
  void stop();

  /// Get currently active CIFS mounts (synchronous snapshot).
  std::vector<MountInfo> active_mounts() const;

  /// Signal emitted when the mount list changes.
  sigc::signal<void(const std::vector<MountInfo>&)> signal_mounts_changed();

private:
  bool poll();

  std::vector<MountInfo> current_mounts_;
  sigc::connection       timeout_conn_;
  sigc::signal<void(const std::vector<MountInfo>&)> signal_mounts_changed_;
};

} // namespace Mounter
