#include "help.h"

#include "resources.h"
#include "ui_descriptions.h"

#include <oui/oui.h>

#include <algorithm>
#include <cctype>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

using namespace olagarro;

namespace
{

// Markdown cut at its second-level headings ("## "), outside code blocks: each section with its title, the
// text before the first heading under `first_title`.
[[nodiscard]] std::vector<std::pair<std::string, std::string>> markdown_sections(std::string_view markdown,
                                                                                 const std::string& first_title)
{
    std::vector<std::pair<std::string, std::string>> sections{{first_title, {}}};
    auto in_code{false};
    for(std::size_t start{}; start < markdown.size();)
    {
        const auto end{std::min(markdown.find('\n', start), markdown.size())};
        const auto line{markdown.substr(start, end - start)};
        if(line.substr(0, 3) == "```")
        {
            in_code = !in_code;
        }
        if(!in_code && line.substr(0, 3) == "## ")
        {
            sections.emplace_back(std::string{line.substr(3)}, std::string{});
        }
        sections.back().second.append(line).append("\n");
        start = end + 1;
    }
    return sections;
}

// A heading's name as a link says it, as the view names them: GitHub's way, letters in lower case, spaces as hyphens,
// and of the rest of ASCII only digits, hyphens and underscores, the marks of code and emphasis gone.
[[nodiscard]] std::string anchor_of(std::string_view title)
{
    std::string anchor;
    for(const auto c : title)
    {
        const auto byte{static_cast<unsigned char>(c)};
        if(byte >= 0x80 || std::isalnum(byte) != 0 || c == '-' || c == '_')
        {
            anchor += byte < 0x80 ? static_cast<char>(std::tolower(byte)) : c;
        }
        else if(c == ' ')
        {
            anchor += '-';
        }
    }
    return anchor;
}

// Text to compare as loosely as a search wants: in lower case, and the accents of Spanish and its ñ gone, so that
// "busqueda" finds "Búsqueda".
[[nodiscard]] std::string folded(std::string_view text)
{
    static const std::vector<std::pair<std::string_view, char>> accented{
        {"á", 'a'}, {"é", 'e'}, {"í", 'i'}, {"ó", 'o'}, {"ú", 'u'}, {"ü", 'u'}, {"ñ", 'n'},
        {"Á", 'a'}, {"É", 'e'}, {"Í", 'i'}, {"Ó", 'o'}, {"Ú", 'u'}, {"Ü", 'u'}, {"Ñ", 'n'}};
    std::string result;
    result.reserve(text.size());
    for(std::size_t at{}; at < text.size();)
    {
        auto replaced{false};
        if(static_cast<unsigned char>(text[at]) >= 0x80)
        {
            for(const auto& [from, to] : accented)
            {
                if(text.substr(at, from.size()) == from)
                {
                    result += to;
                    at += from.size();
                    replaced = true;
                    break;
                }
            }
        }
        if(!replaced)
        {
            result += static_cast<char>(std::tolower(static_cast<unsigned char>(text[at])));
            ++ at;
        }
    }
    return result;
}

// Whether the letters of `word` are in `text` in that order, others between them.
[[nodiscard]] bool in_order(std::string_view word, std::string_view text)
{
    std::size_t at{};
    for(const auto c : word)
    {
        at = text.find(c, at);
        if(at == std::string_view::npos)
        {
            return false;
        }
        ++ at;
    }
    return true;
}

[[nodiscard]] std::vector<std::string> words_of(std::string_view text)
{
    std::vector<std::string> words;
    std::string word;
    for(const auto c : text)
    {
        if(c == ' ' || c == '\t')
        {
            if(!word.empty())
            {
                words.push_back(std::move(word));
                word.clear();
            }
            continue;
        }
        word += c;
    }
    if(!word.empty())
    {
        words.push_back(std::move(word));
    }
    return words;
}

// The topics' list: typing in it, but for what moves in it, goes on in the search over it.
class TopicList final : public oui::List
{
  public:
    explicit TopicList(std::function<void(const oui::Event&)> search) : m_search{std::move(search)} {}

    bool handle_event(const oui::Event& event) override
    {
        if(const auto* const key{std::get_if<oui::KeyEvent>(&event)};
           key != nullptr && key->key == oui::Key::backspace && !key->control && !key->alt)
        {
            m_search(event);
            return true;
        }
        if(const auto* const text{std::get_if<oui::TextEvent>(&event)};
           text != nullptr && !text->control && !text->alt && text->codepoint >= U' ')
        {
            m_search(event);
            return true;
        }
        return List::handle_event(event);
    }

