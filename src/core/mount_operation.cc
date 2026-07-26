/* mount_operation.cc — Mount/umount subprocess execution */

#include "mount_operation.h"
#include "net_util.h"

#include <glibmm/main.h>
#include <giomm/subprocess.h>
#include <nlohmann/json.hpp>

#include <sstream>
#include <filesystem>

#ifndef HELPER_PATH
  #define HELPER_PATH "/usr/libexec/mounter/mounter-helper"
#endif

namespace Mounter {

MountOperation::MountOperation() = default;

MountOperation::~MountOperation()
{
  cancel();
  if (worker_.joinable()) {
    worker_.join();
  }
}

void MountOperation::cancel()
{
  cancelled_.store(true, std::memory_order_release);
}

void MountOperation::mount_async(const MountParams& params,
                                 MountCallback callback)
{
  cancelled_.store(false, std::memory_order_release);

  if (!params.mount_point.empty()) {
    std::error_code ec;
    std::filesystem::create_directories(params.mount_point, ec);
  }

  auto json_payload = build_mount_json(params);

  if (worker_.joinable()) {
    worker_.join();
  }

  worker_ = std::thread([this, callback = std::move(callback), json_payload]() mutable {
    if (cancelled_.load(std::memory_order_acquire)) return;

    auto result = execute_helper("mount", json_payload);

    if (callback && !cancelled_.load(std::memory_order_acquire)) {
      Glib::signal_idle().connect_once([callback = std::move(callback), result]() {
        callback(result);
      });
    }
  });
}

void MountOperation::unmount_async(const std::string& mount_point,
                                   MountCallback callback)
{
  cancelled_.store(false, std::memory_order_release);

  auto json_payload = build_umount_json(mount_point);

  if (worker_.joinable()) {
    worker_.join();
  }

  worker_ = std::thread([this, callback = std::move(callback), json_payload]() mutable {
    if (cancelled_.load(std::memory_order_acquire)) return;

    auto result = execute_helper("umount", json_payload);

    if (callback && !cancelled_.load(std::memory_order_acquire)) {
      Glib::signal_idle().connect_once([callback = std::move(callback), result]() {
        callback(result);
      });
    }
  });
}

std::string MountOperation::build_mount_json(const MountParams& params)
{
  nlohmann::json j;
  j["server"]       = NetUtil::bracket_if_ipv6(params.server);
  j["share"]        = params.share;
  j["mount_point"]  = params.mount_point;
  j["username"]     = params.username;
  j["password"]     = params.password;
  j["domain"]       = params.domain;
  j["smb_version"]  = params.smb_version;
  j["extra_options"] = params.extra_options;
  return j.dump();
}

std::string MountOperation::build_umount_json(const std::string& mount_point)
{
  nlohmann::json j;
  j["mount_point"] = mount_point;
  return j.dump();
}

MountResult MountOperation::execute_helper(const std::string& command,
                                           const std::string& json_payload)
{
  MountResult result;

  try {
    auto proc = Gio::Subprocess::create(
      std::vector<std::string>{"pkexec", HELPER_PATH, command},
      Gio::Subprocess::Flags::STDIN_PIPE |
      Gio::Subprocess::Flags::STDOUT_PIPE |
      Gio::Subprocess::Flags::STDERR_SILENCE
    );

    // Convert JSON string to Glib::Bytes for stdin
    auto stdin_bytes = Glib::Bytes::create(json_payload.data(), json_payload.size());

    // communicate returns pair<RefPtr<Bytes>, RefPtr<Bytes>> (stdout, stderr)
    auto [stdout_bytes, stderr_bytes] = proc->communicate(stdin_bytes, nullptr);

    result.exit_code = proc->get_exit_status();

    // Convert stdout Bytes to string
    std::string stdout_str;
    if (stdout_bytes) {
      gsize size;
      const auto* data = static_cast<const char*>(stdout_bytes->get_data(size));
      stdout_str.assign(data, size);
    }

    if (!stdout_str.empty()) {
      try {
        auto response = nlohmann::json::parse(stdout_str);
        result.success = response.value("success", false);
        if (!result.success) {
          result.error_message = response.value("error",
            std::string{"Unknown error from helper"});
        }
      } catch (const nlohmann::json::exception& e) {
        result.success = false;
        result.error_message = std::string{"Failed to parse helper response: "} + e.what();
      }
    } else {
      result.success = false;
      result.error_message = "Helper produced no output (pkexec may have been cancelled)";
    }

  } catch (const Glib::Error& e) {
    result.success = false;
    result.error_message = std::string{"Failed to launch helper: "} + e.what();
  }

  return result;
}

} // namespace Mounter
