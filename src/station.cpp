/* SPDX-License-Identifier: Unlicense */

#include "station.hpp"

namespace listenomatic {

std::string make_short_name(const std::string& name)
{
  std::string s = name;
  while (s.size() >= 4 && (s.compare(0, 4, "The ") == 0 || s.compare(0, 4, "THE ") == 0))
    s = s.substr(4);
  auto sp = s.find(' ');
  if (sp != std::string::npos && sp > 0 && sp <= 8)
    s = s.substr(0, sp);
  else if (s.size() > 8)
    s = s.substr(0, 8);
  return s.empty() ? "—" : s;
}

}  // namespace listenomatic