  private:
    std::function<void(const oui::Event&)> m_search;
};

} // namespace

std::vector<HelpTopicEntry> help_topics(const std::string& markdown)
{
    std::vector<HelpTopicEntry> topics;
    const auto sections{markdown_sections(markdown, "OUI Code")};
    for(std::size_t section{}; section < sections.size(); ++ section)
    {
        const auto& [title, text] = sections[section];
        topics.push_back({title, 0, section, {}, {}});
        auto in_code{false};
        const std::string_view whole{text};
        for(std::size_t start{}; start < whole.size();)
        {
            const auto end{std::min(whole.find('\n', start), whole.size())};
            const auto line{whole.substr(start, end - start)};
            start = end + 1;
            if(line.substr(0, 3) == "```")
            {
                in_code = !in_code;
            }
            if(!in_code && line.substr(0, 4) == "### ")
            {
                std::string name;
                for(const auto c : line.substr(4))
                {
                    if(c != '`' && c != '*')
                    {
                        name += c;
                    }
                }
                topics.push_back({name, 1, section, anchor_of(name), {}});
                continue;
            }
            topics.back().text.append(line).append("\n");
        }
    }
    return topics;
}

std::vector<std::size_t> find_help_topics(const std::vector<HelpTopicEntry>& topics, std::string_view typed)
{
    std::vector<std::string> words;
    for(auto& word : words_of(typed))
    {
        words.push_back(folded(word));
    }
    std::vector<std::pair<int, std::size_t>> found;
    for(std::size_t index{}; index < topics.size(); ++ index)
    {
        const auto title{folded(topics[index].title)};
        const auto text{folded(topics[index].text)};
        auto score{0};
        for(const auto& word : words)
        {
            const auto points{title.find(word) != std::string::npos ? 4
                              : in_order(word, title)               ? 2
                              : text.find(word) != std::string::npos ? 1
                                                                     : 0};
            if(points == 0)
            {
                score = -1;
                break;
            }
            score += points;
        }
        if(score >= 0)
        {
            found.emplace_back(score, index);
        }
    }
    std::stable_sort(found.begin(), found.end(),
                     [](const auto& first, const auto& second) { return first.first > second.first; });
    std::vector<std::size_t> indices;
    for(const auto& [score, index] : found)
    {
        indices.push_back(index);
    }
    return indices;
}

std::vector<std::string> help_section_titles(const std::string& markdown)
{
    std::vector<std::string> titles;
    for(auto& [title, text] : markdown_sections(markdown, "OUI Code"))
    {
        titles.push_back(std::move(title));
    }
    return titles;
}

std::size_t help_section(HelpTopic topic)
{
    // Found by their titles in the English help, whose parts the Spanish one has in the same order; the calculator's
    // comes after them.
    const auto english{help_section_titles(std::string{embedded_oui_code_help().at(0)})};
    if(topic == HelpTopic::calculator)
    {
        return english.size();
    }
    const auto* const title{topic == HelpTopic::searching             ? "Searching"
                            : topic == HelpTopic::comparing           ? "Comparing files"
                            : topic == HelpTopic::interactive_scripts ? "Interactive scripts"
                            : topic == HelpTopic::interactive_ui      ? "Interactive user interfaces"
                            : topic == HelpTopic::tasks               ? "Things to do"
                            : topic == HelpTopic::terminal            ? "A shell in a tab"
                                                                      : "OUI Code"};
    const auto found{std::find(english.begin(), english.end(), title)};
    return found == english.end() ? 0 : static_cast<std::size_t>(std::distance(english.begin(), found));
}

std::string help_markdown(const std::string& language, const oui::CommandRegistry& commands)
{
    std::string markdown{oui::text_for_language(embedded_oui_code_help(), embedded_oui_code_help_names(), language)};
    auto calculator{oui::calculator_help()};
    for(std::size_t at{calculator.find("\n## ")}; at != std::string::npos; at = calculator.find("\n## ", at + 4))
    {
        calculator.insert(at + 1, "#");
    }
    if(calculator.rfind("# ", 0) == 0)
    {
        calculator.insert(0, "#");
    }
    // Where it is, under its title.
    const auto title_end{calculator.find('\n')};
    calculator.insert(title_end + 1, "\n" + oui::translate("Help", "**Tools > Calculator** opens it in a tab.") + "\n");
    markdown += "\n" + calculator;
    markdown += "\n## " + oui::translate("Help", "All commands with keys") + "\n\n| " + oui::translate("Help", "Keys") +
                " | " + oui::translate("Help", "Command") + " |\n|---|---|\n";
    auto listed{commands.commands()};
    std::sort(listed.begin(), listed.end(), [](const oui::CommandInfo& left, const oui::CommandInfo& right)
              { return oui::strip_mnemonic(left.title) < oui::strip_mnemonic(right.title); });
    for(const auto& command : listed)
    {
        if(command.shortcut)
        {
            markdown +=
                "| " + oui::format_shortcut(*command.shortcut) + " | " + oui::strip_mnemonic(command.title) + " |\n";
        }
    }
    return markdown;
}

