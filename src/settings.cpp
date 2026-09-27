/* SPDX-License-Identifier: Unlicense */

#include "settings.hpp"
#include "paths.hpp"

#include <glibmm/fileutils.h>
#include <glibmm/keyfile.h>
#include <glibmm/miscutils.h>

namespace listenomatic {
namespace {

std::string config_path()
{
  return Glib::build_filename(config_dir(), "listenomatic.ini");
}

void load_live_group(Glib::KeyFile& kf, const char* group, std::vector<Station>* out)
{
  if (!kf.has_group(group) || !kf.has_key(group, "count"))
    return;
  int n = 0;
  try {
    n = kf.get_integer(group, "count");
  } catch (const Glib::Error&) {
    return;
  }
  for (int i = 0; i < n; ++i) {
    const Glib::ustring nk = Glib::ustring::compose("name%1", i);
    const Glib::ustring uk = Glib::ustring::compose("url%1", i);
    if (!kf.has_key(group, nk) || !kf.has_key(group, uk))
      continue;
    Station st;
    try {
      st.name = kf.get_string(group, nk);
      st.url = kf.get_string(group, uk);
    } catch (const Glib::Error&) {
      continue;
    }
    if (st.name.empty() || st.url.empty())
      continue;
    st.short_name = make_short_name(st.name);
    out->push_back(std::move(st));
  }
}

}  // namespace

void Settings::load()
{
  Glib::KeyFile kf;
  try {
    kf.load_from_file(config_path());
  } catch (const Glib::Error&) {
  }
  try {
    if (kf.has_key("window", "band")) {
      const Glib::ustring b = kf.get_string("window", "band");
      band = (b == "shows") ? Band::Shows : Band::Live;
    }
  } catch (const Glib::Error&) {
  }
  try {
    if (kf.has_key("audio", "volume"))
      volume = kf.get_double("audio", "volume");
  } catch (const Glib::Error&) {
  }
  if (volume < 0.0)
    volume = 0.0;
  if (volume > 1.0)
    volume = 1.0;

  load_live_group(kf, "live", &live);
  if (live.empty()) {
    Glib::KeyFile samples;
    const std::string sp = find_data_file("samples.ini");
    if (!sp.empty()) {
      try {
        samples.load_from_file(sp);
        load_live_group(samples, "live", &live);
      } catch (const Glib::Error&) {
      }
    }
  }
  try {
    if (kf.has_key("live", "current"))
      current_live = kf.get_integer("live", "current");
  } catch (const Glib::Error&) {
  }
  if (current_live < 0 || current_live >= static_cast<int>(live.size()))
    current_live = live.empty() ? -1 : 0;
}

void Settings::save() const
{
  Glib::KeyFile kf;
  kf.set_string("window", "band", band == Band::Shows ? "shows" : "live");
  kf.set_double("audio", "volume", volume);
  kf.set_integer("live", "count", static_cast<int>(live.size()));
  kf.set_integer("live", "current", current_live);
  for (int i = 0; i < static_cast<int>(live.size()); ++i) {
    kf.set_string("live", Glib::ustring::compose("name%1", i),
                  live[static_cast<std::size_t>(i)].name);
    kf.set_string("live", Glib::ustring::compose("url%1", i),
                  live[static_cast<std::size_t>(i)].url);
  }
  try {
    kf.save_to_file(config_path());
  } catch (const Glib::Error&) {
  }
}

}  // namespace listenomatic
