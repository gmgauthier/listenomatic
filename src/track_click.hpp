/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <gtkmm/widget.h>

#include <utility>

namespace listenomatic {

/* gtkmm runs a bool-returning click handler after the widget's own handler.
 * The track list consumes a right-click there, so the menu handler has to
 * run first or it never sees the event. */
template <typename Slot>
sigc::connection connect_track_button(Gtk::Widget& widget, Slot&& slot)
{
  return widget.signal_button_press_event().connect(std::forward<Slot>(slot), false);
}

}  // namespace listenomatic
