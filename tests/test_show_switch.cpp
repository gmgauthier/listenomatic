/* SPDX-License-Identifier: Unlicense */

#include "check.hpp"
#include "resume.hpp"
#include "settings.hpp"

#include <cstdint>
#include <string>

namespace {

constexpr std::int64_t kSec = 1000000000LL;
const std::string kOld = "http://shows.example/old.mp3";
const std::string kNew = "http://shows.example/new.mp3";

}  // namespace

int main()
{
  {
    // A different show is about to take the screen. The position belongs to the
    // episode that is playing, and the tracker then forgets it.
    listenomatic::Settings s;
    s.set_resume(kNew, 40 * kSec);
    listenomatic::ResumeTracker t;
    t.start(kOld, 600 * kSec);
    CHECK(listenomatic::handoff_playing_show(t, s, 120 * kSec, 600 * kSec, false));
    CHECK(!t.active());
    CHECK(s.resume_for(kOld) == 120 * kSec);
    CHECK(s.resume_for(kNew) == 40 * kSec);
    // Pause or quit after the switch has no episode to write, so neither resume moves.
    CHECK(!t.save(s, 400 * kSec, 600 * kSec));
    CHECK(s.resume_for(kOld) == 120 * kSec);
    CHECK(s.resume_for(kNew) == 40 * kSec);
  }
  {
    // The player is already stopped. The stored resume stays, and the tracker is dropped.
    listenomatic::Settings s;
    s.set_resume(kOld, 80 * kSec);
    listenomatic::ResumeTracker t;
    t.start(kOld, 600 * kSec);
    CHECK(!listenomatic::handoff_playing_show(t, s, 10 * kSec, 600 * kSec, true));
    CHECK(!t.active());
    CHECK(s.resume_for(kOld) == 80 * kSec);
  }
  {
    // Nothing is tracked (Live, or no episode yet). Handoff writes nothing.
    listenomatic::Settings s;
    s.set_resume(kOld, 80 * kSec);
    listenomatic::ResumeTracker t;
    CHECK(!listenomatic::handoff_playing_show(t, s, 50 * kSec, 0, false));
    CHECK(!t.active());
    CHECK(s.resume_for(kOld) == 80 * kSec);
  }
  {
    // Under 3 s clears the playing episode and leaves the other show's resume.
    listenomatic::Settings s;
    s.set_resume(kOld, 80 * kSec);
    s.set_resume(kNew, 40 * kSec);
    listenomatic::ResumeTracker t;
    t.start(kOld, 600 * kSec);
    CHECK(listenomatic::handoff_playing_show(t, s, 1 * kSec, 600 * kSec, false));
    CHECK(!t.active());
    CHECK(s.resume_for(kOld) == 0);
    CHECK(s.resume_for(kNew) == 40 * kSec);
  }
  {
    // Within 5 s of the end clears, using the player duration when the feed has one.
    listenomatic::Settings s;
    listenomatic::ResumeTracker t;
    t.start(kOld, 600 * kSec);
    CHECK(listenomatic::handoff_playing_show(t, s, 1196 * kSec, 1200 * kSec, false));
    CHECK(s.resume_for(kOld) == 0);
    t.start(kOld, 600 * kSec);
    CHECK(listenomatic::handoff_playing_show(t, s, 200 * kSec, 0, false));
    CHECK(s.resume_for(kOld) == 200 * kSec);
  }
  return suite_test::done("show-switch");
}
