/* dashboard_page.cc — Dashboard implementation */

#include "dashboard_page.h"
#include "i18n.h"
#include "../window.h"
#include "../core/mount_monitor.h"
#include "../ui/pie_chart.h"

#include <giomm/subprocess.h>
#include <sstream>
#include <cmath>
#include <thread>

namespace Mounter {

DashboardPage::DashboardPage(Window& window)
  : Gtk::Box{Gtk::Orientation::VERTICAL}
  , window_(window)
{
  build_ui();
  refresh();

  // Auto-refresh mount status every 5 seconds
  refresh_timer_ = Glib::signal_timeout().connect_seconds(
    sigc::mem_fun(*this, &DashboardPage::refresh), 5);
}

void DashboardPage::build_ui()
{
  set_margin(16);
  set_spacing(16);

  heading_.set_halign(Gtk::Align::START);
  heading_.get_style_context()->add_class("title-1");

  // ── Status cards row ──────────────────────────────────────
  auto cards_row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 12);
  cards_row->set_homogeneous(true);
  build_status_cards(*cards_row);

  // ── Quick actions row ─────────────────────────────────────
  auto actions_label = Gtk::make_managed<Gtk::Label>(_("Quick Actions"));
  actions_label->set_halign(Gtk::Align::START);
  actions_label->get_style_context()->add_class("title-2");
  actions_label->set_margin_top(8);

  auto actions_row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 12);
  actions_row->set_homogeneous(true);
  build_quick_actions(*actions_row);

  append(heading_);
  append(*cards_row);
  append(*actions_label);
  append(*actions_row);
}

// ── Status cards ─────────────────────────────────────────────

static void style_card(Gtk::Box& card)
{
  card.set_orientation(Gtk::Orientation::VERTICAL);
  card.set_spacing(6);
  card.set_margin_start(10);
  card.set_margin_end(10);
  card.set_margin_top(10);
  card.set_margin_bottom(10);
  card.get_style_context()->add_class("card");
  // Add a subtle frame via CSS class
  card.set_name("dashboard-card");
}

void DashboardPage::build_status_cards(Gtk::Box& row)
{
  // ── Card 1: Mounted Shares ────────────────────────────────
  auto mount_card = Gtk::make_managed<Gtk::Box>();
  style_card(*mount_card);

  auto mount_icon = Gtk::make_managed<Gtk::Label>("\360\237\226\245"); // 🖥
  mount_icon->set_halign(Gtk::Align::START);
  mount_count_label_.set_halign(Gtk::Align::START);
  mount_count_label_.get_style_context()->add_class("title-2");
  mount_detail_label_.set_halign(Gtk::Align::START);
  mount_detail_label_.set_opacity(0.6);

  mount_card->append(*mount_icon);
  mount_card->append(mount_count_label_);
  mount_card->append(mount_detail_label_);

  // ── Card 2: Server Reachability ───────────────────────────
  auto server_card = Gtk::make_managed<Gtk::Box>();
  style_card(*server_card);

  auto server_icon = Gtk::make_managed<Gtk::Label>("\360\237\214\220"); // 🌐
  server_icon->set_halign(Gtk::Align::START);

  server_indicator_.set_size_request(12, 12);
  server_indicator_.set_draw_func([](const Cairo::RefPtr<Cairo::Context>& cr, int w, int h) {
    cr->set_source_rgba(0.5, 0.5, 0.5, 1.0);
    cr->arc(w/2.0, h/2.0, w/2.0 - 1, 0, 2 * M_PI);
    cr->fill();
  });

  server_dot_.append(server_indicator_);
  server_dot_.append(server_status_label_);

  server_status_label_.set_halign(Gtk::Align::START);
  server_detail_label_.set_halign(Gtk::Align::START);
  server_detail_label_.set_opacity(0.6);

  server_card->append(*server_icon);
  server_card->append(server_dot_);
  server_card->append(server_detail_label_);

  // ── Card 3: Disk Usage ────────────────────────────────────
  auto disk_card = Gtk::make_managed<Gtk::Box>();
  style_card(*disk_card);

  auto disk_icon = Gtk::make_managed<Gtk::Label>("\360\237\222\276"); // 💾
  disk_icon->set_halign(Gtk::Align::START);

  pie_chart_ = std::make_unique<PieChart>();
  disk_pct_label_.set_halign(Gtk::Align::CENTER);
  disk_pct_label_.set_opacity(0.6);

  disk_card->append(*disk_icon);
  disk_card->append(*pie_chart_);
  disk_card->append(disk_pct_label_);

  row.append(*mount_card);
  row.append(*server_card);
  row.append(*disk_card);
}

// ── Quick actions ────────────────────────────────────────────

static Gtk::Button* make_action_btn(const std::string& emoji,
                                     const std::string& label)
{
  auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
  box->set_margin_top(12);
  box->set_margin_bottom(12);

  auto icon = Gtk::make_managed<Gtk::Label>(emoji);
  icon->set_halign(Gtk::Align::CENTER);
  auto text = Gtk::make_managed<Gtk::Label>(label);
  text->set_halign(Gtk::Align::CENTER);

  box->append(*icon);
  box->append(*text);

  auto btn = Gtk::make_managed<Gtk::Button>();
  btn->set_child(*box);
  btn->get_style_context()->add_class("flat");
  return btn;
}

