/* window.h — Mounter::Window
 *
 * The main application window. Contains a Gtk::Stack with 5 pages
 * navigated via a Gtk::StackSidebar.
 */

#pragma once

#include <gtkmm.h>

namespace Mounter {

class Application;
class DiscoveryPage;
class MountPage;
class MountedPage;
class ProfilesPage;
class DiagnosticsPage;

class Window : public Gtk::ApplicationWindow
{
public:
  explicit Window(Application& app);
  ~Window() override;  // defined in .cc to allow incomplete page types

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  void show_about_dialog();

private:
  void build_ui();

  Application& app_;

  // Pages
  std::unique_ptr<DiscoveryPage>    discovery_page_;
  std::unique_ptr<MountPage>        mount_page_;
  std::unique_ptr<MountedPage>      mounted_page_;
  std::unique_ptr<ProfilesPage>     profiles_page_;
  std::unique_ptr<DiagnosticsPage>  diagnostics_page_;

  // Layout
  Gtk::Stack       stack_;
  Gtk::StackSidebar sidebar_;
  Gtk::Box          main_box_{Gtk::Orientation::HORIZONTAL};
  Gtk::Statusbar    statusbar_;
  Gtk::Box          root_box_{Gtk::Orientation::VERTICAL};
};

} // namespace Mounter
