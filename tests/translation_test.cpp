#include <oui/declarative.h>
#include <oui/ui.h>
#include <oui/command.h>
#include <oui/translation.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{

using namespace olagarro;
using namespace olagarro::oui;

// Catalogs are kept by hand, so these checks stand in for a tool: every text the sources pass to
// translate() must be in every catalog of that part of the repository, translated, and nothing else.

struct SourceText
{
    std::string context;
    std::string text;
    std::string where;
};

std::string read_file(const std::filesystem::path& path)
{
    std::ifstream file{path, std::ios::binary};
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

// The source with comments blanked out, string literals kept.
std::string without_comments(const std::string& source)
{
    std::string result{source};
    for(std::size_t i{}; i < result.size(); ++ i)
    {
        if(result[i] == '"' || result[i] == '\'')
        {
            const auto quote{result[i]};
            for(++ i; i < result.size() && result[i] != quote; ++ i)
            {
                if(result[i] == '\\')
                {
                    ++ i;
                }
            }
        }
        else if(result.compare(i, 2, "//") == 0)
        {
            for(; i < result.size() && result[i] != '\n'; ++ i)
            {
                result[i] = ' ';
            }
        }
        else if(result.compare(i, 2, "/*") == 0)
        {
            const auto end{result.find("*/", i + 2)};
            const auto stop{end == std::string::npos ? result.size() : end + 2};
            for(; i < stop; ++ i)
            {
                result[i] = result[i] == '\n' ? '\n' : ' ';
            }
            -- i;
        }
    }
    return result;
}

std::string unescape_literals(const std::string& literals)
{
    std::string result;
    bool inside{false};
    for(std::size_t i{}; i < literals.size(); ++ i)
    {
        const auto c{literals[i]};
        if(c == '"')
        {
            inside = !inside;
        }
        else if(inside && c == '\\' && i + 1 < literals.size())
        {
            const auto escaped{literals[++ i]};
            result += escaped == 'n' ? '\n' : escaped == 't' ? '\t' : escaped;
        }
        else if(inside)
        {
            result += c;
        }
    }
    return result;
}

// The texts passed to translate() under `folders`. A call whose arguments are not string literals is
// reported in `problems`, since no catalog check could see its text.
std::vector<SourceText> source_texts(const std::vector<std::filesystem::path>& folders,
                                     std::vector<std::string>& problems)
{
    static const std::regex call{R"re(\btranslate\s*\()re"};
    static const std::regex literal_call{
        R"re(^translate\s*\(\s*"((?:[^"\\]|\\.)*)"\s*,\s*((?:"(?:[^"\\]|\\.)*"\s*)+)\))re"};
    std::vector<SourceText> texts;
    for(const auto& folder : folders)
    {
        for(const auto& entry : std::filesystem::recursive_directory_iterator{folder})
        {
            const auto extension{entry.path().extension()};
            // A user interface's description: its texts, under the context its root names. One that names none is a
            // script's, whose texts show as they are written.
            if(extension == ".oui")
            {
                const auto parsed{oui::parse_oui(read_file(entry.path()))};
                if(!parsed.succeeded())
                {
                    problems.push_back(entry.path().filename().string() + ": " + parsed.diagnostics.front().message);
                    continue;
                }
                if(!parsed.root->has("context"))
                {
                    continue;
                }
                for(const auto& text : oui::ui_texts(*parsed.root))
                {
                    texts.push_back(
                        {text.context, text.text, entry.path().filename().string() + ":" + std::to_string(text.line)});
                }
                continue;
            }
            // translation.cpp defines translate(); ui.cpp translates the texts of descriptions, which are read
            // from the .oui files themselves, above.
            if(entry.path().stem() == "translation" || entry.path().filename() == "ui.cpp" ||
               (extension != ".h" && extension != ".cpp"))
            {
                continue;
            }
            const auto source{without_comments(read_file(entry.path()))};
            for(auto found{std::sregex_iterator{source.begin(), source.end(), call}}; found != std::sregex_iterator{};
                ++ found)
            {
                const auto offset{static_cast<std::size_t>(found->position())};
                const auto line{1 +
                                std::count(source.begin(), source.begin() + static_cast<std::ptrdiff_t>(offset), '\n')};
                const auto where{entry.path().filename().string() + ":" + std::to_string(line)};
                std::smatch match;
                const std::string rest{source.substr(offset, 2000)};
                if(!std::regex_search(rest, match, literal_call))
                {
                    problems.push_back(where + ": translate() without string literals");
                    continue;
                }
                texts.push_back(
                    {unescape_literals("\"" + match[1].str() + "\""), unescape_literals(match[2].str()), where});
            }
        }
    }
    return texts;
}

std::set<std::string> placeholders(const std::string& text)
{
    static const std::regex placeholder{R"(\{[0-9]+\})"};
    std::set<std::string> result;
    for(auto found{std::sregex_iterator{text.begin(), text.end(), placeholder}}; found != std::sregex_iterator{};
        ++ found)
    {
        result.insert(found->str());
    }
    return result;
}

void expect_catalogs_cover_sources(const std::filesystem::path& catalogs,
                                   const std::vector<std::filesystem::path>& sources)
{
    std::vector<std::string> problems;
    const auto texts{source_texts(sources, problems)};
    EXPECT_FALSE(texts.empty());
    std::size_t catalog_count{};
    for(const auto& entry : std::filesystem::directory_iterator{catalogs})
    {
        if(entry.path().extension() != ".json")
        {
            continue;
        }
        ++ catalog_count;
        const auto file{entry.path().filename().string()};
        std::string error;
        const auto catalog{parse_translation_catalog(read_file(entry.path()), error)};
        if(!catalog)
        {
            problems.push_back(file + ": " + error);
            continue;
        }
        if(catalog->name.empty())
        {
            problems.push_back(file + ": no \"name\" for the language chooser");
        }
        std::set<std::pair<std::string, std::string>> used;
        for(const auto& text : texts)
        {
            used.insert({text.context, text.text});
            const auto context{catalog->contexts.find(text.context)};
            const auto entry_found{context == catalog->contexts.end() ? nullptr : [&]() -> const std::string*
                                   {
                                       const auto found{context->second.find(text.text)};
                                       return found == context->second.end() ? nullptr : &found->second;
                                   }()};
            const auto quoted{text.context + " \"" + text.text + "\" (" + text.where + ")"};
            if(entry_found == nullptr || entry_found->empty())
            {
                problems.push_back(file + ": untranslated " + quoted);
                continue;
            }
            if(placeholders(*entry_found) != placeholders(text.text))
            {
                problems.push_back(file + ": other placeholders in " + quoted);
            }
            if((mnemonic(*entry_found) == 0) != (mnemonic(text.text) == 0))
            {
                problems.push_back(file + ": the '&' mnemonic comes and goes in " + quoted);
            }
        }
        for(const auto& [context, entries] : catalog->contexts)
        {
            for(const auto& [text, translation] : entries)
            {
                if(used.count({context, text}) == 0)
                {
                    problems.push_back(file + ": no source uses " + context + " \"" + text + "\"");
                }
            }
        }
    }
    EXPECT_GT(catalog_count, 0u);
    std::string report;
    for(const auto& problem : problems)
    {
        report += problem + "\n";
    }
    EXPECT_TRUE(problems.empty()) << report;
}

const std::filesystem::path source_dir{OUI_CODE_SOURCE_DIR};

TEST(TranslationTests, OuiCodesCatalogsCoverItsTexts)
{
    expect_catalogs_cover_sources(source_dir / "oui_code/translations", {source_dir / "oui_code"});
}

TEST(TranslationTests, OuiRunsCatalogsCoverItsTexts)
{
    expect_catalogs_cover_sources(source_dir / "oui_run/translations", {source_dir / "oui_run"});
}

TEST(TranslationTests, OuiCalcsCatalogsCoverItsTexts)
{
    expect_catalogs_cover_sources(source_dir / "oui_calc/translations", {source_dir / "oui_calc"});
}

} // namespace
