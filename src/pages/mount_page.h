/* mount_page.h — Manual mount form */

#pragma once

#include <gtkmm.h>

namespace Mounter {

class Window;

class MountPage : public Gtk::Box
{
public:
  explicit MountPage(Window& window);
  ~MountPage() override = default;

  /// Pre-fill the form from network discovery data.
  /// @param server   IP address or hostname of the SMB server
  /// @param hostname Optional resolved hostname (for display, not used in mount)
  void prefill_from_discovery(const std::string& server,
                              const std::string& hostname);

private:
  void build_ui();
  void on_mount_clicked();
  void try_fill_credentials();
  bool validate_form();

  Window&       window_;

  Gtk::Label    heading_{"Manual Mount"};
  Gtk::Grid     form_;
  Gtk::Label    server_label_{"Server:"};
  Gtk::Entry    server_entry_;
  Gtk::Label    share_label_{"Share:"};
  Gtk::Entry    share_entry_;
  Gtk::Label    username_label_{"Username:"};
  Gtk::Entry    username_entry_;
  Gtk::Label    password_label_{"Password:"};
  Gtk::Entry    password_entry_;
  Gtk::Label    domain_label_{"Domain:"};
  Gtk::Entry    domain_entry_;
  Gtk::Label    mountpoint_label_{"Mount Point:"};
  Gtk::Entry    mountpoint_entry_;
  Gtk::Label    vers_label_{"SMB Version:"};
  Gtk::DropDown vers_dropdown_;
  Gtk::Button   mount_button_{"Mount"};
  Gtk::CheckButton persist_check_{"Make persistent (auto-mount at boot)"};
  Gtk::Spinner  spinner_;
  Gtk::Box      content_{Gtk::Orientation::VERTICAL};
};

} // namespace Mounter
