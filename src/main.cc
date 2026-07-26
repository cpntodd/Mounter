/* main.cc — Entry point for Mounter
 *
 * Parses CLI arguments, initializes the Gtk::Application, and runs the main loop.
 *
 * Usage:
 *   mounter                     Run with auto-detected styling
 *   mounter --style=adwaita     Force libadwaita (GNOME) styling
 *   mounter --style=plain       Force plain GTK4 styling (works on any DE)
 */

#include "application.h"
#include <iostream>

int main(int argc, char* argv[])
{
  auto app = Mounter::Application::create();

  // Parse our custom flags before GTK consumes argv
  for (int i = 1; i < argc; ++i) {
    std::string_view arg{argv[i]};
    if (arg.rfind("--style=", 0) == 0) {
      auto style = arg.substr(8); // after "--style="
      app->set_style_override(std::string{style});
    } else if (arg == "--help" || arg == "-h") {
      std::cout << R"(Usage: mounter [OPTIONS]

A GUI tool for mounting SMB/CIFS network shares on Linux.

Options:
  --style=adwaita   Force libadwaita (GNOME) styling
  --style=plain     Force plain GTK4 styling (works on any DE)
  --help, -h        Show this help message
)" << std::endl;
      return 0;
    }
  }

  return app->run(argc, argv);
}
