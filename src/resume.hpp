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

/* The episode reached the end, or playback failed. Store pos_ns from before
 * stop() zeros the player. Near either end this clears the resume, so a
 * finished episode does not keep an earlier pause. A position in the middle
 * replaces that pause. Nothing is written when no episode is tracked. */
bool store_ending_position(ResumeTracker& playing, Settings& settings, std::int64_t pos_ns,
                           std::int64_t player_duration_ns);

/* A resume seek often fails until the demuxer can answer it. Keep the target
 * and spend a bounded number of tries. A success or a user seek clears it. */
class ResumeSeek {
 public:
  static constexpr int kMaxAttempts = 40;

  void arm(std::int64_t ns)
  {
    target_ = ns > 0 ? ns : 0;
    attempts_ = 0;
  }
  void clear()
  {
    target_ = 0;
    attempts_ = 0;
  }
  bool pending() const
  {
    return target_ > 0;
  }
  std::int64_t target() const
  {
    return target_;
  }

  /* The next target to try, or 0 when nothing is pending or the budget is spent. */
  std::int64_t begin_attempt()
  {
    if (target_ <= 0)
      return 0;
    if (attempts_ >= kMaxAttempts) {
      clear();
      return 0;
    }
    ++attempts_;
    return target_;
  }
  void note_success()
  {
    clear();
  }

 private:
  std::int64_t target_ = 0;
  int attempts_ = 0;
};

/* A pending seek belongs to one episode. Playback of that episode returns the
 * next target to try. Live, and leaving Shows, are not that episode: the seek
 * is dropped and nothing is attempted. */
inline std::int64_t resume_attempt_for(ResumeSeek& seek, bool episode)
{
  if (!episode) {
    seek.clear();
    return 0;
  }
  return seek.begin_attempt();
}

}  // namespace listenomatic
