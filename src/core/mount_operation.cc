/* mount_operation.cc */

#include "mount_operation.h"
#include <glibmm/main.h>

namespace Mounter {

void MountOperation::execute(const MountParams& params, MountCallback callback)
{
  // Phase 2: actually call mounter-helper via pkexec
  // For now, return a placeholder failure
  auto result = MountResult{};
  result.success = false;
  result.error_message = "Mount operation not yet implemented (Phase 2)";

  if (callback) {
    // Post callback to the main loop
    Glib::signal_idle().connect_once([callback, result]() {
      callback(result);
    });
  }
}

MountResult MountOperation::execute_sync(const MountParams& /*params*/)
{
  MountResult result{};
  result.success = false;
  result.error_message = "execute_sync not yet implemented (Phase 2)";
  return result;
}

} // namespace Mounter
