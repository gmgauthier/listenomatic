/* SPDX-License-Identifier: Unlicense */

#include "live_log.hpp"
#include "check.hpp"

#include <string>

int main()
{
  const std::string a = "http://example.test/a";
  const std::string b = "http://example.test/b";
  listenomatic::LiveLog log;
  CHECK(log.tracks(a).empty());

  // The first title is a track straight away.
  CHECK(log.on_title(a, "Song One", "2026-10-02", "10:00"));
  auto t = log.tracks(a);
  CHECK(t.size() == 1);
  if (t.size() == 1) {
    CHECK(t[0].title == "Song One");
    CHECK(t[0].date == "2026-10-02");
    CHECK(t[0].heard == "10:00");
  }

  // The same title again changes nothing.
  CHECK(!log.on_title(a, "Song One", "2026-10-02", "10:01"));
  CHECK(log.tracks(a).size() == 1);

  // A new title goes on top; the one just heard stays listed below it.
  CHECK(log.on_title(a, "Song Two", "2026-10-02", "10:04"));
  t = log.tracks(a);
  CHECK(t.size() == 2);
  if (t.size() == 2) {
    CHECK(t[0].title == "Song Two");
    CHECK(t[1].title == "Song One");
    CHECK(t[1].heard == "10:00");
  }

  // Stations keep separate lists. Empty titles are ignored.
  CHECK(log.tracks(b).empty());
  CHECK(!log.on_title(b, "", "2026-10-02", "10:05"));
  CHECK(log.tracks(b).empty());

  // The list, current title included, is capped at 80.
  for (int i = 0; i < 100; ++i)
    log.on_title(b, "T" + std::to_string(i), "2026-10-02", "11:00");
  t = log.tracks(b);
  CHECK(t.size() == 80);
  if (t.size() == 80) {
    CHECK(t.front().title == "T99");
    CHECK(t.back().title == "T20");
  }

  return suite_test::done("live_log");
}
