/* profiles_page.h — Saved connection profiles */

#pragma once

#include <gtkmm.h>
#include <memory>
#include <vector>

namespace Mounter {

class Window;
struct MountParams;

struct ProfileEntry {
  std::string id;
  std::string name;
  std::string server;
  std::string share;
  std::string username;
  std::string domain;
  std::string mount_point;
  std::string smb_version = "3.1.1";
  bool        auto_mount = false;
};

class ProfilesPage : public Gtk::Box
{
public:
  explicit ProfilesPage(Window& window);
  ~ProfilesPage() override = default;

  /// Add a new profile from mount parameters. Called after successful mount.
  void add_from_mount(const MountParams& params);

private:
  void build_ui();
  void load_profiles();
  void save_profiles();
  void refresh_list();
  void on_mount_profile(const ProfileEntry& profile);
  void on_delete_profile(const std::string& id);

  Window&       window_;

  Gtk::Label    heading_{"Saved Profiles"};
  Gtk::Button   new_button_{"New Profile"};
  Gtk::ListBox  listbox_;
  Gtk::Label    placeholder_{"No saved profiles yet. Profiles are created automatically when you mount a share, or click \"New Profile\"."};
  Gtk::ScrolledWindow scrolled_;
  Gtk::Box      content_{Gtk::Orientation::VERTICAL, 8};

  std::vector<ProfileEntry> profiles_;
};

} // namespace Mounter
