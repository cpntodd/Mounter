/* profiles_page.cc — Profile management */

#include "profiles_page.h"
#include "../window.h"
#include "../core/mount_operation.h"
#include "../core/credential_store.h"

#include <glibmm/miscutils.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <random>

namespace Mounter {

// ── Helpers ─────────────────────────────────────────────────

static std::string profiles_file_path()
{
  return Glib::build_filename(
    Glib::get_user_config_dir(), "mounter", "profiles.json");
}

static std::string generate_uuid()
{
  // Simple UUID v4-like generator
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(0, 15);
  static std::uniform_int_distribution<> dis2(8, 11);

  std::ostringstream ss;
  ss << std::hex;
  for (int i = 0; i < 8; ++i)  ss << dis(gen);
  ss << "-";
  for (int i = 0; i < 4; ++i)  ss << dis(gen);
  ss << "-4"; // version 4
  for (int i = 0; i < 3; ++i)  ss << dis(gen);
  ss << "-";
  ss << dis2(gen);
  for (int i = 0; i < 3; ++i)  ss << dis(gen);
  ss << "-";
  for (int i = 0; i < 12; ++i) ss << dis(gen);
  return ss.str();
}

// ── Construction ────────────────────────────────────────────

ProfilesPage::ProfilesPage(Window& window)
  : Gtk::Box{Gtk::Orientation::VERTICAL}
  , window_(window)
{
  build_ui();
  load_profiles();
  refresh_list();
}

void ProfilesPage::build_ui()
{
  set_margin(12);
  set_spacing(8);

  heading_.set_halign(Gtk::Align::START);
  heading_.get_style_context()->add_class("title-1");

  new_button_.set_halign(Gtk::Align::START);
  new_button_.signal_clicked().connect([this]() {
    // Create a blank profile for manual editing
    ProfileEntry entry;
    entry.id   = generate_uuid();
    entry.name = "New Profile";
    profiles_.push_back(entry);
    save_profiles();
    refresh_list();
  });

  placeholder_.set_halign(Gtk::Align::START);
  placeholder_.set_opacity(0.6);
  placeholder_.set_wrap(true);
  placeholder_.set_margin_top(12);

  listbox_.set_selection_mode(Gtk::SelectionMode::NONE);
  listbox_.get_style_context()->add_class("rich-list");

  scrolled_.set_child(listbox_);
  scrolled_.set_vexpand(true);
  scrolled_.set_visible(false);

  content_.set_vexpand(true);
  content_.append(new_button_);
  content_.append(placeholder_);
  content_.append(scrolled_);

  append(heading_);
  append(content_);
}

// ── Persistence ─────────────────────────────────────────────

void ProfilesPage::load_profiles()
{
  auto path = profiles_file_path();
  if (!std::filesystem::exists(path)) return;

  try {
    std::ifstream f(path);
    auto j = nlohmann::json::parse(f);

    profiles_.clear();
    for (const auto& item : j) {
      ProfileEntry entry;
      entry.id          = item.value("id", "");
      entry.name        = item.value("name", "");
      entry.server      = item.value("server", "");
      entry.share       = item.value("share", "");
      entry.username    = item.value("username", "");
      entry.domain      = item.value("domain", "");
      entry.mount_point = item.value("mount_point", "");
      entry.smb_version = item.value("smb_version", "3.1.1");
      entry.auto_mount  = item.value("auto_mount", false);
      profiles_.push_back(std::move(entry));
    }
  } catch (...) {
    profiles_.clear();
  }
}

void ProfilesPage::save_profiles()
{
  auto path = profiles_file_path();
  auto dir = Glib::build_filename(Glib::get_user_config_dir(), "mounter");
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);

  nlohmann::json j = nlohmann::json::array();
  for (const auto& p : profiles_) {
    nlohmann::json item;
    item["id"]          = p.id;
    item["name"]        = p.name;
    item["server"]      = p.server;
    item["share"]       = p.share;
    item["username"]    = p.username;
    item["domain"]      = p.domain;
    item["mount_point"] = p.mount_point;
    item["smb_version"] = p.smb_version;
    item["auto_mount"]  = p.auto_mount;
    j.push_back(item);
  }

