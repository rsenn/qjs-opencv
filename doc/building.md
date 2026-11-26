# Building qjs-opencv

See the README for the short version and `CLAUDE.md` for the full option matrix. This page covers how QuickJS is
found or built.

## QuickJS: installed, parent tree or built on the fly

`cmake/FindQuickJS.cmake` and `cmake/BuildQuickJS.cmake` are the canonical copies shared by all qjs-* projects
(`~/.claude/skills/quickjs-native-bindings/scripts/cmake-sync.sh status`); do not edit them here, change the
canonical copy and sync. `find_quickjs()` takes the first match of:

1. a parent project that already builds quickjs (`QUICKJS_BUILD_ROOT` set, or `TARGET quickjs` exists)
2. `BUILD_QUICKJS=ON`, or `AUTO` (default) and no installation found: `BuildQuickJS.cmake` builds quickjs at
   configure time in `<build>/quickjs-build`. Source: `QUICKJS_SOURCES_ROOT`, else the parent directory if it is a
   quickjs tree, else a clone of `QUICKJS_GIT_URL` into `third_party/quickjs` (or `<build>/quickjs-src`)
3. an installation: `<prefix>/include/quickjs/quickjs.h`, found under `/usr/local` and `/usr` or given with
   `-DQUICKJS_PREFIX=<prefix>`

```bash
cmake -S . -B build/x -DOpenCV_DIR=...                        # installed quickjs, else built
cmake -S . -B build/x -DBUILD_QUICKJS=ON -DQUICKJS_PREFIX=/none   # always build
cmake -S . -B build/x -DBUILD_QUICKJS=OFF                     # installation required
```

When quickjs is built, headers and `libquickjs` come from the build tree, while rpath and module directories refer
to the install prefix; the built quickjs is installed with the project (`BUILD_QUICKJS_INSTALL`).
`CMakeLists.txt` includes `Functions.cmake` and `Compat.cmake` before `FindQuickJS.cmake` because the canonical
file needs `message_table()` from `Compat.cmake`.

Limits: a fresh clone needs network and submodule access, and a cross build needs a host `qjsc`.
Checked on x86_64 (installed, and `BUILD_QUICKJS=ON` from the parent tree); not yet on the Termux phone.
