#pragma once

#include "scripting.h"

#include <oui/translation.h>

#include <string_view>
#include <vector>

// What OUI Code carries inside it: its translations, its help, its own scripts and OUI's script API, which CMake
// turns into sources from oui_code and oui.d.ts.

// Defined in the source CMake generates from oui_code/translations.
std::vector<std::string_view> embedded_oui_code_catalogs();
// Defined in the source CMake generates from oui_code/help: English, then Spanish.
std::vector<std::string_view> embedded_oui_code_help();
std::vector<std::string_view> embedded_oui_code_help_names();
// Defined in the source CMake generates from oui_code/scripts, in the order built_in_scripts names.
std::vector<std::string_view> embedded_oui_code_scripts();
// oui.d.ts, what OUI's scripts can use, for the suggestions in scripts.
std::vector<std::string_view> embedded_oui_script_api();

// What user.js starts with: how to write a command and a language, ready to uncomment.
[[nodiscard]] std::string_view user_script_template();
// OUI Code's own scripts, which run before the user's: helpers, languages, text commands.
[[nodiscard]] std::vector<Scripting::Script> built_in_scripts();
[[nodiscard]] const std::vector<olagarro::oui::Translation>& oui_code_translations();
