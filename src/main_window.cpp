/* SPDX-License-Identifier: Unlicense */

#include "main_window.hpp"
#include "about_dialog.hpp"
#include "add_dialog.hpp"
#include "catalog_window.hpp"
#include "fetch.hpp"
#include "paths.hpp"
#include "rss.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace listenomatic {
namespace {

constexpr unsigned kFeedRefreshSec = 15 * 60;
constexpr int kWinMinW = 640;
constexpr int kWinMinH = 500;
constexpr int kWinMaxW = 1280;
constexpr int kWinMaxH = 1000;
constexpr int kWinDefaultW = 736;
constexpr int kWinDefaultH = 540;

bool window_size_ok(int w, int h)
{
  return w >= kWinMinW && h >= kWinMinH && w <= kWinMaxW && h <= kWinMaxH;
}

}  // namespace

MainWindow::MainWindow()
{
  set_title("Listen-O-Matic");
  set_resizable(true);
  set_size_request(kWinMinW, kWinMinH);
  set_default_size(kWinDefaultW, kWinDefaultH);
  set_border_width(0);
  get_style_context()->add_class("listenomatic-window");

  accel_ = Gtk::AccelGroup::create();
  add_accel_group(accel_);

  settings_.load();
  if (window_size_ok(settings_.window_w, settings_.window_h))
    set_default_size(settings_.window_w, settings_.window_h);
  player_.set_volume(settings_.volume);
  player_.signal_state_changed().connect(sigc::mem_fun(*this, &MainWindow::on_player_state));
  player_.signal_error().connect(sigc::mem_fun(*this, &MainWindow::on_player_error));
  player_.signal_title().connect(sigc::mem_fun(*this, &MainWindow::on_player_title));
  player_.signal_position().connect(sigc::mem_fun(*this, &MainWindow::on_player_position));
  feed_alive_ = std::make_shared<bool>(true);
  load_css();
  build_menu();

  client_.set_border_width(8);

  live_.set_mode(false);
  shows_.set_mode(false);
  shows_.join_group(live_);
  band_row_.pack_start(live_, Gtk::PACK_SHRINK);
  band_row_.pack_start(shows_, Gtk::PACK_SHRINK);

  volume_.set_range(0, 100);
  volume_.set_increments(1, 10);
  volume_.set_draw_value(false);
  volume_.set_hexpand(true);
  volume_.set_value(settings_.volume * 100.0);
  volume_.signal_value_changed().connect(sigc::mem_fun(*this, &MainWindow::on_volume));
  vol_lab_.set_margin_start(12);
  band_row_.pack_end(volume_, Gtk::PACK_EXPAND_WIDGET);
  band_row_.pack_end(vol_lab_, Gtk::PACK_SHRINK);
  client_.pack_start(band_row_, Gtk::PACK_SHRINK);

  for (int i = 0; i < 6; ++i) {
    presets_[static_cast<std::size_t>(i)].set_label(Glib::ustring::compose("%1\n—", i + 1));
    presets_[static_cast<std::size_t>(i)].set_size_request(68, 44);
    presets_[static_cast<std::size_t>(i)].get_style_context()->add_class("listenomatic-preset");
    presets_[static_cast<std::size_t>(i)].set_hexpand(true);
    presets_[static_cast<std::size_t>(i)].signal_clicked().connect([this, i]() { on_preset(i); });
    preset_row_.pack_start(presets_[static_cast<std::size_t>(i)], Gtk::PACK_EXPAND_WIDGET);
  }
  memory_btn_.set_label("▾");
  memory_btn_.set_tooltip_text("Memory");
  memory_btn_.set_size_request(36, 44);
  memory_btn_.set_direction(Gtk::ARROW_DOWN);
  memory_btn_.get_style_context()->add_class("listenomatic-memory");
  memory_btn_.set_popup(memory_menu_);
  memory_btn_.set_margin_end(8);
  preset_row_.pack_start(memory_btn_, Gtk::PACK_SHRINK);
  client_.pack_start(preset_row_, Gtk::PACK_SHRINK);

  lcd_box_.get_style_context()->add_class("listenomatic-lcd");
  lcd_box_.set_size_request(-1, 88);
  lcd_.get_style_context()->add_class("listenomatic-lcd");
  lcd_.set_border_width(10);
  lcd_station_.set_xalign(0);
  lcd_station_.set_ellipsize(Pango::ELLIPSIZE_END);
  lcd_station_.set_hexpand(true);
  lcd_station_.get_style_context()->add_class("listenomatic-lcd-title");
  lcd_badge_.get_style_context()->add_class("listenomatic-lcd-badge");
  lcd_badge_.set_xalign(1);
  lcd_hint_.set_xalign(0);
  lcd_hint_.set_ellipsize(Pango::ELLIPSIZE_END);
  lcd_hint_.get_style_context()->add_class("listenomatic-lcd-hint");
  lcd_top_.pack_start(lcd_station_, Gtk::PACK_EXPAND_WIDGET);
  lcd_top_.pack_start(lcd_badge_, Gtk::PACK_SHRINK);
  lcd_.pack_start(lcd_top_, Gtk::PACK_SHRINK);
  lcd_.pack_start(lcd_now_, Gtk::PACK_SHRINK);
  lcd_.pack_start(lcd_hint_, Gtk::PACK_SHRINK);
  lcd_box_.add(lcd_);
  client_.pack_start(lcd_box_, Gtk::PACK_SHRINK);

  btn_stop_.set_size_request(36, 28);
  btn_back_.set_size_request(40, 28);
  btn_play_.set_size_request(36, 28);
  btn_fwd_.set_size_request(40, 28);
  btn_back_.set_tooltip_text("Skip back 15 seconds");
  btn_fwd_.set_tooltip_text("Skip forward 15 seconds");
  btn_stop_.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_stop));
  btn_back_.signal_clicked().connect([this]() { on_skip(-15); });
  btn_play_.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_play));
  btn_fwd_.signal_clicked().connect([this]() { on_skip(15); });
  seek_.set_range(0, 1000);
  seek_.set_draw_value(false);
  seek_.set_hexpand(true);
  /* Clearlooks centers a 21px grip on the trough end. At 0 and at the
     end of a show that grip hangs past the scale, into +15 and the time. */
  seek_.set_margin_start(8);
  seek_.set_margin_end(8);
  seek_.set_sensitive(false);
  seek_changed_ = seek_.signal_value_changed().connect(sigc::mem_fun(*this, &MainWindow::on_seek));
  transport_.pack_start(btn_stop_, Gtk::PACK_SHRINK);
  transport_.pack_start(btn_back_, Gtk::PACK_SHRINK);
  transport_.pack_start(btn_play_, Gtk::PACK_SHRINK);
  transport_.pack_start(btn_fwd_, Gtk::PACK_SHRINK);
  transport_.pack_start(seek_, Gtk::PACK_EXPAND_WIDGET);
  transport_.pack_start(seek_lab_, Gtk::PACK_SHRINK);
  client_.pack_start(transport_, Gtk::PACK_SHRINK);

  program_store_ = Gtk::ListStore::create(program_cols_);
  programs_.set_model(program_store_);
  programs_.append_column("Program", program_cols_.title);
  programs_.append_column("Date", program_cols_.date);
  programs_.append_column("Time", program_cols_.length);
  if (auto* c = programs_.get_column(0)) {
    c->set_sizing(Gtk::TREE_VIEW_COLUMN_FIXED);
    c->set_expand(true);
    c->set_min_width(200);
    for (auto* cell : c->get_cells()) {
      if (auto* text = dynamic_cast<Gtk::CellRendererText*>(cell))
        text->property_ellipsize() = Pango::ELLIPSIZE_END;
    }
  }
  if (auto* c = programs_.get_column(1)) {
    c->set_sizing(Gtk::TREE_VIEW_COLUMN_FIXED);
    c->set_fixed_width(92);
    c->set_expand(false);
  }
  if (auto* c = programs_.get_column(2)) {
    c->set_sizing(Gtk::TREE_VIEW_COLUMN_FIXED);
    c->set_fixed_width(56);
    c->set_expand(false);
  }
  programs_.set_fixed_height_mode(true);
  programs_.set_activate_on_single_click(true);
  programs_.signal_row_activated().connect(sigc::mem_fun(*this, &MainWindow::on_program_activated));
  programs_scroll_.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
  programs_scroll_.set_propagate_natural_width(false);
  programs_scroll_.set_min_content_height(160);
  programs_scroll_.set_size_request(-1, 160);
  programs_scroll_.add(programs_);
  programs_frame_.add(programs_scroll_);
  client_.pack_start(programs_frame_, Gtk::PACK_EXPAND_WIDGET);

  status_.set_size_request(-1, 24);
  root_.pack_start(menubar_, Gtk::PACK_SHRINK);
  root_.pack_start(client_, Gtk::PACK_EXPAND_WIDGET);
  root_.pack_end(status_, Gtk::PACK_SHRINK);
  add(root_);

  live_.signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_band_live));
  shows_.signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_band_shows));
  fill_memory();
  refresh_presets();
  if (settings_.band == Band::Shows)
    shows_.set_active(true);
  else
    live_.set_active(true);
  show_all();
  apply_band();
  feed_timer_ = Glib::signal_timeout().connect_seconds(
      [this]() {
        if (on_shows())
          fetch_show(false, true);
        return true;
      },
      kFeedRefreshSec);
  settings_.save();
}

