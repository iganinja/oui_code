# Building

How to get the sources and build OUI Code, oui_run and oui_calc with CMake, which build trees there are and what each
puts in `bin/`, how a release's small portable binaries are made and published, how to build for Windows, and the
tools that format the C++ and regenerate the script API.

- [Requirements](#requirements)
- [Getting the sources](#getting-the-sources)
- [Configuring and building](#configuring-and-building)
- [Options](#options)
- [Targets](#targets)
- [Two build trees: terminal and window](#two-build-trees-terminal-and-window)
- [tools/build.sh](#toolsbuildsh)
- [Release binaries](#release-binaries)
- [Building for Windows](#building-for-windows)
- [Running the tests](#running-the-tests)
- [Formatting the C++](#formatting-the-c)
- [Regenerating the script API](#regenerating-the-script-api)

## Requirements

CMake 3.20 or newer, Ninja, a C++17 compiler, and GoogleTest for the tests. On Debian, Ubuntu or Mint:

```sh
sudo apt install cmake ninja-build g++ libgtest-dev libgmock-dev clang-format
```

On a Mac, `brew install ninja googletest`.

The window build needs SDL3, which OUI links statically. When CMake finds no installed SDL3, OUI fetches SDL's source
and builds it, and that needs the development headers of the display system: X11 (`libx11-dev`, `libxext-dev`,
`libxrandr-dev`, `libxcursor-dev`, `libxi-dev`, `libxfixes-dev`, `libxss-dev`, `libxtst-dev`), `libxkbcommon-dev`,
Wayland (`libwayland-dev`, `libwayland-bin`, `wayland-protocols`), `libegl-dev`, `libgl-dev`, `libdrm-dev` and
`libgbm-dev`, as `tools/build_portable.sh --gui` installs them.

The build treats every warning as an error (`-Wall -Wextra -Wpedantic -Werror`, or `/W4 /WX` with MSVC), in the
programs and in OUI alike. Third-party code (QuickJS-ng, stb_truetype, NanoSVG, SDL3) is built without them.

## Getting the sources

OUI, from [olagarro](https://github.com/iganinja/olagarro), is a git submodule in `third_party/olagarro`:

```sh
git clone --recurse-submodules https://github.com/iganinja/oui_code.git
```

In a clone made without it, `git submodule update --init` fetches it.

The submodule is a fixed commit of olagarro, and the programs are built against that one. A newer OUI comes in by
moving the submodule, with `git -C third_party/olagarro pull` for olagarro's latest or `git -C third_party/olagarro
checkout <commit>` for a given one, building and testing against it, and committing the new commit of the submodule
(`git add third_party/olagarro`). When OUI's `api` comments changed, the script API is made again in the same commit
([Regenerating the script API](#regenerating-the-script-api)).

## Configuring and building

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

The top `CMakeLists.txt` adds OUI with `add_subdirectory(third_party/olagarro)`, without OUI's own tests and examples,
and links the programs against its targets: `olagarro::OUI` and `olagarro::OUIScript`, and `olagarro::OUIWindow` and
`olagarro::OUIGraphics` when the options below ask for them. `CMAKE_EXPORT_COMPILE_COMMANDS` is on, so every build tree
has a `compile_commands.json`.

In a Debug build, OUI Code is compiled with `OUI_CODE_DEBUG_BUILD`, and writes every event it receives to
`oui_code_events.log` in the folder it runs in.

## Options

The pack's own:

| Option | Default | What it does |
|--------|---------|--------------|
| `OUI_CODE_BUILD_TESTS` | `ON` | Builds `oui_code_tests` and registers the tests with CTest. Needs GoogleTest. |
| `OUI_CODE_VERSION` | the project's, `0.4.0` | The version OUI Code shows. A release sets it from its tag. |
| `OUI_CODE_RUNTIME_OUTPUT_DIRECTORY` | `<source>/bin` | Where the programs are written. |

And OUI's, which choose what the programs can show on, set here so OUI builds what they need:

| Option | Default | What it does |
|--------|---------|--------------|
| `OLAGARRO_OUI_WINDOW` | `OFF` | Builds OUI's desktop window, on SDL3 linked statically, and lets the programs open in it with `--window`. The programs take a `_gui` name. |
| `OLAGARRO_OUI_DRM` | `OFF` | Lets OUI Code and oui_calc take the whole screen of a Linux console with `--drm`, through the kernel's mode setting, with no display server. |
| `OLAGARRO_STATIC_LINK` | `OFF` | Links the programs statically, so they run on Linux systems whose C and C++ runtime is older than the build machine's. |

`OLAGARRO_STATIC_LINK` does more than add `-static`. A static binary carries the whole runtime, so the option also
compiles with `-ffunction-sections -fdata-sections` and links with `-Wl,--gc-sections`, dropping every function nobody
calls; and, on Linux, with `-fno-pie` and `-no-pie`, since a program loaded at a fixed address needs no table of
addresses to fix up. With the window module, though, only the C++ runtime goes inside (`-static-libgcc
-static-libstdc++`): SDL3 loads the system's display libraries when it starts, and those take the system's C library.
Such a build runs on systems as new as the one it was built on, or newer.

A build tree configured by hand for a special purpose, as one with sanitizers in `CMAKE_CXX_FLAGS` or one optimised for
size, should set `OUI_CODE_RUNTIME_OUTPUT_DIRECTORY` to a folder of its own, such as `<tree>/bin`, so its programs do
not replace the ones in `bin/`.

## Targets

| Target | In `bin/` | With `OLAGARRO_OUI_WINDOW` |
|--------|-----------|----------------------------|
| `oui_code` | `oui_code` | `oui_code_gui` |
| `oui_run` | `oui_run` | `oui_run_gui` |
| `oui_calc` | `oui_calc` | `oui_calc_gui` |
| `oui_code_tests` | stays in the build tree | |

Each program is compiled with a macro for each place it can show: `OUI_CODE_WINDOW`, `OUI_RUN_WINDOW` and
`OUI_CALC_WINDOW` with the window module; `OUI_CODE_DRM` and `OUI_CALC_DRM` with `OLAGARRO_OUI_DRM`; and, on Linux,
`OUI_CODE_DESKTOP` and `OUI_CALC_DESKTOP`, for OUI's own desktop, with `OUI_CODE_DESKTOP_PIXELS` and
`OUI_CALC_DESKTOP_PIXELS` when the build has the window or the screen. To build one program only:

```sh
cmake --build build --target oui_code
```

## Two build trees: terminal and window

The window module changes the programs, not only adds a library: it links SDL3 into them, defines the macros that
compile in their `--window` code, and changes their names. So a terminal build and a window build are two build trees,
configured with `OLAGARRO_OUI_WINDOW` off and on:

| Tree | Configured with | Puts in `bin/` |
|------|-----------------|----------------|
| `build/` | `OLAGARRO_OUI_WINDOW=OFF`, Debug | `oui_code`, `oui_run`, `oui_calc` |
| `build-gui/` | `OLAGARRO_OUI_WINDOW=ON`, Debug | `oui_code_gui`, `oui_run_gui`, `oui_calc_gui` |
| `build-release/`, `build-gui-release/` | the same, Release | the same names as their Debug tree |

Both write to the same `bin/`, and the different names let the two sets sit side by side. OUI Code keeps its settings
in `oui_code.ini` beside its binary when it can write there (`SettingsLocation::portable`), under that name in both
builds, so `oui_code` and `oui_code_gui` share them. A window build still runs in the terminal when it is started
without `--window`, so it is the more complete of the two; the terminal build is the one that is shipped for the
terminal, being smaller and needing nothing of the display system. A change to OUI or to a program is only in both
binaries once both trees are built again:

```sh
cmake --build build && cmake --build build-gui
```

## tools/build.sh

`tools/build.sh` configures, builds and tests in one step, on Linux and other POSIX systems:

```sh
tools/build.sh [--release] [--gui] [--static] [--no-tests] [--clean]
```

| Option | Effect |
|--------|--------|
| (none) | Debug in `build/`. |
| `--release` | Release in `build-release/`. |
| `--gui` | `OLAGARRO_OUI_WINDOW=ON` in `build-gui/`, or `build-gui-release/` with `--release`. |
| `--static` | `OLAGARRO_STATIC_LINK=ON`. |
| `--no-tests` | Skips `ctest`. |
| `--clean` | Deletes the build tree first, for a cold build. |

It always uses Ninja, and runs `ctest --output-on-failure` with as many jobs as the machine has processors.

## Release binaries

A release carries OUI Code, oui_run and oui_calc for Linux and Windows on x86-64, for Linux on 64-bit ARM (aarch64),
as a Raspberry Pi 3, 4 or 5 with a 64-bit system, and for macOS on Apple's processors (arm64) and Intel's (x86_64), in
two kinds: `terminal`, the programs as the terminal build makes them, and `gui`, those that also open in a window.
Both kinds name them the same, `oui_code` and the rest: a system has one or the other, and the `_gui` names of a
window build here are only so the two sit side by side in `bin/`. Two scripts make them, the same on a developer's
machine as on GitHub's.

`tools/build_portable.sh` builds one system and kind in a container, with podman or docker, mounting the repository
read-only, and writes the programs to `dist/<system>-<processor>-<kind>/`:

```sh
tools/build_portable.sh                   # dist/linux-x86_64-terminal: oui_code, oui_run, oui_calc
tools/build_portable.sh --gui             # dist/linux-x86_64-gui: oui_code, oui_run, oui_calc
                                          # on an ARM machine: dist/linux-aarch64-terminal and -gui
tools/build_portable.sh --windows         # dist/windows-x86_64-terminal: oui_code.exe and the rest
tools/build_portable.sh --windows --gui   # dist/windows-x86_64-gui: oui_code.exe and the rest
```

All are MinSizeRel, with tests off, `OLAGARRO_STATIC_LINK=ON` and link-time optimisation, and stripped.
`OUI_CODE_VERSION` in the environment, as `v0.4.0`, is the version OUI Code shows; `JOBS` says how many compilers run
at a time in a container, every processor's unless set.

- **Linux terminal.** In Alpine 3.20, linked statically against musl, so they run on any Linux of their processor
  whatever its C and C++ runtime: x86-64 on an x86-64 machine, aarch64 on an ARM one. `oui_code` and `oui_calc` are
  built with clang, libc++ (with libc++abi, compiler-rt and LLVM's libunwind), `-Oz`, full link-time optimisation,
  lld and identical code folding (`-Wl,--icf=all`); `oui_run` with GCC, which makes it smaller. `oui_code` comes out
  at about 2.2 MB, oui_run at 1.5 MB and oui_calc at 0.7 MB.
- **Linux gui.** In Ubuntu 22.04: SDL3 and the C++ runtime go inside, the C library and the display libraries come
  from the system, so they run on Ubuntu 22.04 (glibc 2.35) and anything newer. `oui_code` is about 3.7 MB. Ubuntu
  22.04's GCC 11 warns of uninitialised values where there are none, so that one warning is not an error there.
- **Windows.** Cross-built in Ubuntu 22.04 with [llvm-mingw](https://github.com/mstorsjo/llvm-mingw), a fixed version
  checked against its SHA-256 sum, and linked statically: the `.exe` needs only the system's DLLs.
- **macOS.** Built on the Mac itself, as Apple's tools run in no container: `tools/build_portable.sh`, with or without
  `--gui`, run on a Mac makes `dist/macos-<processor>-<kind>`, arm64 on Apple's processors and x86_64 on Intel's. They
  run on macOS 11 and newer, linked against the system's libraries, which every Mac has, unused code dropped
  (`-Wl,-dead_strip`), and signed ad hoc, with no certificate, as a Mac with Apple's processor runs nothing unsigned.
  They are not signed by Apple: a browser marks what it downloads, and macOS then asks before running it the first
  time (right-click, Open); `install.sh` downloads with curl, which marks nothing.

`tools/package_release.sh` then makes the release's files of whatever `dist/` holds, in `dist/release/`:

| File | What it is |
|------|------------|
| `oui_code-<system>-<processor>-<kind>.tar.gz`, `.zip` for Windows | the three programs of that kind, with `LICENSE` and `THIRD_PARTY_NOTICES.txt` |
| `<program>-<system>-<processor>-<kind>`, `.exe` for Windows | each program alone, as `oui_code-linux-x86_64-gui` or `oui_calc-linux-aarch64-terminal` |
| `oui-pack-linux-<processor>-<kind>.deb` | the three Linux programs of that kind as a Debian package, `oui-pack` or `oui-pack-gui`, in `/usr/bin`, for `amd64` or `arm64` |
| `THIRD_PARTY_NOTICES.txt` | the licences of everything any of them carries, for whoever downloads one program alone |
| `SHA256SUMS` | the sum of each file |

`THIRD_PARTY_NOTICES.txt` holds the licences of what the programs carry: OUI itself, QuickJS-ng, stb_truetype,
NanoSVG, Spleen and Jersey 10 in all, SDL3 in the `gui` ones, musl in the Linux terminal ones, and the mingw-w64
runtime and winpthreads in the Windows ones. The script takes them from the submodule, from olagarro's `licenses/`
folder and from beside the code each licence covers.

The Debian packages take their version from `OUI_CODE_VERSION`, as the release workflow sets it from the tag, `0.4.0`
from `v0.4.0`, or, unset, from `git describe` between tags, `0.4.0+3.ged0fda3`, which Debian orders after it. `sudo
dpkg -i` installs one, and a newer one upgrades it the same way. The two conflict, each replacing the other. The `gui`
one depends on `libc6 (>= 2.35)`, as its programs do, and puts OUI Code and OUI Calc in the desktop's menu, with their
icons, from `tools/linux_desktop`; started from there they open in their window. They carry the licences as Debian
keeps them, in `/usr/share/doc/<package>/copyright`, and a changelog saying where they were built from.

The same packages are in an APT repository at `https://iganinja.github.io/oui_code`, suite `stable`, component `main`,
so `apt upgrade` brings each release. `tools/apt_repository.sh <folder with the .deb files> <folder to make>` makes
it: the packages in `pool/main`, `Packages` for each architecture, and `Release` signed as `Release.gpg` and as
`InRelease` with the secret key in gpg's keyring (`OUI_CODE_APT_KEY` chooses among several), whose public part goes
beside them as `oui_code.gpg` for systems to trust. It holds one release only; older ones stay in GitHub's releases.
`install.sh --apt` adds it to a system.

The names carry no version, so a release's files are always at the same address: GitHub's
`https://github.com/iganinja/oui_code/releases/latest/download/<file>` is the latest release's. The installers
download from there: `install.sh` on Linux and macOS and `install.ps1` on Windows show a menu of the programs and
kinds, or take options to choose without asking, check each file against `SHA256SUMS`, and install the programs, in
`~/.local/bin` or in `%LOCALAPPDATA%\Programs\oui_code`, which `install.ps1` adds to the user's `PATH`; or unpack a
whole package into the folder they run in. `OUI_INSTALL_DIR` and `OUI_DOWNLOAD_URL` change where the programs go and
where they come from, and `OUI_APT_URL` the APT repository `install.sh --apt` adds.

GitHub makes the releases with `.github/workflows/release.yml`, which checks the submodule out with the rest. Pushing
a version's tag, as `v0.4.0`, runs the tests in a terminal build and in a `gui` one on x86-64 and ARM64 Linux, and on
GitHub's Macs, Apple's and Intel's, four at a time there and past every error of the build, then builds the ten kinds
with `tools/build_portable.sh` in parallel, each on a machine of its system and processor, then makes the files with
`tools/package_release.sh` and publishes them as the tag's release. Nothing is published when a test fails. Then a
tag's Debian packages become the APT repository, published on GitHub Pages, signed with the key in the repository's
`APT_SIGNING_KEY` secret. Started by hand from the Actions tab, it makes the same files and leaves them as the run's
artifact, publishing nothing and leaving the APT repository as it was.

That needs, once: GitHub Pages published by GitHub Actions (Settings, Pages, Source), tags allowed to deploy to the
`github-pages` environment (Settings, Environments, `github-pages`, a tag rule `v*`), and the signing key as the
secret, made with no passphrase, its armoured secret part as the value:

```sh
export GNUPGHOME="$(mktemp -d)"
gpg --batch --passphrase '' --quick-generate-key "OUI Code APT repository" ed25519 sign never
gpg --armor --export-secret-keys > oui_code-apt-signing-key.asc
```

The file is the only way to sign new releases for systems that already trust the key: kept somewhere safe, it lets
the secret be made again; lost, a new key means each system adding the repository again.

## Building for Windows

The sources build for Windows, as OUI does: the terminal backend uses the Windows console and the pseudo-terminal
uses ConPTY. OUI's desktop and the DRM backend are Linux only and are left out. With MSVC the warnings are
`/W4 /WX`. With GCC targeting Windows (MinGW-w64) the programs are always linked statically (`-static
-static-libgcc -static-libstdc++`), whatever `OLAGARRO_STATIC_LINK` says; with clang they are with
`OLAGARRO_STATIC_LINK=ON`, and without it llvm-mingw's libc++ and libunwind come as DLLs beside them.

`tools/build_portable.sh --windows` makes the release's `.exe` files in a container, as [release
binaries](#release-binaries) tells. By hand, they are cross-compiled from Linux with a MinGW-w64 toolchain.
[llvm-mingw](https://github.com/mstorsjo/llvm-mingw) (clang, lld and libc++) gives the smallest binaries; GNU's
MinGW-w64 linker keeps every function's unwind data, so its `--gc-sections` drops almost nothing. A CMake toolchain
file for it looks like this:

```cmake
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-clang)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-clang++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
# The toolchain's sysroot, and wherever a cross-built GoogleTest is installed.
set(CMAKE_FIND_ROOT_PATH /path/to/llvm-mingw/x86_64-w64-mingw32 /path/to/gtest-windows)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
```

```sh
cmake -S . -B build-windows -G Ninja -DCMAKE_TOOLCHAIN_FILE=windows.cmake -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DOLAGARRO_STATIC_LINK=ON -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
    -DOUI_CODE_RUNTIME_OUTPUT_DIRECTORY=$PWD/build-windows/bin -DOUI_CODE_BUILD_TESTS=OFF
cmake --build build-windows --target oui_code oui_run oui_calc
```

For the tests, GoogleTest has to be built with the same toolchain and installed where `CMAKE_FIND_ROOT_PATH` reaches,
and `CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE=PRE_TEST` keeps CMake from running the `.exe` test binaries on Linux
to list their tests while it builds. They then run on Windows, or under Wine, which needs `LANG=C.UTF-8` for
arguments outside ASCII to arrive whole, and Wine 11 or newer for ConPTY. With libc++, the C++ runtime's DLLs go
beside the test executables.

libc++ is stricter about includes than libstdc++: code that builds on Linux with GCC may miss an include there, as
`<cstdlib>` for `std::abs`.

## Running the tests

```sh
ctest --test-dir build --output-on-failure -j "$(nproc)"
ctest --test-dir build -R OuiCode            # only the tests whose name matches
```

CTest runs `oui_code_tests`, each GoogleTest case a test of its own, and the programs' own tests: OUI Code's
`--smoke-test`, `--stress-test`, `--input-test` and `--help`, the script API check, and the tests that drive the
programs in a pseudo terminal, which take a few minutes and are not registered on Windows. In a window build, the
same tests run against the `_gui` programs. [Testing](testing.md) describes each and how to run one case.

## Formatting the C++

```sh
oui_run tools/format_cpp.js                         # every C++ source in the repository
oui_run tools/format_cpp.js oui_code/workbench.cpp  # only these
oui_run tools/format_cpp.js --clang-format=/usr/bin/clang-format-18
```

The script runs clang-format with the repository's `.clang-format` on every `.h`, `.hpp`, `.cpp`, `.cc` and `.cxx`
file, in batches of forty so the command line stays within what every system takes. It leaves out `.git`,
`third_party` (OUI included, which olagarro formats itself), `dist`, and any folder whose name starts with `bin` or
`build`. Then it puts back the space the conventions want after a prefix increment or decrement, `++ index`, which
clang-format takes away; comments, strings, characters and raw string literals are left as they are. Run it, with
`bin/oui_run` or an installed oui_run, before submitting a change. The conventions are olagarro's, in its
[CONTRIBUTING.md](https://github.com/iganinja/olagarro/blob/master/CONTRIBUTING.md).

## Regenerating the script API

Every name a script can use has a comment marked `api` beside the code that defines it, in OUI's sources and in the
programs':

```cpp
// api(system): oui.readFile(path: string): string | null
// The file's text, or null when it cannot be read.
```

`tools/extract_api.js` reads them from every `.cpp`, `.h` and `.js` under `third_party/olagarro/oui`, `oui_calc`,
`oui_code` and `oui_run`, and writes [oui.d.ts](../oui.d.ts), the TypeScript declarations editors read, and
[doc/oui_api.md](oui_api.md), the reference:

```sh
bin/oui_run tools/extract_api.js           # writes both files
bin/oui_run tools/extract_api.js --check   # writes nothing; fails when they are out of date
```

It runs from the repository's top. With `--check` it also fails when oui_run's own `oui` has a name no comment
declares, or a comment declares one oui_run does not have. CTest runs the check as `OuiRun.ScriptApi`, so an API
change without the regenerated files fails the tests, and so does moving the submodule to a version of OUI whose
comments changed without making the files again. OUI Code builds `oui.d.ts` into itself, for its completion of script
names, so after regenerating it OUI Code has to be built again. [oui_run's
host](oui_run.md#the-api-comments-and-the-generated-reference) explains the comment's format and the scripts each
`<where>` names.
