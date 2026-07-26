/* diagnostics_page.cc */

#include "diagnostics_page.h"
#include "i18n.h"
#include <cstdlib>

namespace Mounter {

DiagnosticsPage::DiagnosticsPage()
  : Gtk::Box{Gtk::Orientation::VERTICAL}
{
  build_ui();
}

void DiagnosticsPage::build_ui()
{
  set_margin(12);
  set_spacing(12);

  heading_.set_halign(Gtk::Align::START);
  heading_.get_style_context()->add_class("title-1");

  check_button_.set_halign(Gtk::Align::START);
  check_button_.signal_clicked().connect(sigc::mem_fun(*this, &DiagnosticsPage::run_checks));

  results_.set_spacing(4);
  results_.set_margin_top(8);

  content_.set_spacing(8);
  content_.append(check_button_);
  content_.append(results_);

  append(heading_);
  append(content_);
}

void DiagnosticsPage::run_checks()
{
  // Clear existing results
  while (auto* child = results_.get_first_child()) {
    results_.remove(*child);
  }

  struct CheckItem {
    const char* name;
    const char* binary;
    bool required;
  };

  const CheckItem checks[] = {
    {"cifs-utils (mount.cifs)", "mount.cifs", true},
    {"pkexec (polkit)",         "pkexec",     true},
    {"smbclient (share list)",  "smbclient",  true},
    {"nmap (network scan)",     "nmap",       true},
    {"systemctl (systemd)",     "systemctl",  true},
    {"secret-tool (keyring)",   "secret-tool",false},
    {"nmblookup (NetBIOS)",     "nmblookup",  false},
  };

  for (const auto& check : checks) {
    // Use 'which' to check if the binary exists in PATH
    std::string cmd = std::string{"which "} + check.binary + " > /dev/null 2>&1";
    int ret = std::system(cmd.c_str());
    bool found = (ret == 0);

    auto row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);

    auto icon = Gtk::make_managed<Gtk::Label>(found ? "✅" : "❌");
    icon->set_width_chars(2);

    auto label = Gtk::make_managed<Gtk::Label>(
      std::string{check.name} + (check.required ? " (required)" : " (optional)"));
    label->set_halign(Gtk::Align::START);
    label->set_opacity(found ? 1.0 : 0.7);

    row->append(*icon);
    row->append(*label);
    results_.append(*row);
  }
}

} // namespace Mounter
