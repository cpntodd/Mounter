/* window.cc — Mounter::Window implementation */

#include "window.h"
#include "application.h"
#include "pages/discovery_page.h"
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

  build_ui();

  // Default to the manual mount page
  stack_.set_visible_child("mount-page");
}

Window::~Window() = default;

void Window::build_ui()
{
  // ── Create pages ──────────────────────────────────────────
  discovery_page_   = std::make_unique<DiscoveryPage>();
  mount_page_       = std::make_unique<MountPage>();
  mounted_page_     = std::make_unique<MountedPage>();
  profiles_page_    = std::make_unique<ProfilesPage>();
  diagnostics_page_ = std::make_unique<DiagnosticsPage>();

  // ── Stack: named pages ────────────────────────────────────
  stack_.add(*discovery_page_,   "discover-page",   "Discover");
  stack_.add(*mount_page_,       "mount-page",      "Manual Mount");
  stack_.add(*mounted_page_,     "mounted-page",    "Mounted");
  stack_.add(*profiles_page_,    "profiles-page",   "Profiles");
  stack_.add(*diagnostics_page_, "diagnostics-page", "Diagnostics");

  // ── Sidebar ───────────────────────────────────────────────
  sidebar_.set_stack(stack_);
  sidebar_.set_size_request(180, -1);

  // ── Assemble ──────────────────────────────────────────────
  main_box_.append(sidebar_);
  main_box_.append(stack_);

  statusbar_.set_margin_start(6);
  statusbar_.set_margin_end(6);
  statusbar_.push("Ready");

  root_box_.append(main_box_);
  root_box_.append(statusbar_);

  set_child(root_box_);
}

void Window::show_about_dialog()
{
  auto about = Gtk::AboutDialog{};
  about.set_transient_for(*this);
  about.set_program_name("Mounter");
  about.set_version("0.1.0");
  about.set_comments("A GUI tool for mounting SMB/CIFS network shares");
  about.set_license_type(Gtk::License::GPL_3_0);
  about.set_website("https://github.com/oddsoul/mounter");
  about.set_copyright("© 2026 Oddsoul");
  about.set_logo_icon_name("com.github.oddsoul.Mounter");

  about.present();
}

} // namespace Mounter
