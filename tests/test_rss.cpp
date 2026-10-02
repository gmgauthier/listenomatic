/* SPDX-License-Identifier: Unlicense */

#include "rss.hpp"
#include "check.hpp"

#include <string>

int main()
{
  listenomatic::PodcastFeed feed;
  std::string error;

  CHECK(!listenomatic::parse_podcast("", feed, error));
  CHECK(error == "Empty feed");

  CHECK(!listenomatic::parse_podcast("<rss></rss>", feed, error));
  CHECK(!error.empty());

  const char* xml =
      "<?xml version=\"1.0\"?>"
      "<rss version=\"2.0\"><channel>"
      "<title> Kitchen </title>"
      "<item><title>No audio</title><link>http://example.test/notes</link></item>"
      "<item>"
      "<title>Episode</title>"
      "<pubDate>Fri, 25 Sep 2026 02:01:51 GMT</pubDate>"
      "<enclosure url=\"http://example.test/ep.mp3\" type=\"audio/mpeg\" length=\"10\"/>"
      "<duration>1:02:03</duration>"
      "</item>"
      "<item>"
      "<title>Short</title>"
      "<pubDate>2026-09-26T00:00:00Z</pubDate>"
      "<enclosure url=\"http://example.test/short.ogg\" type=\"audio/ogg\"/>"
      "<duration>90</duration>"
      "</item>"
      "</channel></rss>";
  CHECK(listenomatic::parse_podcast(xml, feed, error));
  CHECK(error.empty());
  CHECK(feed.title == "Kitchen");
  CHECK(feed.programs.size() == 2);
  CHECK(feed.programs[0].title == "Episode");
  CHECK(feed.programs[0].date == "2026-09-25");
  CHECK(feed.programs[0].enclosure == "http://example.test/ep.mp3");
  CHECK(feed.programs[0].duration_ns == 3723LL * 1000000000LL);
  CHECK(feed.programs[0].length == "1:02:03");
  CHECK(feed.programs[1].date == "2026-09-26");
  CHECK(feed.programs[1].length == "1:30");
  CHECK(feed.programs[1].duration_ns == 90LL * 1000000000LL);

  std::string many = "<?xml version=\"1.0\"?><rss><channel><title>Many</title>";
  for (int i = 0; i < 90; ++i) {
    many += "<item><title>E" + std::to_string(i) + "</title>";
    many += "<enclosure url=\"http://example.test/" + std::to_string(i) + ".mp3\" type=\"audio/mpeg\"/>";
    many += "</item>";
  }
  many += "</channel></rss>";
  CHECK(listenomatic::parse_podcast(many, feed, error));
  CHECK(feed.programs.size() == 80);
  CHECK(feed.programs.front().title == "E0");
  CHECK(feed.programs.back().title == "E79");

  const char* atom =
      "<feed><title>Atom</title>"
      "<entry><title>A</title>"
      "<published>Mon, 01 Jan 2024 00:00:00 GMT</published>"
      "<link rel=\"enclosure\" href=\"http://example.test/a.m4a\" type=\"audio/mp4\"/>"
      "</entry></feed>";
  CHECK(listenomatic::parse_podcast(atom, feed, error));
  CHECK(feed.programs.size() == 1);
  CHECK(feed.programs[0].enclosure == "http://example.test/a.m4a");
  CHECK(feed.programs[0].date == "2024-01-01");

  {
    // With no audio type, the extension is read from the path, not the query string.
    const char* q =
        "<rss><channel><title>Q</title>"
        "<item><title>Token</title>"
        "<enclosure url=\"https://cdn.example.com/ep.mp3?token=abc\"/></item>"
        "<item><title>Frag</title>"
        "<enclosure url=\"https://cdn.example.com/b.M4A#t=10\" type=\"application/octet-stream\"/>"
        "</item>"
        "<item><title>Dotted query</title>"
        "<enclosure url=\"https://cdn.example.com/page?file=x.mp3\" type=\"text/html\"/></item>"
        "</channel></rss>";
    CHECK(listenomatic::parse_podcast(q, feed, error));
    CHECK(feed.programs.size() == 2);
    if (feed.programs.size() == 2) {
      CHECK(feed.programs[0].enclosure == "https://cdn.example.com/ep.mp3?token=abc");
      CHECK(feed.programs[1].enclosure == "https://cdn.example.com/b.M4A#t=10");
    }
  }

  {
    // A blank audio enclosure must not hide a later real one.
    const char* blank =
        "<rss><channel><title>B</title>"
        "<item><title>Second wins</title>"
        "<enclosure url=\"\" type=\"audio/mpeg\"/>"
        "<enclosure url=\"  \" type=\"audio/mpeg\"/>"
        "<enclosure url=\"http://example.test/real.mp3\" type=\"audio/mpeg\"/>"
        "</item>"
        "<item><title>Only blank</title><enclosure url=\"\" type=\"audio/mpeg\"/></item>"
        "</channel></rss>";
    CHECK(listenomatic::parse_podcast(blank, feed, error));
    CHECK(feed.programs.size() == 1);
    if (feed.programs.size() == 1)
      CHECK(feed.programs[0].enclosure == "http://example.test/real.mp3");
  }

  {
    // A spelled-out weekday keeps the whole date.
    const char* days =
        "<rss><channel><title>D</title>"
        "<item><title>Long</title><pubDate>Friday, 25 Sep 2026 12:00:00 GMT</pubDate>"
        "<enclosure url=\"http://example.test/1.mp3\" type=\"audio/mpeg\"/></item>"
        "<item><title>No comma</title><pubDate>Wednesday 30 Sep 2026 08:00:00 +0000</pubDate>"
        "<enclosure url=\"http://example.test/2.mp3\" type=\"audio/mpeg\"/></item>"
        "<item><title>No weekday</title><pubDate>1 Oct 2026 08:00:00 GMT</pubDate>"
        "<enclosure url=\"http://example.test/3.mp3\" type=\"audio/mpeg\"/></item>"
        "</channel></rss>";
    CHECK(listenomatic::parse_podcast(days, feed, error));
    CHECK(feed.programs.size() == 3);
    if (feed.programs.size() == 3) {
      CHECK(feed.programs[0].date == "2026-09-25");
      CHECK(feed.programs[1].date == "2026-09-30");
      CHECK(feed.programs[2].date == "2026-10-01");
    }
  }

  return suite_test::done("rss");
}
