/* mounted_page.h — Active mounts list with unmount capability */

#pragma once

#include <gtkmm.h>
#include <memory>
#include <vector>

namespace Mounter {

class Window;
struct MountInfo;

class MountedPage : public Gtk::Box
{
public:
  explicit MountedPage(Window& window);
  ~MountedPage() override = default;

private:
  void build_ui();
  void on_mounts_changed(const std::vector<MountInfo>& mounts);
  void unmount_share(const std::string& mount_point);

  Window&       window_;
  Gtk::Label    heading_{"Mounted Shares"};
  Gtk::ListBox  listbox_;
  Gtk::Label    placeholder_{"No SMB shares currently mounted."};
  Gtk::Box      content_{Gtk::Orientation::VERTICAL};
  Gtk::ScrolledWindow scrolled_;
};

} // namespace Mounter
