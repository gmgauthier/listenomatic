/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "live_log.hpp"
#include "marquee.hpp"
#include "player.hpp"
#include "program.hpp"
#include "resume.hpp"
#include "settings.hpp"

#include <gtkmm.h>

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace listenomatic {

class MainWindow : public Gtk::Window {
 public:
  MainWindow();
  ~MainWindow() override;

 protected:
  void on_map() override;
  void on_size_allocate(Gtk::Allocation& allocation) override;

 private:
  void load_css();
  void fit_window();
  void remember_window_size();
  void build_menu();
  void apply_band();
  bool on_shows() const;
  std::vector<Station>& stations();
  const std::vector<Station>& stations() const;
  int& current_index();
  std::array<int, 6>& presets();
  void fill_memory();
  void fill_live_tracks();
  void refresh_presets();
  void refresh_face();
  void play_current();
  void play_program(int index);
  void save_progress();
  void end_show_playback();
  void try_pending_resume();
  void on_skip(int seconds);
  void select_station(int index, bool play);
  void load_show_feed(bool play_latest);
  void fetch_show(bool play_latest, bool quiet);
  void apply_feed(PodcastFeed feed, std::string error, bool play_latest, bool quiet,
                  const std::string& keep_enclosure);
  void fill_programs();
  void select_program_row(int index);
  const Station* current() const;
  void set_status(const Glib::ustring& text);
  void on_player_state(Player::State state);
  void on_player_error(const Glib::ustring& msg);
  void on_playback_ended();
  void on_player_title(const Glib::ustring& title);
  void on_player_position(gint64 pos, gint64 dur);
  void on_seek();
  void on_program_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* col);
  void on_quit();
  void on_about();
  void on_station_add();
  void on_catalog();
  void add_from_catalog(Station st, bool is_show);
  void on_station_remove();
  void on_store_preset(int slot);
  void on_band_live();
  void on_band_shows();
  void on_volume();
  void on_stop();
  void on_play();
  void on_preset(int slot);
  void on_memory_pick(int index);
  bool on_track_button(GdkEventButton* event);
  void on_track_copy();
  void on_track_search();

  Gtk::MenuItem* add_item(Gtk::Menu& menu, const Glib::ustring& label,
                          const sigc::slot<void()>& slot, guint key = 0,
                          Gdk::ModifierType mods = Gdk::ModifierType(0));

  Settings settings_;
  Player player_;
  class CatalogWindow* catalog_ = nullptr;
  Glib::RefPtr<Gtk::AccelGroup> accel_;
  sigc::connection seek_changed_;

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
  Gtk::MenuButton memory_btn_;
  Gtk::Menu memory_menu_;

  Gtk::EventBox lcd_box_;
  Gtk::Box lcd_{Gtk::ORIENTATION_VERTICAL, 4};
  Gtk::Box lcd_top_{Gtk::ORIENTATION_HORIZONTAL, 8};
  Gtk::Label lcd_station_;
  Gtk::Label lcd_badge_;
  Marquee lcd_now_;
  Gtk::Label lcd_hint_;

  Gtk::Box transport_{Gtk::ORIENTATION_HORIZONTAL, 6};
  Gtk::Button btn_stop_{"■"};
  Gtk::Button btn_back_{"−15"};
  Gtk::Button btn_play_{"►"};
  Gtk::Button btn_fwd_{"+15"};
  Gtk::Scale seek_{Gtk::ORIENTATION_HORIZONTAL};
  Gtk::Label seek_lab_{"0:00 / 0:00"};

  Gtk::Frame programs_frame_{"Programs"};
  Gtk::ScrolledWindow programs_scroll_;
  Gtk::TreeView programs_;
  Gtk::Menu track_menu_;
  Glib::ustring track_menu_title_;
  class ProgramColumns : public Gtk::TreeModel::ColumnRecord {
   public:
    ProgramColumns()
    {
      add(title);
      add(date);
      add(length);
      add(url);
    }
    Gtk::TreeModelColumn<Glib::ustring> title;
    Gtk::TreeModelColumn<Glib::ustring> date;
    Gtk::TreeModelColumn<Glib::ustring> length;
    Gtk::TreeModelColumn<Glib::ustring> url;
  };
  ProgramColumns program_cols_;
  Glib::RefPtr<Gtk::ListStore> program_store_;

  Gtk::Statusbar status_;

  std::vector<Program> episodes_;
  LiveLog live_log_;
  int current_program_ = -1;
  ResumeTracker playing_;
  ResumeSeek resume_seek_;
  bool seek_from_player_ = false;
  bool size_ready_ = false;
  std::uint64_t feed_gen_ = 0;
  sigc::connection feed_timer_;
  sigc::connection size_save_;
  std::shared_ptr<bool> feed_alive_;
};

}  // namespace listenomatic
