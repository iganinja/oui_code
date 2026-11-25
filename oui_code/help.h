#pragma once

#include <oui/application.h>
#include <oui/command.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

// What the help is opened on: F1 opens it at the part about what is in the tab, as a calculator or an interactive
// script; on a document, at its start.
enum class HelpTopic
{
    general,
    searching,
    comparing,
    interactive_scripts,
    interactive_ui,
    tasks,
    terminal,
    calculator,
};

// The part of the help, from 0 as show_help counts them, about the topic.
[[nodiscard]] std::size_t help_section(HelpTopic topic);
// The titles of the parts of the help, as show_help lists them.
[[nodiscard]] std::vector<std::string> help_section_titles(const std::string& markdown);

// The help in the interface's language, ending with every command that has keys, read from the registry so
// that it never falls behind.
[[nodiscard]] std::string help_markdown(const std::string& language, const olagarro::oui::CommandRegistry& commands);
// A topic of the help, as its list shows it: a part ("## "), or a part within one ("### "), shown indented.
struct HelpTopicEntry
{
    std::string title;
    // 0 for a part, 1 for one within it.
    int depth{0};
    // The part it is in, from 0 as help_section_titles counts them.
    std::size_t section{};
    // For a part within one, its heading's name as a link says it, to show it at the top.
    std::string anchor;
    // What it says, up to the next topic, as the search looks in it.
    std::string text;
};
// Every topic of the help, in its order.
[[nodiscard]] std::vector<HelpTopicEntry> help_topics(const std::string& markdown);
// The topics that what is typed finds, best first, from 0 as `topics` counts them: every word of it, whatever the
// case or the accents, in a topic's title, or its letters in order there, as "shl" finds "A shell in a tab", or in
// what it says. Nothing typed finds every topic in its order.
[[nodiscard]] std::vector<std::size_t> find_help_topics(const std::vector<HelpTopicEntry>& topics,
                                                        std::string_view typed);

// The help in a window: its topics on the left, the one chosen on the right, and over them a field that finds
// topics by what they say, lighting it where they say it. Typing in the list goes to the field, Enter in either goes
// to the text. It opens at the part given.
void show_help(olagarro::oui::Application& application, const std::string& markdown, std::size_t section = 0);
