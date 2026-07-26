/* style_manager.h — Mounter::StyleManager
 *
 * Singleton that detects the desktop environment and decides
 * whether to load libadwaita theming. Supports explicit override
 * via the --style= CLI flag.
 */

#pragma once

#include <string>

namespace Mounter {

class StyleManager
{
public:
  static StyleManager& instance();

  /// Detect the DE and configure styling. Call once at startup.
  void initialize(const std::string& style_override);

  /// Returns true if libadwaita (GNOME) styling is active.
  bool is_adwaita() const { return use_adwaita_; }

  /// Returns a CSS class suffix for the current style: "adwaita" or "plain".
  const std::string& style_name() const { return style_name_; }

private:
  StyleManager() = default;

  // Returns "gnome", "kde", "xfce", "sway", etc.
  static std::string detect_desktop_env();

  bool use_adwaita_ = false;
  std::string style_name_ = "plain";
};

} // namespace Mounter
