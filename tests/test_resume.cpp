/* SPDX-License-Identifier: Unlicense */

#include "resume.hpp"
#include "settings.hpp"
#include "check.hpp"

#include <cstdint>

namespace {

constexpr std::int64_t kSec = 1000000000LL;

}  // namespace

int main()
{
  {
    // Nothing tracked (a live stream, or no episode yet): a save writes nothing.
    listenomatic::Settings s;
    s.set_resume("http://example.test/a.mp3", 100 * kSec);
    listenomatic::ResumeTracker t;
    CHECK(!t.active());
    CHECK(!t.save(s, 1 * kSec, 0));
    CHECK(!t.save(s, 500 * kSec, 0));
    CHECK(s.resume_for("http://example.test/a.mp3") == 100 * kSec);
  }
  {
    // Shows -> Live: the position goes to the episode that was playing, whatever the band.
    listenomatic::Settings s;
    listenomatic::ResumeTracker t;
    t.start("http://example.test/a.mp3", 600 * kSec);
    CHECK(t.active());
    CHECK(t.enclosure() == "http://example.test/a.mp3");
    CHECK(t.save(s, 120 * kSec, 0));
    CHECK(s.resume_for("http://example.test/a.mp3") == 120 * kSec);
    // Leaving Shows forgets the episode. Live -> Shows later must not touch its resume,
    // not even with a near-zero live position.
    t.forget();
    CHECK(!t.active());
    CHECK(!t.save(s, 1 * kSec, 0));
    CHECK(!t.save(s, 300 * kSec, 0));
    CHECK(s.resume_for("http://example.test/a.mp3") == 120 * kSec);
  }
  {
    // The usual edges: under 3 s and within 5 s of the end clear the resume.
    listenomatic::Settings s;
    listenomatic::ResumeTracker t;
    t.start("http://example.test/b.mp3", 600 * kSec);
    CHECK(t.save(s, 200 * kSec, 0));
    CHECK(t.save(s, 2 * kSec, 0));
    CHECK(s.resume_for("http://example.test/b.mp3") == 0);
    CHECK(t.save(s, 200 * kSec, 0));
    CHECK(t.save(s, 597 * kSec, 0));
    CHECK(s.resume_for("http://example.test/b.mp3") == 0);
    // The player's duration wins over the feed's.
    CHECK(t.save(s, 597 * kSec, 1200 * kSec));
    CHECK(s.resume_for("http://example.test/b.mp3") == 597 * kSec);
  }
  {
    // The episode finished. The position at the end clears the earlier pause.
    listenomatic::Settings s;
    listenomatic::ResumeTracker t;
    t.start("http://example.test/a.mp3", 600 * kSec);
    s.set_resume("http://example.test/a.mp3", 80 * kSec);
    CHECK(listenomatic::store_ending_position(t, s, 598 * kSec, 600 * kSec));
    CHECK(t.active());
    CHECK(s.resume_for("http://example.test/a.mp3") == 0);
    // An error in the middle replaces that pause with the position still playing.
    s.set_resume("http://example.test/a.mp3", 80 * kSec);
    CHECK(listenomatic::store_ending_position(t, s, 200 * kSec, 600 * kSec));
    CHECK(s.resume_for("http://example.test/a.mp3") == 200 * kSec);
  }
  {
    // Live has no episode. Ending the stream leaves a show's resume alone.
    listenomatic::Settings s;
    s.set_resume("http://example.test/a.mp3", 80 * kSec);
    listenomatic::ResumeTracker t;
    CHECK(!listenomatic::store_ending_position(t, s, 0, 0));
    CHECK(s.resume_for("http://example.test/a.mp3") == 80 * kSec);
  }
  return suite_test::done("resume");
}