void MainWindow::fit_window()
{
  int w = kWinDefaultW;
  int h = kWinDefaultH;
  if (window_size_ok(settings_.window_w, settings_.window_h)) {
    w = settings_.window_w;
    h = settings_.window_h;
  }
  Gtk::Requisition min, nat;
  get_preferred_size(min, nat);
  (void)nat;
  if (min.width > w)
    w = min.width;
  if (min.height > h)
    h = min.height;
  if (w > kWinMaxW)
    w = kWinMaxW;
  if (h > kWinMaxH)
    h = kWinMaxH;
  resize(w, h);
  if (auto gdk = get_window())
    gdk->resize(w, h);
}

void MainWindow::remember_window_size()
{
  if (!size_ready_)
    return;
  int w = 0;
  int h = 0;
  get_size(w, h);
  if (!window_size_ok(w, h))
    return;
  if (w == settings_.window_w && h == settings_.window_h)
    return;
  settings_.window_w = w;
  settings_.window_h = h;
}

void MainWindow::on_map()
{
  Gtk::Window::on_map();
  Glib::signal_idle().connect_once([this]() {
    fit_window();
    size_ready_ = true;
  });
}

void MainWindow::on_size_allocate(Gtk::Allocation& allocation)
{
  Gtk::Window::on_size_allocate(allocation);
  if (!size_ready_)
    return;
  const int w = allocation.get_width();
  const int h = allocation.get_height();
  if (!window_size_ok(w, h) || (w == settings_.window_w && h == settings_.window_h))
    return;
  settings_.window_w = w;
  settings_.window_h = h;
  size_save_.disconnect();
  size_save_ = Glib::signal_timeout().connect_seconds(
      [this]() {
        settings_.save();
        return false;
      },
      1);
}

