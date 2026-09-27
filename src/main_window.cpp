/* SPDX-License-Identifier: Unlicense */

#include "main_window.hpp"
#include "about_dialog.hpp"
#include "paths.hpp"

#include <cstdlib>
#include <iostream>

namespace listenomatic {

MainWindow::MainWindow()
{
  set_title("Listen-O-Matic");
  set_resizable(false);
  set_size_request(628, -1);
  set_border_width(0);
  get_style_context()->add_class("listenomatic-window");

  accel_ = Gtk::AccelGroup::create();
  add_accel_group(accel_);

  settings_.load();
  player_.set_volume(settings_.volume);
  player_.signal_state_changed().connect(sigc::mem_fun(*this, &MainWindow::on_player_state));
  player_.signal_error().connect(sigc::mem_fun(*this, &MainWindow::on_player_error));
  player_.signal_title().connect(sigc::mem_fun(*this, &MainWindow::on_player_title));
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
    presets_[static_cast<std::size_t>(i)].set_size_request(72, 44);
    presets_[static_cast<std::size_t>(i)].signal_clicked().connect([this, i]() { on_preset(i); });
    preset_row_.pack_start(presets_[static_cast<std::size_t>(i)], Gtk::PACK_EXPAND_WIDGET);
  }
  memory_.set_size_request(92, 44);
  memory_changed_ = memory_.signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_memory));
  preset_row_.pack_start(memory_, Gtk::PACK_SHRINK);
  client_.pack_start(preset_row_, Gtk::PACK_SHRINK);

  lcd_box_.get_style_context()->add_class("listenomatic-lcd");
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
  btn_play_.set_size_request(36, 28);
  btn_stop_.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_stop));
  btn_play_.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_play));
  seek_.set_range(0, 100);
  seek_.set_draw_value(false);
  seek_.set_hexpand(true);
  seek_.set_sensitive(false);
  seek_.set_no_show_all();
  seek_lab_.set_no_show_all();
  transport_.pack_start(btn_stop_, Gtk::PACK_SHRINK);
  transport_.pack_start(btn_play_, Gtk::PACK_SHRINK);
  transport_.pack_start(seek_, Gtk::PACK_EXPAND_WIDGET);
  transport_.pack_start(seek_lab_, Gtk::PACK_SHRINK);
  client_.pack_start(transport_, Gtk::PACK_SHRINK);

  program_store_ = Gtk::ListStore::create(program_cols_);
  programs_.set_model(program_store_);
  programs_.append_column("Program", program_cols_.title);
  programs_.append_column("Date", program_cols_.date);
  programs_.append_column("Time", program_cols_.length);
  if (auto* c = programs_.get_column(0))
    c->set_expand(true);
  programs_scroll_.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  programs_scroll_.set_min_content_height(160);
  programs_scroll_.set_size_request(-1, 160);
  programs_scroll_.add(programs_);
  programs_frame_.add(programs_scroll_);
  client_.pack_start(programs_frame_, Gtk::PACK_EXPAND_WIDGET);

  root_.pack_start(menubar_, Gtk::PACK_SHRINK);
  root_.pack_start(client_, Gtk::PACK_EXPAND_WIDGET);
  root_.pack_start(status_, Gtk::PACK_SHRINK);
  add(root_);

  live_.signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_band_live));
  shows_.signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_band_shows));
  fill_memory();
  if (settings_.band == Band::Shows)
    shows_.set_active(true);
  else
    live_.set_active(true);
  show_all();
  apply_band();
  settings_.save();
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

void MainWindow::fill_memory()
{
  memory_changed_.block();
  memory_.remove_all();
  if (settings_.live.empty()) {
    memory_.append("-1", "(empty)");
    memory_.set_active(0);
  } else {
    for (int i = 0; i < static_cast<int>(settings_.live.size()); ++i) {
      memory_.append(Glib::ustring::compose("%1", i),
                     settings_.live[static_cast<std::size_t>(i)].name);
    }
    if (settings_.current_live >= 0)
      memory_.set_active_id(Glib::ustring::compose("%1", settings_.current_live));
    else
      memory_.set_active(0);
  }
  memory_changed_.unblock();
}

const Station* MainWindow::current() const
{
  if (settings_.current_live < 0 ||
      settings_.current_live >= static_cast<int>(settings_.live.size()))
    return nullptr;
  return &settings_.live[static_cast<std::size_t>(settings_.current_live)];
}

void MainWindow::refresh_face()
{
  const bool shows = shows_.get_active();
  const Station* st = current();
  const bool playing = player_.state() == Player::State::Playing;

  if (shows) {
    lcd_badge_.set_text("SHOWS");
    lcd_station_.set_text("No station");
    lcd_now_.set_text("");
    lcd_hint_.set_text("Shows are M4.");
    set_status("Shows — add a podcast in M4");
    return;
  }

  lcd_badge_.set_text(playing ? "ON AIR" : "LIVE");
  if (!st) {
    lcd_station_.set_text("No station");
    lcd_now_.set_text("");
    lcd_hint_.set_text("Station → Add… or pick Memory.");
    set_status("No stations in memory");
    return;
  }
  lcd_station_.set_text(st->name);
  if (!playing)
    lcd_now_.set_text("");
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
  const bool shows = shows_.get_active();
  settings_.band = shows ? Band::Shows : Band::Live;
  seek_.set_visible(shows);
  seek_lab_.set_visible(shows);
  if (shows) {
    if (program_store_->children().empty()) {
      auto row = *program_store_->append();
      row[program_cols_.title] = "(no programs yet — M4)";
      row[program_cols_.date] = "";
      row[program_cols_.length] = "";
    }
    programs_frame_.show();
  } else {
    programs_frame_.hide();
  }
  refresh_face();
  settings_.save();
}

