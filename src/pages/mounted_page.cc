/* mounted_page.cc — Active mounts list with unmount */

#include "mounted_page.h"
#include "../window.h"
#include "../core/mount_monitor.h"
#include "../core/mount_operation.h"

namespace Mounter {

MountedPage::MountedPage(Window& window)
  : Gtk::Box{Gtk::Orientation::VERTICAL}
  , window_(window)
{
  build_ui();

  window_.mount_monitor().signal_mounts_changed().connect(
    sigc::mem_fun(*this, &MountedPage::on_mounts_changed));

  on_mounts_changed(window_.mount_monitor().active_mounts());
}

void MountedPage::build_ui()
{
  set_margin(12);
  set_spacing(12);

  heading_.set_halign(Gtk::Align::START);
  heading_.get_style_context()->add_class("title-1");

  placeholder_.set_halign(Gtk::Align::START);
  placeholder_.set_opacity(0.6);

  scrolled_.set_child(listbox_);
  scrolled_.set_vexpand(true);

  listbox_.set_selection_mode(Gtk::SelectionMode::NONE);
  listbox_.get_style_context()->add_class("rich-list");

  content_.set_spacing(8);
  content_.set_vexpand(true);
  content_.append(scrolled_);

  append(heading_);
  append(content_);
}

void MountedPage::on_mounts_changed(const std::vector<MountInfo>& mounts)
{
  while (auto* child = listbox_.get_first_child()) {
    listbox_.remove(*child);
  }

  auto count = mounts.size();
  heading_.set_text("Mounted Shares (" + std::to_string(count) + ")");

  if (mounts.empty()) {
    if (placeholder_.get_parent() == nullptr) {
      content_.prepend(placeholder_);
    }
    scrolled_.set_visible(false);
    return;
  }

  if (placeholder_.get_parent() != nullptr) {
    content_.remove(placeholder_);
  }
  scrolled_.set_visible(true);

  for (const auto& m : mounts) {
    auto row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 12);
    row->set_margin_start(8);
    row->set_margin_end(8);
    row->set_margin_top(4);
    row->set_margin_bottom(4);

    auto info_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 2);
    info_box->set_hexpand(true);

    auto share_label = Gtk::make_managed<Gtk::Label>(
      "//" + m.server + "/" + m.share);
    share_label->set_halign(Gtk::Align::START);
    share_label->get_style_context()->add_class("heading");

    auto mount_label = Gtk::make_managed<Gtk::Label>(
      "\342\206\222 " + m.mount_point);
    mount_label->set_halign(Gtk::Align::START);
    mount_label->set_opacity(0.7);

    info_box->append(*share_label);
    info_box->append(*mount_label);

    auto unmount_btn = Gtk::make_managed<Gtk::Button>("Unmount");
    unmount_btn->set_valign(Gtk::Align::CENTER);
    unmount_btn->get_style_context()->add_class("destructive-action");

    auto mount_point = m.mount_point;
    unmount_btn->signal_clicked().connect([this, mount_point]() {
      unmount_share(mount_point);
    });

    row->append(*info_box);
    row->append(*unmount_btn);
    listbox_.append(*row);
  }
}

void MountedPage::unmount_share(const std::string& mount_point)
{
  window_.set_status("Unmounting " + mount_point + "...");

  window_.mount_operation().unmount_async(mount_point,
    [this, mount_point](const auto& result) {
      if (result.success) {
        window_.set_status("Unmounted " + mount_point);
      } else {
        window_.set_status("Failed to unmount: " + result.error_message);
      }
    });
}

} // namespace Mounter
