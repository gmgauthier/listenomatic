/* SPDX-License-Identifier: Unlicense */

#include "check.hpp"
#include "resume.hpp"

#include <cstdint>

int main()
{
  constexpr std::int64_t kTarget = 120 * 1000000000LL;
  {
    listenomatic::ResumeSeek seek;
    CHECK(!seek.pending());
    CHECK(seek.begin_attempt() == 0);
    seek.arm(kTarget);
    CHECK(seek.pending());
    CHECK(seek.target() == kTarget);
    // A rejected seek keeps the target for the next try.
    CHECK(seek.begin_attempt() == kTarget);
    CHECK(seek.pending());
    CHECK(seek.target() == kTarget);
    seek.note_success();
    CHECK(!seek.pending());
    CHECK(seek.begin_attempt() == 0);
  }
  {
    // The budget is finite. After the last rejection the target is dropped.
    listenomatic::ResumeSeek seek;
    seek.arm(kTarget);
    for (int i = 0; i < listenomatic::ResumeSeek::kMaxAttempts; ++i)
      CHECK(seek.begin_attempt() == kTarget);
    CHECK(seek.pending());
    CHECK(seek.begin_attempt() == 0);
    CHECK(!seek.pending());
  }
  {
    // A new episode replaces the previous target and its spent tries.
    listenomatic::ResumeSeek seek;
    seek.arm(kTarget);
    CHECK(seek.begin_attempt() == kTarget);
    constexpr std::int64_t kOther = 40 * 1000000000LL;
    seek.arm(kOther);
    CHECK(seek.target() == kOther);
    CHECK(seek.begin_attempt() == kOther);
    // A user seek or Stop drops it.
    seek.clear();
    CHECK(!seek.pending());
    CHECK(seek.begin_attempt() == 0);
  }
  {
    listenomatic::ResumeSeek seek;
    seek.arm(0);
    CHECK(!seek.pending());
    seek.arm(-1);
    CHECK(!seek.pending());
  }
  {
    // Live, or leaving Shows, drops a seek armed for an episode.
    listenomatic::ResumeSeek seek;
    seek.arm(kTarget);
    CHECK(listenomatic::resume_attempt_for(seek, false) == 0);
    CHECK(!seek.pending());
    CHECK(listenomatic::resume_attempt_for(seek, true) == 0);
    // The episode itself still retries the same target.
    seek.arm(kTarget);
    CHECK(listenomatic::resume_attempt_for(seek, true) == kTarget);
    CHECK(seek.pending());
    CHECK(listenomatic::resume_attempt_for(seek, false) == 0);
    CHECK(!seek.pending());
  }
  return suite_test::done("resume-seek");
}
