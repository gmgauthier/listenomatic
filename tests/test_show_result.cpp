/* SPDX-License-Identifier: Unlicense */

#include "check.hpp"
#include "show_result.hpp"

int main()
{
  using listenomatic::itunes_result_applies;

  // The search that is still the current one may paint its rows.
  CHECK(itunes_result_applies(1, 1, false));

  // Starter was pressed before this response arrived.
  CHECK(!itunes_result_applies(1, 2, true));

  // The starter list is up even if a generation was not bumped.
  CHECK(!itunes_result_applies(2, 2, true));

  // An older search must not paint over a newer one.
  CHECK(!itunes_result_applies(1, 3, false));
  CHECK(itunes_result_applies(3, 3, false));

  return suite_test::done("show-result");
}
