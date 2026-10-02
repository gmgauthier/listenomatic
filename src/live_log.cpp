/* SPDX-License-Identifier: Unlicense */

#include "live_log.hpp"

#include <utility>

namespace listenomatic {

bool LiveLog::on_title(const std::string& station, const std::string& title,
                       const std::string& date, const std::string& heard)
{
  if (title.empty())
    return false;
  auto& list = tracks_[station];
  if (!list.empty() && list.front().title == title)
    return false;
  list.insert(list.begin(), HeardTrack{title, date, heard});
  if (list.size() > kMax)
    list.resize(kMax);
  return true;
}

const std::vector<HeardTrack>& LiveLog::tracks(const std::string& station) const
{
  static const std::vector<HeardTrack> none;
  const auto it = tracks_.find(station);
  return it == tracks_.end() ? none : it->second;
}

}  // namespace listenomatic
