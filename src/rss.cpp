/* SPDX-License-Identifier: Unlicense */

#include "rss.hpp"

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <cstdint>
#include <cctype>
#include <cstdio>
#include <cstring>

namespace listenomatic {
namespace {

constexpr int kMaxPrograms = 80;

bool ieq(const xmlChar* a, const char* b)
{
  return a && b && xmlStrcasecmp(a, BAD_CAST b) == 0;
}

xmlNodePtr first_elem(xmlNodePtr n, const char* local)
{
  for (xmlNodePtr c = n ? n->children : nullptr; c; c = c->next) {
    if (c->type == XML_ELEMENT_NODE && ieq(c->name, local))
      return c;
  }
  return nullptr;
}

std::string node_text(xmlNodePtr n)
{
  if (!n)
    return {};
  xmlChar* t = xmlNodeGetContent(n);
  std::string s = t ? reinterpret_cast<char*>(t) : "";
  xmlFree(t);
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
    s.erase(s.begin());
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
    s.pop_back();
  return s;
}

std::string child_text(xmlNodePtr n, const char* local)
{
  return node_text(first_elem(n, local));
}

std::string attr(xmlNodePtr n, const char* name)
{
  if (!n)
    return {};
  xmlChar* v = xmlGetProp(n, BAD_CAST name);
  std::string s = v ? reinterpret_cast<char*>(v) : "";
  xmlFree(v);
  return s;
}

bool is_audio_enclosure(const std::string& url, const std::string& type)
{
  std::string t = type;
  for (char& c : t)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  if (t.compare(0, 6, "audio/") == 0)
    return true;
  if (t.compare(0, 6, "video/") == 0)
    return false;
  // The extension belongs to the last path segment, before any query or fragment.
  const std::string path = url.substr(0, url.find_first_of("?#"));
  const auto slash = path.find_last_of('/');
  auto dot = path.find_last_of('.');
  if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
    return !url.empty() && t.empty();
  std::string ext = path.substr(dot);
  for (char& c : ext)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return ext == ".mp3" || ext == ".m4a" || ext == ".aac" || ext == ".ogg" || ext == ".opus" ||
         ext == ".oga";
}

std::string format_clock(int sec)
{
  if (sec < 0)
    sec = 0;
  const int h = sec / 3600;
  const int m = (sec % 3600) / 60;
  const int s = sec % 60;
  char buf[16];
  if (h > 0)
    std::snprintf(buf, sizeof(buf), "%d:%02d:%02d", h, m, s);
  else
    std::snprintf(buf, sizeof(buf), "%d:%02d", m, s);
  return buf;
}

int parse_duration(const std::string& raw)
{
  if (raw.empty())
    return 0;
  int h = 0, m = 0, s = 0;
  if (std::sscanf(raw.c_str(), "%d:%d:%d", &h, &m, &s) == 3)
    return h * 3600 + m * 60 + s;
  if (std::sscanf(raw.c_str(), "%d:%d", &m, &s) == 2)
    return m * 60 + s;
  int sec = 0;
  if (std::sscanf(raw.c_str(), "%d", &sec) == 1)
    return sec;
  return 0;
}

const char* kMonths[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                         "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

std::string format_pubdate(const std::string& raw)
{
  /* RFC 822: Fri, 25 Sep 2026 02:01:51 GMT */
  int day = 0, year = 0;
  char mon[8] = {};
  if (std::sscanf(raw.c_str(), "%*3s, %d %3s %d", &day, mon, &year) == 3 ||
      std::sscanf(raw.c_str(), "%d %3s %d", &day, mon, &year) == 3) {
    int mi = 0;
    for (int i = 0; i < 12; ++i) {
      if (std::strncmp(mon, kMonths[i], 3) == 0) {
        mi = i + 1;
        break;
      }
    }
    if (mi && year > 0 && day > 0) {
      char buf[16];
      std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", year, mi, day);
      return buf;
    }
  }
  if (raw.size() >= 10 && raw[4] == '-' && raw[7] == '-')
    return raw.substr(0, 10);
  return raw.size() > 16 ? raw.substr(0, 16) : raw;
}

std::string trimmed(std::string s)
{
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
    s.erase(s.begin());
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
    s.pop_back();
  return s;
}

std::string item_enclosure(xmlNodePtr item)
{
  // A blank URL is never an enclosure, whatever its type; keep looking past it.
  for (xmlNodePtr c = item ? item->children : nullptr; c; c = c->next) {
    if (c->type != XML_ELEMENT_NODE)
      continue;
    if (ieq(c->name, "enclosure")) {
      const std::string url = trimmed(attr(c, "url"));
      if (!url.empty() && is_audio_enclosure(url, attr(c, "type")))
        return url;
    }
    if (ieq(c->name, "link")) {
      const std::string rel = attr(c, "rel");
      const std::string href = trimmed(attr(c, "href"));
      if ((rel == "enclosure" || rel == "media") && !href.empty() &&
          is_audio_enclosure(href, attr(c, "type")))
        return href;
    }
  }
  return {};
}

xmlNodePtr find_channel(xmlNodePtr root)
{
  if (!root)
    return nullptr;
  if (ieq(root->name, "rss") || ieq(root->name, "RDF"))
    return first_elem(root, "channel");
  if (ieq(root->name, "feed"))
    return root;
  return first_elem(root, "channel");
}

}  // namespace

bool parse_podcast(const std::string& xml, PodcastFeed& out, std::string& error)
{
  error.clear();
  out = {};
  if (xml.empty()) {
    error = "Empty feed";
    return false;
  }
  xmlDocPtr doc =
      xmlReadMemory(xml.c_str(), static_cast<int>(xml.size()), "feed.xml", nullptr,
                    XML_PARSE_NONET | XML_PARSE_NOBLANKS | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
  if (!doc) {
    error = "Could not parse feed";
    return false;
  }
  xmlNodePtr channel = find_channel(xmlDocGetRootElement(doc));
  if (!channel) {
    xmlFreeDoc(doc);
    error = "No channel in feed";
    return false;
  }
  out.title = child_text(channel, "title");
  for (xmlNodePtr c = channel->children; c; c = c->next) {
    if (c->type != XML_ELEMENT_NODE)
      continue;
    if (!ieq(c->name, "item") && !ieq(c->name, "entry"))
      continue;
    const std::string enc = item_enclosure(c);
    if (enc.empty())
      continue;
    Program p;
    p.title = child_text(c, "title");
    if (p.title.empty())
      p.title = enc;
    p.date = format_pubdate(child_text(c, "pubDate"));
    if (p.date.empty())
      p.date = format_pubdate(child_text(c, "published"));
    const int sec = parse_duration(child_text(c, "duration"));
    p.duration_ns = static_cast<std::int64_t>(sec) * 1000000000LL;
    p.length = sec > 0 ? format_clock(sec) : "";
    p.enclosure = enc;
    out.programs.push_back(std::move(p));
    if (static_cast<int>(out.programs.size()) >= kMaxPrograms)
      break;
  }
  xmlFreeDoc(doc);
  if (out.programs.empty()) {
    error = "No audio programs in this feed";
    return false;
  }
  return true;
}

}  // namespace listenomatic
