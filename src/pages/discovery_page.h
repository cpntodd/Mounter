/* discovery_page.h — Network discovery page
 *
 * Scans the local network for SMB hosts, lists available shares,
 * and allows one-click mounting.
 */

#pragma once

#include <gtkmm.h>

namespace Mounter {

class DiscoveryPage : public Gtk::Box
{
public:
  DiscoveryPage();
  ~DiscoveryPage() override = default;

private:
  void build_ui();

  Gtk::Label  heading_{"Network Discovery"};
  Gtk::Button scan_button_{"Scan Network"};
  Gtk::Label  placeholder_{"Click \"Scan Network\" to discover SMB shares on your local network."};
  Gtk::Box    content_{Gtk::Orientation::VERTICAL};
};

} // namespace Mounter
