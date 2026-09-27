/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "program.hpp"

#include <string>

namespace listenomatic {

/* Parse RSS 2.0 / iTunes podcast XML. Items without an audio enclosure are skipped. */
bool parse_podcast(const std::string& xml, PodcastFeed& out, std::string& error);

}  // namespace listenomatic
