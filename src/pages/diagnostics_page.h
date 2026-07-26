/* diagnostics_page.h — System diagnostics page */

#pragma once

#include <gtkmm.h>
#include <vector>
#include <string>

namespace Mounter {

class DiagnosticsPage : public Gtk::Box
{
public:
  DiagnosticsPage();
  ~DiagnosticsPage() override = default;

private:
  void build_ui();
  void run_checks();
  void install_missing();

  struct CheckItem {
    std::string name;
    std::string binary;
    std::string pkg;      // Debian package name
    bool        required;
    bool        found = false;
  };

  std::vector<CheckItem> checks_;

  Gtk::Label    heading_{"Diagnostics"};
  Gtk::Button   check_button_{"Run Checks"};
  Gtk::Button   install_button_{"Install"};
  Gtk::Box      button_row_{Gtk::Orientation::HORIZONTAL, 8};
  Gtk::Box      results_{Gtk::Orientation::VERTICAL};
  Gtk::Spinner  spinner_;
  Gtk::Box      content_{Gtk::Orientation::VERTICAL};
};

} // namespace Mounter
