/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "settings.hpp"

#include <cstdint>
#include <string>

namespace listenomatic {

/* The episode the player is actually on. Resume positions are written against it,
 * not against whatever band, show, or program list the window shows now. */
class ResumeTracker {
 public:
  void start(const std::string& enclosure, std::int64_t feed_duration_ns);
  void forget();
  bool active() const
  {
    return !enclosure_.empty();
  }
  const std::string& enclosure() const
  {
    return enclosure_;
  }

  /* Store pos_ns as the tracked episode's resume (or clear it near either end).
   * player_duration_ns <= 0 falls back to the feed's duration. Returns false when
   * nothing is tracked. */
  bool save(Settings& settings, std::int64_t pos_ns, std::int64_t player_duration_ns) const;

 private:
  std::string enclosure_;
  std::int64_t feed_duration_ns_ = 0;
};

/* The show on screen is about to change. Store the playing episode's position
 * unless the player is already stopped, then drop the tracker so a later save
 * cannot attach this playback to the new show. Returns true when a resume row
 * was written or cleared. */
bool handoff_playing_show(ResumeTracker& playing, Settings& settings, std::int64_t pos_ns,
                          std::int64_t player_duration_ns, bool player_stopped);

}  // namespace listenomatic
