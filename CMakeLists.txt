cmake_minimum_required(VERSION 3.20)

project(OuiCode VERSION 0.4.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Editors (VS Code, clangd) read build/compile_commands.json for include paths and flags.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(OUI_CODE_BUILD_TESTS "Build the test suite of OUI Code, oui_run and oui_calc" ON)
# OUI's own options choose what the programs can show on: OLAGARRO_OUI_WINDOW a desktop window, OLAGARRO_OUI_DRM the
# whole screen with no display server, and OLAGARRO_STATIC_LINK links them fully statically. They are set here, as
# OUI's, so OUI builds what the programs need.
option(OLAGARRO_OUI_WINDOW
    "Build the programs able to open in a desktop window, on SDL3 linked statically, named apart: oui_code_gui" OFF)
option(OLAGARRO_OUI_DRM
    "Let the programs take the whole screen through Linux's mode setting (--drm), with no display server" OFF)
option(OLAGARRO_STATIC_LINK
    "Link the programs fully statically, so they run on Linux systems with an older C and C++ runtime" OFF)
set(OUI_CODE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin" CACHE PATH "Where the programs are placed")

# OUI, without its own tests and examples.
set(OLAGARRO_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(OLAGARRO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/olagarro EXCLUDE_FROM_ALL)

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()
if(OLAGARRO_STATIC_LINK AND NOT MSVC)
    # A static binary carries the whole runtime; drop every function and object nobody references.
    add_compile_options(-ffunction-sections -fdata-sections)
    add_link_options(-Wl,--gc-sections)
    # Not position-independent either, on Linux: a program loaded at a fixed address needs no table of addresses to
    # fix up. A Windows program is not ELF, and the flags mean nothing to it.
    if(NOT WIN32)
        add_compile_options(-fno-pie)
        add_link_options(-no-pie)
    endif()
endif()

# The version OUI Code shows: a release's, from its tag, or the project's.
set(OUI_CODE_VERSION "${PROJECT_VERSION}" CACHE STRING "The version OUI Code shows")
add_compile_definitions(OUI_CODE_VERSION="${OUI_CODE_VERSION}")

# OUI Code's windows and panels, as user interface descriptions, in the order OuiCodeUi in ui_descriptions.h names
# them; its tests build the panels too.
set(OUI_CODE_UI
    oui_code/ui/find_in_files.oui
    oui_code/ui/preferences.oui
    oui_code/ui/replace.oui
    oui_code/ui/filter_running.oui
    oui_code/ui/about.oui
    oui_code/ui/markdown.oui
    oui_code/ui/help.oui
    oui_code/ui/find_bar.oui
    oui_code/ui/tasks.oui
    oui_code/ui/script_output.oui
    oui_code/ui/interactive.oui
    oui_code/ui/main_window.oui
    oui_code/ui/key_test.oui
    oui_code/ui/interactive_ui.oui)
add_executable(oui_code
    oui_code/main.cpp
    oui_code/dialogs.cpp
    oui_code/edit_commands.cpp
    oui_code/documents.cpp
    oui_code/editors.cpp
    oui_code/event_log.cpp
    oui_code/file_commands.cpp
    oui_code/find_bar.cpp
    oui_code/help.cpp
    oui_code/ignore_rules.cpp
    oui_code/interactive_view.cpp
    oui_code/languages.cpp
    oui_code/live_script.cpp
    oui_code/navigation.cpp
    oui_code/panel_commands.cpp
    oui_code/panels.cpp
    oui_code/preferences.cpp
    oui_code/project_search.cpp
    oui_code/recovery.cpp
    oui_code/resources.cpp
    oui_code/ui_descriptions.cpp
    oui_code/ui_preview.cpp
    oui_code/ui_skeleton.cpp
    oui_code/run_script.cpp
    oui_code/comparison.cpp
    oui_code/session_entry.cpp
    oui_code/script_run.cpp
    oui_code/scripting.cpp
    oui_code/search_commands.cpp
    oui_code/self_tests.cpp
    oui_code/session.cpp
    oui_code/task_list.cpp
    oui_code/tools_commands.cpp
    oui_code/view_commands.cpp
    oui_code/workbench.cpp)
target_link_libraries(oui_code PRIVATE olagarro::OUI olagarro::OUIScript)
# The scripts OUI Code runs before the user's, in this order; resources.cpp names them in the same order.
olagarro_embed_files(oui_code FUNCTION embedded_oui_code_scripts
    FILES
        oui_code/scripts/prelude.js
        oui_code/scripts/languages/cpp.js
        oui_code/scripts/languages/javascript.js
        oui_code/scripts/languages/python.js
        oui_code/scripts/languages/gdscript.js
        oui_code/scripts/languages/json.js
        oui_code/scripts/languages/markdown.js
        oui_code/scripts/languages/cmake.js
        oui_code/scripts/languages/shell.js
        oui_code/scripts/languages/oui.js
        oui_code/scripts/languages/others.js
        oui_code/scripts/commands.js)
olagarro_embed_files(oui_code FUNCTION embedded_oui_code_catalogs FILES oui_code/translations/es.json)
olagarro_embed_files(oui_code FUNCTION embedded_oui_script_api FILES oui.d.ts)
olagarro_embed_files(oui_code FUNCTION embedded_oui_code_ui FILES ${OUI_CODE_UI})
olagarro_embed_files(oui_code FUNCTION embedded_oui_code_help FILES oui_code/help/help.md oui_code/help/help.es.md)
target_compile_definitions(oui_code PRIVATE $<$<CONFIG:Debug>:OUI_CODE_DEBUG_BUILD>)

# oui_calc: a calculator as a prompt, in the terminal, a desktop window or the whole screen.
add_executable(oui_calc oui_calc/main.cpp)
target_link_libraries(oui_calc PRIVATE olagarro::OUI)
olagarro_embed_files(oui_calc FUNCTION embedded_oui_calc_catalogs FILES oui_calc/translations/es.json)

# oui_run: a JavaScript file run as a terminal application.
add_executable(oui_run oui_run/main.cpp oui_run/host.cpp oui_run/quiet_backend.cpp)
target_link_libraries(oui_run PRIVATE olagarro::OUI olagarro::OUIScript)
olagarro_embed_files(oui_run FUNCTION embedded_oui_run_catalogs FILES oui_run/translations/es.json)
olagarro_embed_files(oui_run FUNCTION embedded_oui_run_prelude FILES oui_run/prelude.js)

set(OUI_CODE_PROGRAMS oui_code oui_calc oui_run)
foreach(program IN LISTS OUI_CODE_PROGRAMS)
    string(TOUPPER ${program} macro)
    # With the window module each can open a desktop window too, and is named apart, oui_code_gui, so the two builds
    # sit side by side and share their settings.
    if(OLAGARRO_OUI_WINDOW)
        target_link_libraries(${program} PRIVATE olagarro::OUIWindow)
        target_compile_definitions(${program} PRIVATE ${macro}_WINDOW)
        set(name ${program}_gui)
    else()
        set(name ${program})
    endif()
    set_target_properties(${program} PROPERTIES OUTPUT_NAME ${name}
        RUNTIME_OUTPUT_DIRECTORY ${OUI_CODE_RUNTIME_OUTPUT_DIRECTORY})
endforeach()
foreach(program oui_code oui_calc)
    string(TOUPPER ${program} macro)
    if(OLAGARRO_OUI_DRM)
        target_link_libraries(${program} PRIVATE olagarro::OUIGraphics)
        target_compile_definitions(${program} PRIVATE ${macro}_DRM)
    endif()
    # On Linux they show on OUI's own desktop, oui_desktop, with --desktop: in characters always, and in pixels too
    # when they are built with the graphics, as the desktop is.
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_compile_definitions(${program} PRIVATE ${macro}_DESKTOP)
        if(OLAGARRO_OUI_DRM OR OLAGARRO_OUI_WINDOW)
            target_link_libraries(${program} PRIVATE olagarro::OUIGraphics)
            target_compile_definitions(${program} PRIVATE ${macro}_DESKTOP_PIXELS)
        endif()
    endif()
endforeach()
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    foreach(program IN LISTS OUI_CODE_PROGRAMS)
        target_link_options(${program} PRIVATE -static -static-libgcc -static-libstdc++)
    endforeach()
elseif(OLAGARRO_STATIC_LINK)
    foreach(program IN LISTS OUI_CODE_PROGRAMS)
        if(OLAGARRO_OUI_WINDOW)
            # A window needs the system's display libraries, which SDL3 loads when it starts: that takes the system's
            # C library, so only the C++ runtime goes inside. Build it on an old system to run on many.
            target_link_options(${program} PRIVATE -static-libgcc -static-libstdc++)
        else()
            # A static libstdc++ alone is not enough: it still asks for the build machine's glibc version.
            target_link_options(${program} PRIVATE -static)
        endif()
    endforeach()
endif()

if(OUI_CODE_BUILD_TESTS)
    enable_testing()
    find_package(GTest CONFIG REQUIRED)
    include(GoogleTest)
    # GoogleTest's warnings are not ours to fix. Its targets say its headers are the system's, but CMake drops a
    # folder the compiler searches anyway, as Homebrew's /usr/local/include on an Intel Mac, where the compiler then
    # warns in them as in our own. /usr/include stays as it is: it is the system's already, and marked again it comes
    # before the C++ library's own headers.
    get_target_property(oui_code_gtest_includes GTest::gtest INTERFACE_INCLUDE_DIRECTORIES)
    foreach(oui_code_gtest_include IN LISTS oui_code_gtest_includes)
        if(NOT oui_code_gtest_include STREQUAL "/usr/include")
            set_property(TARGET GTest::gtest APPEND PROPERTY INTERFACE_COMPILE_OPTIONS
                "SHELL:-isystem ${oui_code_gtest_include}")
        endif()
    endforeach()

    add_executable(oui_code_tests
        tests/oui_code/ignore_rules_test.cpp
        tests/oui_code/scripting_test.cpp
        oui_code/ignore_rules.cpp
        oui_code/find_bar.cpp
        oui_code/languages.cpp
        oui_code/live_script.cpp
        oui_code/script_run.cpp
        oui_code/scripting.cpp
        oui_code/task_list.cpp
        oui_code/ui_descriptions.cpp
        oui_code/interactive_view.cpp
        oui_code/ui_preview.cpp
        tests/oui_code/ui_preview_test.cpp
        oui_code/ui_skeleton.cpp
        tests/oui_code/ui_skeleton_test.cpp
        tests/oui_code/generated_window.cpp
        tests/oui_code/find_bar_test.cpp
        tests/oui_code/live_script_test.cpp
        tests/oui_code/script_run_test.cpp
        tests/oui_code/task_list_test.cpp
        oui_code/help.cpp
        tests/oui_code/help_test.cpp
        oui_code/session_entry.cpp
        tests/oui_code/session_entry_test.cpp
        tests/translation_test.cpp)
    target_include_directories(oui_code_tests PRIVATE oui_code)
    olagarro_embed_files(oui_code_tests FUNCTION embedded_oui_code_help
        FILES oui_code/help/help.md oui_code/help/help.es.md)
    olagarro_embed_files(oui_code_tests FUNCTION embedded_oui_code_ui FILES ${OUI_CODE_UI})
    # As the C++ Generate writes asks, for the code it wrote for this description, which the tests build.
    olagarro_embed_files(oui_code_tests FUNCTION embedded_rename_files_ui FILES tests/oui_code/rename_files.oui)
    target_link_libraries(oui_code_tests PRIVATE olagarro::OUIScript GTest::gtest_main)
    # The built-in scripts, to check they all load and colour as they should, and the sources and catalogs, to check
    # the catalogs cover every text.
    target_compile_definitions(oui_code_tests PRIVATE OUI_CODE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    gtest_discover_tests(oui_code_tests)

    add_test(NAME OuiCode.Smoke COMMAND oui_code --smoke-test)
    # The options as they are typed: "--help", never "-- help", which a formatter once made of them.
    add_test(NAME OuiCode.Help COMMAND oui_code --help)
    set_tests_properties(OuiCode.Help PROPERTIES PASS_REGULAR_EXPRESSION "  --help " FAIL_REGULAR_EXPRESSION "-- ")
    add_test(NAME OuiCode.Stress COMMAND oui_code --stress-test)
    set_tests_properties(OuiCode.Stress PROPERTIES TIMEOUT 30)
    add_test(NAME OuiCode.Input COMMAND oui_code --input-test)
    set_tests_properties(OuiCode.Input PROPERTIES TIMEOUT 30 PASS_REGULAR_EXPRESSION
        "INPUT_TEST_RESULT aâeêiîoôuûaáeéiíoóuúaàeèiìoòuùaäeëiïoöuüaãeẽiĩoõuũAÂEÊIÎOÔUÛ\\^ ñçüöêêêê\n")
    # oui.d.ts and doc/oui_api.md are what the comments marked "api" make, and oui_run's own `oui` has every name
    # they declare.
    add_test(NAME OuiRun.ScriptApi COMMAND oui_run tools/extract_api.js --check
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    # oui_run's own behaviour and the recipes in oui_run/recipes, each run on files made for it. They run shell
    # commands (mktemp, mv, printf), so not on Windows.
    if(NOT WIN32)
        # In three parts that run side by side, every third case each: the whole takes some three minutes.
        foreach(part 1 2 3)
            add_test(NAME OuiRun.Scripts.${part}
                COMMAND oui_run tests/oui_run/run_tests.js $<TARGET_FILE:oui_run> "part ${part} of 3"
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
            set_tests_properties(OuiRun.Scripts.${part} PROPERTIES TIMEOUT 300)
        endforeach()
        # OUI Code as a person uses it, in a pseudo terminal, from a copy of its own beside which it keeps its settings.
        # Each case runs OUI Code for a few seconds of keys: they take about four minutes, more on a slower machine, so
        # they run as four parts side by side, every fourth case each.
        foreach(part 1 2 3 4)
            add_test(NAME OuiCode.EndToEnd.${part}
                COMMAND oui_run tests/oui_code/end_to_end.js $<TARGET_FILE:oui_code> "part ${part} of 4"
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
            set_tests_properties(OuiCode.EndToEnd.${part} PROPERTIES TIMEOUT 300)
        endforeach()
        add_test(NAME OuiCalc.EndToEnd
            COMMAND oui_run tests/oui_calc/end_to_end.js $<TARGET_FILE:oui_calc>
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        set_tests_properties(OuiCalc.EndToEnd PROPERTIES TIMEOUT 120)
    endif()
endif()
