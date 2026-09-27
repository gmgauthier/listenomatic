/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "station.hpp"

#include <gtkmm.h>

#include <memory>
#include <vector>

namespace listenomatic {

class AddDialog : public Gtk::Dialog {
 public:
  explicit AddDialog(Gtk::Window& parent);
  ~AddDialog() override;

  bool is_show() const;
  Station station() const;

 private:
  void on_type();
  void on_search();
  void on_hit();
  void apply_hits(std::vector<Station> hits, std::string error);

  Gtk::RadioButton type_live_{"Live stream"};
  Gtk::RadioButton type_show_{"Show"};
  Gtk::Entry name_;
  Gtk::Entry url_;
  Gtk::Box search_box_{Gtk::ORIENTATION_VERTICAL, 6};
  Gtk::Entry query_;
  Gtk::Button search_btn_{"Search"};
  Gtk::Label search_status_;
  Gtk::TreeView hits_;
  class HitColumns : public Gtk::TreeModel::ColumnRecord {
   public:
    HitColumns()
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
  HitColumns hit_cols_;
  Glib::RefPtr<Gtk::ListStore> hit_store_;
  Gtk::ScrolledWindow hits_scroll_;

  std::shared_ptr<bool> alive_;
  Glib::RefPtr<Gio::Cancellable> cancel_;
  bool searching_ = false;
};

}  // namespace listenomatic
