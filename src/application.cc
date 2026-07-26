/* application.cc — Mounter::Application implementation */

#include "application.h"
#include "window.h"
#include "ui/style_manager.h"
#include <iostream>

namespace Mounter {

Glib::RefPtr<Application> Application::create()
{
  return Glib::make_refptr_for_instance<Application>(new Application{});
}

Application::Application()
  : Gtk::Application("com.github.oddsoul.Mounter",
                     Gio::Application::Flags::NONE)
{
  // Application ID is used for D-Bus, desktop file matching, and settings path
}

void Application::on_startup()
{
  Gtk::Application::on_startup();

  // Register actions
  add_action("about", [this]() {
    if (window_) {
      window_->show_about_dialog();
    }
  });
  add_action("quit", [this]() {
    quit();
  });

  // Set up accelerator for quit
  set_accel_for_action("app.quit", "<Control>q");

  // Initialize style
  StyleManager::instance().initialize(style_override_);
}

void Application::on_activate()
{
  if (!window_) {
    window_ = std::make_unique<Window>(*this);
    add_window(*window_);
  }

  // Explicitly show before present — required on some Wayland compositors
  window_->set_visible(true);
  window_->present();
}

void Application::set_style_override(const std::string& style)
{
  style_override_ = style;
}

} // namespace Mounter
