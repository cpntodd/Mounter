/* dashboard_page.h — Dashboard: status overview + quick actions */

#pragma once

#include <gtkmm.h>
#include <memory>

#include "../ui/pie_chart.h"

namespace Mounter {

class Window;
class PieChart;

class DashboardPage : public Gtk::Box
{
public:
  explicit DashboardPage(Window& window);
  ~DashboardPage() override = default;

  /// Refresh all status cards. Returns true to keep timer running.
  bool refresh();

private:
  void build_ui();
  void build_status_cards(Gtk::Box& row);
  void build_quick_actions(Gtk::Box& row);
  void update_mount_status();
  void update_server_status();
  void update_disk_usage();

  Window& window_;

  Gtk::Label heading_{"Dashboard"};

  // Status card widgets
  Gtk::Label  mount_count_label_;
  Gtk::Label  mount_detail_label_;
  Gtk::Label  server_status_label_;
  Gtk::Label  server_detail_label_;
  Gtk::Box    server_dot_{Gtk::Orientation::HORIZONTAL, 6};
  Gtk::DrawingArea server_indicator_;  // small colored circle
  std::unique_ptr<PieChart> pie_chart_;
  Gtk::Label  disk_pct_label_;

  // Timer for periodic refresh
  sigc::connection refresh_timer_;
};

} // namespace Mounter
