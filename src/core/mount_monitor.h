/* mount_monitor.h — Polls /proc/mounts for CIFS entries */

#pragma once

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
        && mount_point == other.mount_point;
  }
  bool operator!=(const MountInfo& other) const { return !(*this == other); }
};

class MountMonitor
{
public:
  MountMonitor();
  ~MountMonitor();

  /// Start polling every `interval_ms` milliseconds.
  void start(unsigned int interval_ms = 2000);

  /// Stop polling.
  void stop();

  /// Get currently active CIFS mounts (synchronous snapshot).
  std::vector<MountInfo> active_mounts() const;

  /// Signal emitted when the mount list changes.
  sigc::signal<void(const std::vector<MountInfo>&)>& signal_mounts_changed();

private:
  bool poll();

  std::vector<MountInfo> current_mounts_;
  sigc::connection        timeout_conn_;
  sigc::signal<void(const std::vector<MountInfo>&)> signal_mounts_changed_;
};

} // namespace Mounter
