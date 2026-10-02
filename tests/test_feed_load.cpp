/* SPDX-License-Identifier: Unlicense */

#include "check.hpp"
#include "program.hpp"

#include <string>
#include <vector>

int main()
{
  listenomatic::Program old_show;
  old_show.title = "Previous episode";
  old_show.enclosure = "http://shows.example/old.mp3";
  listenomatic::Program other;
  other.title = "Another previous episode";
  other.enclosure = "http://shows.example/other.mp3";

  std::vector<listenomatic::Program> episodes{old_show, other};
  int current = 1;
  CHECK(listenomatic::episode_playable(0, episodes.size()));
  CHECK(listenomatic::episode_playable(1, episodes.size()));

  // The loading row occupies index 0. The previous show's episodes are gone,
  // so activating that row does not start them or record their enclosure.
  listenomatic::clear_programs_for_load(episodes, current);
  CHECK(episodes.empty());
  CHECK(current == -1);
  CHECK(!listenomatic::episode_playable(0, episodes.size()));
  CHECK(!listenomatic::episode_playable(-1, episodes.size()));

  // A real episode list is playable at its own indexes only.
  episodes.push_back(old_show);
  CHECK(listenomatic::episode_playable(0, episodes.size()));
  CHECK(!listenomatic::episode_playable(1, episodes.size()));

  return suite_test::done("feed-load");
}
