/* SPDX-License-Identifier: Unlicense */

#include "station.hpp"
#include "check.hpp"

#include <string>

int main()
{
  using listenomatic::make_short_name;

  // ASCII behaviour is unchanged.
  CHECK(make_short_name("The Kitchen Radio") == "Kitchen");
  CHECK(make_short_name("ABCDEFGHIJK") == "ABCDEFGH");
  CHECK(make_short_name("WFMT Classical") == "WFMT");
  CHECK(make_short_name("") == "—");

  // Multibyte names are cut on characters, never inside a UTF-8 sequence.
  CHECK(make_short_name("日本語放送局") == "日本語放送局");
  CHECK(make_short_name("日本語放送局テスト局") == "日本語放送局テス");
  CHECK(make_short_name("Café Olé Radio") == "Café");
  CHECK(make_short_name("Ééééééééé") == "Éééééééé");
  CHECK(make_short_name("Radioφωνία Ελλάδα") == "Radioφων");

  return suite_test::done("station");
}
