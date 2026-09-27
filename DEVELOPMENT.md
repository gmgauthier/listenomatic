# Listen-O-Matic development plan

A gtkmm-3 **kitchen radio** for LCOS. Live streams and podcast shows on one face.

Display name: **Listen-O-Matic**
Binary / repo / package: `listenomatic`
APP_ID: `org.gmgauthier.ListenOMatic`
License: The Unlicense (`UNLICENSE`)
Repos: https://gitea.scriptorium/gmgauthier/listenomatic (origin), https://github.com/gmgauthier/listenomatic
Suite design: `lcos-projects/LISTEN-O-MATIC.md`

## Status (2026-09-27)

Unreleased. `master` is Unlicense only. Tag `v1.0.0` after this stack merges.

| Slice | Branch / PR | State |
|---|---|---|
| **M0** Window | `feature/m0-window` — Gitea #1 | Done. Deck chrome. Not merged. |
| **M1** Live play | `feature/m1-live-stream` — Gitea #2 (on M0) | Done. playbin, Add URL, Memory, volume, LCD name, ICY marquee. Not merged. |
| **M2** Presets | `feature/m2-presets` | Done. Punch 1–6, Store on Preset, Remove. Not merged. |
| **M3** Find live | `feature/m3-find-live` | Done. Search radio-browser inside Add…. Not merged. |
| **M4** One show | `feature/m4-one-show` | Done. RSS, programs, enclosure, seek. Not merged. |
| **M5** Catalog | `feature/m5-catalog` | Done. Station → Catalog…. Not merged. |
| **M6** Polish | `feature/m6-polish` | Done. Last program, window size, 15-minute feed refresh. Not merged. |
| **M7** Package | `feature/m7-package` | In tree. `.deb`, tarball, AppImage. Tag `v1.0.0` after merge. |

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
| Network | User-configured stream URLs and podcast RSS. No account, no daemon. Catalog looks up live on radio-browser.info and podcasts on Apple's public iTunes Search API (no key) |
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

Menus: File → Exit. Station → Add…, Catalog…, Remove, Store on Preset ▸ 1–6. Band → Live, Shows. Help → About.

## 3. Architecture

`MainWindow` owns the face. M1 adds a `Player` that wraps GStreamer `playbin` on a worker; do not block the GTK loop. ICY / tags via the bus. RSS with libxml2 (same as Dispatch). Fetch on a worker.

Preset buttons are ordinary `Gtk::Button`s. Do not draw a photorealistic car stereo.

## 4. Milestones

v1 is M0 through M7. Do not open download-cache, queues, or video until this set has been lived with.

### M0 — Window

Menus, band switch, six empty presets, Memory combo, LCD well, transport stubs, About. Matches the Deck mockup. **Done** on `feature/m0-window`.

### M1 — One live stream

Add a stream URL; Play / Stop; volume into playbin; LCD shows the name. First-run Memory from `data/samples.ini` (sample URLs, not a GoStations importer). Long ICY titles marquee in the LCD; the window does not grow. **Done** on `feature/m1-live-stream`. Presets stay empty until M2. Shows stay stub until M4.

### M2 — Presets + memory

Punch 1–6. Store on Preset. Labels show a short name. Independent live preset bank (Shows bank comes with M4). Memory combo and last station / volume already persist from M1.

### M3 — Find live (in Add…)

**Done** on `feature/m3-find-live`.

Station → **Add…** is Tune: paste a URL, or look up **one live stream**. When Type is **Live stream**, a **Search radio-browser** field (not a generic “Find”) queries radio-browser.info. Pick a row; name + URL fill; Add puts it in Memory. Down stations pruned. User-agent set.

When Type is **Show**, that search is hidden — RSS URL only (M4). Add is never a podcast directory.

Do not label the button **Find**. **Search** + “radio-browser.info” so it does not collide with Catalog.

### M4 — One show

Add an RSS URL; program list fills; play enclosure; seek. Skip feeds with no audio enclosure. Sample: Lunduke Journal podcast RSS.

### M5 — Catalog

Station → **Catalog…** opens a **separate window** (not the home screen, not the Add dialog). You **browse**:

- **Live stations** — radio-browser.info (same database as Add’s search, different verb: wander vs look up one).
- **Podcasts** — a shipped starter list (`data/podcasts.ini`, ~100 shows in Sci/Tech, News/Politics, Sports, Entertainment). **Search** looks up more on Apple's iTunes Search API (`feedUrl`, no account). Shows Apple does not list can still be added by RSS URL. Not Dispatch’s text feeds.

Select adds to Memory on the matching band. Store on Preset stays a face verb. Do not make Catalog the first thing you see.

### M6 — Polish

**Done** on `feature/m6-polish`.

- **Last program.** Each show remembers the episode you played (`program` group in the ini, keyed by RSS URL). Opening that show selects it and the LCD names it. Play starts there; the M4 resume position still applies. No auto-play.
- **Window size.** Default frame is 736×540. Long program titles ellipsize inside the list, and loading a feed does not grow the window. The frame can be resized; a size from 640×500 through 1280×1000 is stored as `window` `w` and `h`.
- **In-process refresh.** While the Shows band is up, the tuned feed reloads every 15 minutes. Playback is left alone. New episodes appear in the list.

ICY title already in M1. Resume-from-last-position and ±15s skip landed with M4.

### M7 — Package

**In tree** on `feature/m7-package`. Version `1.0.0`.

`scripts/release.sh` writes `dist/listenomatic-1.0.0.tar.xz`, `dist/listenomatic_1.0.0-1_amd64.deb`, and an AppImage when `linuxdeploy` is on `PATH`. The `.deb` is the LCOS install. The tag `v1.0.0` is cut after this stack merges to `master`. Sideboard gets a catalog row only after that GitHub release exists.

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

Do not bump for unreleased work. First tag is `v1.0.0` at M7: the finished radio, not a break from an older release. A later break of the ini or the face is `2.0.0`.
