/* profiles_page.cc */

#include "profiles_page.h"

namespace Mounter {

ProfilesPage::ProfilesPage()
  : Gtk::Box{Gtk::Orientation::VERTICAL}
{
  build_ui();
}

void ProfilesPage::build_ui()
{
  set_margin(12);
  set_spacing(12);

  heading_.set_halign(Gtk::Align::START);
  heading_.get_style_context()->add_class("title-1");

  placeholder_.set_halign(Gtk::Align::START);
  placeholder_.set_opacity(0.6);
  placeholder_.set_wrap(true);

  content_.set_spacing(8);
  content_.append(placeholder_);

  append(heading_);
  append(content_);
}

} // namespace Mounter
