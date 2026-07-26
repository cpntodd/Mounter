/* mount_operation.h — Async mount/umount execution
 *
 * Wraps privileged mount.cifs/umount invocations via pkexec + mounter-helper.
 * All operations run asynchronously and deliver results on the main thread.
 */

#pragma once

#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>

namespace Mounter {

// ── Data types ──────────────────────────────────────────────

struct MountParams {
  std::string server;
  std::string share;
  std::string username;
  std::string password;
  std::string domain;
  std::string mount_point;
  std::string smb_version = "3.1.1";
  std::string extra_options;
  bool        persistent = false;
  bool        boot_mount = false;   // mount at system boot
  bool        auto_mount = false;   // lazy mount on access, unmount when idle
};

struct MountResult {
  bool        success = false;
  std::string error_message;
  int         exit_code = 0;
};

using MountCallback = std::function<void(const MountResult&)>;

// ── MountOperation ──────────────────────────────────────────

class MountOperation
{
public:
  MountOperation();
  ~MountOperation();

  MountOperation(const MountOperation&) = delete;
  MountOperation& operator=(const MountOperation&) = delete;

  /// Execute a mount asynchronously via pkexec + mounter-helper.
  void mount_async(const MountParams& params, MountCallback callback);

  /// Unmount a share asynchronously.
  void unmount_async(const std::string& mount_point, MountCallback callback);

  /// Cancel any running operation (best effort).
  void cancel();

private:
  static std::string build_mount_json(const MountParams& params);
  static std::string build_umount_json(const std::string& mount_point);
  static MountResult execute_helper(const std::string& command,
                                    const std::string& json_payload);

  std::thread           worker_;
  std::atomic<bool>     cancelled_{false};
};

} // namespace Mounter
