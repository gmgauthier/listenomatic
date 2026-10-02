# Bug backlog

Reviewed 2026-10-01 against the 1.0.1 sources.

`meson test` runs `tests/test_rss.cpp` (`rss`). It checks an empty feed, a skipped item with no audio, RFC 822 and Atom dates, `1:02:03` duration, and the deliberate 80-item cap. The 8 px seek-bar margins are the shipped Clearlooks fix and are not a defect. A normal 4-digit `pubDate` fits in `format_pubdate`'s 16-byte buffer.

## Open

### Changing show leaves the old episode playing and saves its position onto the new one

- Severity: data-loss
- Confidence: high
- Where: `src/main_window.cpp:460`, `src/main_window.cpp:589`
- Trigger: Play a show, then pick another show from Memory, a preset, or Catalog on the Shows band.
- Outcome: `select_station` calls `load_show_feed(false)` and returns. It does not call `save_progress()` or `player_.stop()`. Audio keeps playing the previous enclosure while the list and `current_program_` switch to the new show. The next pause, stop, or quit writes that position onto the new episode's enclosure, and the episode you were listening to is not updated.

### Clicking the loading row plays the previous show's episode

- Severity: incorrect
- Confidence: high
- Where: `src/main_window.cpp:642`, `src/main_window.cpp:567`
- Trigger: Change shows and activate the "Loading programs…" row before the fetch returns.
- Outcome: `fetch_show` replaces the tree with that row and does not clear `episodes_`. `play_program` indexes `episodes_` by the row. Episode 0 of the previous show starts, and `set_last_program` records that other show's enclosure on the new show.

### Resume seek is attempted once and then forgotten

- Severity: incorrect
- Confidence: medium
- Where: `src/main_window.cpp:964`
- Trigger: Play an episode that has a saved resume. The seek runs inside the playbin `PLAYING` state change, which is often before an HTTP demuxer answers a seek query.
- Outcome: `pending_resume_ns_` is cleared immediately. `seek` does not check the return value and is not retried. Playback stays at the start. The UI can show the resume point until the next position query overwrites it.

### iTunes results replace Starter after the user already went back

- Severity: incorrect
- Confidence: high
- Where: `src/catalog_window.cpp:287`, `src/catalog_window.cpp:315`
- Trigger: Search iTunes, then press Starter before the response arrives.
- Outcome: `on_show_starter` does not cancel the in-flight search. The idle callback always calls `apply_shows`, which replaces the list. Starter paints, then the late results paint over it. `show_starter_` can be left true while the list is the search result, so the next keystroke filters the starter list instead.

### Place search only queries the station name field

- Severity: incorrect
- Confidence: high
- Where: `src/radiobrowser.cpp:90`
- Trigger: Catalog or Add search for a country or state that is not in the station name. The field says "name, place, or call letters".
- Outcome: The URL is `/json/stations/search?name=` plus the term. There is no `country`, `countrycode`, or `state` parameter. A country query misses stations that are filed only under those fields. Call letters still match when they are in the name.

### The current live title is not a track until a different title arrives

- Severity: incorrect
- Confidence: high
- Where: `src/main_window.cpp:984`
- Trigger: Tune a live station that sends one title, or stop before a second distinct title.
- Outcome: The LCD shows the title. Tracks lists `live_heard_` only. The current title is pushed onto that list only when a different title arrives and replaces `live_now_`. The song just heard stays off the list.

## Closed

### Leaving Shows does not save the resume, and coming back can write the live position onto the episode

- Severity: data-loss
- Confidence: high
- Where: `src/main_window.cpp:511`, `src/main_window.cpp:589`
- Trigger: While a show is playing, switch to Live. Or switch Live → Shows after a show was loaded earlier in the same process.
- Outcome: `apply_band` updates `settings_.band`, then calls `save_progress()`, then stops the player. `save_progress` returns immediately unless `on_shows()` is already true. Shows → Live therefore writes nothing. Live → Shows runs `save_progress` while playbin is still on the live stream and `episodes_` / `current_program_` still describe the old episode, so the live position is stored as that episode's resume. A near-zero live position clears the resume.
- Fixed: v1.0.3

### An empty or unreadable station list is replaced with the samples and saved

- Severity: data-loss
- Confidence: high
- Where: `src/settings.cpp:107`, `src/settings.cpp:142`, `src/main_window.cpp:202`
- Trigger: Remove every Live or Shows station and restart. Or start with a `listenomatic.ini` that `Glib::KeyFile` rejects.
- Outcome: `load()` swallows a failed `load_from_file`. An empty live or shows group is filled from `samples.ini`. The window constructor always calls `settings_.save()`, so the sample list is written back. Resume positions and last-program entries from a file that failed to parse are dropped. An intentional empty Memory comes back full.
- Fixed: v1.0.4

### Preset short names are cut mid-codepoint

- Severity: incorrect
- Confidence: high
- Where: `src/station.cpp:15`
- Trigger: A station whose short name is longer than 8 bytes and the cut lands inside a UTF-8 sequence. `日本語放送局` is one. The cut is on `std::string::size`, which is bytes.
- Outcome: `refresh_presets` uses that string as the button label. It is not valid UTF-8, so the preset does not show a real short name.
- Fixed: v1.0.5

### An enclosure URL's query string hides the audio extension

- Severity: incorrect
- Confidence: high
- Where: `src/rss.cpp:70`
- Trigger: An `<enclosure>` whose `type` is missing or not `audio/*` / `video/*`, and whose URL is `https://cdn.example.com/ep.mp3?token=abc`.
- Outcome: The extension is everything after the last dot, including the query. `.mp3?token=abc` is not `.mp3`, so `is_audio_enclosure` is false and the item is skipped. A feed of only those items returns "No audio programs in this feed". The same URL with `type="audio/mpeg"` is kept.
- Fixed: v1.0.6

### A blank audio enclosure hides a later real one

- Severity: incorrect
- Confidence: high
- Where: `src/rss.cpp:143`
- Trigger: An item whose first `<enclosure type="audio/mpeg">` has an empty `url`, followed by a second enclosure with a real audio URL.
- Outcome: `is_audio_enclosure` is true for any `audio/*` type even when the URL is empty, and that empty string is returned immediately. The item is then dropped as if it had no enclosure.
- Fixed: v1.0.7

### A spelled-out weekday is cut through the year

- Severity: incorrect
- Confidence: high
- Where: `src/rss.cpp:118`, `src/rss.cpp:135`
- Trigger: `pubDate` of `Friday, 25 Sep 2026 12:00:00 GMT`. Three-letter `Fri,` is fine.
- Outcome: `%*3s` skips at most three characters. Both `sscanf` forms fail. The fallback keeps the first 16 bytes: `Friday, 25 Sep 2`. The year is gone.
- Fixed: v1.0.8