void MainWindow::play_current()
{
  const Station* st = current();
  if (!st) {
    set_status("Add a live stream first");
    return;
  }
  if (!player_.open(st->url))
    return;
  player_.set_volume(settings_.volume);
  player_.play();
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
  Gtk::Dialog dlg("Add Station", *this, true);
  dlg.set_default_size(460, 280);
  dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dlg.add_button("_Add", Gtk::RESPONSE_OK);
  dlg.set_default_response(Gtk::RESPONSE_OK);

  auto* box = dlg.get_content_area();
  box->set_border_width(10);
  box->set_spacing(8);

  Gtk::Box type_row{Gtk::ORIENTATION_HORIZONTAL, 12};
  Gtk::Label type_lab{"Type"};
  type_lab.set_width_chars(6);
  type_lab.set_xalign(1);
  Gtk::RadioButton type_live{"Live stream"};
  Gtk::RadioButton type_show{"Show"};
  type_show.join_group(type_live);
  type_row.pack_start(type_lab, Gtk::PACK_SHRINK);
  type_row.pack_start(type_live, Gtk::PACK_SHRINK);
  type_row.pack_start(type_show, Gtk::PACK_SHRINK);

  Gtk::Box name_row{Gtk::ORIENTATION_HORIZONTAL, 8};
  Gtk::Label name_lab{"Name"};
  name_lab.set_width_chars(6);
  name_lab.set_xalign(1);
  Gtk::Entry name;
  name_row.pack_start(name_lab, Gtk::PACK_SHRINK);
  name_row.pack_start(name, Gtk::PACK_EXPAND_WIDGET);

  Gtk::Box url_row{Gtk::ORIENTATION_HORIZONTAL, 8};
  Gtk::Label url_lab{"URL"};
  url_lab.set_width_chars(6);
  url_lab.set_xalign(1);
  Gtk::Entry url;
  url_row.pack_start(url_lab, Gtk::PACK_SHRINK);
  url_row.pack_start(url, Gtk::PACK_EXPAND_WIDGET);

  Gtk::Label search_note{"Live: paste a stream URL. Search is M3. Shows are M4."};
  search_note.set_xalign(0);

  box->pack_start(type_row, Gtk::PACK_SHRINK);
  box->pack_start(name_row, Gtk::PACK_SHRINK);
  box->pack_start(url_row, Gtk::PACK_SHRINK);
  box->pack_start(search_note, Gtk::PACK_SHRINK);
  dlg.show_all();
  if (dlg.run() != Gtk::RESPONSE_OK)
    return;
  if (type_show.get_active()) {
    set_status("Shows are M4");
    return;
  }
  Station st;
  st.name = name.get_text();
  st.url = url.get_text();
  if (st.url.empty()) {
    set_status("Need a stream URL");
    return;
  }
  if (st.name.empty())
    st.name = st.url;
  st.short_name = make_short_name(st.name);
  for (int i = 0; i < static_cast<int>(settings_.live.size()); ++i) {
    if (settings_.live[static_cast<std::size_t>(i)].url == st.url) {
      settings_.current_live = i;
      fill_memory();
      settings_.save();
      refresh_face();
      return;
    }
  }
  settings_.live.push_back(std::move(st));
  settings_.current_live = static_cast<int>(settings_.live.size()) - 1;
  fill_memory();
  settings_.save();
  refresh_face();
}

void MainWindow::on_station_remove()
{
  set_status("Remove is M2");
}

void MainWindow::on_store_preset(int slot)
{
  set_status(Glib::ustring::compose("Store on Preset %1 is M2", slot + 1));
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
  player_.stop();
  refresh_face();
}

void MainWindow::on_play()
{
  if (player_.state() == Player::State::Playing) {
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
  set_status(Glib::ustring::compose("Preset %1 is M2", slot + 1));
}

void MainWindow::on_memory()
{
  const Glib::ustring id = memory_.get_active_id();
  if (id.empty() || id == "-1")
    return;
  settings_.current_live = std::atoi(id.c_str());
  settings_.save();
  play_current();
}

void MainWindow::on_player_state(Player::State)
{
  btn_play_.set_label(player_.state() == Player::State::Playing ? "❚❚" : "►");
  refresh_face();
}

void MainWindow::on_player_error(const Glib::ustring& msg)
{
  set_status(msg);
}

void MainWindow::on_player_title(const Glib::ustring& title)
{
  if (shows_.get_active())
    return;
  lcd_now_.set_text(title);
}

}  // namespace listenomatic
