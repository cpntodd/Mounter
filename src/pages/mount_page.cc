/* mount_page.cc — Manual mount form with real mount execution */

#include "mount_page.h"
#include "../window.h"
#include "../core/mount_operation.h"
#include "../core/credential_store.h"

namespace Mounter {

MountPage::MountPage(Window& window)
  : Gtk::Box{Gtk::Orientation::VERTICAL}
  , window_(window)
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

  server_entry_.set_placeholder_text("192.168.1.100 or nas.local");
  server_entry_.set_width_chars(35);
  share_entry_.set_placeholder_text("media");
  share_entry_.set_width_chars(35);
  username_entry_.set_placeholder_text("username (leave empty for guest)");
  username_entry_.set_width_chars(35);
  password_entry_.set_placeholder_text("password");
  password_entry_.set_visibility(false);
  password_entry_.set_width_chars(35);
  domain_entry_.set_placeholder_text("WORKGROUP");
  domain_entry_.set_width_chars(35);
  mountpoint_entry_.set_placeholder_text("/mnt/share-name");
  mountpoint_entry_.set_text("/mnt/");
  mountpoint_entry_.set_width_chars(35);

  // Auto-fill mount point when share name changes
  share_entry_.signal_changed().connect([this]() {
    auto share = share_entry_.get_text();
    if (!share.empty() && mountpoint_entry_.get_text() == "/mnt/") {
      mountpoint_entry_.set_text("/mnt/" + share);
    }
  });

  // SMB version dropdown
  auto vers_model = Gtk::StringList::create({
    "3.1.1 (default)",
    "3.0",
    "2.1",
    "2.0",
    "1.0 (insecure)",
  });
  vers_dropdown_.set_model(vers_model);
  vers_dropdown_.set_selected(0);

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

  // ── Action row ────────────────────────────────────────────
  mount_button_.set_halign(Gtk::Align::START);
  mount_button_.get_style_context()->add_class("suggested-action");
  mount_button_.signal_clicked().connect(
    sigc::mem_fun(*this, &MountPage::on_mount_clicked));

  persist_check_.set_active(true);

  auto action_row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 12);
  action_row->append(mount_button_);
  action_row->append(spinner_);
  action_row->set_margin_top(4);

  // ── Assemble ──────────────────────────────────────────────
  content_.set_spacing(8);
  content_.append(form_);
  content_.append(persist_check_);
  content_.append(*action_row);

  append(heading_);
  append(content_);
}

void MountPage::on_mount_clicked()
{
  // Gather parameters
  MountParams params;
  params.server      = server_entry_.get_text();
  params.share       = share_entry_.get_text();
  params.username    = username_entry_.get_text();
  params.password    = password_entry_.get_text();
  params.domain      = domain_entry_.get_text();
  params.mount_point = mountpoint_entry_.get_text();
  params.persistent  = persist_check_.get_active();

  // Validate required fields
  if (params.server.empty() || params.share.empty() || params.mount_point.empty()) {
    window_.set_status("Error: Server, Share, and Mount Point are required.");
    return;
  }

  // Resolve SMB version from dropdown
  auto selected = vers_dropdown_.get_selected();
  switch (selected) {
    case 0: params.smb_version = "3.1.1"; break;
    case 1: params.smb_version = "3.0";   break;
    case 2: params.smb_version = "2.1";   break;
    case 3: params.smb_version = "2.0";   break;
    case 4: params.smb_version = "1.0";   break;
    default: params.smb_version = "3.1.1"; break;
  }

  // UI feedback
  mount_button_.set_sensitive(false);
  spinner_.start();
  window_.set_status("Mounting //" + params.server + "/" + params.share + " ...");

  // Execute
  window_.mount_operation().mount_async(params,
    [this, params](const MountResult& result) {
      // Restore UI
      mount_button_.set_sensitive(true);
      spinner_.stop();

      if (result.success) {
        window_.set_status(
          "Mounted //" + params.server + "/" + params.share +
          " at " + params.mount_point);

        // Save credentials on successful mount
        CredentialEntry cred;
        cred.server   = params.server;
        cred.share    = params.share;
        cred.username = params.username;
        cred.password = params.password;
        cred.domain   = params.domain;
        window_.credential_store().store(cred);

        // Clear password field for security
        password_entry_.set_text("");
      } else {
        window_.set_status("Mount failed: " + result.error_message);
      }
    });
}

} // namespace Mounter
