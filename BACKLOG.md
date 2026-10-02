# Listen-O-Matic backlog

Current release: **v1.0.14**. Last updated: 2026-10-02.

Kitchen radio. Live streams and podcast shows. Binary `listenomatic`. Suite catalog: `lcos-projects/PRODUCT-BACKLOG.md`. Design: `lcos-projects/LISTEN-O-MATIC.md`. Plan: [DEVELOPMENT.md](DEVELOPMENT.md).

## High Priority

None.

## Low Priority

- **Tracks context menu on Live.** Right-clicking a row in the Live tab's Tracks list opens a context menu with two items:
  - **Copy track name**: puts the track string, exactly as shown in the row, on the clipboard.
  - **Search for track online**: opens the default web browser (via the desktop's URI handler) on a standard web search for the track string, URL-encoded.

## Out of Scope

- WebKit show notes
- gPodder download-all / keep-playing queue / video
- A gtkmm clone of GoStations
- Split packing (programs beside the LCD)
- Dispatch text feeds as stations
- EarBlaster / Dispatch as the home of this UI
- Custom title bar; Bryan’s seal; Electron; a user-bus daemon

## Shipped

**v1.0.14** — Fix: live search also matches country, state, and a two-letter country code.

**v1.0.13** — Fix: a late iTunes result no longer replaces the starter list.

**v1.0.12** — Fix: a resume seek is retried until the pipeline accepts it.

**v1.0.11** — Fix: the "Loading programs…" row is not an episode, so it cannot start the previous show.

**v1.0.10** — Fix: choosing a different show stores the playing episode's position and stops that episode.

**v1.0.9** — Fix: the current live title is listed in Tracks as soon as it arrives.

**v1.0.8** — Fix: a spelled-out weekday in pubDate no longer cuts the date through the year.

**v1.0.7** — Fix: a blank audio enclosure no longer hides a later real one.

**v1.0.6** — Fix: an enclosure URL's query string no longer hides the audio extension.

**v1.0.5** — Fix: preset short names are cut on characters, so non-ASCII names show a real label.

**v1.0.4** — Fix: an empty Memory is no longer refilled from the samples, and an unreadable listenomatic.ini is kept as listenomatic.ini.bad.

**v1.0.3** — Fix: leaving Shows saves the resume; coming back no longer writes the live position onto the episode.

**v1.0.2** — Headless meson test suite, and known defects recorded in BUG-BACKLOG.md.

**v1.0.1** — Clearlooks seek grip stays off +15 and the time.

**v1.0.0** — M0–M7. Deck, live streams, presets, Catalog, shows, and the package.
