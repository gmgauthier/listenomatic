/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "station.hpp"

#include <gio/gio.h>

#include <string>
#include <vector>

namespace listenomatic {

/* Look up podcasts on Apple's iTunes Search API. No account. Skip hits with
 * no public RSS feedUrl. Blocking. Call from a worker. */
std::vector<Station> search_itunes_podcasts(const std::string& term, std::string& error,
                                            GCancellable* cancel = nullptr);

}  // namespace listenomatic
