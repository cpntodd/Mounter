/* mount_monitor.cc — Parses /proc/mounts for CIFS entries */

#include "mount_monitor.h"
#include <glibmm/main.h>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace Mounter {

MountMonitor::MountMonitor() = default;
MountMonitor::~MountMonitor() { stop(); }

void MountMonitor::start(unsigned int interval_ms)
{
  // Poll once immediately
  poll();

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
  std::vector<MountInfo> mounts;

  std::ifstream mounts_file("/proc/mounts");
  if (mounts_file.is_open()) {
    std::string line;
    while (std::getline(mounts_file, line)) {
      // Format: device mountpoint fstype options dump pass
      // Example: //192.168.1.100/media /mnt/nas-media cifs rw,relatime,vers=3.1.1,... 0 0
      std::istringstream iss(line);
      std::string device, mount_point, fstype, options;
      int dump, pass;

      if (!(iss >> device >> mount_point >> fstype >> options >> dump >> pass))
        continue;

      // Only care about CIFS mounts
      if (fstype != "cifs" && fstype != "smb3") continue;

      MountInfo info;
      info.mount_point = mount_point;
      info.options = options;

      // Parse device field: //server/share
      // Strip leading "//"
      if (device.size() > 2 && device[0] == '/' && device[1] == '/') {
        auto path = device.substr(2);
        auto slash = path.find('/');
        if (slash != std::string::npos) {
          info.server = path.substr(0, slash);
          info.share  = path.substr(slash + 1);
        } else {
          info.server = path;
        }
      }

      // Handle IPv6 addresses in device (enclosed in brackets or with colons)
      // e.g., //[fe80::1]/share — strip brackets
      if (!info.server.empty() && info.server.front() == '[' && info.server.back() == ']') {
        info.server = info.server.substr(1, info.server.size() - 2);
      }

      mounts.push_back(std::move(info));
    }
  }

  // Only emit if the list actually changed
  if (mounts != current_mounts_) {
    current_mounts_ = std::move(mounts);
    signal_mounts_changed_.emit(current_mounts_);
  }

  return true; // keep the timer running
}

sigc::signal<void(const std::vector<MountInfo>&)>& MountMonitor::signal_mounts_changed()
{
  return signal_mounts_changed_;
}

} // namespace Mounter
