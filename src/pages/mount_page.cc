/* mount_page.cc — Manual mount form with systemd integration */

#include "mount_page.h"
#include "i18n.h"
#include "../window.h"
#include "../core/mount_operation.h"
#include "../core/credential_store.h"
#include "../core/mount_monitor.h"

namespace Mounter {

MountPage::MountPage(Window& window)
  : Gtk::Box{Gtk::Orientation::VERTICAL}
  , window_(window)
{
  build_ui();
}

void MountPage::prefill_from_discovery(const std::string& server,
                                       const std::string& hostname)
{
  // Pre-fill server (use hostname for display if available, but IP for connection)
  if (!hostname.empty()) {
    // Show hostname as a hint, but put IP in the server field
    server_entry_.set_text(server);
    server_entry_.set_tooltip_text("Resolved hostname: " + hostname);
  } else {
    server_entry_.set_text(server);
  }

  // Clear share — user must enter it
  share_entry_.set_text("");
  share_entry_.set_tooltip_text("Enter the share name (e.g., home, media, public)");
  share_entry_.grab_focus();

  // Clear credentials (will be auto-filled from keyring if available)
  username_entry_.set_text("");
  password_entry_.set_text("");
  domain_entry_.set_text("WORKGROUP");

  // Reset mount point to default
  mountpoint_entry_.set_text("/media/");

  // Try to auto-fill credentials from stored keyring
  try_fill_credentials();

  // Switch to this tab
  window_.switch_to_tab("mount-page");
  window_.set_status("Server pre-filled from discovery. Enter share name and credentials, then click Mount.");
}

bool MountPage::validate_form()
{
  auto server = server_entry_.get_text();
  auto share  = share_entry_.get_text();

  if (server.empty()) {
    window_.set_status("Error: Server is required.");
    server_entry_.grab_focus();
    return false;
  }
  if (share.empty()) {
    window_.set_status("Error: Share name is required (e.g., home, media).");
    share_entry_.grab_focus();
    return false;
  }
  return true;
}

void MountPage::build_ui()
{
  set_margin(12);
  set_spacing(12);

  heading_.set_halign(Gtk::Align::START);
  heading_.get_style_context()->add_class("title-1");

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
  mountpoint_entry_.set_placeholder_text("/media/share-name");
  mountpoint_entry_.set_text("/media/");
  mountpoint_entry_.set_width_chars(35);

  share_entry_.signal_changed().connect([this]() {
    auto share = share_entry_.get_text();
    if (!share.empty()) {
      mountpoint_entry_.set_text("/media/" + share);
    }
    try_fill_credentials();
  });

  server_entry_.signal_changed().connect([this]() {
    try_fill_credentials();
  });

  auto vers_model = Gtk::StringList::create({
    "3.1.1 (default)", "3.0", "2.1", "2.0", "1.0 (insecure)",
  });
  vers_dropdown_.set_model(vers_model);
  vers_dropdown_.set_selected(0);

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

  // Pressing Enter in any field triggers mount
  auto enter_controller = Gtk::EventControllerKey::create();
  enter_controller->signal_key_pressed().connect(
    [this](guint keyval, guint, Gdk::ModifierType) -> bool {
      if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) {
        on_mount_clicked();
        return true;
      }
      return false;
    }, false);
  password_entry_.add_controller(enter_controller);
  server_entry_.add_controller(Gtk::EventControllerKey::create());
  // Share the same handler pattern for server_entry too
  auto enter_controller2 = Gtk::EventControllerKey::create();
  enter_controller2->signal_key_pressed().connect(
    [this](guint keyval, guint, Gdk::ModifierType) -> bool {
      if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) {
        on_mount_clicked();
        return true;
      }
      return false;
    }, false);
  server_entry_.add_controller(enter_controller2);
  attach_row(domain_label_, domain_entry_);
  attach_row(mountpoint_label_, mountpoint_entry_);
  attach_row(vers_label_, vers_dropdown_);

  mount_button_.set_halign(Gtk::Align::START);
  mount_button_.get_style_context()->add_class("suggested-action");
  mount_button_.signal_clicked().connect(
    sigc::mem_fun(*this, &MountPage::on_mount_clicked));

  persist_check_.set_active(true);

  auto action_row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 12);
  action_row->append(mount_button_);
  action_row->append(spinner_);
  action_row->set_margin_top(4);

  content_.set_spacing(8);
  content_.append(form_);
  content_.append(persist_check_);
  content_.append(*action_row);

  append(heading_);
  append(content_);
}

void MountPage::try_fill_credentials()
{
  auto server = server_entry_.get_text();
  auto share  = share_entry_.get_text();

  if (server.empty() || share.empty()) return;

  // Only auto-fill if username is currently empty (don't overwrite user input)
  if (!username_entry_.get_text().empty()) return;

  auto cred = window_.credential_store().lookup(server, share);
  if (cred) {
    username_entry_.set_text(cred->username);
    password_entry_.set_text(cred->password);
    domain_entry_.set_text(cred->domain);
    window_.set_status("Credentials loaded from keyring for " + server + "/" + share);
  }
}

void MountPage::on_mount_clicked()
{
  if (!validate_form()) return;

  MountParams params;
  params.server      = server_entry_.get_text();
  params.share       = share_entry_.get_text();
  params.username    = username_entry_.get_text();
  params.password    = password_entry_.get_text();
  params.domain      = domain_entry_.get_text();
  params.mount_point = mountpoint_entry_.get_text();
  params.persistent  = persist_check_.get_active();

  // Check if this share or mount point is already mounted
  for (const auto& m : window_.mount_monitor().active_mounts()) {
    if (m.mount_point == params.mount_point ||
        (m.server == params.server && m.share == params.share)) {
      window_.set_status(
        "Already mounted: //" + m.server + "/" + m.share +
        " → " + m.mount_point + ". Switching to Mounted tab.");
      window_.switch_to_tab("mounted-page");
      return;
    }
  }

  auto selected = vers_dropdown_.get_selected();
  switch (selected) {
    case 0: params.smb_version = "3.1.1"; break;
    case 1: params.smb_version = "3.0";   break;
    case 2: params.smb_version = "2.1";   break;
    case 3: params.smb_version = "2.0";   break;
    case 4: params.smb_version = "1.0";   break;
    default: params.smb_version = "3.1.1"; break;
  }

  mount_button_.set_sensitive(false);
  spinner_.start();
  window_.set_status("Mounting //" + params.server + "/" + params.share + " ...");

  window_.mount_operation().mount_async(params,
    [this, params](const MountResult& result) {
      mount_button_.set_sensitive(true);
      spinner_.stop();

      if (result.success) {
        window_.set_status(
          "Mounted //" + params.server + "/" + params.share +
          " at " + params.mount_point);

        // Save credentials
        CredentialEntry cred;
        cred.server   = params.server;
        cred.share    = params.share;
        cred.username = params.username;
        cred.password = params.password;
        cred.domain   = params.domain;
        window_.credential_store().store(cred);

        password_entry_.set_text("");

        // Switch to Mounted tab to show the new mount
        window_.switch_to_tab("mounted-page");
      } else {
        window_.set_status("Mount failed: " + result.error_message);
      }
    });
}

} // namespace Mounter
