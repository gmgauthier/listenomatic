# Installing Listen-O-Matic

Four ways to get a binary, in the order LCOS cares about:

| Artifact | Who it is for |
|---|---|
| **`.deb`** | LCOS, Devuan Excalibur, Debian Trixie. Preferred. |
| **Source tarball** | Distro packagers and `meson setup && ninja install`. |
| **AppImage** | Fallback for distros that do not install `.deb` files. gtkmm only. Published on the GitHub/Gitea release. |
| **Git build** | Developers. See below. |

Version comes from `meson.build` (currently `1.0.11`).

## Runtime needs

- GTK 3 / gtkmm-3.0
- GStreamer 1.0 playbin (plugins-base, plugins-good)
- glib-networking (HTTPS for Catalog, radio-browser, and podcast RSS)

On Debian / Devuan / LCOS:

```
sudo apt install libgtkmm-3.0-1t64 gstreamer1.0-plugins-base gstreamer1.0-plugins-good glib-networking
```

## 1. Debian package (preferred)

From a release `.deb`:

```
sudo apt install ./dist/listenomatic_1.0.11-1_amd64.deb
```

Or, from this tree:

```
./scripts/release.sh deb
sudo apt install ./dist/listenomatic_1.0.11-1_amd64.deb
```

That installs:

- `/usr/bin/listenomatic`
- `/usr/share/applications/listenomatic.desktop`
- `/usr/share/icons/hicolor/scalable/apps/listenomatic.svg`
- `/usr/share/listenomatic/skin/lcos/lcos.css`
- `/usr/share/listenomatic/samples.ini`
- `/usr/share/listenomatic/podcasts.ini`
- `/usr/share/listenomatic/brand/icon-tile.svg`

Launch from the menu or `listenomatic`. Config is `~/.config/listenomatic/listenomatic.ini`.

Uninstall: `sudo apt remove listenomatic`.

## 2. Source tarball

`meson dist` produces `build/meson-dist/listenomatic-VERSION.tar.xz`.

```
tar -xf listenomatic-1.0.11.tar.xz
cd listenomatic-1.0.11
sudo apt install build-essential meson ninja-build pkg-config g++ \
  libgtkmm-3.0-dev libgstreamer1.0-dev libsoup-3.0-dev libjson-glib-dev libxml2-dev \
  gstreamer1.0-plugins-base gstreamer1.0-plugins-good
meson setup build --prefix=/usr
meson compile -C build
sudo meson install -C build
```

`./scripts/release.sh tarball` runs `meson dist` for you.

## 3. AppImage (fallback)

The image bundles gtkmm from the build host. Playback still uses the host GStreamer plugins (plugins-base and plugins-good). HTTPS uses the bundled GIO GnuTLS module when the build host has it.

```
./scripts/release.sh appimage
```

Requires `linuxdeploy` on `$PATH` (see <https://github.com/linuxdeploy/linuxdeploy>). Output lands under `dist/`.

```
chmod +x Listen-O-Matic-*.AppImage listenomatic-*.AppImage
./Listen-O-Matic-*.AppImage
```

The AppImage runtime sets `APPDIR`. Listen-O-Matic looks for the skin, samples, and starter podcasts under `$APPDIR/usr/share/listenomatic`. Leave `APPDIR` unset for `.deb` and `meson install` builds.

## 4. Developer build (no install)

```
sudo apt install build-essential meson ninja-build pkg-config g++ \
  libgtkmm-3.0-dev libgstreamer1.0-dev libsoup-3.0-dev libjson-glib-dev libxml2-dev \
  gstreamer1.0-plugins-base gstreamer1.0-plugins-good clang-format cppcheck
meson setup build
meson compile -C build
./build/listenomatic
```

The binary finds CSS, `samples.ini`, and `podcasts.ini` via `SOURCE_ROOT` in the build tree. `LISTENOMATIC_DATA` overrides that.

## One command for every artifact

```
./scripts/release.sh all
```

Writes tarball, `.deb`, and AppImage (if `linuxdeploy` is there) under `dist/`. The GitHub/Gitea release includes the AppImage as the non-deb fallback.

Sideboard lists this package only after that release is published. It installs the `.deb` from GitHub Releases and ignores the tarball and AppImage.
