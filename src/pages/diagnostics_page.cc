/* diagnostics_page.cc — System dependency checks + auto-install */

#include "diagnostics_page.h"
#include "i18n.h"
#include <giomm/subprocess.h>
#include <cstdlib>
#include <sstream>
#include <thread>
#include <unistd.h>

namespace Mounter {

DiagnosticsPage::DiagnosticsPage()
  : Gtk::Box{Gtk::Orientation::VERTICAL}
{
  // Define all checks with their Debian package mappings
  checks_ = {
    {"cifs-utils (mount.cifs)", "mount.cifs", "cifs-utils",       true},
    {"pkexec (polkit)",         "pkexec",     "pkexec",            true},
    {"smbclient (share list)",  "smbclient",  "smbclient",         true},
    {"nmap (network scan)",     "nmap",       "nmap",              true},
    {"systemctl (systemd)",     "systemctl",  "systemd",           true},
    {"secret-tool (keyring)",   "secret-tool","libsecret-tools",   false},
    {"nmblookup (NetBIOS)",     "nmblookup",  "samba-common-bin",  false},
  };

  build_ui();
  run_checks();
}

void DiagnosticsPage::build_ui()
{
  set_margin(12);
  set_spacing(12);

  heading_.set_halign(Gtk::Align::START);
  heading_.get_style_context()->add_class("title-1");

  check_button_.set_halign(Gtk::Align::START);
  check_button_.signal_clicked().connect(
    sigc::mem_fun(*this, &DiagnosticsPage::run_checks));

  install_button_.set_halign(Gtk::Align::START);
  install_button_.set_sensitive(false);  // disabled until checks run
  install_button_.signal_clicked().connect(
    sigc::mem_fun(*this, &DiagnosticsPage::install_missing));

  button_row_.append(check_button_);
  button_row_.append(install_button_);
  button_row_.append(spinner_);

  results_.set_spacing(4);
  results_.set_margin_top(8);

  content_.set_spacing(8);
  content_.append(button_row_);
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

  bool any_missing = false;

  for (auto& check : checks_) {
    // Check multiple common binary paths — /sbin and /usr/sbin aren't
    // in regular users' PATH but contain system binaries like mount.cifs
    static const char* paths[] = {
      "/usr/bin/", "/usr/sbin/", "/sbin/", "/bin/", nullptr
    };
    check.found = false;
    for (int i = 0; paths[i] != nullptr; ++i) {
      std::string full_path = std::string{paths[i]} + check.binary;
      if (access(full_path.c_str(), X_OK) == 0) {
        check.found = true;
        break;
      }
    }
    // Also try 'which' as fallback for PATH-based binaries
    if (!check.found) {
      std::string cmd = std::string{"which "} + check.binary + " > /dev/null 2>&1";
      check.found = (std::system(cmd.c_str()) == 0);
    }

    if (!check.found) any_missing = true;

    auto row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);

    auto icon = Gtk::make_managed<Gtk::Label>(check.found ? "✅" : "❌");
    icon->set_width_chars(2);

    auto label = Gtk::make_managed<Gtk::Label>(
      check.name + (check.required ? " (required)" : " (optional)"));
    label->set_halign(Gtk::Align::START);
    label->set_opacity(check.found ? 1.0 : 0.7);

    row->append(*icon);
    row->append(*label);
    results_.append(*row);
  }

  install_button_.set_sensitive(any_missing);
}

void DiagnosticsPage::install_missing()
{
  // Collect missing package names
  std::vector<std::string> missing_pkgs;
  for (const auto& check : checks_) {
    if (!check.found && !check.pkg.empty()) {
      // Avoid duplicates (e.g., smbclient and samba-common-bin overlap)
      bool dup = false;
      for (const auto& p : missing_pkgs) {
        if (p == check.pkg) { dup = true; break; }
      }
      if (!dup) missing_pkgs.push_back(check.pkg);
    }
  }

  if (missing_pkgs.empty()) {
    return;
  }

  // Build install command
  std::ostringstream cmd;
  cmd << "apt-get install -y";
  for (const auto& pkg : missing_pkgs) {
    cmd << " " << pkg;
  }

  // UI feedback
  install_button_.set_sensitive(false);
  check_button_.set_sensitive(false);
  spinner_.start();

  // Run in background thread so GUI stays responsive
  std::thread([this, cmd_str = cmd.str()]() {
    try {
      auto proc = Gio::Subprocess::create(
        std::vector<std::string>{"pkexec", "sh", "-c", cmd_str},
        Gio::Subprocess::Flags::STDOUT_SILENCE |
        Gio::Subprocess::Flags::STDERR_SILENCE);
      proc->wait();

      // Re-run checks on main thread
      Glib::signal_idle().connect_once([this]() {
        spinner_.stop();
        install_button_.set_sensitive(false);
        check_button_.set_sensitive(true);
        run_checks();
      });

    } catch (const Glib::Error&) {
      Glib::signal_idle().connect_once([this]() {
        spinner_.stop();
        install_button_.set_sensitive(true);
        check_button_.set_sensitive(true);
      });
    }
  }).detach();
}

} // namespace Mounter
