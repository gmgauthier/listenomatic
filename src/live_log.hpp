/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <map>
#include <string>
#include <vector>

namespace listenomatic {

struct HeardTrack {
  std::string title;
  std::string date;
  std::string heard;
};

/* Per-station ICY title history for the Tracks list. The title playing now is
 * a track from the moment it arrives, listed above the ones already heard. */
class LiveLog {
 public:
  /* Record a title from the stream. Returns true when the list changed. */
  bool on_title(const std::string& station, const std::string& title, const std::string& date,
                const std::string& heard);
  /* Newest first, the current title included. At most kMax entries. */
  const std::vector<HeardTrack>& tracks(const std::string& station) const;

  static constexpr std::size_t kMax = 80;

 private:
  std::map<std::string, std::vector<HeardTrack>> tracks_;
};

}  // namespace listenomatic
