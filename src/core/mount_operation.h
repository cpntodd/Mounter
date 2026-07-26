/* mount_operation.h — Async mount execution
 *
 * Wraps the mount.cifs invocation. Communicates with the privileged
 * helper binary via pkexec for actual mount execution.
 */

#pragma once

#include <string>
#include <functional>
#include <memory>

namespace Mounter {

struct MountParams {
  std::string server;
  std::string share;
  std::string username;
  std::string password;
  std::string domain;
  std::string mount_point;
  std::string smb_version = "3.1.1";
  bool        persistent = false;
};

struct MountResult {
  bool        success = false;
  std::string error_message;
  int         exit_code = 0;
};

using MountCallback = std::function<void(const MountResult&)>;

class MountOperation
{
public:
  MountOperation() = default;

  /// Execute a mount asynchronously. Calls back on the main thread.
  void execute(const MountParams& params, MountCallback callback);

  /// Synchronous version for use in helper context.
  static MountResult execute_sync(const MountParams& params);
};

} // namespace Mounter
