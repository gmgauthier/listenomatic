/* SPDX-License-Identifier: Unlicense */

#include "catalog_window.hpp"
#include "itunes.hpp"
#include "paths.hpp"
#include "radiobrowser.hpp"

#include <giomm.h>
#include <glibmm/fileutils.h>
#include <glibmm/keyfile.h>
#include <glibmm/main.h>
#include <glibmm/miscutils.h>

#include <thread>

namespace listenomatic {

CatalogWindow::CatalogWindow()
    : alive_(std::make_shared<bool>(true)),
      live_cancel_(Gio::Cancellable::create()),
      show_cancel_(Gio::Cancellable::create())
{
  set_title("Catalog");
  set_default_size(640, 480);
  signal_delete_event().connect([this](GdkEventAny*) {
    hide();
    return true;
  });
  set_border_width(8);

  auto* live_qrow = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 6));
  live_query_.set_placeholder_text("name, place, or call letters");
  live_query_.signal_activate().connect(sigc::mem_fun(*this, &CatalogWindow::on_live_search));
  live_search_btn_.signal_clicked().connect(sigc::mem_fun(*this, &CatalogWindow::on_live_search));
  live_popular_btn_.signal_clicked().connect(sigc::mem_fun(*this, &CatalogWindow::on_live_popular));
  live_qrow->pack_start(live_query_, Gtk::PACK_EXPAND_WIDGET);
  live_qrow->pack_start(live_search_btn_, Gtk::PACK_SHRINK);
  live_qrow->pack_start(live_popular_btn_, Gtk::PACK_SHRINK);
  live_status_.set_xalign(0);
  live_status_.set_text("Popular live stations from radio-browser.info.");

  live_store_ = Gtk::ListStore::create(live_cols_);
  live_view_.set_model(live_store_);
  live_view_.append_column("Station", live_cols_.name);
  live_view_.append_column("Place", live_cols_.place);
  live_view_.append_column("Codec", live_cols_.codec);
  live_view_.append_column("kbps", live_cols_.bitrate);
  if (auto* c = live_view_.get_column(0)) {
    c->set_sizing(Gtk::TREE_VIEW_COLUMN_FIXED);
    c->set_expand(true);
    c->set_min_width(220);
  }
  if (auto* c = live_view_.get_column(1)) {
    c->set_sizing(Gtk::TREE_VIEW_COLUMN_FIXED);
    c->set_fixed_width(56);
  }
  if (auto* c = live_view_.get_column(2)) {
    c->set_sizing(Gtk::TREE_VIEW_COLUMN_FIXED);
    c->set_fixed_width(64);
  }
  if (auto* c = live_view_.get_column(3)) {
    c->set_sizing(Gtk::TREE_VIEW_COLUMN_FIXED);
    c->set_fixed_width(48);
  }
  live_view_.set_fixed_height_mode(true);
  live_view_.set_activate_on_single_click(false);
  live_view_.signal_row_activated().connect(
      [this](const Gtk::TreeModel::Path&, Gtk::TreeViewColumn*) { on_add_clicked(); });
  live_scroll_.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  live_scroll_.add(live_view_);
  live_page_.pack_start(*live_qrow, Gtk::PACK_SHRINK);
  live_page_.pack_start(live_status_, Gtk::PACK_SHRINK);
  live_page_.pack_start(live_scroll_, Gtk::PACK_EXPAND_WIDGET);

  auto* show_qrow = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 6));
  show_query_.set_placeholder_text("show name");
  show_query_.signal_activate().connect(sigc::mem_fun(*this, &CatalogWindow::on_show_search));
  show_query_.signal_changed().connect(sigc::mem_fun(*this, &CatalogWindow::on_show_query));
  show_search_btn_.signal_clicked().connect(sigc::mem_fun(*this, &CatalogWindow::on_show_search));
  show_starter_btn_.signal_clicked().connect(sigc::mem_fun(*this, &CatalogWindow::on_show_starter));
  show_qrow->pack_start(show_query_, Gtk::PACK_EXPAND_WIDGET);
  show_qrow->pack_start(show_search_btn_, Gtk::PACK_SHRINK);
  show_qrow->pack_start(show_starter_btn_, Gtk::PACK_SHRINK);
  show_status_.set_xalign(0);
  show_status_.set_text("Starter list. Type to filter, or Search iTunes for more.");
  show_store_ = Gtk::ListStore::create(show_cols_);
  show_view_.set_model(show_store_);
  show_view_.append_column("Show", show_cols_.name);
  show_view_.append_column("Kind", show_cols_.place);
  if (auto* c = show_view_.get_column(0)) {
    c->set_sizing(Gtk::TREE_VIEW_COLUMN_FIXED);
    c->set_expand(true);
    c->set_min_width(220);
  }
  if (auto* c = show_view_.get_column(1)) {
    c->set_sizing(Gtk::TREE_VIEW_COLUMN_FIXED);
    c->set_fixed_width(110);
  }
  show_view_.set_fixed_height_mode(true);
  show_view_.signal_row_activated().connect(
      [this](const Gtk::TreeModel::Path&, Gtk::TreeViewColumn*) { on_add_clicked(); });
  show_scroll_.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  show_scroll_.add(show_view_);
  show_page_.pack_start(*show_qrow, Gtk::PACK_SHRINK);
  show_page_.pack_start(show_status_, Gtk::PACK_SHRINK);
  show_page_.pack_start(show_scroll_, Gtk::PACK_EXPAND_WIDGET);

  notebook_.append_page(live_page_, "Live stations");
  notebook_.append_page(show_page_, "Podcasts");

  add_btn_.signal_clicked().connect(sigc::mem_fun(*this, &CatalogWindow::on_add_clicked));
  close_btn_.signal_clicked().connect([this]() { hide(); });
  buttons_.set_layout(Gtk::BUTTONBOX_END);
  buttons_.set_spacing(8);
  buttons_.pack_start(add_btn_);
  buttons_.pack_start(close_btn_);

  root_.pack_start(notebook_, Gtk::PACK_EXPAND_WIDGET);
  root_.pack_start(buttons_, Gtk::PACK_SHRINK);
  add(root_);
  show_all();

  load_podcasts();
  show_starter();
  on_live_popular();
}

