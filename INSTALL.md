# Installing Listen-O-Matic

Unreleased. M0 is a git build. `.deb` / tarball / AppImage arrive at M6 (`v0.1.0`).

## Runtime needs

- GTK 3 / gtkmm-3.0
- From M1: GStreamer 1.0 playbin (plugins-base, plugins-good)

```
sudo apt install libgtkmm-3.0-1t64
```

## Git build

```
sudo apt install build-essential meson ninja-build pkg-config g++ libgtkmm-3.0-dev
meson setup build
meson compile -C build
./build/listenomatic
```

Config (when M2 lands): `~/.config/listenomatic/listenomatic.ini`.
