/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "station.hpp"

#include <gio/gio.h>

#include <string>
#include <vector>

namespace listenomatic {

/* Look up live streams on radio-browser.info. Prunes lastcheckok != 1.
 * Blocking. Call from a worker. */
std::vector<Station> search_radio_browser(const std::string& term, std::string& error,
                                          GCancellable* cancel = nullptr);

}  // namespace listenomatic
