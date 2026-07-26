/* mount_page.h — Manual mount page
 *
 * Provides a form for manually entering SMB share connection details
 * and mounting the share.
 */

#pragma once

#include <gtkmm.h>

namespace Mounter {

class MountPage : public Gtk::Box
{
public:
  MountPage();
  ~MountPage() override = default;

private:
  void build_ui();

  Gtk::Label   heading_{"Manual Mount"};
  Gtk::Grid    form_;
  Gtk::Label   server_label_{"Server:"};
  Gtk::Entry   server_entry_;
  Gtk::Label   share_label_{"Share:"};
  Gtk::Entry   share_entry_;
  Gtk::Label   username_label_{"Username:"};
  Gtk::Entry   username_entry_;
  Gtk::Label   password_label_{"Password:"};
  Gtk::Entry   password_entry_;
  Gtk::Label   domain_label_{"Domain:"};
  Gtk::Entry   domain_entry_;
  Gtk::Label   mountpoint_label_{"Mount Point:"};
  Gtk::Entry   mountpoint_entry_;
  Gtk::Label   vers_label_{"SMB Version:"};
  Gtk::DropDown vers_dropdown_;
  Gtk::Button  mount_button_{"Mount"};
  Gtk::CheckButton persist_check_{"Make persistent (systemd automount)"};
  Gtk::Box     content_{Gtk::Orientation::VERTICAL};
};

} // namespace Mounter
