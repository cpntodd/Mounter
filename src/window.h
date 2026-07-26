/* window.h — Mounter::Window */

#pragma once

#include <gtkmm.h>
#include <memory>

namespace Mounter {

class Application;
class DashboardPage;
class DiscoveryPage;
class MountPage;
class MountedPage;
class ProfilesPage;
class DiagnosticsPage;
class MountMonitor;
class MountOperation;
class CredentialStore;

class Window : public Gtk::ApplicationWindow
{
public:
  explicit Window(Application& app);
  ~Window() override;

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  void show_about_dialog();
  void setup_shortcuts();

  // Expose core services to pages
  MountMonitor&     mount_monitor()     { return *mount_monitor_; }
  MountOperation&   mount_operation()   { return *mount_operation_; }
  CredentialStore&  credential_store()  { return *credential_store_; }

  // Expose pages for cross-tab coordination
  MountPage&        mount_page()        { return *mount_page_; }
  ProfilesPage&     profiles_page()     { return *profiles_page_; }

  // Tab navigation
  void switch_to_tab(const std::string& name);

  // Status bar
  void set_status(const std::string& text);

private:
  void build_ui();

  Application& app_;

  // Core services (owned by window, shared with pages)
  std::unique_ptr<MountMonitor>     mount_monitor_;
  std::unique_ptr<MountOperation>   mount_operation_;
  std::unique_ptr<CredentialStore>  credential_store_;

  // Pages
  std::unique_ptr<DashboardPage>    dashboard_page_;
  std::unique_ptr<DiscoveryPage>    discovery_page_;
  std::unique_ptr<MountPage>        mount_page_;
  std::unique_ptr<MountedPage>      mounted_page_;
  std::unique_ptr<ProfilesPage>     profiles_page_;
  std::unique_ptr<DiagnosticsPage>  diagnostics_page_;

  // Layout
  Gtk::Stack        stack_;
  Gtk::Box          sidebar_box_{Gtk::Orientation::VERTICAL};
  Gtk::ListBox      sidebar_list_;
  Gtk::Button       about_button_{"About"};
  Gtk::Box          main_box_{Gtk::Orientation::HORIZONTAL};
  Gtk::Statusbar    statusbar_;
  Gtk::Box          root_box_{Gtk::Orientation::VERTICAL};
};

} // namespace Mounter