CatalogWindow::~CatalogWindow()
{
  *alive_ = false;
  live_cancel_->cancel();
  show_cancel_->cancel();
}

void CatalogWindow::load_podcasts()
{
  podcasts_.clear();
  const std::string path = find_data_file("podcasts.ini");
  if (path.empty())
    return;
  Glib::KeyFile kf;
  try {
    kf.load_from_file(path);
  } catch (const Glib::Error&) {
    return;
  }
  if (!kf.has_group("podcasts") || !kf.has_key("podcasts", "count"))
    return;
  int n = 0;
  try {
    n = kf.get_integer("podcasts", "count");
  } catch (const Glib::Error&) {
    return;
  }
  for (int i = 0; i < n; ++i) {
    Station st;
    try {
      st.name = kf.get_string("podcasts", Glib::ustring::compose("name%1", i));
      st.url = kf.get_string("podcasts", Glib::ustring::compose("url%1", i));
    } catch (const Glib::Error&) {
      continue;
    }
    if (st.name.empty() || st.url.empty())
      continue;
    st.short_name = make_short_name(st.name);
    try {
      if (kf.has_key("podcasts", Glib::ustring::compose("place%1", i)))
        st.place = kf.get_string("podcasts", Glib::ustring::compose("place%1", i));
    } catch (const Glib::Error&) {
    }
    podcasts_.push_back(std::move(st));
  }
}

void CatalogWindow::fill_live(const std::vector<Station>& hits)
{
  live_store_->clear();
  for (const auto& st : hits) {
    auto row = *live_store_->append();
    row[live_cols_.name] = st.name;
    row[live_cols_.place] = st.place;
    row[live_cols_.codec] = st.codec;
    row[live_cols_.bitrate] = st.bitrate > 0 ? Glib::ustring::compose("%1", st.bitrate) : "";
    row[live_cols_.url] = st.url;
  }
}

void CatalogWindow::fill_shows(const std::vector<Station>& hits)
{
  show_store_->clear();
  for (const auto& st : hits) {
    auto row = *show_store_->append();
    row[show_cols_.name] = st.name;
    row[show_cols_.place] = st.place;
    row[show_cols_.url] = st.url;
  }
}

void CatalogWindow::show_starter()
{
  show_starter_ = true;
  const Glib::ustring needle = show_query_.get_text().lowercase();
  std::vector<Station> hits;
  hits.reserve(podcasts_.size());
  for (const auto& st : podcasts_) {
    if (!needle.empty() && Glib::ustring(st.name).lowercase().find(needle) == Glib::ustring::npos)
      continue;
    hits.push_back(st);
  }
  fill_shows(hits);
  if (needle.empty())
    show_status_.set_text(
        Glib::ustring::compose("%1 starter shows. Search iTunes for more.", hits.size()));
  else
    show_status_.set_text(Glib::ustring::compose("%1 starter shows matching “%2”.", hits.size(),
                                                 show_query_.get_text()));
}

