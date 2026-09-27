/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <string>

namespace listenomatic {

std::string find_data_file(const std::string& relative);
std::string config_dir();

}  // namespace listenomatic
