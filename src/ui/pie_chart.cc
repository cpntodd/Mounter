/* pie_chart.cc — Cairo-drawn donut chart */

#include "pie_chart.h"
#include <cairomm/context.h>
#include <numeric>
#include <cmath>

namespace Mounter {

PieChart::PieChart()
{
  set_draw_func([this](const Cairo::RefPtr<Cairo::Context>& cr, int w, int h) {
    on_draw(cr, w, h);
  });
  set_size_request(120, 120);
}

void PieChart::set_data(const std::vector<std::tuple<double, std::string, Gdk::RGBA>>& segments)
{
  segments_ = segments;
  queue_draw();
}

void PieChart::clear()
{
  segments_.clear();
  queue_draw();
}

void PieChart::on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height)
{
  if (segments_.empty()) {
    // Draw empty ring
    auto cx = width / 2.0;
    auto cy = height / 2.0;
    auto radius = std::min(cx, cy) * 0.65;

    cr->set_source_rgba(0.5, 0.5, 0.5, 0.2);
    cr->set_line_width(radius * 0.35);
    cr->arc(cx, cy, radius, 0, 2 * M_PI);
    cr->stroke();

    // Center text
    cr->set_source_rgba(0.7, 0.7, 0.7, 0.6);
    cr->set_font_size(10);
    cr->move_to(cx - 18, cy + 4);
    cr->show_text("No data");
    return;
  }

  double total = 0;
  for (const auto& seg : segments_) {
    total += std::get<0>(seg);
  }
  if (total <= 0) total = 1;

  auto cx = width / 2.0;
  auto cy = height / 2.0;
  auto outer_radius = std::min(cx, cy) * 0.7;
  auto inner_radius = outer_radius * 0.55;

  double start_angle = -M_PI / 2; // start from top

  for (const auto& seg : segments_) {
    double value   = std::get<0>(seg);
    const auto& color = std::get<2>(seg);
    double sweep = (value / total) * 2 * M_PI;

    if (sweep < 0.01) continue;

    // Draw arc segment
    cr->set_source_rgba(color.get_red(), color.get_green(), color.get_blue(), 0.85);
    cr->set_line_width(outer_radius - inner_radius);
    cr->set_line_cap(static_cast<Cairo::Context::LineCap>(CAIRO_LINE_CAP_BUTT));
    cr->arc(cx, cy, (outer_radius + inner_radius) / 2, start_angle, start_angle + sweep);
    cr->stroke();

    start_angle += sweep;
  }

  // Center text: percentage used
  cr->set_source_rgba(0.9, 0.9, 0.9, 0.9);
  cr->set_font_size(11);
  auto pct_text = std::to_string(static_cast<int>(total)) + "%";
  cr->move_to(cx - 12, cy + 4);
  cr->show_text(pct_text);
}

} // namespace Mounter
