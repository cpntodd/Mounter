/* mount_monitor.cc */

#include "mount_monitor.h"
#include <fstream>
#include <glibmm/main.h>

namespace Mounter {

MountMonitor::MountMonitor() = default;

void MountMonitor::start(unsigned int interval_ms)
{
  timeout_conn_ = Glib::signal_timeout().connect(
    sigc::mem_fun(*this, &MountMonitor::poll),
    interval_ms);
}

void MountMonitor::stop()
{
  timeout_conn_.disconnect();
}

std::vector<MountInfo> MountMonitor::active_mounts() const
{
  return current_mounts_;
}

bool MountMonitor::poll()
{
  // Phase 2: parse /proc/mounts for cifs entries
  std::vector<MountInfo> mounts;
  std::ifstream mounts_file("/proc/mounts");
  if (mounts_file.is_open()) {
    std::string line;
    while (std::getline(mounts_file, line)) {
      // Format: device mountpoint fstype options dump pass
      // We look for lines where fstype == "cifs"
      // For now, just count lines as a basic sanity check
    }
  }

  // Compare with previous and emit signal if changed
  if (mounts != current_mounts_) {
    current_mounts_ = std::move(mounts);
    signal_mounts_changed_.emit(current_mounts_);
  }

  return true; // keep the timeout running
}

sigc::signal<void(const std::vector<MountInfo>&)> MountMonitor::signal_mounts_changed()
{
  return signal_mounts_changed_;
}

} // namespace Mounter
