# Listen-O-Matic development plan

A gtkmm-3 **kitchen radio** for LCOS. Live streams and podcast shows on one face.

Display name: **Listen-O-Matic**
Binary / repo / package: `listenomatic`
APP_ID: `org.gmgauthier.ListenOMatic`
License: The Unlicense (`UNLICENSE`)
Repos: https://gitea.scriptorium/gmgauthier/listenomatic (origin), https://github.com/gmgauthier/listenomatic
Suite design: `lcos-projects/LISTEN-O-MATIC.md`

## Status (2026-09-27)

**M1 in progress** on `feature/m1-live-stream`. M0 window is on `feature/m0-window`. First-run Memory is seeded from `data/samples.ini` (shipped sample streams, not a GoStations importer). Lunduke Journal RSS is the M4 sample show — not this slice.

## 1. Locked decisions

| Decision | Choice |
|---|---|
| Product | Original. Chrome is a kitchen radio / car stereo, not a directory |
| Name | Listen-O-Matic. Binary `listenomatic`. APP_ID `org.gmgauthier.ListenOMatic` |
| Rejected | TuneIn, Antenna, RadioActive, Signals, Dial, Bandstand, Callsign, Radio |
| Toolkit | C++17, gtkmm-3.0, GTK3 CSS, Meson |
| Engine | GStreamer `playbin`, audio only (`video-sink` is `fakesink`). Same stack as EarBlaster. Not mpv |
| Look | Deck packing. Presets in a row. LCD under them. On Shows, the program list sits **under** the LCD |
| Bands | **Live** and **Shows**, independent preset banks (six each) and Memory lists |
| State | `~/.config/listenomatic/listenomatic.ini` |
| Network | User-configured stream URLs and podcast RSS. No account, no daemon |
| Never as v1 | Download-cache, keep-playing queue, video, WebKit show notes, Split packing, GoStations home screen |
| Init | No systemd |
| Brand | LCOS beige / navy LCD `#0B1D38`. No Bryan’s seal |
| License | The Unlicense |
| Versioning | Semantic. `meson.build` is the source of truth. See **Process** |

## 2. Window

```
File  Station  Band  Help
[LIVE] [Shows]                         Volume ———●——
[1 —] [2 —] [3 —] [4 —] [5 —] [6 —] [Memory]
┌ LCD ─────────────────────────────────────────┐
│  No station                          LIVE    │
│  Station → Add… then Store on Preset.        │
└──────────────────────────────────────────────┘
 [ ■ ] [ ► ]
status
```

Shows: seek slider next to transport; program `TreeView` (title, date, time) under the LCD.

Menus: File → Exit. Station → Add…, Remove, Store on Preset ▸ 1–6. Band → Live, Shows. Help → About.

## 3. Architecture

`MainWindow` owns the face. M1 adds a `Player` that wraps GStreamer `playbin` on a worker; do not block the GTK loop. ICY / tags via the bus. RSS with libxml2 (same as Dispatch). Fetch on a worker.

Preset buttons are ordinary `Gtk::Button`s. Do not draw a photorealistic car stereo.

## 4. Milestones

v1 is M0 through M6. Do not open download-cache, queues, or video until this set has been lived with.

### M0 — Window (this slice)

Menus, band switch, six empty presets, Memory combo, LCD well, transport stubs, About. Add Station dialog is chrome only (no fetch). Matches the Deck mockup. No HTTP. No playbin.

### M1 — One live stream (this slice)

Add a stream URL; Play / Stop; volume into playbin; LCD shows the name. GStreamer `playbin`, audio only. First-run Memory is filled from `data/samples.ini` so there is something to play. Presets stay empty until M2. Shows stay stub until M4.

### M2 — Presets + memory

Several live stations persist in the ini. Punch 1–6. Memory combo. Store on Preset. Last station / volume.

### M3 — Find live

Add Station search against radio-browser.info. Down stations pruned. User-agent set. Search is still a dialog, not the home screen.

### M4 — One show

Add an RSS URL; program list fills; play enclosure; seek. Skip feeds with no audio enclosure.

### M5 — Polish

ICY / stream title in the LCD. Last program on a show. Window size. Auto-refresh show feeds while open (interval, in-process).

### M6 — Package

`debian/` is already in the tree. `scripts/release.sh` → `.deb`, tarball, AppImage. Tag `v0.1.0`.

## 5. Parked

OPML import of podcast feeds. GoStations `favorites.json` import. Episode download/cache. Keep-playing queue. Video podcasts. Split packing (programs to the right). Show-notes `TextView` under the list.

## 6. Traps

- Putting this UI in Dispatch or EarBlaster
- A gtkmm clone of GoStations (search-results home screen)
- WebKit “for show notes”
- gPodder: download all episodes, keep-playing queue
- Seeking on live streams as a product
- Starting as Split / EarBlaster two-pane
- Mixing Dispatch’s text feeds into Memory
- Custom title bar; Bryan’s seal; Electron; a user-bus daemon

## Process

Do not commit to `master`. Every change lands through a pull request.

### Branches

- `feature/<short-name>` — new user-visible work
- `fix/<short-name>` — bugs, packaging nits, regressions

Open a pull request into `master`. Merge only after review.

### Gates

A pull request must pass **lint** before merge. CI runs `./scripts/lint.sh` (no `--fix`). Locally:

- `./scripts/lint.sh --fix` — clang-format rewrites `src/`
- `./scripts/lint.sh` — SPDX headers, no tabs, clang-format `--dry-run --Werror`, cppcheck (`warning`) on `src/`
- `meson compile` with this tree’s `warning_level=2` is clean (no new warnings)

Do not pass `--fix` in CI. Do not merge a red PR.

**Tests** are required when they exist (`meson test -C build`). Until a test suite lands, the gate is lint plus a clean compile plus a manual pass of the change.

### Semantic versioning

Every **shipped** pull request — merged to `master` and tagged as a release — bumps the version. `meson.build` is the source of truth. Keep these in lockstep in the same PR:

- `meson.build` `version:`
- `debian/changelog` (new stanza)
- git tag `vMAJOR.MINOR.PATCH` after merge

Then `./scripts/release.sh` produces `.deb`, tarball, and AppImage.

| Bump | When |
|---|---|
| **PATCH** (`x.y.Z`) | Bug fix or packaging. No new user-facing feature. |
| **MINOR** (`x.Y.0`) | New backward-compatible feature. |
| **MAJOR** (`X.0.0`) | Breaking change: native file format, dropped config keys, removed UI users rely on. |

Do not bump for unreleased M0. First tag is `v0.1.0` at M6.
