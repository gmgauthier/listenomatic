/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "marquee.hpp"
#include "player.hpp"
#include "settings.hpp"

#include <gtkmm.h>

#include <array>

namespace listenomatic {

class MainWindow : public Gtk::Window {
 public:
  MainWindow();

 private:
  void load_css();
  void build_menu();
  void apply_band();
  void fill_memory();
  void refresh_face();
  void play_current();
  const Station* current() const;
  void set_status(const Glib::ustring& text);
  void on_player_state(Player::State state);
  void on_player_error(const Glib::ustring& msg);
  void on_player_title(const Glib::ustring& title);
  void on_quit();
  void on_about();
  void on_station_add();
  void on_station_remove();
  void on_store_preset(int slot);
  void on_band_live();
  void on_band_shows();
  void on_volume();
  void on_stop();
  void on_play();
  void on_preset(int slot);
  void on_memory();

  Gtk::MenuItem* add_item(Gtk::Menu& menu, const Glib::ustring& label,
                          const sigc::slot<void()>& slot, guint key = 0,
                          Gdk::ModifierType mods = Gdk::ModifierType(0));

  Settings settings_;
  Player player_;
  Glib::RefPtr<Gtk::AccelGroup> accel_;
  sigc::connection memory_changed_;

  Gtk::Box root_{Gtk::ORIENTATION_VERTICAL, 0};
  Gtk::MenuBar menubar_;
  Gtk::Box client_{Gtk::ORIENTATION_VERTICAL, 8};

  Gtk::Box band_row_{Gtk::ORIENTATION_HORIZONTAL, 8};
  Gtk::RadioButton live_{"Live"};
  Gtk::RadioButton shows_{"Shows"};
  Gtk::Label vol_lab_{"Volume"};
  Gtk::Scale volume_{Gtk::ORIENTATION_HORIZONTAL};

  Gtk::Box preset_row_{Gtk::ORIENTATION_HORIZONTAL, 4};
  std::array<Gtk::Button, 6> presets_;
  Gtk::ComboBoxText memory_;

  Gtk::EventBox lcd_box_;
  Gtk::Box lcd_{Gtk::ORIENTATION_VERTICAL, 4};
  Gtk::Box lcd_top_{Gtk::ORIENTATION_HORIZONTAL, 8};
  Gtk::Label lcd_station_;
  Gtk::Label lcd_badge_;
  Marquee lcd_now_;
  Gtk::Label lcd_hint_;

  Gtk::Box transport_{Gtk::ORIENTATION_HORIZONTAL, 6};
  Gtk::Button btn_stop_{"■"};
  Gtk::Button btn_play_{"►"};
  Gtk::Scale seek_{Gtk::ORIENTATION_HORIZONTAL};
  Gtk::Label seek_lab_{"0:00 / 0:00"};

  Gtk::Frame programs_frame_{"Programs"};
  Gtk::ScrolledWindow programs_scroll_;
  Gtk::TreeView programs_;
  class ProgramColumns : public Gtk::TreeModel::ColumnRecord {
   public:
    ProgramColumns()
    {
      add(title);
      add(date);
      add(length);
    }
    Gtk::TreeModelColumn<Glib::ustring> title;
    Gtk::TreeModelColumn<Glib::ustring> date;
    Gtk::TreeModelColumn<Glib::ustring> length;
  };
  ProgramColumns program_cols_;
  Glib::RefPtr<Gtk::ListStore> program_store_;

  Gtk::Statusbar status_;
};

}  // namespace listenomatic
