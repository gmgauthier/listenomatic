/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <string>

namespace listenomatic {

enum class Band { Live, Shows };

struct Settings {
  Band band = Band::Live;
  double volume = 0.72;

  void load();
  void save() const;
};

}  // namespace listenomatic
