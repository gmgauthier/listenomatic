/* SPDX-License-Identifier: Unlicense */

#include "marquee.hpp"

#include <glib.h>

namespace listenomatic {
namespace {

constexpr int kGapPx = 48;
constexpr int kTickMs = 30;
constexpr int kHoldTicks = 28; /* ~0.8s pause at the start of each loop */
constexpr int kStepPx = 1;

}  // namespace

Marquee::Marquee()
{
  get_style_context()->add_class("listenomatic-lcd-now");
  set_hexpand(true);
  set_valign(Gtk::ALIGN_CENTER);
  set_size_request(32, 18);
}

Marquee::~Marquee()
{
  stop_timer();
}

void Marquee::set_text(const Glib::ustring& text)
{
  if (text_ == text)
    return;
  text_ = text;
  offset_ = 0;
  hold_ = kHoldTicks;
  measure();
  maybe_start_timer();
  queue_draw();
}

void Marquee::measure()
{
  auto layout = create_pango_layout(text_);
  layout->set_font_description(Pango::FontDescription("DejaVu Sans Mono 13"));
  int w = 0;
  int h = 0;
  layout->get_pixel_size(w, h);
  text_w_ = w;
  text_h_ = (h > 0) ? h : 16;
  set_size_request(32, text_h_);
}

void Marquee::stop_timer()
{
  if (!timer_id_)
    return;
  g_source_remove(timer_id_);
  timer_id_ = 0;
}

void Marquee::maybe_start_timer()
{
  const int view = get_allocated_width();
  if (text_.empty() || text_w_ <= view || view <= 0) {
    stop_timer();
    offset_ = 0;
    return;
  }
  if (timer_id_)
    return;
  timer_id_ = g_timeout_add(
      kTickMs,
      [](gpointer self) -> gboolean {
        return static_cast<Marquee*>(self)->on_tick() ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
      },
      this);
}

bool Marquee::on_tick()
{
  const int view = get_allocated_width();
  if (text_.empty() || text_w_ <= view || view <= 0) {
    timer_id_ = 0;
    offset_ = 0;
    queue_draw();
    return false;
  }
  if (hold_ > 0) {
    --hold_;
    return true;
  }
  offset_ += kStepPx;
  const int loop = text_w_ + kGapPx;
  if (offset_ >= loop) {
    offset_ = 0;
    hold_ = kHoldTicks;
  }
  queue_draw();
  return true;
}

void Marquee::on_size_allocate(Gtk::Allocation& allocation)
{
  Gtk::DrawingArea::on_size_allocate(allocation);
  maybe_start_timer();
}

void Marquee::on_unrealize()
{
  stop_timer();
  Gtk::DrawingArea::on_unrealize();
}

bool Marquee::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
  const int view = get_allocated_width();
  const int h = get_allocated_height();
  cr->set_source_rgb(0x9E / 255.0, 0xE7 / 255.0, 0xA0 / 255.0);

  auto layout = create_pango_layout(text_);
  layout->set_font_description(Pango::FontDescription("DejaVu Sans Mono 13"));

  const int y = (h - text_h_ > 0) ? (h - text_h_) / 2 : 0;
  if (text_.empty() || text_w_ <= view) {
    cr->move_to(0, y);
    layout->show_in_cairo_context(cr);
    return true;
  }

  cr->rectangle(0, 0, view, h);
  cr->clip();
  const int loop = text_w_ + kGapPx;
  cr->move_to(-offset_, y);
  layout->show_in_cairo_context(cr);
  cr->move_to(-offset_ + loop, y);
  layout->show_in_cairo_context(cr);
  return true;
}

}  // namespace listenomatic
