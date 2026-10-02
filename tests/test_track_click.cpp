/* SPDX-License-Identifier: Unlicense */

#include "check.hpp"
#include "track_click.hpp"

#include <gtkmm.h>

namespace {

class Cols : public Gtk::TreeModel::ColumnRecord {
 public:
  Cols()
  {
    add(title);
    add(date);
  }
  Gtk::TreeModelColumn<Glib::ustring> title;
  Gtk::TreeModelColumn<Glib::ustring> date;
};

}  // namespace

int main(int argc, char** argv)
{
  Gtk::Main kit(argc, argv);

  Cols cols;
  auto store = Gtk::ListStore::create(cols);
  {
    auto row = *store->append();
    row[cols.title] = "Song & Band";
    row[cols.date] = "2026-10-02";
  }

  Gtk::Window window;
  window.set_default_size(420, 200);
  Gtk::TreeView view;
  view.set_model(store);
  view.append_column("Program", cols.title);
  view.append_column("Date", cols.date);
  window.add(view);
  window.show_all();

  int early = 0;
  int late = 0;
  listenomatic::connect_track_button(view, [&](GdkEventButton* event) {
    ++early;
    return event && event->button == 3;
  });
  view.signal_button_press_event().connect([&](GdkEventButton*) {
    ++late;
    return false;
  });

  Glib::signal_timeout().connect(
      [&]() {
        auto bin = view.get_bin_window();
        if (!view.get_mapped() || !bin)
          return true;
        Gtk::TreeModel::Path path("0");
        Gdk::Rectangle rect;
        view.get_cell_area(path, *view.get_column(0), rect);
        GdkEvent* ev = gdk_event_new(GDK_BUTTON_PRESS);
        ev->button.window = static_cast<GdkWindow*>(g_object_ref(bin->gobj()));
        ev->button.send_event = TRUE;
        ev->button.button = 3;
        ev->button.x = rect.get_x() + 8;
        ev->button.y = rect.get_y() + rect.get_height() / 2;
        ev->button.time = GDK_CURRENT_TIME;
        auto seat = gdk_display_get_default_seat(gdk_display_get_default());
        ev->button.device = gdk_seat_get_pointer(seat);
        gtk_widget_event(GTK_WIDGET(view.gobj()), ev);
        gdk_event_free(ev);
        Gtk::Main::quit();
        return false;
      },
      50);
  Glib::signal_timeout().connect(
      [&]() {
        Gtk::Main::quit();
        return false;
      },
      3000);

  Gtk::Main::run(window);
  CHECK(early == 1);
  CHECK(late == 0);
  return suite_test::done("track-click");
}
