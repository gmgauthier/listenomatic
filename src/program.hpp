/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace listenomatic {

struct Program {
  std::string title;
  std::string date;
  std::string length;
  std::string enclosure;
  std::int64_t duration_ns = 0;
};

struct PodcastFeed {
  std::string title;
  std::vector<Program> programs;
};

/* The programs list is about to show "Loading programs…". Drop the previous
 * show's episodes so that placeholder row cannot start one of them. */
inline void clear_programs_for_load(std::vector<Program>& episodes, int& current_program)
{
  episodes.clear();
  current_program = -1;
}

/* A row index is an episode only while the episode vector still has that slot.
 * The loading placeholder is not an episode. */
inline bool episode_playable(int index, std::size_t episode_count)
{
  return index >= 0 && static_cast<std::size_t>(index) < episode_count;
}

}  // namespace listenomatic