MainWindow::~MainWindow()
{
  size_save_.disconnect();
  feed_timer_.disconnect();
  if (feed_alive_)
    *feed_alive_ = false;
  remember_window_size();
  save_progress();
  settings_.save();
  delete catalog_;
  catalog_ = nullptr;
}

void MainWindow::load_css()
{
  const std::string css_path = find_data_file("skin/lcos/lcos.css");
  if (css_path.empty()) {
    std::cerr << "listenomatic: lcos.css not found\n";
    return;
  }
  try {
    auto css = Gtk::CssProvider::create();
    css->load_from_path(css_path);
    Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(), css,
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  } catch (const Glib::Error& e) {
    std::cerr << "listenomatic: CSS: " << e.what() << "\n";
  }
}

Gtk::MenuItem* MainWindow::add_item(Gtk::Menu& menu, const Glib::ustring& label,
                                    const sigc::slot<void()>& slot, guint key,
                                    Gdk::ModifierType mods)
{
  auto* item = Gtk::manage(new Gtk::MenuItem(label, true));
  item->signal_activate().connect(slot);
  if (key != 0)
    item->add_accelerator("activate", accel_, key, mods, Gtk::ACCEL_VISIBLE);
  menu.append(*item);
  return item;
}

void MainWindow::build_menu()
{
  auto* file_item = Gtk::manage(new Gtk::MenuItem("_File", true));
  auto* file_menu = Gtk::manage(new Gtk::Menu());
  add_item(*file_menu, "E_xit", sigc::mem_fun(*this, &MainWindow::on_quit), GDK_KEY_q,
           Gdk::CONTROL_MASK);
  file_item->set_submenu(*file_menu);
  menubar_.append(*file_item);

  auto* st_item = Gtk::manage(new Gtk::MenuItem("_Station", true));
  auto* st_menu = Gtk::manage(new Gtk::Menu());
  add_item(*st_menu, "_Add…", sigc::mem_fun(*this, &MainWindow::on_station_add));
  add_item(*st_menu, "_Catalog…", sigc::mem_fun(*this, &MainWindow::on_catalog));
  add_item(*st_menu, "_Remove", sigc::mem_fun(*this, &MainWindow::on_station_remove));
  auto* store_item = Gtk::manage(new Gtk::MenuItem("Store on Preset", true));
  auto* store_menu = Gtk::manage(new Gtk::Menu());
  for (int i = 0; i < 6; ++i) {
    add_item(*store_menu, Glib::ustring::compose("_%1", i + 1),
             [this, i]() { on_store_preset(i); });
  }
  store_item->set_submenu(*store_menu);
  st_menu->append(*store_item);
  st_item->set_submenu(*st_menu);
  menubar_.append(*st_item);

  auto* band_item = Gtk::manage(new Gtk::MenuItem("_Band", true));
  auto* band_menu = Gtk::manage(new Gtk::Menu());
  add_item(*band_menu, "_Live", [this]() { live_.set_active(true); });
  add_item(*band_menu, "_Shows", [this]() { shows_.set_active(true); });
  band_item->set_submenu(*band_menu);
  menubar_.append(*band_item);

  auto* help_item = Gtk::manage(new Gtk::MenuItem("_Help", true));
  auto* help_menu = Gtk::manage(new Gtk::Menu());
  add_item(*help_menu, "_About Listen-O-Matic", sigc::mem_fun(*this, &MainWindow::on_about));
  help_item->set_submenu(*help_menu);
  menubar_.append(*help_item);
}

