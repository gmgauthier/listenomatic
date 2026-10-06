/* SPDX-License-Identifier: Unlicense */

#include "resume.hpp"

namespace listenomatic {
namespace {

constexpr std::int64_t kSec = 1000000000LL;

}  // namespace

void ResumeTracker::start(const std::string& enclosure, std::int64_t feed_duration_ns)
{
  enclosure_ = enclosure;
  feed_duration_ns_ = feed_duration_ns;
}

void ResumeTracker::forget()
{
  enclosure_.clear();
  feed_duration_ns_ = 0;
}

bool ResumeTracker::save(Settings& settings, std::int64_t pos_ns,
                         std::int64_t player_duration_ns) const
{
  if (enclosure_.empty())
    return false;
  const std::int64_t dur = player_duration_ns > 0 ? player_duration_ns : feed_duration_ns_;
  if (pos_ns < 3 * kSec)
    settings.clear_resume(enclosure_);
  else if (dur > 0 && pos_ns >= dur - 5 * kSec)
    settings.clear_resume(enclosure_);
  else
    settings.set_resume(enclosure_, pos_ns);
  return true;
}

bool handoff_playing_show(ResumeTracker& playing, Settings& settings, std::int64_t pos_ns,
                          std::int64_t player_duration_ns, bool player_stopped)
{
  bool wrote = false;
  if (playing.active() && !player_stopped)
    wrote = playing.save(settings, pos_ns, player_duration_ns);
  playing.forget();
  return wrote;
}

bool store_ending_position(ResumeTracker& playing, Settings& settings, std::int64_t pos_ns,
                           std::int64_t player_duration_ns)
{
  if (!playing.active())
    return false;
  return playing.save(settings, pos_ns, player_duration_ns);
}

}  // namespace listenomatic