void DashboardPage::build_quick_actions(Gtk::Box& row)
{
  auto mount_btn = make_action_btn("\360\237\224\227", _("Mount Share"));  // 🔗
  mount_btn->signal_clicked().connect([this]() {
    window_.switch_to_tab("mount-page");
  });

  auto scan_btn = make_action_btn("\360\237\224\215", _("Scan Network"));  // 🔍
  scan_btn->signal_clicked().connect([this]() {
    window_.switch_to_tab("discover-page");
  });

  auto browse_btn = make_action_btn("\360\237\223\202", _("Browse Mounts")); // 📂
  browse_btn->signal_clicked().connect([this]() {
    window_.switch_to_tab("mounted-page");
  });

  auto profiles_btn = make_action_btn("\342\255\220", _("Profiles"));        // ⭐
  profiles_btn->signal_clicked().connect([this]() {
    window_.switch_to_tab("profiles-page");
  });

  row.append(*mount_btn);
  row.append(*scan_btn);
  row.append(*browse_btn);
  row.append(*profiles_btn);
}

// ── Refresh logic ────────────────────────────────────────────

bool DashboardPage::refresh()
{
  update_mount_status();
  update_server_status();
  update_disk_usage();
  return true;  // keep timer running
}

void DashboardPage::update_mount_status()
{
  auto mounts = window_.mount_monitor().active_mounts();
  auto count = mounts.size();

  mount_count_label_.set_text(
    std::to_string(count) + " share" + (count != 1 ? "s" : "") + " mounted");

  if (count == 0) {
    mount_detail_label_.set_text(_("No SMB shares connected."));
  } else {
    std::string detail;
    for (size_t i = 0; i < mounts.size() && i < 3; ++i) {
      if (i > 0) detail += ", ";
      detail += mounts[i].server + "/" + mounts[i].share;
    }
    if (mounts.size() > 3) detail += " +" + std::to_string(mounts.size() - 3) + " more";
    mount_detail_label_.set_text(detail);
  }
}

void DashboardPage::update_server_status()
{
  // Check the most recently mounted server (or first active mount)
  auto mounts = window_.mount_monitor().active_mounts();

  if (mounts.empty()) {
    server_status_label_.set_text(_("No server"));
    server_detail_label_.set_text(_("Mount a share to monitor status."));

    server_indicator_.set_draw_func([](const Cairo::RefPtr<Cairo::Context>& cr, int w, int h) {
      cr->set_source_rgba(0.5, 0.5, 0.5, 1.0);
      cr->arc(w/2.0, h/2.0, w/2.0 - 1, 0, 2 * M_PI);
      cr->fill();
    });
    return;
  }

  auto server = mounts[0].server;
  server_status_label_.set_text(server);

  // Ping the server in background
  std::thread([this, server]() {
    auto proc = Gio::Subprocess::create(
      std::vector<std::string>{"ping", "-c", "1", "-W", "2", server},
      Gio::Subprocess::Flags::STDOUT_SILENCE | Gio::Subprocess::Flags::STDERR_SILENCE);
    proc->wait();
    bool reachable = (proc->get_exit_status() == 0);

    Glib::signal_idle().connect_once([this, reachable, server]() {
      server_indicator_.set_draw_func([reachable](const Cairo::RefPtr<Cairo::Context>& cr, int w, int h) {
        if (reachable)
          cr->set_source_rgba(0.2, 0.8, 0.3, 1.0);  // green
        else
          cr->set_source_rgba(0.9, 0.3, 0.2, 1.0);   // red
        cr->arc(w/2.0, h/2.0, w/2.0 - 1, 0, 2 * M_PI);
        cr->fill();
      });
      server_detail_label_.set_text(reachable ? _("Online") : _("Offline"));
    });
  }).detach();
}

void DashboardPage::update_disk_usage()
{
  auto mounts = window_.mount_monitor().active_mounts();

  if (mounts.empty()) {
    pie_chart_->clear();
    disk_pct_label_.set_text(_("No mounts"));
    return;
  }

  // Run df in background to get usage of first mount
  auto mount_point = mounts[0].mount_point;

  std::thread([this, mount_point]() {
    try {
      auto proc = Gio::Subprocess::create(
        std::vector<std::string>{"df", "-h", mount_point},
        Gio::Subprocess::Flags::STDOUT_PIPE | Gio::Subprocess::Flags::STDERR_SILENCE);

      auto [stdout_bytes, stderr_bytes] = proc->communicate(nullptr, nullptr);
      proc->wait();

      if (stdout_bytes && proc->get_exit_status() == 0) {
        gsize size;
        const auto* data = static_cast<const char*>(stdout_bytes->get_data(size));
        std::string output(data, size);

        // Parse df output: Filesystem ... Use% Mounted on
        // Last line contains the mount info
        std::istringstream iss(output);
        std::string line, last_line;
        while (std::getline(iss, line)) {
          if (!line.empty()) last_line = line;
        }

        // Extract Use% (second-to-last field typically)
        std::istringstream lss(last_line);
        std::string field;
        std::vector<std::string> fields;
        while (lss >> field) fields.push_back(field);

        if (fields.size() >= 5) {
          std::string pct_str;
          for (const auto& f : fields) {
            if (f.find('%') != std::string::npos) {
              pct_str = f;
              break;
            }
          }

          if (!pct_str.empty() && pct_str.back() == '%') {
            pct_str.pop_back();
            double used = std::stod(pct_str);

            Glib::signal_idle().connect_once([this, used]() {
              Gdk::RGBA blue{};
              blue.set_rgba(0.21, 0.51, 0.89, 1.0);  // #3584e4
              Gdk::RGBA gray{};
              gray.set_rgba(0.3, 0.3, 0.3, 1.0);

              pie_chart_->set_data({
                {used, "Used", blue},
                {100.0 - used, "Free", gray}
              });
              disk_pct_label_.set_text(std::to_string(static_cast<int>(used)) + "% used");
            });
          }
        }
      }
    } catch (...) {}
  }).detach();
}

} // namespace Mounter
