/* SPDX-License-Identifier: Unlicense */

#include "player.hpp"

namespace listenomatic {
namespace {

std::string as_uri(const std::string& url)
{
  if (url.compare(0, 4, "http") == 0 || url.compare(0, 4, "file") == 0)
    return url;
  return {};
}

}  // namespace

Player::Player()
{
  playbin_ = gst_element_factory_make("playbin", "listenomatic-playbin");
  if (!playbin_)
    return;

  GstElement* vsink = gst_element_factory_make("fakesink", "listenomatic-vsink");
  if (vsink) {
    g_object_set(vsink, "sync", TRUE, nullptr);
    g_object_set(playbin_, "video-sink", vsink, nullptr);
  }
  g_object_set(playbin_, "volume", volume_, nullptr);

  GstBus* bus = gst_element_get_bus(playbin_);
  bus_watch_id_ = gst_bus_add_watch(bus, &Player::on_bus, this);
  gst_object_unref(bus);
}

Player::~Player()
{
  if (bus_watch_id_) {
    g_source_remove(bus_watch_id_);
    bus_watch_id_ = 0;
  }
  if (playbin_) {
    gst_element_set_state(playbin_, GST_STATE_NULL);
    gst_object_unref(playbin_);
    playbin_ = nullptr;
  }
}

bool Player::open(const std::string& url)
{
  if (!playbin_) {
    signal_error_.emit("playbin is not available");
    return false;
  }
  const std::string uri = as_uri(url);
  if (uri.empty()) {
    signal_error_.emit("Not a stream URL");
    return false;
  }
  gst_element_set_state(playbin_, GST_STATE_NULL);
  uri_ = uri;
  g_object_set(playbin_, "uri", uri_.c_str(), "volume", volume_, nullptr);
  return true;
}

void Player::play()
{
  if (!playbin_ || uri_.empty())
    return;
  gst_element_set_state(playbin_, GST_STATE_PLAYING);
}

void Player::pause()
{
  if (!playbin_ || state_ != State::Playing)
    return;
  gst_element_set_state(playbin_, GST_STATE_PAUSED);
}

void Player::stop()
{
  if (!playbin_)
    return;
  gst_element_set_state(playbin_, GST_STATE_NULL);
  set_state(State::Stopped);
}

void Player::set_volume(double volume)
{
  if (volume < 0.0)
    volume = 0.0;
  if (volume > 1.0)
    volume = 1.0;
  volume_ = volume;
  if (playbin_)
    g_object_set(playbin_, "volume", volume_, nullptr);
}

void Player::set_state(State state)
{
  if (state_ == state)
    return;
  state_ = state;
  signal_state_changed_.emit(state_);
}

gboolean Player::on_bus(GstBus*, GstMessage* msg, gpointer self)
{
  auto* p = static_cast<Player*>(self);
  switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_STATE_CHANGED: {
      if (GST_MESSAGE_SRC(msg) != GST_OBJECT(p->playbin_))
        break;
      GstState old_st, new_st, pending;
      gst_message_parse_state_changed(msg, &old_st, &new_st, &pending);
      if (new_st == GST_STATE_PLAYING)
        p->set_state(State::Playing);
      else if (new_st == GST_STATE_PAUSED && p->state_ != State::Stopped)
        p->set_state(State::Paused);
      else if (new_st == GST_STATE_NULL || new_st == GST_STATE_READY)
        p->set_state(State::Stopped);
      break;
    }
    case GST_MESSAGE_ERROR: {
      GError* err = nullptr;
      gst_message_parse_error(msg, &err, nullptr);
      Glib::ustring text = err ? err->message : "Playback error";
      if (err)
        g_error_free(err);
      p->signal_error_.emit(text);
      p->stop();
      break;
    }
    case GST_MESSAGE_TAG: {
      GstTagList* tags = nullptr;
      gst_message_parse_tag(msg, &tags);
      if (tags) {
        gchar* title = nullptr;
        gst_tag_list_get_string(tags, GST_TAG_TITLE, &title);
        if (title) {
          p->signal_title_.emit(Glib::ustring(title));
          g_free(title);
        }
        gst_tag_list_unref(tags);
      }
      break;
    }
    default:
      break;
  }
  return TRUE;
}

}  // namespace listenomatic
