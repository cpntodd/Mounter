/* mounted_page.cc */

#include "mounted_page.h"

namespace Mounter {

MountedPage::MountedPage()
  : Gtk::Box{Gtk::Orientation::VERTICAL}
{
  build_ui();
}

void MountedPage::build_ui()
{
  set_margin(12);
  set_spacing(12);

  heading_.set_halign(Gtk::Align::START);
  heading_.get_style_context()->add_class("title-1");

  placeholder_.set_halign(Gtk::Align::START);
  placeholder_.set_opacity(0.6);

  content_.set_spacing(8);
  content_.append(placeholder_);

  append(heading_);
  append(content_);
}

} // namespace Mounter
