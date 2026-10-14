# OUI Code's documentation

How OUI Code, oui_run and oui_calc are built, tested and put together, and how to write scripts for oui_run, for
whoever changes the programs or writes scripts for them. Each document stands on its own and links to the others where
they meet; this page says which to read for what. In OUI Code, F9 shows this page as it reads: a link opens its
document in the same view, and Alt+Left goes back.

## Start here

- [Building](building.md): getting the sources with OUI as a submodule, CMake and its options, the terminal and window
  builds, the release binaries and how a release is published, Windows, and the tools that format the code and
  regenerate the script API.
- [Testing](testing.md): the unit tests, the programs' own checks, the end-to-end tests that drive the programs in a
  pseudo terminal, and how to run one case.

## Scripts

- [Scripting with oui_run](scripting.md): writing programs and tools in JavaScript, for script writers.
- [The script API](oui_api.md): every name scripts can use, with its signature; made from the sources.
- [oui_run's host](oui_run.md): how oui_run gives scripts their `oui`, the `api` comments the reference is made from,
  and how to add a call or a widget to oui_run.

## OUI Code

- [OUI Code's architecture](oui_code.md): how the editor is put together, for whoever changes it. Its user manual is
  [its help](../oui_code/help/help.md), which F1 opens.

## OUI and the conventions

The programs are made with OUI, from [olagarro](https://github.com/iganinja/olagarro), whose documents describe it:
[the overview](https://github.com/iganinja/olagarro/blob/master/doc/overview.md),
[layout and events](https://github.com/iganinja/olagarro/blob/master/doc/oui_layout.md),
[descriptions](https://github.com/iganinja/olagarro/blob/master/doc/oui_descriptions.md) (`.oui` files),
[rendering](https://github.com/iganinja/olagarro/blob/master/doc/oui_rendering.md),
[themes](https://github.com/iganinja/olagarro/blob/master/doc/themes.md),
[translations](https://github.com/iganinja/olagarro/blob/master/doc/translations.md) and
[OUIScript](https://github.com/iganinja/olagarro/blob/master/doc/ouiscript.md), the JavaScript engine for C++; and
[all of them](https://github.com/iganinja/olagarro/blob/master/doc/README.md).

The code follows olagarro's conventions, in its
[CONTRIBUTING.md](https://github.com/iganinja/olagarro/blob/master/CONTRIBUTING.md): naming, formatting, braces, no
C++ streams or `std::regex`, and a test for every bug fix.
