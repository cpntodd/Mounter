/* pie_chart.h — Simple donut pie chart widget using Cairo */

#pragma once

#include <gtkmm.h>
#include <vector>
#include <tuple>
#include <string>

namespace Mounter {

/// A simple donut/ring chart drawn with Cairo.
/// Each segment is defined by a value, label, and RGBA color.
class PieChart : public Gtk::DrawingArea
{
public:
  PieChart();
  ~PieChart() override = default;

  /// Set the chart data. Values are proportional (auto-summed to 100%).
  /// Each tuple: {value, label, rgba_color}
  void set_data(const std::vector<std::tuple<double, std::string, Gdk::RGBA>>& segments);

  /// Clear all data.
  void clear();

protected:
  void on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height);

private:
  std::vector<std::tuple<double, std::string, Gdk::RGBA>> segments_;
};

} // namespace Mounter
