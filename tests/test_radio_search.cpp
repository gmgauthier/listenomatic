/* SPDX-License-Identifier: Unlicense */

#include "check.hpp"
#include "radiobrowser.hpp"

#include <string>

namespace {

bool has_query(const std::vector<std::string>& urls, const std::string& query)
{
  for (const auto& url : urls) {
    if (url.find(query) != std::string::npos)
      return true;
  }
  return false;
}

}  // namespace

int main()
{
  {
    const auto urls = listenomatic::radio_browser_search_urls("");
    CHECK(urls.empty());
  }
  {
    // A place is not AND-ed with the station name. Each field is its own URL.
    const auto urls = listenomatic::radio_browser_search_urls("New York");
    CHECK(urls.size() == 3);
    CHECK(has_query(urls, "?country=New%20York&"));
    CHECK(has_query(urls, "?state=New%20York&"));
    CHECK(has_query(urls, "?name=New%20York&"));
    CHECK(!has_query(urls, "countrycode="));
    CHECK(has_query(urls, "&limit=40&hidebroken=true"));
    CHECK(has_query(urls, "https://all.api.radio-browser.info/json/stations/search?"));
  }
  {
    // Two letters are also an uppercase country code, ahead of the name.
    const auto urls = listenomatic::radio_browser_search_urls("de");
    CHECK(urls.size() == 4);
    CHECK(urls[0].find("?countrycode=DE&") != std::string::npos);
    CHECK(has_query(urls, "?country=de&"));
    CHECK(has_query(urls, "?state=de&"));
    CHECK(has_query(urls, "?name=de&"));
  }
  {
    const auto urls = listenomatic::radio_browser_search_urls("KEXP");
    CHECK(urls.size() == 3);
    CHECK(!has_query(urls, "countrycode="));
    CHECK(has_query(urls, "?name=KEXP&"));
  }
  {
    std::string code;
    CHECK(!listenomatic::radio_browser_country_code("A1", code));
    CHECK(listenomatic::radio_browser_country_code("uk", code));
    CHECK(code == "UK");
  }
  return suite_test::done("radio-search");
}
