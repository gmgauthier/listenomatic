/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "station.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace listenomatic {

enum class Band { Live, Shows };

struct Settings {
  Band band = Band::Live;
  double volume = 0.72;
  std::vector<Station> live;
  int current_live = -1;
  std::array<int, 6> live_presets{{-1, -1, -1, -1, -1, -1}};
  std::vector<Station> shows;
  int current_show = -1;
  std::array<int, 6> show_presets{{-1, -1, -1, -1, -1, -1}};

  struct Resume {
    std::string enclosure;
    std::int64_t position_ns = 0;
  };
  std::vector<Resume> resumes;

  /* Last episode played, keyed by the show's RSS URL. */
  struct LastProgram {
    std::string feed;
    std::string enclosure;
  };
  std::vector<LastProgram> last_programs;

  /* 0 means the user has not resized the frame yet. */
  int window_w = 0;
  int window_h = 0;

  std::int64_t resume_for(const std::string& enclosure) const;
  void set_resume(const std::string& enclosure, std::int64_t position_ns);
  void clear_resume(const std::string& enclosure);
  std::string last_program_for(const std::string& feed) const;
  void set_last_program(const std::string& feed, const std::string& enclosure);

  void load();
  void save() const;
};

}  // namespace listenomatic
