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

}  // namespace

void Settings::load()
{
  Glib::KeyFile kf;
  try {
    kf.load_from_file(config_path());
  } catch (const Glib::Error&) {
    return;
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
}

void Settings::save() const
{
  Glib::KeyFile kf;
  try {
    kf.load_from_file(config_path());
  } catch (const Glib::Error&) {
  }
  kf.set_string("window", "band", band == Band::Shows ? "shows" : "live");
  kf.set_double("audio", "volume", volume);
  try {
    kf.save_to_file(config_path());
  } catch (const Glib::Error&) {
  }
}

}  // namespace listenomatic
