/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <glibmm/ustring.h>
#include <sigc++/signal.h>

#include <gst/gst.h>

#include <string>

namespace listenomatic {

class Player {
 public:
  enum class State { Stopped, Playing, Paused };

  Player();
  ~Player();

  Player(const Player&) = delete;
  Player& operator=(const Player&) = delete;

  bool open(const std::string& url);
  void play();
  void pause();
  void stop();
  void set_volume(double volume);

  State state() const
  {
    return state_;
  }

  sigc::signal<void, State>& signal_state_changed()
  {
    return signal_state_changed_;
  }
  sigc::signal<void, Glib::ustring>& signal_error()
  {
    return signal_error_;
  }
  sigc::signal<void, Glib::ustring>& signal_title()
  {
    return signal_title_;
  }

 private:
  static gboolean on_bus(GstBus* bus, GstMessage* msg, gpointer self);
  void set_state(State state);

  GstElement* playbin_ = nullptr;
  guint bus_watch_id_ = 0;
  std::string uri_;
  State state_ = State::Stopped;
  double volume_ = 0.72;

  sigc::signal<void, State> signal_state_changed_;
  sigc::signal<void, Glib::ustring> signal_error_;
  sigc::signal<void, Glib::ustring> signal_title_;
};

}  // namespace listenomatic