bool MainWindow::on_shows() const
{
  return shows_.get_active();
}

std::vector<Station>& MainWindow::stations()
{
  return on_shows() ? settings_.shows : settings_.live;
}

const std::vector<Station>& MainWindow::stations() const
{
  return on_shows() ? settings_.shows : settings_.live;
}

int& MainWindow::current_index()
{
  return on_shows() ? settings_.current_show : settings_.current_live;
}

std::array<int, 6>& MainWindow::presets()
{
  return on_shows() ? settings_.show_presets : settings_.live_presets;
}

void MainWindow::fill_memory()
{
  const auto children = memory_menu_.get_children();
  for (auto* w : children)
    memory_menu_.remove(*w);
  const auto& list = stations();
  const int cur = on_shows() ? settings_.current_show : settings_.current_live;
  if (list.empty()) {
    auto* empty = Gtk::manage(new Gtk::MenuItem("(empty)"));
    empty->set_sensitive(false);
    memory_menu_.append(*empty);
  } else {
    for (int i = 0; i < static_cast<int>(list.size()); ++i) {
      auto* item = Gtk::manage(new Gtk::MenuItem(list[static_cast<std::size_t>(i)].name));
      if (i == cur)
        item->set_label("●  " + list[static_cast<std::size_t>(i)].name);
      item->signal_activate().connect([this, i]() { on_memory_pick(i); });
      memory_menu_.append(*item);
    }
  }
  memory_menu_.show_all();
}

void MainWindow::fill_live_tracks()
{
  program_store_->clear();
  programs_frame_.set_label("Tracks");
  const Station* st = current();
  if (!st) {
    auto row = *program_store_->append();
    row[program_cols_.title] = "No station";
    return;
  }
  const auto& tracks = live_log_.tracks(st->url);
  if (tracks.empty()) {
    auto row = *program_store_->append();
    row[program_cols_.title] = "Tracks appear here when the station sends titles";
    return;
  }
  for (const auto& t : tracks) {
    auto row = *program_store_->append();
    row[program_cols_.title] = t.title;
    row[program_cols_.date] = t.date;
    row[program_cols_.length] = t.heard;
  }
}

void MainWindow::refresh_presets()
{
  const auto& list = stations();
  const auto& slots = on_shows() ? settings_.show_presets : settings_.live_presets;
  const int cur = on_shows() ? settings_.current_show : settings_.current_live;
  for (int i = 0; i < 6; ++i) {
    const int idx = slots[static_cast<std::size_t>(i)];
    Glib::ustring label;
    bool on = false;
    if (idx >= 0 && idx < static_cast<int>(list.size())) {
      label =
          Glib::ustring::compose("%1\n%2", i + 1, list[static_cast<std::size_t>(idx)].short_name);
      on = idx == cur;
    } else {
      label = Glib::ustring::compose("%1\n—", i + 1);
    }
    presets_[static_cast<std::size_t>(i)].set_label(label);
    auto ctx = presets_[static_cast<std::size_t>(i)].get_style_context();
    if (on)
      ctx->add_class("listenomatic-preset-on");
    else
      ctx->remove_class("listenomatic-preset-on");
  }
}

