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
  void seek(gint64 ns);
  void set_volume(double volume);
  void refresh_position();

  State state() const
  {
    return state_;
  }
  gint64 position() const
  {
    return position_;
  }
  gint64 duration() const
  {
    return duration_;
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
  sigc::signal<void, gint64, gint64>& signal_position()
  {
    return signal_position_;
  }

 private:
  static gboolean on_bus(GstBus* bus, GstMessage* msg, gpointer self);
  static gboolean on_position_timeout(gpointer self);
  void set_state(State state);
  void start_position_timer();
  void stop_position_timer();
  void query_position();

  GstElement* playbin_ = nullptr;
  guint bus_watch_id_ = 0;
  guint pos_timer_id_ = 0;
  std::string uri_;
  State state_ = State::Stopped;
  double volume_ = 0.72;
  gint64 position_ = 0;
  gint64 duration_ = 0;

  sigc::signal<void, State> signal_state_changed_;
  sigc::signal<void, Glib::ustring> signal_error_;
  sigc::signal<void, Glib::ustring> signal_title_;
  sigc::signal<void, gint64, gint64> signal_position_;
};

}  // namespace listenomatic