void show_help(oui::Application& application, const std::string& markdown, std::size_t section)
{
    const auto sections{markdown_sections(markdown, "OUI Code")};
    const auto topics{help_topics(markdown)};
    auto ui{build_oui_code_ui(OuiCodeUi::help)};
    auto& search{ui.get<oui::LineEdit>("search")};
    auto& page{ui.put("page", oui::markdown_view())};
    // Apart from the documents, as a manual page.
    page.set_background(&oui::Theme::reading_background);
    page.set_foreground(&oui::Theme::reading_foreground);
    auto& list{ui.put("topics", std::make_unique<TopicList>(
                                     [&application, &search](const oui::Event& event)
                                     {
                                         application.focus(search);
                                         static_cast<void>(search.handle_event(event));
                                     }))};
    // Which topic each row is, as the search leaves them, and the part the page shows.
    std::vector<std::size_t> rows;
    std::optional<std::size_t> shown_section;
    oui::SearchOptions loose;
    loose.match_case = false;
    const auto show{[&](std::size_t row)
                    {
                        if(row >= rows.size())
                        {
                            return;
                        }
                        const auto& topic{topics.at(rows.at(row))};
                        if(shown_section != topic.section)
                        {
                            page.set_markdown(sections.at(topic.section).second);
                            shown_section = topic.section;
                        }
                        page.scroll_to(0);
                        if(!topic.anchor.empty())
                        {
                            static_cast<void>(page.scroll_to_anchor(topic.anchor));
                        }
                        // What was looked for lights up where it is, the first of it after the topic's start in
                        // view.
                        const auto words{words_of(search.text())};
                        if(words.empty())
                        {
                            page.clear_search_highlight();
                            return;
                        }
                        page.set_search_highlight(words.front(), loose);
                        // Found by its title, it is where it starts; found by what it says, where it says it.
                        if(folded(topic.title).find(folded(words.front())) == std::string::npos)
                        {
                            static_cast<void>(page.find_next(words.front(), loose));
                        }
                    }};
    const auto fill{[&](std::optional<std::size_t> keep)
                    {
                        // In the manual's order, so that a part within one shows under it, the best found chosen.
                        const auto found{find_help_topics(topics, search.text())};
                        rows = found;
                        std::sort(rows.begin(), rows.end());
                        if(!keep && !found.empty())
                        {
                            keep = found.front();
                        }
                        list.clear();
                        for(const auto index : rows)
                        {
                            const auto& topic{topics.at(index)};
                            list.add_line({(topic.depth > 0 ? "  " : "") + topic.title});
                        }
                        if(rows.empty())
                        {
                            return;
                        }
                        auto row{std::size_t{}};
                        if(keep)
                        {
                            const auto at{std::find(rows.begin(), rows.end(), *keep)};
                            row = at == rows.end() ? 0 : static_cast<std::size_t>(std::distance(rows.begin(), at));
                        }
                        list.select_row(row);
                        show(row);
                    }};
    const auto selected{list.on_selection_changed([&show](std::size_t row) { show(row); })};
    const auto activated{list.on_row_activated([&application, &page](std::size_t) { application.focus(page); })};
    const auto typed{search.on_changed([&fill](std::string_view) { fill(std::nullopt); })};
    const auto submitted{search.on_submitted([&application, &page](std::string_view) { application.focus(page); })};
    // The part asked for, where its topic is in the list.
    std::optional<std::size_t> first;
    for(std::size_t index{}; index < topics.size(); ++ index)
    {
        if(topics[index].depth == 0 && topics[index].section == std::min(section, sections.size() - 1))
        {
            first = index;
            break;
        }
    }
    fill(first);
    application.post([&application, &list] { application.focus(list); });
    static_cast<void>(application.show_modal(ui.take_window()));
}
