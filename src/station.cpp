/* SPDX-License-Identifier: Unlicense */

#include "station.hpp"

#include <cstddef>

namespace listenomatic {
namespace {

constexpr std::size_t kShortChars = 8;

bool continuation(unsigned char c)
{
  return (c & 0xC0) == 0x80;
}

/* Byte offset just past the first n UTF-8 characters of s (or s.size()). */
std::size_t utf8_prefix(const std::string& s, std::size_t n)
{
  std::size_t i = 0;
  for (std::size_t chars = 0; i < s.size() && chars < n; ++chars) {
    ++i;
    while (i < s.size() && continuation(static_cast<unsigned char>(s[i])))
      ++i;
  }
  return i;
}

std::size_t utf8_length(const std::string& s, std::size_t bytes)
{
  std::size_t chars = 0;
  for (std::size_t i = 0; i < bytes && i < s.size(); ++i) {
    if (!continuation(static_cast<unsigned char>(s[i])))
      ++chars;
  }
  return chars;
}

}  // namespace

std::string make_short_name(const std::string& name)
{
  std::string s = name;
  while (s.size() >= 4 && (s.compare(0, 4, "The ") == 0 || s.compare(0, 4, "THE ") == 0))
    s = s.substr(4);
  // Lengths are in characters, so a cut never lands inside a UTF-8 sequence.
  auto sp = s.find(' ');
  if (sp != std::string::npos && sp > 0 && utf8_length(s, sp) <= kShortChars)
    s = s.substr(0, sp);
  else
    s = s.substr(0, utf8_prefix(s, kShortChars));
  return s.empty() ? "—" : s;
}

}  // namespace listenomatic
