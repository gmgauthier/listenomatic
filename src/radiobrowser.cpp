/* SPDX-License-Identifier: Unlicense */

#include "radiobrowser.hpp"
#include "fetch.hpp"

#include <glib.h>
#include <json-glib/json-glib.h>

namespace listenomatic {
namespace {

const char* kHost = "https://all.api.radio-browser.info";
const int kLimit = 40;

std::string member_str(JsonObject* obj, const char* key)
{
  if (!obj || !json_object_has_member(obj, key))
    return {};
  const char* s = json_object_get_string_member(obj, key);
  return s ? s : "";
}

int member_int(JsonObject* obj, const char* key)
{
  if (!obj || !json_object_has_member(obj, key))
    return 0;
  return static_cast<int>(json_object_get_int_member(obj, key));
}

}  // namespace

std::vector<Station> search_radio_browser(const std::string& term, std::string& error,
                                          GCancellable* cancel)
{
  error.clear();
  if (term.empty()) {
    error = "Type a name, place, or call letters";
    return {};
  }

  gchar* esc = g_uri_escape_string(term.c_str(), nullptr, FALSE);
  const std::string url = std::string(kHost) + "/json/stations/search?name=" + (esc ? esc : "") +
                          "&limit=" + std::to_string(kLimit) + "&hidebroken=true";
  g_free(esc);

  const std::string body = http_get(url, error, cancel);
  if (body.empty())
    return {};

  JsonParser* parser = json_parser_new();
  GError* gerr = nullptr;
  if (!json_parser_load_from_data(parser, body.c_str(), static_cast<gssize>(body.size()), &gerr)) {
    error = gerr && gerr->message ? gerr->message : "Bad JSON";
    if (gerr)
      g_error_free(gerr);
    g_object_unref(parser);
    return {};
  }

  JsonNode* root = json_parser_get_root(parser);
  if (!root || !JSON_NODE_HOLDS_ARRAY(root)) {
    error = "Unexpected response";
    g_object_unref(parser);
    return {};
  }

  JsonArray* arr = json_node_get_array(root);
  const guint n = json_array_get_length(arr);
  std::vector<Station> out;
  out.reserve(n);
  for (guint i = 0; i < n; ++i) {
    JsonObject* obj = json_array_get_object_element(arr, i);
    if (!obj)
      continue;
    if (member_int(obj, "lastcheckok") != 1)
      continue;
    Station st;
    st.name = member_str(obj, "name");
    st.url = member_str(obj, "url_resolved");
    if (st.url.empty())
      st.url = member_str(obj, "url");
    if (st.name.empty() || st.url.empty())
      continue;
    st.short_name = make_short_name(st.name);
    st.codec = member_str(obj, "codec");
    st.bitrate = member_int(obj, "bitrate");
    st.place = member_str(obj, "countrycode");
    if (st.place.empty())
      st.place = member_str(obj, "country");
    out.push_back(std::move(st));
  }
  g_object_unref(parser);
  if (out.empty())
    error = "No live stations matched";
  return out;
}

}  // namespace listenomatic
