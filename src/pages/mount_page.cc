/* mount_page.cc */

#include "mount_page.h"

namespace Mounter {

MountPage::MountPage()
  : Gtk::Box{Gtk::Orientation::VERTICAL}
{
  build_ui();
}

void MountPage::build_ui()
{
  set_margin(12);
  set_spacing(12);

  heading_.set_halign(Gtk::Align::START);
  heading_.get_style_context()->add_class("title-1");

  // ── Form grid ─────────────────────────────────────────────
  form_.set_row_spacing(8);
  form_.set_column_spacing(8);
  form_.set_halign(Gtk::Align::START);

  server_entry_.set_placeholder_text("192.168.1.100");
  server_entry_.set_width_chars(30);
  share_entry_.set_placeholder_text("media");
  share_entry_.set_width_chars(30);
  username_entry_.set_placeholder_text("username");
  username_entry_.set_width_chars(30);
  password_entry_.set_placeholder_text("password");
  password_entry_.set_visibility(false); // hidden by default
  password_entry_.set_width_chars(30);
  domain_entry_.set_placeholder_text("WORKGROUP");
  domain_entry_.set_width_chars(30);
  mountpoint_entry_.set_placeholder_text("/mnt/share-name");
  mountpoint_entry_.set_text("/mnt/");
  mountpoint_entry_.set_width_chars(30);

  // SMB version dropdown
  auto vers_model = Gtk::StringList::create({
    "3.1.1 (default)",
    "3.0",
    "2.1",
    "2.0",
    "1.0 (insecure)",
  });
  vers_dropdown_.set_model(vers_model);
  vers_dropdown_.set_selected(0); // default: 3.1.1

  // Layout: labels in col 0, entries in col 1
  int row = 0;
  auto attach_row = [&](Gtk::Label& lbl, Gtk::Widget& widget) {
    lbl.set_halign(Gtk::Align::END);
    lbl.set_valign(Gtk::Align::CENTER);
    form_.attach(lbl, 0, row);
    form_.attach(widget, 1, row);
    ++row;
  };

  attach_row(server_label_, server_entry_);
  attach_row(share_label_, share_entry_);
  attach_row(username_label_, username_entry_);
  attach_row(password_label_, password_entry_);
  attach_row(domain_label_, domain_entry_);
  attach_row(mountpoint_label_, mountpoint_entry_);
  attach_row(vers_label_, vers_dropdown_);

  mount_button_.set_halign(Gtk::Align::START);
  mount_button_.get_style_context()->add_class("suggested-action");

  persist_check_.set_active(true);

  content_.set_spacing(8);
  content_.append(form_);
  content_.append(persist_check_);
  content_.append(mount_button_);

  append(heading_);
  append(content_);
}

} // namespace Mounter
