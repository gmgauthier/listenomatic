/* SPDX-License-Identifier: Unlicense */

#include "settings.hpp"
#include "paths.hpp"

#include <glibmm/fileutils.h>
#include <glibmm/keyfile.h>
#include <glibmm/miscutils.h>

#include <glib/gstdio.h>

#include <algorithm>
#include <ctime>

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

/* True when the user's file already owns this list, even an empty one. */
bool has_station_group(Glib::KeyFile& kf, const char* group)
{
  try {
    return kf.has_group(group) && kf.has_key(group, "count");
  } catch (const Glib::Error&) {
    return false;
  }
}

/* Move an unreadable config out of the way. Returns false if it is still in place. */
bool set_aside(const std::string& path)
{
  std::string to = path + ".bad";
  if (Glib::file_test(to, Glib::FILE_TEST_EXISTS))
    to += "." + std::to_string(static_cast<long long>(std::time(nullptr)));
  return g_rename(path.c_str(), to.c_str()) == 0;
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
  const std::string path = config_path();
  Glib::KeyFile file;
  Glib::KeyFile none;
  bool parsed = false;
  try {
    parsed = file.load_from_file(path);
  } catch (const Glib::Error&) {
    parsed = false;
  }
  if (!parsed && Glib::file_test(path, Glib::FILE_TEST_EXISTS))
    keep_unreadable_file_ = !set_aside(path);
  Glib::KeyFile& kf = parsed ? file : none;
  try {
    if (kf.has_key("window", "band")) {
      const Glib::ustring b = kf.get_string("window", "band");
      band = (b == "shows") ? Band::Shows : Band::Live;
    }
  } catch (const Glib::Error&) {
  }
  try {
    if (kf.has_key("window", "w"))
      window_w = kf.get_integer("window", "w");
    if (kf.has_key("window", "h"))
      window_h = kf.get_integer("window", "h");
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

  // Samples seed a first run only. A saved list, even an empty one, is the user's.
  if (has_station_group(kf, "live"))
    load_station_group(kf, "live", &live);
  else
    load_station_group(samples, "live", &live);
  try {
    if (kf.has_key("live", "current"))
      current_live = kf.get_integer("live", "current");
  } catch (const Glib::Error&) {
  }
  current_live = clamp_current(current_live, live);
  load_presets(kf, "live", live, &live_presets);

  if (has_station_group(kf, "shows"))
    load_station_group(kf, "shows", &shows);
  else
    load_station_group(samples, "shows", &shows);
  try {
    if (kf.has_key("shows", "current"))
      current_show = kf.get_integer("shows", "current");
  } catch (const Glib::Error&) {
  }
  current_show = clamp_current(current_show, shows);
  load_presets(kf, "shows", shows, &show_presets);

  resumes.clear();
  try {
    if (kf.has_group("resume") && kf.has_key("resume", "count")) {
      const int n = kf.get_integer("resume", "count");
      for (int i = 0; i < n; ++i) {
        Resume r;
        try {
          r.enclosure = kf.get_string("resume", Glib::ustring::compose("url%1", i));
          r.position_ns = kf.get_int64("resume", Glib::ustring::compose("pos%1", i));
        } catch (const Glib::Error&) {
          continue;
        }
        if (!r.enclosure.empty() && r.position_ns > 0)
          resumes.push_back(std::move(r));
      }
    }
  } catch (const Glib::Error&) {
  }

  last_programs.clear();
  try {
    if (kf.has_group("program") && kf.has_key("program", "count")) {
      const int n = kf.get_integer("program", "count");
      for (int i = 0; i < n; ++i) {
        LastProgram p;
        try {
          p.feed = kf.get_string("program", Glib::ustring::compose("feed%1", i));
          p.enclosure = kf.get_string("program", Glib::ustring::compose("enc%1", i));
        } catch (const Glib::Error&) {
          continue;
        }
        if (!p.feed.empty() && !p.enclosure.empty())
          last_programs.push_back(std::move(p));
      }
    }
  } catch (const Glib::Error&) {
  }
}

void Settings::save() const
{
  if (keep_unreadable_file_)
    return;
  Glib::KeyFile kf;
  kf.set_string("window", "band", band == Band::Shows ? "shows" : "live");
  if (window_w > 0 && window_h > 0) {
    kf.set_integer("window", "w", window_w);
    kf.set_integer("window", "h", window_h);
  }
  kf.set_double("audio", "volume", volume);
  save_station_group(kf, "live", live, current_live, live_presets);
  save_station_group(kf, "shows", shows, current_show, show_presets);
  kf.set_integer("program", "count", static_cast<int>(last_programs.size()));
  for (int i = 0; i < static_cast<int>(last_programs.size()); ++i) {
    kf.set_string("program", Glib::ustring::compose("feed%1", i),
                  last_programs[static_cast<std::size_t>(i)].feed);
    kf.set_string("program", Glib::ustring::compose("enc%1", i),
                  last_programs[static_cast<std::size_t>(i)].enclosure);
  }
  kf.set_integer("resume", "count", static_cast<int>(resumes.size()));
  for (int i = 0; i < static_cast<int>(resumes.size()); ++i) {
    kf.set_string("resume", Glib::ustring::compose("url%1", i),
                  resumes[static_cast<std::size_t>(i)].enclosure);
    kf.set_int64("resume", Glib::ustring::compose("pos%1", i),
                 resumes[static_cast<std::size_t>(i)].position_ns);
  }
  try {
    kf.save_to_file(config_path());
  } catch (const Glib::Error&) {
  }
}

std::int64_t Settings::resume_for(const std::string& enclosure) const
{
  for (const auto& r : resumes) {
    if (r.enclosure == enclosure)
      return r.position_ns;
  }
  return 0;
}

void Settings::set_resume(const std::string& enclosure, std::int64_t position_ns)
{
  if (enclosure.empty() || position_ns <= 0)
    return;
  clear_resume(enclosure);
  resumes.insert(resumes.begin(), Resume{enclosure, position_ns});
  constexpr int kMax = 80;
  if (static_cast<int>(resumes.size()) > kMax)
    resumes.resize(static_cast<std::size_t>(kMax));
}

void Settings::clear_resume(const std::string& enclosure)
{
  resumes.erase(std::remove_if(resumes.begin(), resumes.end(),
                               [&](const Resume& r) { return r.enclosure == enclosure; }),
                resumes.end());
}

std::string Settings::last_program_for(const std::string& feed) const
{
  for (const auto& p : last_programs) {
    if (p.feed == feed)
      return p.enclosure;
  }
  return {};
}

void Settings::set_last_program(const std::string& feed, const std::string& enclosure)
{
  if (feed.empty() || enclosure.empty())
    return;
  last_programs.erase(std::remove_if(last_programs.begin(), last_programs.end(),
                                     [&](const LastProgram& p) { return p.feed == feed; }),
                      last_programs.end());
  last_programs.insert(last_programs.begin(), LastProgram{feed, enclosure});
  constexpr int kMax = 80;
  if (static_cast<int>(last_programs.size()) > kMax)
    last_programs.resize(static_cast<std::size_t>(kMax));
}

}  // namespace listenomatic
