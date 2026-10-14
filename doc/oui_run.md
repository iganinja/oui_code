# oui_run's host

How oui_run is built on OUIScript: how its `Host` gives a script the `oui` object, the comments marked `api` from
which [oui.d.ts](../oui.d.ts) and [the API reference](oui_api.md) are made, and how to add a call or a widget to
oui_run. It is for whoever changes oui_run; writing scripts is the subject of [scripting](scripting.md). The engine
underneath, `ScriptEngine`, its values, promises and time limits, and `SystemApi`, the calls every scripting program
shares, are OUI's, described in [OUIScript](https://github.com/iganinja/olagarro/blob/master/doc/ouiscript.md).

- [How oui_run's host is built](#how-oui_runs-host-is-built)
- [The api comments and the generated reference](#the-api-comments-and-the-generated-reference)
- [Adding a call or a widget to oui_run](#adding-a-call-or-a-widget-to-oui_run)

## How oui_run's host is built

oui_run is three files and a script:

| File | What it does |
|------|--------------|
| [main.cpp](../oui_run/main.cpp) | reads the command line, makes the `Application` and the `Host`, runs frames while the host keeps running |
| [quiet_backend.cpp](../oui_run/quiet_backend.cpp) | the backend before a window: no screen, no input, sleeps until something wakes the loop |
| [host.cpp](../oui_run/host.cpp) | `Host`: the engine, `SystemApi`, the native `__oui` calls, the widgets, the window |
| [prelude.js](../oui_run/prelude.js) | the classes and functions scripts see as `oui`, built on `__oui` |

prelude.js is built into the program by `olagarro_embed_files` (as `embedded_oui_run_prelude()`), so oui_run is a
single file.

### Starting and ending

A script that only prints and runs commands is a terminal program like any other, so oui_run starts on
`QuietBackend`: the terminal is not taken over. `Host::open_window`, the first time a script calls `oui.window`,
switches the application to the terminal backend (or, with `--window` in the `oui_run_gui` build, a desktop window),
sets the theme unless the script chose one, and builds a frameless window of three `Part`s in a `DockLayout`: the menu
bar on top, the status bar at the bottom, the script's content between them.

`Host::run` runs the script as a module. Then `main` runs frames while `keeps_running()`: there is a window, or
`SystemApi::pending()` is above zero. `oui.quit()` ends it after the current work; `oui.exit(code)` throws
`ScriptStop`. While a window holds the terminal, what the script prints is kept and written once the terminal is given
back; an error is printed, and shown in a message box when there is a window.

The constructor sets the engine up in this order: time limit, print, unhandled rejections, `SystemApi`,
`define_everything()` for `__oui`, then prelude.js. Its members are declared so that the engine and `SystemApi` are
destroyed first: "what scripts hold dies before the widgets they reach, and the work they started before the engine its
callbacks run in."

### Native calls and JavaScript classes

`Host::define(name, ...)` defines `__oui.<name>`. These calls are plain and numeric: a widget is an integer id, and
each call does one thing, as `setChecked(id, value)`. prelude.js takes `__oui` into a local `n`, deletes it from the
global object, and builds the classes on it inside a block, so nothing it defines leaks into the script's scope:

```js
class CheckButton extends Widget {
    constructor(text, options = {}) {
        super(n.checkButton(String(text)));
        if (options.checked) this.checked = true;
        if (options.onToggle) this.onToggle = options.onToggle;
    }
    get checked() { return n.getChecked(this.id); }
    set checked(value) { n.setChecked(this.id, !!value); }
    set onToggle(fn) { n.on(this.id, "toggle", fn); }
}
```

The split puts each concern where it is easiest: argument conversion, defaults, options objects and getters and
setters in JavaScript; touching OUI's widgets in C++. At its end prelude.js freezes `globalThis.oui`, the system's calls
spread into it beside its own.

What C++ cannot ask a widget for, the class remembers: `TextEdit` and `Table` keep a private `#told` with the options
set on them, so their getters answer.

### Widget ids: adopt, take, widget

- `adopt(std::unique_ptr<Kind>)` gives a new widget the next id, records it in `m_widgets` (id to pointer) and keeps
  it in `m_loose` (made, not placed anywhere), which owns it.
- `take(id)` hands a loose widget over to whatever it goes in: a box, a grid, a tab, the window. A widget goes in one
  place only: taking it twice says "that widget is in place already".
- `widget<Kind>(id, "what")` finds a widget and checks its type with `dynamic_cast`; a wrong one throws
  `ScriptError{"not a " + what}`, which the script sees as a `TypeError`.
- `remove` takes a placed widget out of its parent and back into `m_loose`.

A description built with `oui.ui` gets its widgets numbered too: `Host::describe` adopts the root and records every
widget with an id, and returns their numbers and kinds (from `kind_of`, a chain of `dynamic_cast`s). prelude.js's
`existing(kind, id)` makes an object of the right class for each without making the widget again.

### Events through on()

Every event a script can hear goes through one native call, `__oui.on(id, event, fn)`, and `Host::on`:

- The function is stored in `m_handlers` under `(id, event)`. Only the first time is the widget's C++ signal
  connected, to a handler that looks the function up each time: setting `onClick` again replaces the function without
  stacking connections.
- The event name, and the widget's type where a name serves several, choose the signal: `"select"` is `on_selected`
  on a `ComboBox`, `on_selection_changed` on a `TabView` or a `Table`, and a path on a `TreeView`.
- Arguments go to the script as `Value`s; a function that throws is reported through `Host::call`.
- Where a handler must not run inside layout (a canvas's resize), it is posted.

`onKey` and context menus have native calls of their own (`onKey`, `contextMenu`), as they are not signals:
`set_key_handler`, and a right click seen by the application's event observer.

### Dialogs

`new oui.Dialog(title, content)` makes a `Window` and adopts it. `show()` calls `dialogShow`, which returns a promise
and posts the modal loop (`Application::show_modal_and_keep`), for the reason `later` gives: the script's call returns
first. When the dialog closes, it goes back to `m_loose`, whole, to be shown again, and the promise resolves with
whether it was accepted; the value `close(value)` gave is kept on the JavaScript side.

### Settings

`oui.settings.get/set/remove` keep values as JSON text, a key per line, in `<script>.ini` beside oui_run's own settings
in the user's configuration folder (`~/.config/oui_run` on Linux), saved as each is set.

## The api comments and the generated reference

Every name a script can use has a comment beside the code that defines it:

```cpp
// api(system): oui.readFile(path: string): string | null
// The file's text, or null when it cannot be read.
```

The first line says which scripts have it and declares it as TypeScript writes it; the comment lines right under it
describe it. A blank line or any other line ends the entry.

| Where | Which scripts |
|-------|---------------|
| `everywhere` | every script (`console.log`) |
| `system` | oui_run, and OUI Code's Run Script and Interactive Script (`SystemApi`) |
| `oui_run` | oui_run alone (prelude.js) |
| `oui_code` | OUI Code's language and command scripts (`oui.languages.register`, `oui.commands.register`) |
| `colouring` | both of those (`oui.words`) |

The declaration is one of:

| Form | Example |
|------|---------|
| a function | `oui.readFile(path: string): string \| null` |
| a value | `oui.args: string[]` |
| a class | `class oui.Label extends oui.Widget` |
| a constructor | `new oui.Label(text?: string, options?: {bold?: boolean, wrap?: boolean})` |
| a member | `oui.Label#text: string`, `oui.TextEdit#append(text: string): void` |

A class must be declared before its constructor and members. The same name may be declared twice, for two forms of a
function, as `oui.window(title, content)` and `oui.window(ui)`.

[tools/extract_api.js](../tools/extract_api.js) reads every `.cpp`, `.h` and `.js` under `third_party/olagarro/oui`,
OUI's sources, and under `oui_calc`, `oui_code` and `oui_run`, and writes two files from the comments:

- [oui.d.ts](../oui.d.ts): TypeScript declarations, a namespace per dotted name, for editors. OUI Code embeds it and
  reads it (OUI's `ScriptApi`, in
  [completion.h](https://github.com/iganinja/olagarro/blob/master/oui/include/oui/completion.h)) for its suggestions and
  the signature shown while one writes a call.
- [doc/oui_api.md](oui_api.md): the reference, every name under its namespace, with which scripts have it.

Both are generated: they are never edited by hand.

```sh
bin/oui_run tools/extract_api.js            # writes both, from the repository's top
bin/oui_run tools/extract_api.js --check    # writes nothing; fails when they are out of date
```

`--check` also compares the declarations with oui_run's live `oui` object: a name on `oui` without a comment, or a
comment declaring a top-level `oui.x` that `oui` lacks, fails. The CTest test `OuiRun.ScriptApi` runs it with the
oui_run just built, so a change that forgets a comment, or forgets to regenerate, fails the build's tests. A new
version of OUI whose `api` comments changed, as one with a new `SystemApi` call, is the same: once the submodule points
at it, the files are made again and committed with it.

## Adding a call or a widget to oui_run

The steps, with `CheckButton` as the model, as it has a constructor, a property and an event.

**1. Decide where it goes.** A call every scripting program should have (a file operation, a question) goes in OUI's
`SystemApi` ([system_api.cpp](https://github.com/iganinja/olagarro/blob/master/oui/src/script/system_api.cpp)): a
`define` in `define_everything`, a wrapper in its prelude, and, when it writes, runs or asks, its name in the
`read_only` list. That is a change to olagarro, which reaches here when the submodule moves to it. A call only oui_run
has, which is anything about widgets and windows, goes in `Host`. The rest of these steps are for `Host`.

**2. Define the native calls** in `Host::define_everything`, one small call per operation, ids as numbers:

```cpp
define("checkButton",
       [this](const Arguments& a) { return m_engine.number(adopt(oui::check_button(text_of(argument(a, 0))))); });
define("getChecked", [this](const Arguments& a)
       { return m_engine.boolean(widget<oui::CheckButton>(id_of(a, 0), "check button").is_checked()); });
define("setChecked",
       [this](const Arguments& a)
       {
           widget<oui::CheckButton>(id_of(a, 0), "check button").set_checked(argument(a, 1).to_bool());
           return Value{};
       });
```

Use `argument(a, n)` rather than `a[n]` (a missing argument is `undefined`), `text_of` for strings (`null` and
`undefined` become empty), and throw `ScriptError` with a message a script writer understands. A call that waits on
the user returns a promise and posts its work, as `dialogShow` does.

**3. Wire the events** in `Host::on`: a branch for the event name, and the widget's type when the name is shared,
connecting the C++ signal to `handler` with the arguments as `Value`s:

```cpp
else if(event == "toggle")
{
    widget<oui::CheckButton>(id, "check button")
        .on_toggled([this, handler](bool checked) { handler({m_engine.boolean(checked)}); });
}
```

**4. Write the class or function in prelude.js**, inside its block, converting arguments (`String`, `Number`, `!!`)
before they reach C++; then add it to the frozen `oui` object at the end. A widget that a description can make also
goes in `kinds` there, and gets its name in `kind_of` in host.cpp, so `ui.someId` is an object of its class.

**5. Write the api comments** at the top of prelude.js, with the others: the class, its constructor, each member. The
text is what the reference and OUI Code's signature tips show, so write it for the script's author.

```js
// api(oui_run): class oui.CheckButton extends oui.Widget
// A box to tick, with its text beside it.
// api(oui_run): new oui.CheckButton(text: string, options?: {checked?: boolean, onToggle?: (checked: boolean) => void})
// api(oui_run): oui.CheckButton#checked: boolean
// Whether it is ticked.
// api(oui_run): oui.CheckButton#onToggle: (checked: boolean) => void
// Called when it is ticked or unticked.
```

**6. Test it** in [tests/oui_run/run_tests.js](../tests/oui_run/run_tests.js): `runScript` writes a script in a
temporary folder, runs the oui_run under test on it, and gives what it printed and its exit code. Widgets work without
a window, so most tests build them and print what they hold:

```js
test("a check button is ticked from code, and says so", async () => {
    const {code, output} = await runScript(
        `const box = new oui.CheckButton("Loud", {checked: true});\n` +
        `box.checked = !box.checked;\n` +
        `console.log(box.checked);\n`);
    expect("code", code, 0);
    expect("output", output, "false\n");
});
```

`inTerminal(script)` runs a script with its window in a pseudo-terminal 40 columns by 10 rows and gives what it wrote,
escape sequences included, for what only shows on screen. Run the suite with `bin/oui_run
tests/oui_run/run_tests.js bin/oui_run`, and add a second argument to run the cases whose name contains it.

**7. Regenerate** `oui.d.ts` and `doc/oui_api.md` with `bin/oui_run tools/extract_api.js`, and commit them with the
change. If the host shows a new text to the user through `translate`, add it to `oui_run/translations/es.json`: a test
checks that every catalog covers every text of the sources (see
[translations](https://github.com/iganinja/olagarro/blob/master/doc/translations.md)). Then build and run `ctest`, which
includes `OuiRun.ScriptApi` and `OuiRun.Scripts`; see [testing](testing.md).
