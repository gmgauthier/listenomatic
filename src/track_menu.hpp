/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <glib.h>

#include <string>

namespace listenomatic {

/* A heard title has both a name and the date it was logged. The empty-list
 * placeholders only fill the title column. */
inline bool track_row_actionable(const std::string& title, const std::string& date)
{
  return !title.empty() && !date.empty();
}

/* Desktop URI for a web search of the track string, exactly as shown. */
inline std::string track_web_search_uri(const std::string& title)
{
  if (title.empty())
    return {};
  gchar* raw = g_uri_escape_string(title.c_str(), nullptr, FALSE);
  const std::string esc = raw ? raw : "";
  g_free(raw);
  return "https://www.google.com/search?q=" + esc;
}

}  // namespace listenomatic
