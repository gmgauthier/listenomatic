/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <string>

namespace listenomatic {

struct Station {
  std::string name;
  std::string short_name;
  std::string url;
  std::string codec;
  int bitrate = 0;
};

std::string make_short_name(const std::string& name);

}  // namespace listenomatic