void MainWindow::select_station(int index, bool play)
{
  auto& list = stations();
  if (index < 0 || index >= static_cast<int>(list.size()))
    return;
  const int previous = current_index();
  current_index() = index;
  fill_memory();
  refresh_presets();
  settings_.save();
  if (on_shows()) {
    if (index != previous)
      end_show_playback();
    load_show_feed(false);
    return;
  }
  if (play)
    play_current();
  fill_live_tracks();
  refresh_face();
}

const Station* MainWindow::current() const
{
  const auto& list = stations();
  const int cur = on_shows() ? settings_.current_show : settings_.current_live;
  if (cur < 0 || cur >= static_cast<int>(list.size()))
    return nullptr;
  return &list[static_cast<std::size_t>(cur)];
}

void MainWindow::refresh_face()
{
  const Station* st = current();
  const bool playing = player_.state() == Player::State::Playing;
  const bool shows = on_shows();

  lcd_badge_.set_text(shows ? (playing ? "PROGRAM" : "SHOWS") : (playing ? "ON AIR" : "LIVE"));
  if (!st) {
    lcd_station_.set_text("No station");
    lcd_now_.set_text("");
    lcd_hint_.set_text("Station → Add… or Memory ▾");
    set_status(shows ? "No shows in memory" : "No stations in memory");
    return;
  }
  lcd_station_.set_text(st->name);
  if (shows) {
    if (current_program_ >= 0 && current_program_ < static_cast<int>(episodes_.size()))
      lcd_now_.set_text(episodes_[static_cast<std::size_t>(current_program_)].title);
    else if (!playing)
      lcd_now_.set_text("");
  } else if (!playing) {
    lcd_now_.set_text("");
  }
  lcd_hint_.set_text("");
  if (playing)
    set_status("Playing");
  else if (player_.state() == Player::State::Paused)
    set_status("Paused");
  else
    set_status("Stopped");
}

void MainWindow::apply_band()
{
  const bool shows = on_shows();
  settings_.band = shows ? Band::Shows : Band::Live;
  save_progress();
  player_.stop();
  playing_.forget();
  btn_back_.set_sensitive(shows);
  btn_fwd_.set_sensitive(shows);
  seek_.set_sensitive(false);
  seek_from_player_ = true;
  seek_.set_value(0);
  seek_from_player_ = false;
  seek_lab_.set_text("0:00 / 0:00");
  fill_memory();
  refresh_presets();
  if (shows) {
    programs_frame_.set_label("Programs");
    if (settings_.current_show >= 0)
      load_show_feed(false);
    else {
      episodes_.clear();
      program_store_->clear();
      auto row = *program_store_->append();
      row[program_cols_.title] = "No show — Station → Add…";
      refresh_face();
    }
  } else {
    fill_live_tracks();
    refresh_face();
  }
  settings_.save();
}

void MainWindow::play_current()
{
  if (on_shows()) {
    if (episodes_.empty()) {
      set_status("No programs yet");
      return;
    }
    play_program(current_program_ >= 0 ? current_program_ : 0);
    return;
  }
  const Station* st = current();
  if (!st) {
    set_status("Add a live stream first");
    return;
  }
  save_progress();
  playing_.forget();
  if (!player_.open(st->url))
    return;
  player_.set_volume(settings_.volume);
  player_.play();
}

void MainWindow::play_program(int index)
{
  if (index < 0 || index >= static_cast<int>(episodes_.size()))
    return;
  save_progress();
  current_program_ = index;
  const Program& p = episodes_[static_cast<std::size_t>(index)];
  pending_resume_ns_ = 0;
  const std::int64_t saved = settings_.resume_for(p.enclosure);
  const std::int64_t dur = p.duration_ns;
  if (saved > 3 * 1000000000LL && (dur <= 0 || saved < dur - 5 * 1000000000LL))
    pending_resume_ns_ = saved;
  playing_.forget();
  if (!player_.open(p.enclosure))
    return;
  playing_.start(p.enclosure, p.duration_ns);
  if (const Station* st = current())
    settings_.set_last_program(st->url, p.enclosure);
  player_.set_volume(settings_.volume);
  player_.play();
  lcd_now_.set_text(p.title);
  select_program_row(index);
  refresh_face();
  settings_.save();
}

