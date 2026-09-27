/* SPDX-License-Identifier: Unlicense */

#include "paths.hpp"
#include "config.hpp"

#include <glib.h>
#include <glibmm.h>

#include <vector>

namespace listenomatic {
namespace {

bool exists_regular(const std::string& path)
{
  return Glib::file_test(path, Glib::FILE_TEST_IS_REGULAR);
}

}  // namespace

std::string find_data_file(const std::string& relative)
{
  std::vector<std::string> roots;

  if (const char* env = g_getenv("LISTENOMATIC_DATA"))
    roots.emplace_back(env);

  if (const char* appdir = g_getenv("APPDIR"))
    roots.emplace_back(Glib::build_filename(appdir, "usr/share/listenomatic"));

  roots.emplace_back(SOURCE_ROOT);
  roots.emplace_back(std::string(SOURCE_ROOT) + "/data");
  roots.emplace_back(DATADIR);

  for (const auto& root : roots) {
    const std::string candidate = Glib::build_filename(root, relative);
    if (exists_regular(candidate))
      return candidate;
  }
  return {};
}

std::string config_dir()
{
  const std::string dir = Glib::build_filename(Glib::get_user_config_dir(), "listenomatic");
  g_mkdir_with_parents(dir.c_str(), 0700);
  return dir;
}

}  // namespace listenomatic
