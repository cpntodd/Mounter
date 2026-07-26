/* profiles_page.h — Saved connection profiles
 *
 * Manages saved SMB connection profiles for quick re-mounting.
 */

#pragma once

#include <gtkmm.h>

namespace Mounter {

class ProfilesPage : public Gtk::Box
{
public:
  ProfilesPage();
  ~ProfilesPage() override = default;

private:
  void build_ui();

  Gtk::Label  heading_{"Saved Profiles"};
  Gtk::Label  placeholder_{"No saved profiles. Mount a share and save it as a profile for quick access."};
  Gtk::Box    content_{Gtk::Orientation::VERTICAL};
};

} // namespace Mounter
