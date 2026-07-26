/* application.h — Mounter::Application
 *
 * Subclass of Gtk::Application. Handles CLI parsing, style detection,
 * and primary window lifecycle.
 */

#pragma once

#include <gtkmm.h>
#include <string>
#include <memory>

namespace Mounter {

class Window;

class Application : public Gtk::Application
{
public:
  static Glib::RefPtr<Application> create();

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;
  ~Application() override = default;

  /// Override the auto-detected style. Valid values: "adwaita", "plain".
  void set_style_override(const std::string& style);
  const std::string& style_override() const { return style_override_; }

protected:
  Application();

  void on_startup() override;
  void on_activate() override;

private:
  std::string style_override_;
  std::unique_ptr<Window> window_;
};

} // namespace Mounter
