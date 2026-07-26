/* style_manager.cc — Mounter::StyleManager implementation */

#include "style_manager.h"
#include <cstdlib>

#ifdef HAVE_LIBADWAITA
  #include <adwaita.h>
#endif

namespace Mounter {

StyleManager& StyleManager::instance()
{
  static StyleManager mgr;
  return mgr;
}

std::string StyleManager::detect_desktop_env()
{
  // Check environment variables in priority order
  const char* xdg = std::getenv("XDG_CURRENT_DESKTOP");
  const char* desktop = std::getenv("DESKTOP_SESSION");
  const char* gdm_session = std::getenv("GDMSESSION");

  auto contains_ignore_case = [](const char* str, const char* substr) -> bool {
    if (!str) return false;
    std::string s{str};
    std::string sub{substr};
    // Simple case-insensitive search
    auto it = std::search(s.begin(), s.end(), sub.begin(), sub.end(),
      [](char a, char b) { return std::tolower(a) == std::tolower(b); });
    return it != s.end();
  };

  // Check known desktop identifiers
  if (xdg) {
    std::string s{xdg};
    if (contains_ignore_case(xdg, "gnome") ||
        contains_ignore_case(xdg, "pantheon") ||
        contains_ignore_case(xdg, "budgie")) {
      return "gnome";
    }
    if (contains_ignore_case(xdg, "kde") ||
        contains_ignore_case(xdg, "plasma")) {
      return "kde";
    }
    if (contains_ignore_case(xdg, "xfce")) return "xfce";
    if (contains_ignore_case(xdg, "sway"))  return "sway";
    if (contains_ignore_case(xdg, "hyprland")) return "hyprland";
    if (contains_ignore_case(xdg, "mate"))  return "mate";
    if (contains_ignore_case(xdg, "cinnamon")) return "cinnamon";
  }

  if (desktop) {
    if (contains_ignore_case(desktop, "gnome")) return "gnome";
    if (contains_ignore_case(desktop, "plasma")) return "kde";
    if (contains_ignore_case(desktop, "xfce")) return "xfce";
  }

  return "unknown";
}

void StyleManager::initialize(const std::string& style_override)
{
  auto de = detect_desktop_env();

  if (!style_override.empty()) {
    if (style_override == "adwaita") {
      use_adwaita_ = true;
      style_name_ = "adwaita";
    } else {
      use_adwaita_ = false;
      style_name_ = "plain";
    }
  } else {
    // Auto-detect: use libadwaita on GNOME, plain GTK4 elsewhere
    if (de == "gnome" || de == "pantheon" || de == "budgie") {
#ifdef HAVE_LIBADWAITA
      use_adwaita_ = true;
#else
      use_adwaita_ = false;
#endif
      style_name_ = use_adwaita_ ? "adwaita" : "plain";
    } else {
      use_adwaita_ = false;
      style_name_ = "plain";
    }
  }

#ifdef HAVE_LIBADWAITA
  if (use_adwaita_) {
    adw_init();
  }
#endif
}

} // namespace Mounter
