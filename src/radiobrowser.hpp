/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "station.hpp"

#include <gio/gio.h>

#include <string>
#include <vector>

namespace listenomatic {

inline constexpr const char* kRadioBrowserHost = "https://all.api.radio-browser.info";
inline constexpr int kRadioBrowserSearchLimit = 40;

/* Two ASCII letters, uppercased. Anything else is not a country code. */
inline bool radio_browser_country_code(const std::string& term, std::string& code)
{
  if (term.size() != 2)
    return false;
  code.resize(2);
  for (std::size_t i = 0; i < 2; ++i) {
    const unsigned char c = static_cast<unsigned char>(term[i]);
    if (c >= 'A' && c <= 'Z')
      code[i] = static_cast<char>(c);
    else if (c >= 'a' && c <= 'z')
      code[i] = static_cast<char>(c - 'a' + 'A');
    else
      return false;
  }
  return true;
}

/* radio-browser ANDs every field on one request, so a place query is its own
 * URL. Country code, country, and state are tried before the station name. */
inline std::vector<std::string> radio_browser_search_urls(const std::string& term)
{
  std::vector<std::string> urls;
  if (term.empty())
    return urls;
  gchar* raw = g_uri_escape_string(term.c_str(), nullptr, FALSE);
  const std::string esc = raw ? raw : "";
  g_free(raw);
  const std::string tail =
      "&limit=" + std::to_string(kRadioBrowserSearchLimit) + "&hidebroken=true";
  const std::string base = std::string(kRadioBrowserHost) + "/json/stations/search?";
  auto add = [&](const char* key, const std::string& value) {
    urls.push_back(base + key + "=" + value + tail);
  };
  std::string code;
  if (radio_browser_country_code(term, code))
    add("countrycode", code);
  add("country", esc);
  add("state", esc);
  add("name", esc);
  return urls;
}

/* Look up live streams on radio-browser.info. Prunes lastcheckok != 1.
 * Blocking. Call from a worker. */
std::vector<Station> search_radio_browser(const std::string& term, std::string& error,
                                          GCancellable* cancel = nullptr);
std::vector<Station> browse_radio_browser_popular(std::string& error,
                                                  GCancellable* cancel = nullptr);

}  // namespace listenomatic