void MainWindow::save_progress()
{
  // Save against the episode the player is on, not the band or list on screen.
  if (!playing_.active() || player_.state() == Player::State::Stopped)
    return;
  player_.refresh_position();
  if (playing_.save(settings_, player_.position(), player_.duration()))
    settings_.save();
}

void MainWindow::end_show_playback()
{
  player_.refresh_position();
  if (handoff_playing_show(playing_, settings_, player_.position(), player_.duration(),
                           player_.state() == Player::State::Stopped))
    settings_.save();
  player_.stop();
  pending_resume_ns_ = 0;
  current_program_ = -1;
}

void MainWindow::fill_programs()
{
  program_store_->clear();
  for (const auto& p : episodes_) {
    auto row = *program_store_->append();
    row[program_cols_.title] = p.title;
    row[program_cols_.date] = p.date;
    row[program_cols_.length] = p.length;
    row[program_cols_.url] = p.enclosure;
  }
}

void MainWindow::load_show_feed(bool play_latest)
{
  fetch_show(play_latest, false);
}

void MainWindow::fetch_show(bool play_latest, bool quiet)
{
  const Station* st = current();
  if (!st) {
    if (!quiet) {
      episodes_.clear();
      program_store_->clear();
      refresh_face();
    }
    return;
  }
  const std::string url = st->url;
  std::string keep;
  if (quiet && current_program_ >= 0 && current_program_ < static_cast<int>(episodes_.size()))
    keep = episodes_[static_cast<std::size_t>(current_program_)].enclosure;
  const std::uint64_t gen = ++feed_gen_;
  if (!quiet) {
    program_store_->clear();
    auto row = *program_store_->append();
    row[program_cols_.title] = "Loading programs…";
    set_status("Loading feed…");
    refresh_face();
  }
  auto alive = feed_alive_;
  std::thread([this, alive, url, play_latest, quiet, keep, gen]() {
    std::string err;
    const std::string xml = http_get(url, err);
    PodcastFeed feed;
    if (err.empty() && !parse_podcast(xml, feed, err)) {
      /* err already set */
    }
    Glib::signal_idle().connect_once([this, alive, feed = std::move(feed), err = std::move(err),
                                      url, play_latest, quiet, keep, gen]() {
      if (!*alive || gen != feed_gen_)
        return;
      if (!on_shows())
        return;
      const Station* now = current();
      if (!now || now->url != url)
        return;
      apply_feed(feed, err, play_latest, quiet, keep);
    });
  }).detach();
}

void MainWindow::apply_feed(PodcastFeed feed, std::string error, bool play_latest, bool quiet,
                            const std::string& keep_enclosure)
{
  if (!error.empty() && feed.programs.empty()) {
    if (quiet && !episodes_.empty()) {
      if (player_.state() == Player::State::Stopped)
        set_status(error);
      return;
    }
    episodes_.clear();
    current_program_ = -1;
    program_store_->clear();
    auto row = *program_store_->append();
    row[program_cols_.title] = error;
    set_status(error);
    refresh_face();
    return;
  }
  if (!feed.title.empty()) {
    auto& list = settings_.shows;
    const int idx = settings_.current_show;
    if (idx >= 0 && idx < static_cast<int>(list.size()) &&
        (list[static_cast<std::size_t>(idx)].name.empty() ||
         list[static_cast<std::size_t>(idx)].name == list[static_cast<std::size_t>(idx)].url)) {
      list[static_cast<std::size_t>(idx)].name = feed.title;
      list[static_cast<std::size_t>(idx)].short_name = make_short_name(feed.title);
      fill_memory();
      refresh_presets();
      settings_.save();
    }
  }
  const Station* st = current();
  const std::string saved = st ? settings_.last_program_for(st->url) : std::string();
  episodes_ = std::move(feed.programs);
  const std::string want = !keep_enclosure.empty() ? keep_enclosure : saved;
  int pick = -1;
  if (!want.empty()) {
    for (int i = 0; i < static_cast<int>(episodes_.size()); ++i) {
      if (episodes_[static_cast<std::size_t>(i)].enclosure == want) {
        pick = i;
        break;
      }
    }
  }
  const bool playing_missing =
      pick < 0 && !keep_enclosure.empty() &&
      (player_.state() == Player::State::Playing || player_.state() == Player::State::Paused);
  if (pick < 0 && !playing_missing && !episodes_.empty())
    pick = 0;
  current_program_ = pick;
  fill_programs();
  select_program_row(pick);
  refresh_face();
  const bool busy =
      player_.state() == Player::State::Playing || player_.state() == Player::State::Paused;
  if (play_latest && current_program_ >= 0)
    play_program(current_program_);
  else if (!busy)
    set_status(Glib::ustring::compose("%1 programs", episodes_.size()));
}

