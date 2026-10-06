# Listen-O-Matic backlog

Current release: **v1.0.19**. Last updated: 2026-10-06.

Kitchen radio. Live streams and podcast shows. Binary `listenomatic`. Suite catalog: `lcos-projects/PRODUCT-BACKLOG.md`. Design: `lcos-projects/LISTEN-O-MATIC.md`. Plan: [DEVELOPMENT.md](DEVELOPMENT.md).

## High Priority

None.

## Low Priority

None.

## Out of Scope

- WebKit show notes
- gPodder download-all / keep-playing queue / video
- A gtkmm clone of GoStations
- Split packing (programs beside the LCD)
- Dispatch text feeds as stations
- EarBlaster / Dispatch as the home of this UI
- Custom title bar; Bryan’s seal; Electron; a user-bus daemon

## Shipped

**v1.0.19** — Removing a station stores the playing episode's position.

**v1.0.18** — The end of an episode clears the earlier pause. An error in the middle stores that position.

**v1.0.17** — A pending episode seek does not jump a Live stream.

**v1.0.16** — Right-click a heard title on Live and the menu opens.

**v1.0.15** — Right-click a heard title on Live to copy that track string, or to search for it online.

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
