/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "station.hpp"

#include <array>
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

  void load();
  void save() const;
};

}  // namespace listenomatic
