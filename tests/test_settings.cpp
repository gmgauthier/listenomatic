/* SPDX-License-Identifier: Unlicense */

#include "settings.hpp"
#include "check.hpp"

#include <glib.h>
#include <glib/gstdio.h>

#include <fstream>
#include <iterator>
#include <string>

namespace {

std::string slurp(const std::string& path)
{
  std::ifstream in(path, std::ios::binary);
  if (!in)
    return {};
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

void spit(const std::string& path, const std::string& bytes)
{
  std::ofstream out(path, std::ios::binary);
  out << bytes;
}

}  // namespace

int main()
{
  gchar* tmp = g_dir_make_tmp("listenomatic-settings-XXXXXX", nullptr);
  CHECK(tmp != nullptr);
  if (!tmp)
    return suite_test::done("settings");
  const std::string dir = tmp;
  g_free(tmp);
  g_setenv("XDG_CONFIG_HOME", dir.c_str(), TRUE);
  const std::string cfg_dir = dir + "/listenomatic";
  g_mkdir_with_parents(cfg_dir.c_str(), 0700);
  const std::string ini = cfg_dir + "/listenomatic.ini";

  {
    // First run (no file): Memory is seeded from samples.ini.
    listenomatic::Settings s;
    s.load();
    CHECK(!s.live.empty());
    CHECK(!s.shows.empty());
  }

  {
    // An intentionally empty Memory stays empty, through load and save.
    spit(ini,
         "[window]\nband=live\n"
         "[live]\ncount=0\ncurrent=-1\n"
         "[shows]\ncount=0\ncurrent=-1\n");
    listenomatic::Settings s;
    s.load();
    CHECK(s.live.empty());
    CHECK(s.shows.empty());
    CHECK(s.current_live == -1);
    CHECK(s.current_show == -1);
    s.save();
    listenomatic::Settings again;
    again.load();
    CHECK(again.live.empty());
    CHECK(again.shows.empty());
  }

  {
    // A file KeyFile rejects is kept aside, not overwritten by the next save.
    const std::string broken =
        "[live]\ncount=1\nname0=Mine\nurl0=http://example.test/mine\n"
        "this line is not a key file line\n"
        "[resume]\ncount=1\nurl0=http://example.test/ep.mp3\npos0=120000000000\n";
    spit(ini, broken);
    listenomatic::Settings s;
    s.load();
    s.save();
    CHECK(slurp(ini + ".bad") == broken);
    CHECK(slurp(ini) != broken);
  }

  return suite_test::done("settings");
}