void MainWindow::select_program_row(int index)
{
  if (index < 0 || !programs_.get_selection())
    return;
  Gtk::TreePath path;
  path.push_back(index);
  programs_.get_selection()->select(path);
  if (get_realized())
    programs_.scroll_to_row(path);
}

void MainWindow::set_status(const Glib::ustring& text)
{
  status_.remove_all_messages();
  status_.push(text);
}

void MainWindow::on_quit()
{
  hide();
}

void MainWindow::on_about()
{
  AboutDialog dlg(*this);
  dlg.run();
}

void MainWindow::on_station_add()
{
  AddDialog dlg(*this);
  if (dlg.run() != Gtk::RESPONSE_OK)
    return;
  Station st = dlg.station();
  if (st.url.empty()) {
    set_status("Need a URL");
    return;
  }
  auto& list = dlg.is_show() ? settings_.shows : settings_.live;
  int& cur = dlg.is_show() ? settings_.current_show : settings_.current_live;
  for (int i = 0; i < static_cast<int>(list.size()); ++i) {
    if (list[static_cast<std::size_t>(i)].url == st.url) {
      const int previous = cur;
      cur = i;
      if (dlg.is_show() == on_shows()) {
        if (on_shows() && i != previous)
          end_show_playback();
        fill_memory();
        refresh_presets();
        if (on_shows())
          load_show_feed(false);
        else
          refresh_face();
      }
      settings_.save();
      return;
    }
  }
  list.push_back(std::move(st));
  cur = static_cast<int>(list.size()) - 1;
  settings_.save();
  if (dlg.is_show() != on_shows()) {
    if (dlg.is_show())
      shows_.set_active(true);
    else
      live_.set_active(true);
    return;
  }
  if (on_shows())
    end_show_playback();
  fill_memory();
  refresh_presets();
  if (on_shows())
    load_show_feed(false);
  else {
    fill_live_tracks();
    refresh_face();
  }
}

void MainWindow::on_catalog()
{
  if (!catalog_) {
    catalog_ = new CatalogWindow();
    catalog_->set_transient_for(*this);
    catalog_->signal_add().connect(sigc::mem_fun(*this, &MainWindow::add_from_catalog));
  }
  catalog_->present();
}

void MainWindow::add_from_catalog(Station st, bool is_show)
{
  auto& list = is_show ? settings_.shows : settings_.live;
  int& cur = is_show ? settings_.current_show : settings_.current_live;
  for (int i = 0; i < static_cast<int>(list.size()); ++i) {
    if (list[static_cast<std::size_t>(i)].url == st.url) {
      const int previous = cur;
      cur = i;
      settings_.save();
      if (is_show == on_shows()) {
        if (is_show && i != previous)
          end_show_playback();
        fill_memory();
        refresh_presets();
        if (is_show && i != previous)
          load_show_feed(false);
      }
      set_status(Glib::ustring::compose("Already in Memory: %1", st.name));
      return;
    }
  }
  list.push_back(std::move(st));
  cur = static_cast<int>(list.size()) - 1;
  settings_.save();
  if (is_show == on_shows()) {
    if (is_show)
      end_show_playback();
    fill_memory();
    refresh_presets();
    if (is_show)
      load_show_feed(false);
    else
      fill_live_tracks();
  }
  set_status(Glib::ustring::compose("Added %1", list.back().name));
}

void MainWindow::on_station_remove()
{
  auto& list = stations();
  int& cur = current_index();
  const int idx = cur;
  if (idx < 0 || idx >= static_cast<int>(list.size())) {
    set_status("Nothing to remove");
    return;
  }
  player_.stop();
  playing_.forget();
  list.erase(list.begin() + idx);
  auto& slots = presets();
  for (int i = 0; i < 6; ++i) {
    int& p = slots[static_cast<std::size_t>(i)];
    if (p == idx)
      p = -1;
    else if (p > idx)
      --p;
  }
  if (cur >= static_cast<int>(list.size()))
    cur = static_cast<int>(list.size()) - 1;
  fill_memory();
  refresh_presets();
  settings_.save();
  if (on_shows())
    load_show_feed(false);
  else {
    fill_live_tracks();
    refresh_face();
  }
}

