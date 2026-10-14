# Testing

How OUI Code, oui_run and oui_calc are tested: the C++ unit tests of OUI Code's parts, the programs' own checks that
CMake registers beside them, and the tests that run the programs in a pseudo terminal the way a person uses them. It
ends with how to run one case and the rules a change follows. OUI itself is tested in olagarro, as its
[testing document](https://github.com/iganinja/olagarro/blob/master/doc/testing.md) tells.

- [Running the tests](#running-the-tests)
- [What ctest runs](#what-ctest-runs)
- [Unit tests in C++](#unit-tests-in-c)
- [Programs in a pseudo terminal](#programs-in-a-pseudo-terminal)
- [oui_run's own tests](#oui_runs-own-tests)
- [Running one case](#running-one-case)
- [House rules](#house-rules)

## Running the tests

Tests are built unless `OUI_CODE_BUILD_TESTS` is `OFF`. They need GoogleTest (`libgtest-dev` on Debian and Ubuntu,
`googletest` from Homebrew). From the repository:

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

`tools/build.sh` configures, builds and then runs `ctest --output-on-failure -j` with one job per core;
`--no-tests` skips that last step. The test program, `oui_code_tests`, stays in the build folder; the programs land
in `bin/`. In a window build, `tools/build.sh --gui`, the same tests run against `oui_code_gui` and the rest.

The build treats warnings as errors, tests included.

## What ctest runs

`CMakeLists.txt` registers two kinds of test. `oui_code_tests` goes through `gtest_discover_tests`, so every `TEST` in
it is a ctest test of its own, named `Suite.Case`. The rest are commands added with `add_test`, named after the
program they test.

| ctest name | What it runs | What it checks |
|------------|--------------|----------------|
| `FindBarTests.*`, `LiveScriptTests.*`, `ScriptRunTests.*`, `OuiCode...Tests.*` | `oui_code_tests`, `tests/oui_code/*_test.cpp` with OUI Code's sources | the find bar, scripts and the built-in ones, Interactive Script, Run Script, ignore rules, the task list, help, session entries, the UI preview and skeletons |
| `TranslationTests.*` | `oui_code_tests`, `tests/translation_test.cpp` | each program's catalog covers every text of its sources, and nothing else |
| `OuiCode.Smoke` | `oui_code --smoke-test` | OUI Code starts, renders once, and exits |
| `OuiCode.Help` | `oui_code --help` | the options print as `--help`, never as `-- help` |
| `OuiCode.Stress` | `oui_code --stress-test` | OUI Code's dialogs and commands, played from a script of events, within 30 s |
| `OuiCode.Input` | `oui_code --input-test` | dead-key accents typed into a document come out as the expected text |
| `OuiRun.ScriptApi` | `oui_run tools/extract_api.js --check` | `oui.d.ts` and `doc/oui_api.md` match the `api` comments, and `oui` has no name without one |
| `OuiRun.Scripts.1` to `.3` | `oui_run tests/oui_run/run_tests.js` | oui_run itself and the recipes, every third case each |
| `OuiCode.EndToEnd.1` to `.4` | `oui_run tests/oui_code/end_to_end.js` | OUI Code in a pseudo terminal, keys pressed, every fourth case each |
| `OuiCalc.EndToEnd` | `oui_run tests/oui_calc/end_to_end.js` | OUI Calc the same way |

The tests that are scripts are run by the oui_run just built, from the repository's top. The three script-driven
suites (`OuiRun.Scripts`, `OuiCode.EndToEnd`, `OuiCalc.EndToEnd`) need `script(1)`, `mkfifo`, `mktemp` and a POSIX
shell, so they are not registered on Windows. The two long ones run as parts side by side, each given its part as
`"part 2 of 4"` after the program, as a name given there runs the cases that have it; each part has 300 s, and
`OuiCalc.EndToEnd` 120 s: each end-to-end case runs the program for a few seconds of keys.

## Unit tests in C++

`oui_code_tests` builds the parts of OUI Code that do not need the `Workbench`, the sources of `oui_code/` it needs
compiled into it, with GoogleTest. A test names what it checks as a sentence, and its comments say why the case
matters.

Some tests read the repository rather than the program: the test program is built with `OUI_CODE_SOURCE_DIR` pointing at
it. `OuiCodeScriptingTests` load and run OUI Code's built-in scripts from `oui_code/scripts`;
`tests/oui_code/generated_window.cpp` is what Interactive UI's Generate writes for `tests/oui_code/rename_files.oui`,
compiled with the tests so what Generate writes builds, and `OuiCodeUiSkeletonTests` checks Generate still writes it;
the translation tests read every `.h`, `.cpp` and `.oui` under `oui_code`, `oui_run` and `oui_calc` and check each
program's catalogs against them, as OUI's own catalog test does, which
[translations](https://github.com/iganinja/olagarro/blob/master/doc/translations.md#tests-that-check-catalogs)
describes.

OUI Code's `--stress-test` and `--input-test` play their events through OUI's `ScriptedBackend`, which keeps the
frames the application drew; olagarro's testing document tells how widgets are tested that way.

## Programs in a pseudo terminal

`tests/oui_code/end_to_end.js` and `tests/oui_calc/end_to_end.js` test OUI Code and OUI Calc as a person uses them:
the real program, in a real pseudo terminal, keys pressed one after another, and the screen read back. They are
JavaScript run by oui_run, and share `tests/lib/terminal_session.js`.

`place(program, files)` makes a temporary folder (`mktemp -d`), copies the program into it, and writes the files
given. The program keeps its settings beside itself, so a copy in a folder of its own never touches the settings,
session or recent files of anyone's own copy. `run(args, keys, {columns, rows})` on what `place` gave runs it there:

- `script -qec` gives the program a pseudo terminal, sized with `stty cols ... rows ...` (100 by 24 unless told),
  with `LANG=C` and `LANGUAGE=en` so the texts are the English ones.
- The keys go through a named pipe (`mkfifo`) the terminal reads, held open for reading and writing so the program
  never sees its input end.
- Each key is written once the program has answered the one before: until its output has grown and then been quiet
  for 200 ms, or 3 s at most for a key that changes nothing. The first key waits up to 15 s for the program to start.
  A machine with much else to do is waited for rather than pressed past. An empty string as a key waits 600 ms.
- After the last key the terminal is stopped under the program, and `screen()` replays what it wrote.

A key is the text a terminal sends for it: `"\r"` for Enter, `"\x1bOP"` for F1, `"\x1bt"` for Alt+T, `"\x13"` for
Ctrl+S, and SGR mouse reports such as `` `\x1b[<0;${column};${row}M` `` for a click. `session(program, args, files,
keys, size)` does `place`, one `run` and `remove` in one call; a case that runs the program twice, to see what it
keeps from one start to the next, uses `place` and calls `run` twice.

`screen()` understands what OUI writes: cursor moves (`CSI row;column H`) and colours (`CSI ... m`), skipping any
other sequence. What it returns has `row(n)`, the text of a row; `find(text, offset)`, the row, column and colours
of the first place a text shows, or `null`; `all()`; and `written`, the raw output, for what leaves no trace on the
screen. Colours are the SGR numbers, `"97;41"` for bright white on red:

```js
test("--diff opens the comparison alone, the removed line in red", async () => {
    const shown = await session(ouiCode, ["--diff", "left.txt", "right.txt"], files, []);
    expect("the tab", shown.find("left.txt vs right.txt") !== null, true);
    expect("the removed line", shown.find("gone")?.colours, "97;41");
});
```

`expect(what, actual, expected)` compares with `!==` and throws with both values. `run(cases, only)` runs every case,
or those whose name contains `only`, prints `ok` or `FAIL` and the reason for each, and ends with 1 when any failed.

## oui_run's own tests

`tests/oui_run/run_tests.js PATH_TO_OUI_RUN` tests oui_run with oui_run. Most cases write a script as `main.js` in a
temporary folder, run the oui_run given on it, and compare what it printed and its exit code (`runScript`, and
`runScriptWith` to give it input). Others:

- `inTerminal` runs a script with a window under `script(1)` in a 40 by 10 terminal and looks at the escape sequences
  it wrote, to check colours.
- `pressing` runs a script in a 60 by 14 pseudo terminal through `place` from `tests/lib/terminal_session.js`, with
  keys, as the end-to-end tests do.
- `recipe` runs a script from `oui_run/recipes` in a folder filled with a fixture (Markdown notes, sources,
  a CSV, a JSON file, a binary file, photos), and returns what it printed, its code, and every file in the folder
  after, so a recipe that writes is checked by what it wrote. `calling` builds a driver that imports one of the
  `recipes/oui_code` scripts and calls it, as OUI Code would.

## Running one case

A GoogleTest case, through ctest or straight from its executable:

```sh
ctest --test-dir build -R 'FindBarTests' --output-on-failure
build/oui_code_tests --gtest_filter='OuiCodeTaskListTests.*'
```

A script suite takes the program to test and, second, part of a case's name; it runs from the repository:

```sh
bin/oui_run tests/oui_code/end_to_end.js bin/oui_code 'Calculator'
bin/oui_run tests/oui_calc/end_to_end.js bin/oui_calc
bin/oui_run tests/oui_run/run_tests.js bin/oui_run 'csv_summary'
```

The end-to-end tests run a copy of the program, never the one given, so a session of OUI Code open elsewhere is
left alone.

## House rules

The pack follows olagarro's conventions, in its
[CONTRIBUTING.md](https://github.com/iganinja/olagarro/blob/master/CONTRIBUTING.md), and the way the suites are written:

- Every bug fix includes a test that reproduces the bug: a test that fails without the fix and passes with it. Seeing
  it fail first is what shows it tests the bug; its comment says what used to go wrong.
- Tests follow the code conventions: C++ is formatted with `oui_run tools/format_cpp.js`, and warnings are errors.
- A name scripts can use has an `api` comment, or `OuiRun.ScriptApi` fails; after changing one, run
  `oui_run tools/extract_api.js` to remake `oui.d.ts` and `doc/oui_api.md`.
- A text a program shows goes through `translate` and into its catalog, or `TranslationTests` fails.
