/* SPDX-License-Identifier: Unlicense */

#include "check.hpp"
#include "track_menu.hpp"

#include <string>

int main()
{
  CHECK(!listenomatic::track_row_actionable("", "2026-10-02"));
  CHECK(!listenomatic::track_row_actionable("Tracks appear here when the station sends titles", ""));
  CHECK(!listenomatic::track_row_actionable("No station", ""));
  CHECK(listenomatic::track_row_actionable("Song & Band", "2026-10-02"));

  CHECK(listenomatic::track_web_search_uri("").empty());
  const std::string uri = listenomatic::track_web_search_uri("Song & Band");
  CHECK(uri == "https://www.google.com/search?q=Song%20%26%20Band");

  return suite_test::done("track-menu");
}
