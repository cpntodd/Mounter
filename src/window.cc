/* window.cc — Mounter::Window implementation */

#include "window.h"
#include "i18n.h"
#include "application.h"
#include "core/mount_monitor.h"
#include "core/mount_operation.h"
#include "core/credential_store.h"
#include "pages/discovery_page.h"
#include "pages/dashboard_page.h"
#include "pages/mount_page.h"
#include "pages/mounted_page.h"
#include "pages/profiles_page.h"
#include "pages/diagnostics_page.h"

namespace Mounter {

Window::Window(Application& app)
  : app_(app)
{
  set_title("Mounter");
  set_default_size(900, 600);
  set_icon_name("com.github.oddsoul.Mounter");

  // Initialize core services
  mount_monitor_    = std::make_unique<MountMonitor>();
  mount_operation_  = std::make_unique<MountOperation>();
  credential_store_ = std::make_unique<CredentialStore>();

  build_ui();
  setup_shortcuts();

  // Start monitoring mounts
  mount_monitor_->start(2000);

  // Default to the dashboard
  stack_.set_visible_child("dashboard-page");
}

Window::~Window() = default;

void Window::setup_shortcuts()
{
  // Tab switching with Ctrl+1 through Ctrl+6
  auto add_tab_shortcut = [this](int index, const char* page_name) {
    auto action = Gio::SimpleAction::create(
      "switch-tab-" + std::to_string(index));
    action->signal_activate().connect([this, page_name](const Glib::VariantBase&) {
      stack_.set_visible_child(page_name);
    });
    app_.add_action(action);
    app_.set_accel_for_action(
      "app.switch-tab-" + std::to_string(index),
      "<Control>" + std::to_string(index));
  };

  add_tab_shortcut(1, "dashboard-page");
  add_tab_shortcut(2, "discover-page");
  add_tab_shortcut(3, "mount-page");
  add_tab_shortcut(4, "mounted-page");
  add_tab_shortcut(5, "profiles-page");
  add_tab_shortcut(6, "diagnostics-page");

  // Ctrl+R: refresh / rescan
  auto refresh_action = Gio::SimpleAction::create("refresh");
  refresh_action->signal_activate().connect([this](const Glib::VariantBase&) {
    // Trigger scan if on discovery page
    if (stack_.get_visible_child_name() == "discover-page") {
      set_status("Press \"Scan Network\" to discover shares.");
    }
  });
  app_.add_action(refresh_action);
  app_.set_accel_for_action("app.refresh", "<Control>r");

  // Ctrl+W: close window
  auto close_action = Gio::SimpleAction::create("close-window");
  close_action->signal_activate().connect([this](const Glib::VariantBase&) {
    close();
  });
  app_.add_action(close_action);
  app_.set_accel_for_action("app.close-window", "<Control>w");

  // Escape: clear status
  auto escape_action = Gio::SimpleAction::create("clear-status");
  escape_action->signal_activate().connect([this](const Glib::VariantBase&) {
    set_status(_("Ready"));
  });
  app_.add_action(escape_action);
  app_.set_accel_for_action("app.clear-status", "Escape");
}

void Window::build_ui()
{
  // ── Create pages (pass core services where needed) ────────
  dashboard_page_   = std::make_unique<DashboardPage>(*this);
  discovery_page_   = std::make_unique<DiscoveryPage>(*this);
  mount_page_       = std::make_unique<MountPage>(*this);
  mounted_page_     = std::make_unique<MountedPage>(*this);
  profiles_page_    = std::make_unique<ProfilesPage>(*this);
  diagnostics_page_ = std::make_unique<DiagnosticsPage>(*this);

  // ── Stack ─────────────────────────────────────────────────
  stack_.add(*dashboard_page_,   "dashboard-page",  "Dashboard");
  stack_.add(*discovery_page_,   "discover-page",   "Discover");
  stack_.add(*mount_page_,       "mount-page",      _("Manual Mount"));
  stack_.add(*mounted_page_,     "mounted-page",    "Mounted");
  stack_.add(*profiles_page_,    "profiles-page",   "Profiles");
  stack_.add(*diagnostics_page_, "diagnostics-page", _("Diagnostics"));

  // ── Custom sidebar: ListBox + About button ─────────────────
  sidebar_list_.set_size_request(160, -1);
  sidebar_list_.get_style_context()->add_class("navigation-sidebar");

  // Populate sidebar from stack pages
  // The stack's pages are indexed; we recreate the list to match
  struct PageInfo { std::string name; std::string title; };
  std::vector<PageInfo> pages = {
    {"dashboard-page",  "Dashboard"},
    {"discover-page",   "Discover"},
    {"mount-page",      _("Manual Mount")},
    {"mounted-page",    "Mounted"},
    {"profiles-page",   "Profiles"},
    {"diagnostics-page", _("Diagnostics")},
  };

  for (const auto& p : pages) {
    auto row = Gtk::make_managed<Gtk::Label>(p.title);
    row->set_halign(Gtk::Align::START);
    row->set_margin_start(12);
    row->set_margin_top(8);
    row->set_margin_bottom(8);

    sidebar_list_.append(*row);
  }

  sidebar_list_.signal_row_activated().connect([this, pages](Gtk::ListBoxRow* row) {
    auto idx = row->get_index();
    if (idx >= 0 && static_cast<size_t>(idx) < pages.size()) {
      stack_.set_visible_child(pages[idx].name);
    }
  });

  // About button at bottom of sidebar
  about_button_.set_margin_start(8);
  about_button_.set_margin_end(8);
  about_button_.set_margin_bottom(8);
  about_button_.set_valign(Gtk::Align::END);
  about_button_.set_vexpand(true);
  about_button_.signal_clicked().connect([this]() {
    show_about_dialog();
  });

  sidebar_box_.append(sidebar_list_);
  sidebar_box_.append(about_button_);

  // ── Assemble ──────────────────────────────────────────────
  main_box_.append(sidebar_box_);
  main_box_.append(stack_);

  statusbar_.set_margin_start(6);
  statusbar_.set_margin_end(6);
  statusbar_.push(_("Ready"));

  root_box_.append(main_box_);
  root_box_.append(statusbar_);

  set_child(root_box_);
}

void Window::set_status(const std::string& text)
{
  statusbar_.remove_all_messages();
  statusbar_.push(text);
}

void Window::switch_to_tab(const std::string& name)
{
  stack_.set_visible_child(name);
}

void Window::show_about_dialog()
{
  auto about = new Gtk::AboutDialog();
  about->set_transient_for(*this);
  about->set_program_name("Mounter");
  about->set_version("0.1.0");
  about->set_comments(
    "A GUI tool for mounting SMB/CIFS network shares on Linux.\n\n"
    "Developer: github.com/cpntodd\n\n"
    "Tech stack: C++17 · gtkmm-4.0 · GTK4 · libsecret · polkit\n"
    "Meson · Cairo · nlohmann/json");
  about->set_license_type(Gtk::License::GPL_3_0);
  about->set_website("https://github.com/cpntodd/Mounter");
  about->set_website_label("GitHub Repository");
  about->set_copyright("\302\251 2026 cpntodd");
  about->set_logo_icon_name("com.github.oddsoul.Mounter");

  // Self-delete when closed
  about->signal_close_request().connect([about]() -> bool {
    delete about;
    return false;
  }, false);

  about->set_visible(true);
  about->present();
}

} // namespace Mounter
