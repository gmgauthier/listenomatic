/* SPDX-License-Identifier: Unlicense */

#include "add_dialog.hpp"
#include "radiobrowser.hpp"

#include <giomm.h>
#include <glibmm/main.h>

#include <thread>
#include <utility>

namespace listenomatic {

AddDialog::AddDialog(Gtk::Window& parent)
    : Gtk::Dialog("Add Station", parent, true),
      alive_(std::make_shared<bool>(true)),
      cancel_(Gio::Cancellable::create())
{
  set_default_size(540, 420);
  add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  add_button("_Add", Gtk::RESPONSE_OK);
  set_default_response(Gtk::RESPONSE_OK);

  auto* box = get_content_area();
  box->set_border_width(10);
  box->set_spacing(8);

  auto* type_row = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 12));
  auto* type_lab = Gtk::manage(new Gtk::Label("Type"));
  type_lab->set_width_chars(6);
  type_lab->set_xalign(1);
  type_show_.join_group(type_live_);
  type_row->pack_start(*type_lab, Gtk::PACK_SHRINK);
  type_row->pack_start(type_live_, Gtk::PACK_SHRINK);
  type_row->pack_start(type_show_, Gtk::PACK_SHRINK);
  type_live_.signal_toggled().connect(sigc::mem_fun(*this, &AddDialog::on_type));

  auto* name_row = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
  auto* name_lab = Gtk::manage(new Gtk::Label("Name"));
  name_lab->set_width_chars(6);
  name_lab->set_xalign(1);
  name_row->pack_start(*name_lab, Gtk::PACK_SHRINK);
  name_row->pack_start(name_, Gtk::PACK_EXPAND_WIDGET);

  auto* url_row = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
  auto* url_lab = Gtk::manage(new Gtk::Label("URL"));
  url_lab->set_width_chars(6);
  url_lab->set_xalign(1);
  url_row->pack_start(*url_lab, Gtk::PACK_SHRINK);
  url_row->pack_start(url_, Gtk::PACK_EXPAND_WIDGET);

  auto* qrow = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 6));
  url_.set_placeholder_text("stream URL or podcast RSS");
  query_.set_placeholder_text("name, place, or call letters");
  query_.signal_activate().connect(sigc::mem_fun(*this, &AddDialog::on_search));
  search_btn_.signal_clicked().connect(sigc::mem_fun(*this, &AddDialog::on_search));
  qrow->pack_start(query_, Gtk::PACK_EXPAND_WIDGET);
  qrow->pack_start(search_btn_, Gtk::PACK_SHRINK);

  search_status_.set_xalign(0);
  search_status_.set_text("Looks up live streams on radio-browser.info.");

  hit_store_ = Gtk::ListStore::create(hit_cols_);
  hits_.set_model(hit_store_);
  hits_.append_column("Station", hit_cols_.name);
  hits_.append_column("Place", hit_cols_.place);
  hits_.append_column("Codec", hit_cols_.codec);
  hits_.append_column("kbps", hit_cols_.bitrate);
  if (auto* c = hits_.get_column(0))
    c->set_expand(true);
  hits_.signal_cursor_changed().connect(sigc::mem_fun(*this, &AddDialog::on_hit));
  hits_scroll_.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  hits_scroll_.set_min_content_height(140);
  hits_scroll_.add(hits_);

  search_box_.pack_start(*qrow, Gtk::PACK_SHRINK);
  search_box_.pack_start(search_status_, Gtk::PACK_SHRINK);
  search_box_.pack_start(hits_scroll_, Gtk::PACK_EXPAND_WIDGET);

  box->pack_start(*type_row, Gtk::PACK_SHRINK);
  box->pack_start(*name_row, Gtk::PACK_SHRINK);
  box->pack_start(*url_row, Gtk::PACK_SHRINK);
  box->pack_start(search_box_, Gtk::PACK_EXPAND_WIDGET);
  show_all();
}

AddDialog::~AddDialog()
{
  *alive_ = false;
  cancel_->cancel();
}

bool AddDialog::is_show() const
{
  return type_show_.get_active();
}

Station AddDialog::station() const
{
  Station st;
  st.name = name_.get_text();
  st.url = url_.get_text();
  if (st.name.empty())
    st.name = st.url;
  st.short_name = make_short_name(st.name);
  return st;
}

void AddDialog::on_type()
{
  search_box_.set_visible(type_live_.get_active());
}

void AddDialog::on_search()
{
  if (!type_live_.get_active() || searching_)
    return;
  const Glib::ustring term = query_.get_text();
  if (term.empty()) {
    search_status_.set_text("Type a name, place, or call letters.");
    return;
  }
  searching_ = true;
  search_btn_.set_sensitive(false);
  search_status_.set_text("Searching radio-browser.info…");
  cancel_->cancel();
  cancel_ = Gio::Cancellable::create();
  auto alive = alive_;
  auto cancel = cancel_;
  std::thread([this, alive, cancel, term]() {
    std::string err;
    auto hits = search_radio_browser(term, err, cancel->gobj());
    Glib::signal_idle().connect_once([this, alive, hits = std::move(hits), err = std::move(err)]() {
      if (!*alive)
        return;
      apply_hits(hits, err);
    });
  }).detach();
}

void AddDialog::apply_hits(std::vector<Station> hits, std::string error)
{
  searching_ = false;
  search_btn_.set_sensitive(true);
  hit_store_->clear();
  for (const auto& st : hits) {
    auto row = *hit_store_->append();
    row[hit_cols_.name] = st.name;
    row[hit_cols_.place] = st.place;
    row[hit_cols_.codec] = st.codec;
    row[hit_cols_.bitrate] = st.bitrate > 0 ? Glib::ustring::compose("%1", st.bitrate) : "";
    row[hit_cols_.url] = st.url;
  }
  if (!error.empty())
    search_status_.set_text(error);
  else
    search_status_.set_text(Glib::ustring::compose("%1 live stations", hits.size()));
}

void AddDialog::on_hit()
{
  auto sel = hits_.get_selection();
  if (!sel)
    return;
  auto iter = sel->get_selected();
  if (!iter)
    return;
  name_.set_text((*iter)[hit_cols_.name]);
  url_.set_text((*iter)[hit_cols_.url]);
}

}  // namespace listenomatic
