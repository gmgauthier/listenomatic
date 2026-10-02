/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <cstdint>

namespace listenomatic {

/* An iTunes payload belongs to the search that started it. It does not paint
 * once that generation has moved on, and it does not paint over the starter list. */
inline bool itunes_result_applies(std::uint64_t result_generation, std::uint64_t current_generation,
                                  bool showing_starter)
{
  return result_generation == current_generation && !showing_starter;
}

}  // namespace listenomatic
