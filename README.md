# Listen-O-Matic

**Vended by Grok Build**

![Listen-O-Matic on LCOS](brand/screenshot.png)

A **kitchen radio** for The Lunduke Computer Operating System (LCOS). Live streams and podcast shows on one face: six presets per band, a Memory list, a now-playing well.

Binary: `listenomatic`. Unlicense.

LCOS itself: [https://github.com/BryanLunduke/LCOS](https://github.com/BryanLunduke/LCOS)

## Status

**M6 done, unreleased.** Live, Shows, Catalog. A show reopens on the last episode, the frame keeps its size, and the tuned feed reloads every 15 minutes. Package is later. See [INSTALL.md](INSTALL.md) and [DEVELOPMENT.md](DEVELOPMENT.md).

| Doc | What |
|---|---|
| [DEVELOPMENT.md](DEVELOPMENT.md) | Locked decisions, architecture, milestones, branching, semver, lint |

## Build

```
sudo apt install build-essential meson ninja-build pkg-config g++ libgtkmm-3.0-dev clang-format cppcheck
meson setup build
meson compile -C build
./build/listenomatic
```

PR lint gate: `./scripts/lint.sh` (CI runs this; no `--fix`). Format `src/` locally with `./scripts/lint.sh --fix`.

## License

[The Unlicense](https://unlicense.org). See [UNLICENSE](UNLICENSE).
