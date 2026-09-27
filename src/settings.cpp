/* SPDX-License-Identifier: Unlicense */

#include "settings.hpp"
#include "paths.hpp"

#include <glibmm/fileutils.h>
#include <glibmm/keyfile.h>
#include <glibmm/miscutils.h>

#include <algorithm>

namespace listenomatic {
namespace {

std::string config_path()
{
  return Glib::build_filename(config_dir(), "listenomatic.ini");
}

void load_station_group(Glib::KeyFile& kf, const char* group, std::vector<Station>* out)
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

void load_presets(Glib::KeyFile& kf, const char* group, const std::vector<Station>& list,
                  std::array<int, 6>* presets)
{
  bool any = false;
  for (int i = 0; i < 6; ++i) {
    (*presets)[static_cast<std::size_t>(i)] = -1;
    const Glib::ustring k = Glib::ustring::compose("preset%1", i);
    try {
      if (!kf.has_group(group) || !kf.has_key(group, k))
        continue;
    } catch (const Glib::Error&) {
      continue;
    }
    any = true;
    try {
      const int idx = kf.get_integer(group, k);
      if (idx >= 0 && idx < static_cast<int>(list.size()))
        (*presets)[static_cast<std::size_t>(i)] = idx;
    } catch (const Glib::Error&) {
    }
  }
  if (!any) {
    const int n = std::min(6, static_cast<int>(list.size()));
    for (int i = 0; i < n; ++i)
      (*presets)[static_cast<std::size_t>(i)] = i;
  }
}

void save_station_group(Glib::KeyFile& kf, const char* group, const std::vector<Station>& list,
                        int current, const std::array<int, 6>& presets)
{
  kf.set_integer(group, "count", static_cast<int>(list.size()));
  kf.set_integer(group, "current", current);
  for (int i = 0; i < static_cast<int>(list.size()); ++i) {
    kf.set_string(group, Glib::ustring::compose("name%1", i),
                  list[static_cast<std::size_t>(i)].name);
    kf.set_string(group, Glib::ustring::compose("url%1", i), list[static_cast<std::size_t>(i)].url);
  }
  for (int i = 0; i < 6; ++i) {
    kf.set_integer(group, Glib::ustring::compose("preset%1", i),
                   presets[static_cast<std::size_t>(i)]);
  }
}

int clamp_current(int cur, const std::vector<Station>& list)
{
  if (cur < 0 || cur >= static_cast<int>(list.size()))
    return list.empty() ? -1 : 0;
  return cur;
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

  Glib::KeyFile samples;
  const std::string sp = find_data_file("samples.ini");
  if (!sp.empty()) {
    try {
      samples.load_from_file(sp);
    } catch (const Glib::Error&) {
    }
  }

  load_station_group(kf, "live", &live);
  if (live.empty())
    load_station_group(samples, "live", &live);
  try {
    if (kf.has_key("live", "current"))
      current_live = kf.get_integer("live", "current");
  } catch (const Glib::Error&) {
  }
  current_live = clamp_current(current_live, live);
  load_presets(kf, "live", live, &live_presets);

  load_station_group(kf, "shows", &shows);
  if (shows.empty())
    load_station_group(samples, "shows", &shows);
  try {
    if (kf.has_key("shows", "current"))
      current_show = kf.get_integer("shows", "current");
  } catch (const Glib::Error&) {
  }
  current_show = clamp_current(current_show, shows);
  load_presets(kf, "shows", shows, &show_presets);
}

void Settings::save() const
{
  Glib::KeyFile kf;
  kf.set_string("window", "band", band == Band::Shows ? "shows" : "live");
  kf.set_double("audio", "volume", volume);
  save_station_group(kf, "live", live, current_live, live_presets);
  save_station_group(kf, "shows", shows, current_show, show_presets);
  try {
    kf.save_to_file(config_path());
  } catch (const Glib::Error&) {
  }
}

}  // namespace listenomatic
