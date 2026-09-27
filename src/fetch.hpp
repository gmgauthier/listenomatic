/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <gio/gio.h>

#include <string>

namespace listenomatic {

/* Blocking GET. Call from a worker, not the GTK loop. */
std::string http_get(const std::string& url, std::string& error, GCancellable* cancel = nullptr);

}  // namespace listenomatic