  std::ofstream f(path);
  f << j.dump(2);
}

void ProfilesPage::add_from_mount(const MountParams& params)
{
  ProfileEntry entry;
  entry.id          = generate_uuid();
  entry.name        = params.server + "/" + params.share;
  entry.server      = params.server;
  entry.share       = params.share;
  entry.username    = params.username;
  entry.domain      = params.domain;
  entry.mount_point = params.mount_point;
  entry.smb_version = params.smb_version;

  profiles_.push_back(entry);
  save_profiles();
  refresh_list();
}

// ── UI refresh ──────────────────────────────────────────────

void ProfilesPage::refresh_list()
{
  while (auto* child = listbox_.get_first_child()) {
    listbox_.remove(*child);
  }

  if (profiles_.empty()) {
    placeholder_.set_visible(true);
    scrolled_.set_visible(false);
    return;
  }

  placeholder_.set_visible(false);
  scrolled_.set_visible(true);

  for (const auto& profile : profiles_) {
    auto row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    row->set_margin_start(8);
    row->set_margin_end(8);
    row->set_margin_top(4);
    row->set_margin_bottom(4);

    // Info column
    auto info = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 2);
    info->set_hexpand(true);

    auto name_label = Gtk::make_managed<Gtk::Label>(profile.name);
    name_label->set_halign(Gtk::Align::START);
    name_label->get_style_context()->add_class("heading");

    auto detail = Gtk::make_managed<Gtk::Label>(
      "//" + profile.server + "/" + profile.share +
      " \342\206\222 " + profile.mount_point);
    detail->set_halign(Gtk::Align::START);
    detail->set_opacity(0.6);

    info->append(*name_label);
    info->append(*detail);

    // Action buttons
    auto mount_btn = Gtk::make_managed<Gtk::Button>("Mount");
    mount_btn->get_style_context()->add_class("suggested-action");

    auto delete_btn = Gtk::make_managed<Gtk::Button>("Delete");
    delete_btn->get_style_context()->add_class("destructive-action");

    auto profile_copy = profile; // capture by value
    mount_btn->signal_clicked().connect([this, profile_copy]() {
      on_mount_profile(profile_copy);
    });

    auto profile_id = profile.id;
    delete_btn->signal_clicked().connect([this, profile_id]() {
      on_delete_profile(profile_id);
    });

    row->append(*info);
    row->append(*mount_btn);
    row->append(*delete_btn);
    listbox_.append(*row);
  }
}

// ── Actions ─────────────────────────────────────────────────

void ProfilesPage::on_mount_profile(const ProfileEntry& profile)
{
  MountParams params;
  params.server      = profile.server;
  params.share       = profile.share;
  params.username    = profile.username;
  params.domain      = profile.domain;
  params.mount_point = profile.mount_point;
  params.smb_version = profile.smb_version;

  // Try to get password from credential store
  auto cred = window_.credential_store().lookup(profile.server, profile.share);
  if (cred) {
    params.password = cred->password;
    params.username = cred->username;
    params.domain   = cred->domain;
  }

  window_.set_status("Mounting " + profile.name + " ...");

  window_.mount_operation().mount_async(params,
    [this, profile](const MountResult& result) {
      if (result.success) {
        window_.set_status("Mounted " + profile.name);
      } else {
        window_.set_status("Failed to mount " + profile.name + ": " +
                           result.error_message);
      }
    });
}

void ProfilesPage::on_delete_profile(const std::string& id)
{
  profiles_.erase(
    std::remove_if(profiles_.begin(), profiles_.end(),
      [&id](const auto& p) { return p.id == id; }),
    profiles_.end());

  save_profiles();
  refresh_list();
  window_.set_status("Profile deleted.");
}

} // namespace Mounter
