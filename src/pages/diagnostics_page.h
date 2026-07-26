/* diagnostics_page.h — System diagnostics page
 *
 * Checks for required and optional system dependencies and displays
 * their installation status.
 */

#pragma once

#include <gtkmm.h>

namespace Mounter {

class DiagnosticsPage : public Gtk::Box
{
public:
  DiagnosticsPage();
  ~DiagnosticsPage() override = default;

private:
  void build_ui();
  void run_checks();

  Gtk::Label  heading_{"Diagnostics"};
  Gtk::Button check_button_{"Run Checks"};
  Gtk::Box    results_{Gtk::Orientation::VERTICAL};
  Gtk::Box    content_{Gtk::Orientation::VERTICAL};
};

} // namespace Mounter
