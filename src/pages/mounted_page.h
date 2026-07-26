/* mounted_page.h — Active mounts list
 *
 * Displays currently mounted SMB/CIFS shares and allows unmounting.
 */

#pragma once

#include <gtkmm.h>

namespace Mounter {

class MountedPage : public Gtk::Box
{
public:
  MountedPage();
  ~MountedPage() override = default;

private:
  void build_ui();

  Gtk::Label  heading_{"Mounted Shares"};
  Gtk::Label  placeholder_{"No SMB shares currently mounted."};
  Gtk::Box    content_{Gtk::Orientation::VERTICAL};
};

} // namespace Mounter
