/* SPDX-License-Identifier: Unlicense */

#include "itunes.hpp"
#include "fetch.hpp"

#include <glib.h>
#include <json-glib/json-glib.h>

#include <unordered_set>

namespace listenomatic {
namespace {

const char* kSearch = "https://itunes.apple.com/search";
const int kLimit = 40;

std::string member_str(JsonObject* obj, const char* key)
{
  if (!obj || !json_object_has_member(obj, key))
    return {};
  const char* s = json_object_get_string_member(obj, key);
  return s ? s : "";
}

std::vector<Station> parse_results(const std::string& body, std::string& error)
{
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
  if (!root || !JSON_NODE_HOLDS_OBJECT(root)) {
    error = "Unexpected response";
    g_object_unref(parser);
    return {};
  }

  JsonObject* obj = json_node_get_object(root);
  if (!obj || !json_object_has_member(obj, "results") ||
      !JSON_NODE_HOLDS_ARRAY(json_object_get_member(obj, "results"))) {
    error = "Unexpected response";
    g_object_unref(parser);
    return {};
  }

  JsonArray* arr = json_object_get_array_member(obj, "results");
  const guint n = json_array_get_length(arr);
  std::vector<Station> out;
  std::unordered_set<std::string> seen;
  out.reserve(n);
  for (guint i = 0; i < n; ++i) {
    JsonObject* hit = json_array_get_object_element(arr, i);
    if (!hit)
      continue;
    Station st;
    st.name = member_str(hit, "collectionName");
    st.url = member_str(hit, "feedUrl");
    if (st.name.empty() || st.url.empty())
      continue;
    if (!seen.insert(st.url).second)
      continue;
    st.short_name = make_short_name(st.name);
    st.place = member_str(hit, "primaryGenreName");
    if (st.place.empty())
      st.place = member_str(hit, "country");
    out.push_back(std::move(st));
  }
  g_object_unref(parser);
  return out;
}

}  // namespace

std::vector<Station> search_itunes_podcasts(const std::string& term, std::string& error,
                                            GCancellable* cancel)
{
  error.clear();
  if (term.empty()) {
    error = "Type a show name";
    return {};
  }

  gchar* esc = g_uri_escape_string(term.c_str(), nullptr, FALSE);
  const std::string url = std::string(kSearch) + "?term=" + (esc ? esc : "") +
                          "&media=podcast&entity=podcast&limit=" + std::to_string(kLimit);
  g_free(esc);

  const std::string body = http_get(url, error, cancel);
  if (body.empty())
    return {};
  auto out = parse_results(body, error);
  if (out.empty() && error.empty())
    error = "No podcasts matched";
  return out;
}

}  // namespace listenomatic
