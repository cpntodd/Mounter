/* discovery_page.h — Network discovery page */

#pragma once

#include <gtkmm.h>
#include <memory>
#include <vector>

namespace Mounter {

class Window;
struct DiscoveredHost;
class DiscoveryEngine;

class DiscoveryPage : public Gtk::Box
{
public:
  explicit DiscoveryPage(Window& window);
  ~DiscoveryPage() override = default;

private:
  void build_ui();
  void on_scan_clicked();
  void on_results(const std::vector<DiscoveredHost>& hosts);
  void on_progress(const std::string& status);
  void on_mount_share(const std::string& server, const std::string& share);

  Window&       window_;

  // Top bar
  Gtk::Label    heading_{"Network Discovery"};
  Gtk::Entry    subnet_entry_;
  Gtk::Button   scan_button_{"Scan Network"};
  Gtk::Button   stop_button_{"Stop"};
  Gtk::Label    progress_label_;
  Gtk::Spinner  spinner_;
  Gtk::Box      top_bar_{Gtk::Orientation::HORIZONTAL, 8};

  // Results area
  Gtk::ScrolledWindow scrolled_;
  Gtk::ListBox   results_list_;
  Gtk::Label     placeholder_{"Enter a subnet and click \"Scan Network\" to discover SMB shares."};
  Gtk::Box       content_{Gtk::Orientation::VERTICAL, 8};
};

} // namespace Mounter
