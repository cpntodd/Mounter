/* systemd_manager.cc — systemd unit generation & helper integration */

#include "systemd_manager.h"

#include <glibmm/main.h>
#include <giomm/subprocess.h>

#include <sstream>
#include <thread>
#include <nlohmann/json.hpp>

#ifndef HELPER_PATH
  #define HELPER_PATH "/usr/libexec/mounter/mounter-helper"
#endif

namespace Mounter {

// ── Async API ───────────────────────────────────────────────

void SystemdManager::create_and_enable_async(const SystemdMountConfig& config,
                                             SystemdCallback callback)
{
  // Run in background thread since it involves multiple subprocess calls
  std::thread([config, callback = std::move(callback)]() {
    bool success = true;
    std::string error_msg;

    try {
      auto escaped = escape_path(config.mount_point);

      // 1. Write credentials file
      nlohmann::json cred_json;
      cred_json["path"] = config.credentials_file;
      // The username/password should already be in the cred file from the mount operation
      // We just need to ensure the directory exists for future credential writes

      // 2. Write the .mount unit
      std::string mount_unit_content = generate_mount_unit(config);
      nlohmann::json mount_json;
      mount_json["path"]    = "/etc/systemd/system/" + escaped + ".mount";
      mount_json["content"] = mount_unit_content;

      auto mount_bytes = Glib::Bytes::create(
        mount_json.dump().data(), mount_json.dump().size());

      auto proc = Gio::Subprocess::create(
        std::vector<std::string>{"pkexec", HELPER_PATH, "write-unit"},
        Gio::Subprocess::Flags::STDIN_PIPE |
        Gio::Subprocess::Flags::STDOUT_PIPE |
        Gio::Subprocess::Flags::STDERR_SILENCE);

      auto [stdout_bytes, stderr_bytes] = proc->communicate(mount_bytes, nullptr);

      if (proc->get_exit_status() != 0) {
        success = false;
        error_msg = "Failed to write .mount unit file";
      }

      // 3. Write the .automount unit
      if (success) {
        std::string automount_content = generate_automount_unit(config);
        nlohmann::json am_json;
        am_json["path"]    = "/etc/systemd/system/" + escaped + ".automount";
        am_json["content"] = automount_content;

        auto am_bytes = Glib::Bytes::create(
          am_json.dump().data(), am_json.dump().size());

        auto proc2 = Gio::Subprocess::create(
          std::vector<std::string>{"pkexec", HELPER_PATH, "write-unit"},
          Gio::Subprocess::Flags::STDIN_PIPE |
          Gio::Subprocess::Flags::STDOUT_PIPE |
          Gio::Subprocess::Flags::STDERR_SILENCE);

        auto [am_stdout, am_stderr] = proc2->communicate(am_bytes, nullptr);

        if (proc2->get_exit_status() != 0) {
          success = false;
          error_msg = "Failed to write .automount unit file";
        }
      }

      // 4. systemctl daemon-reload
      if (success) {
        auto proc3 = Gio::Subprocess::create(
          std::vector<std::string>{"pkexec", "systemctl", "daemon-reload"},
          Gio::Subprocess::Flags::STDOUT_SILENCE |
          Gio::Subprocess::Flags::STDERR_SILENCE);
        proc3->wait();

        // 5. systemctl enable the automount unit
        auto proc4 = Gio::Subprocess::create(
          std::vector<std::string>{"pkexec", "systemctl", "enable", "--now",
            escaped + ".automount"},
          Gio::Subprocess::Flags::STDOUT_SILENCE |
          Gio::Subprocess::Flags::STDERR_SILENCE);
        proc4->wait();

        if (proc4->get_exit_status() != 0) {
          // enable failed but the unit files are written — soft error
          error_msg = "Units written but enable failed (check systemctl status)";
        }
      }

    } catch (const Glib::Error& e) {
      success = false;
      error_msg = std::string{"Systemd setup failed: "} + e.what();
    }

    // Post result to main thread
    if (callback) {
      Glib::signal_idle().connect_once([callback = std::move(callback), success, error_msg]() {
        callback(success, error_msg);
      });
    }
  }).detach();
}

// ── Unit file generation ────────────────────────────────────

std::string SystemdManager::escape_path(const std::string& path)
{
  std::string result;
  for (char c : path) {
    if (c == '/') {
      if (!result.empty()) result += '-';
    } else if (c == '-') {
      result += "\\x2d";
    } else {
      result += c;
    }
  }
  // Remove leading dash
  if (!result.empty() && result[0] == '-') result.erase(0, 1);
  return result;
}

std::string SystemdManager::generate_mount_unit(const SystemdMountConfig& config)
{
  std::ostringstream unit;
  unit << "[Unit]\n"
       << "Description=Mount SMB share: " << config.server << "/" << config.share << "\n"
       << "After=network-online.target\n"
       << "Wants=network-online.target\n"
       << "\n"
       << "[Mount]\n"
       << "What=//" << config.server << "/" << config.share << "\n"
       << "Where=" << config.mount_point << "\n"
       << "Type=cifs\n"
       << "Options=credentials=" << config.credentials_file
       << ",vers=" << config.smb_version
       << ",uid=" << config.uid << ",gid=" << config.gid
       << ",file_mode=0644,dir_mode=0755,rw,iocharset=utf8,noperm";
  if (!config.extra_options.empty()) {
    unit << "," << config.extra_options;
  }
  unit << "\n"
       << "TimeoutSec=30\n"
       << "\n"
       << "[Install]\n"
       << "WantedBy=multi-user.target\n";
  return unit.str();
}

std::string SystemdManager::generate_automount_unit(const SystemdMountConfig& config)
{
  std::ostringstream unit;
  unit << "[Unit]\n"
       << "Description=Automount SMB share: " << config.server << "/" << config.share << "\n"
       << "\n"
       << "[Automount]\n"
       << "Where=" << config.mount_point << "\n"
       << "TimeoutIdleSec=600\n"
       << "\n"
       << "[Install]\n"
       << "WantedBy=multi-user.target\n";
  return unit.str();
}

} // namespace Mounter
