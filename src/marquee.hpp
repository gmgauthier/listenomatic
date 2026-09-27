/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <gtkmm.h>

namespace listenomatic {

/* Fixed-width LCD line. Long strings marquee right-to-left and loop. */
class Marquee : public Gtk::DrawingArea {
 public:
  Marquee();
  ~Marquee() override;

  void set_text(const Glib::ustring& text);
  Glib::ustring text() const
  {
    return text_;
  }

 protected:
  bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;
  void on_size_allocate(Gtk::Allocation& allocation) override;
  void on_unrealize() override;

 private:
  void stop_timer();
  void maybe_start_timer();
  bool on_tick();
  void measure();

  Glib::ustring text_;
  int text_w_ = 0;
  int text_h_ = 16;
  int offset_ = 0;
  int hold_ = 0;
  guint timer_id_ = 0;
};

}  // namespace listenomatic