void MainWindow::on_store_preset(int slot)
{
  if (current_index() < 0) {
    set_status("Tune a station first");
    return;
  }
  presets()[static_cast<std::size_t>(slot)] = current_index();
  refresh_presets();
  settings_.save();
  set_status(Glib::ustring::compose("Stored on preset %1", slot + 1));
}

void MainWindow::on_band_live()
{
  if (live_.get_active())
    apply_band();
}

void MainWindow::on_band_shows()
{
  if (shows_.get_active())
    apply_band();
}

void MainWindow::on_volume()
{
  settings_.volume = volume_.get_value() / 100.0;
  player_.set_volume(settings_.volume);
  settings_.save();
}

void MainWindow::on_stop()
{
  save_progress();
  player_.stop();
  refresh_face();
}

void MainWindow::on_skip(int seconds)
{
  if (!on_shows())
    return;
  player_.refresh_position();
  gint64 ns = player_.position() + static_cast<gint64>(seconds) * GST_SECOND;
  if (ns < 0)
    ns = 0;
  const gint64 dur = player_.duration();
  if (dur > 0 && ns > dur)
    ns = dur;
  player_.seek(ns);
  player_.refresh_position();
}

void MainWindow::on_play()
{
  if (player_.state() == Player::State::Playing) {
    save_progress();
    player_.pause();
    return;
  }
  if (player_.state() == Player::State::Paused) {
    player_.play();
    return;
  }
  play_current();
}

void MainWindow::on_preset(int slot)
{
  const int idx = presets()[static_cast<std::size_t>(slot)];
  if (idx < 0) {
    set_status("Empty preset — Station → Store on Preset");
    return;
  }
  select_station(idx, true);
}

void MainWindow::on_memory_pick(int index)
{
  select_station(index, true);
}

void MainWindow::on_player_state(Player::State state)
{
  btn_play_.set_label(player_.state() == Player::State::Playing ? "❚❚" : "►");
  if (state == Player::State::Playing && pending_resume_ns_ > 0) {
    player_.seek(pending_resume_ns_);
    pending_resume_ns_ = 0;
  }
  refresh_face();
}

void MainWindow::on_player_error(const Glib::ustring& msg)
{
  set_status(msg);
}

void MainWindow::on_player_title(const Glib::ustring& title)
{
  if (on_shows() || title.empty())
    return;
  lcd_now_.set_text(title);
  const Station* st = current();
  if (!st)
    return;
  // The title playing now is listed at once, above the ones already heard.
  const auto when = Glib::DateTime::create_now_local();
  if (!live_log_.on_title(st->url, title.raw(), when.format("%Y-%m-%d").raw(),
                          when.format("%H:%M").raw()))
    return;
  fill_live_tracks();
}

void MainWindow::on_player_position(gint64 pos, gint64 dur)
{
  if (!on_shows())
    return;
  seek_.set_sensitive(dur > 0);
  auto fmt = [](gint64 ns) {
    if (ns < 0)
      ns = 0;
    const int sec = static_cast<int>(ns / 1000000000LL);
    const int h = sec / 3600;
    const int m = (sec % 3600) / 60;
    const int s = sec % 60;
    char buf[16];
    if (h > 0)
      std::snprintf(buf, sizeof(buf), "%d:%02d:%02d", h, m, s);
    else
      std::snprintf(buf, sizeof(buf), "%d:%02d", m, s);
    return Glib::ustring(buf);
  };
  seek_from_player_ = true;
  if (dur > 0)
    seek_.set_value(1000.0 * static_cast<double>(pos) / static_cast<double>(dur));
  else
    seek_.set_value(0);
  seek_from_player_ = false;
  if (dur > 0)
    seek_lab_.set_text(fmt(pos) + " / " + fmt(dur));
  else
    seek_lab_.set_text(fmt(pos));
}

void MainWindow::on_seek()
{
  if (seek_from_player_ || !on_shows())
    return;
  const gint64 dur = player_.duration();
  if (dur <= 0)
    return;
  const double f = seek_.get_value() / 1000.0;
  player_.seek(static_cast<gint64>(f * static_cast<double>(dur)));
}

void MainWindow::on_program_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn*)
{
  if (!on_shows() || path.empty())
    return;
  play_program(path[0]);
}

}  // namespace listenomatic
