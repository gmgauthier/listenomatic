/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "station.hpp"

#include <gtkmm.h>

#include <memory>
#include <vector>

namespace listenomatic {

class CatalogWindow : public Gtk::Window {
 public:
  CatalogWindow();
  ~CatalogWindow() override;

  sigc::signal<void, Station, bool>& signal_add()
  {
    return signal_add_;
  }

 private:
  void load_podcasts();
  void fill_live(const std::vector<Station>& hits);
  void fill_shows(const std::vector<Station>& hits);
  void show_starter();
  void on_live_search();
  void on_live_popular();
  void on_show_search();
  void on_show_starter();
  void on_show_query();
  void on_add_clicked();
  void apply_live(std::vector<Station> hits, std::string error);
  void apply_shows(std::vector<Station> hits, std::string error);
  bool selected_live(Station* out);
  bool selected_show(Station* out);

  Gtk::Box root_{Gtk::ORIENTATION_VERTICAL, 8};
  Gtk::Notebook notebook_;

  Gtk::Box live_page_{Gtk::ORIENTATION_VERTICAL, 6};
  Gtk::Entry live_query_;
  Gtk::Button live_search_btn_{"Search"};
  Gtk::Button live_popular_btn_{"Popular"};
  Gtk::Label live_status_;
  Gtk::TreeView live_view_;
  class LiveColumns : public Gtk::TreeModel::ColumnRecord {
   public:
    LiveColumns()
    {
      add(name);
      add(place);
      add(codec);
      add(bitrate);
      add(url);
    }
    Gtk::TreeModelColumn<Glib::ustring> name;
    Gtk::TreeModelColumn<Glib::ustring> place;
    Gtk::TreeModelColumn<Glib::ustring> codec;
    Gtk::TreeModelColumn<Glib::ustring> bitrate;
    Gtk::TreeModelColumn<Glib::ustring> url;
  };
  LiveColumns live_cols_;
  Glib::RefPtr<Gtk::ListStore> live_store_;
  Gtk::ScrolledWindow live_scroll_;

  Gtk::Box show_page_{Gtk::ORIENTATION_VERTICAL, 6};
  Gtk::Entry show_query_;
  Gtk::Button show_search_btn_{"Search"};
  Gtk::Button show_starter_btn_{"Starter"};
  Gtk::Label show_status_;
  Gtk::TreeView show_view_;
  class ShowColumns : public Gtk::TreeModel::ColumnRecord {
   public:
    ShowColumns()
    {
      add(name);
      add(place);
      add(url);
    }
    Gtk::TreeModelColumn<Glib::ustring> name;
    Gtk::TreeModelColumn<Glib::ustring> place;
    Gtk::TreeModelColumn<Glib::ustring> url;
  };
  ShowColumns show_cols_;
  Glib::RefPtr<Gtk::ListStore> show_store_;
  Gtk::ScrolledWindow show_scroll_;
  std::vector<Station> podcasts_;

  Gtk::ButtonBox buttons_{Gtk::ORIENTATION_HORIZONTAL};
  Gtk::Button add_btn_{"Add to Memory"};
  Gtk::Button close_btn_{"Close"};

  std::shared_ptr<bool> alive_;
  Glib::RefPtr<Gio::Cancellable> live_cancel_;
  Glib::RefPtr<Gio::Cancellable> show_cancel_;
  bool live_busy_ = false;
  bool show_busy_ = false;
  bool show_starter_ = true;
  sigc::signal<void, Station, bool> signal_add_;
};

}  // namespace listenomatic