void CatalogWindow::on_live_search()
{
  if (live_busy_)
    return;
  const Glib::ustring term = live_query_.get_text();
  if (term.empty()) {
    on_live_popular();
    return;
  }
  live_busy_ = true;
  live_search_btn_.set_sensitive(false);
  live_status_.set_text("Searching radio-browser.info…");
  live_cancel_->cancel();
  live_cancel_ = Gio::Cancellable::create();
  auto alive = alive_;
  auto cancel = live_cancel_;
  std::thread([this, alive, cancel, term]() {
    std::string err;
    auto hits = search_radio_browser(term, err, cancel->gobj());
    Glib::signal_idle().connect_once([this, alive, hits = std::move(hits), err = std::move(err)]() {
      if (!*alive)
        return;
      apply_live(hits, err);
    });
  }).detach();
}

void CatalogWindow::on_live_popular()
{
  if (live_busy_)
    return;
  live_busy_ = true;
  live_search_btn_.set_sensitive(false);
  live_popular_btn_.set_sensitive(false);
  live_status_.set_text("Loading popular stations…");
  live_cancel_->cancel();
  live_cancel_ = Gio::Cancellable::create();
  auto alive = alive_;
  auto cancel = live_cancel_;
  std::thread([this, alive, cancel]() {
    std::string err;
    auto hits = browse_radio_browser_popular(err, cancel->gobj());
    Glib::signal_idle().connect_once([this, alive, hits = std::move(hits), err = std::move(err)]() {
      if (!*alive)
        return;
      apply_live(hits, err);
    });
  }).detach();
}

void CatalogWindow::apply_live(std::vector<Station> hits, std::string error)
{
  live_busy_ = false;
  live_search_btn_.set_sensitive(true);
  live_popular_btn_.set_sensitive(true);
  fill_live(hits);
  if (!error.empty())
    live_status_.set_text(error);
  else
    live_status_.set_text(Glib::ustring::compose("%1 live stations", hits.size()));
}

void CatalogWindow::on_show_query()
{
  if (show_starter_)
    show_starter();
}

void CatalogWindow::on_show_starter()
{
  show_starter_ = true;
  if (!show_query_.get_text().empty())
    show_query_.set_text("");
  show_starter();
}

void CatalogWindow::on_show_search()
{
  if (show_busy_)
    return;
  const Glib::ustring term = show_query_.get_text();
  if (term.empty()) {
    show_starter();
    return;
  }
  show_busy_ = true;
  show_starter_ = false;
  show_search_btn_.set_sensitive(false);
  show_status_.set_text("Searching iTunes…");
  show_cancel_->cancel();
  show_cancel_ = Gio::Cancellable::create();
  auto alive = alive_;
  auto cancel = show_cancel_;
  std::thread([this, alive, cancel, term]() {
    std::string err;
    auto hits = search_itunes_podcasts(term, err, cancel->gobj());
    Glib::signal_idle().connect_once([this, alive, hits = std::move(hits), err = std::move(err)]() {
      if (!*alive)
        return;
      apply_shows(hits, err);
    });
  }).detach();
}

void CatalogWindow::apply_shows(std::vector<Station> hits, std::string error)
{
  show_busy_ = false;
  show_search_btn_.set_sensitive(true);
  fill_shows(hits);
  if (!error.empty())
    show_status_.set_text(error);
  else
    show_status_.set_text(Glib::ustring::compose("%1 shows from iTunes", hits.size()));
}

bool CatalogWindow::selected_live(Station* out)
{
  auto sel = live_view_.get_selection();
  if (!sel || !out)
    return false;
  Gtk::TreeModel::iterator iter = sel->get_selected();
  if (!iter)
    return false;
  out->name = Glib::ustring((*iter)[live_cols_.name]);
  out->url = Glib::ustring((*iter)[live_cols_.url]);
  out->place = Glib::ustring((*iter)[live_cols_.place]);
  out->codec = Glib::ustring((*iter)[live_cols_.codec]);
  out->short_name = make_short_name(out->name);
  return !out->url.empty();
}

bool CatalogWindow::selected_show(Station* out)
{
  auto sel = show_view_.get_selection();
  if (!sel || !out)
    return false;
  Gtk::TreeModel::iterator iter = sel->get_selected();
  if (!iter)
    return false;
  out->name = Glib::ustring((*iter)[show_cols_.name]);
  out->url = Glib::ustring((*iter)[show_cols_.url]);
  out->place = Glib::ustring((*iter)[show_cols_.place]);
  out->short_name = make_short_name(out->name);
  return !out->url.empty();
}

void CatalogWindow::on_add_clicked()
{
  Station st;
  const bool shows = notebook_.get_current_page() == 1;
  const bool ok = shows ? selected_show(&st) : selected_live(&st);
  if (!ok) {
    if (shows)
      show_status_.set_text("Select a podcast first.");
    else
      live_status_.set_text("Select a station first.");
    return;
  }
  signal_add_.emit(st, shows);
  if (shows)
    show_status_.set_text("Added “" + st.name + "” to Shows Memory.");
  else
    live_status_.set_text("Added “" + st.name + "” to Live Memory.");
}

}  // namespace listenomatic
