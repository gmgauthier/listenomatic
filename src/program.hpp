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

}  // namespace listenomatic
