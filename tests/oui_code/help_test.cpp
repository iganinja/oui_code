#include "help.h"

#include "resources.h"

#include <oui/translation.h>

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace olagarro;

namespace
{

// The title of the section F1 shows for the topic, in the help as it reads in the language.
[[nodiscard]] std::string shown_for(HelpTopic topic, const std::string& language)
{
    oui::install_language(language, {});
    const oui::CommandRegistry commands;
    const auto titles{help_section_titles(help_markdown(language, commands))};
    oui::remove_translations();
    const auto section{help_section(topic)};
    return section < titles.size() ? titles[section] : "(none)";
}

} // namespace

TEST(OuiCodeHelpTests, BothLanguagesHaveTheSamePartsInTheSameOrder)
{
    // F1 finds a part by its place, so the two must go together.
    const auto help{embedded_oui_code_help()};
    EXPECT_EQ(help_section_titles(std::string{help.at(0)}).size(), help_section_titles(std::string{help.at(1)}).size());
}

TEST(OuiCodeHelpTests, F1ShowsThePartOnWhatIsInTheTab)
{
    EXPECT_EQ(shown_for(HelpTopic::general, "en"), "OUI Code");
    EXPECT_EQ(shown_for(HelpTopic::searching, "en"), "Searching");
    EXPECT_EQ(shown_for(HelpTopic::comparing, "en"), "Comparing files");
    EXPECT_EQ(shown_for(HelpTopic::interactive_scripts, "en"), "Interactive scripts");
    EXPECT_EQ(shown_for(HelpTopic::tasks, "en"), "Things to do");
    EXPECT_EQ(shown_for(HelpTopic::terminal, "en"), "A shell in a tab");
    EXPECT_EQ(shown_for(HelpTopic::calculator, "en"), "Calculator");
    EXPECT_EQ(shown_for(HelpTopic::interactive_scripts, "es"), "Scripts interactivos");
    EXPECT_EQ(shown_for(HelpTopic::calculator, "es"), "Calculadora");
    EXPECT_EQ(shown_for(HelpTopic::interactive_ui, "en"), "Interactive user interfaces");
    EXPECT_EQ(shown_for(HelpTopic::interactive_ui, "es"), "Interfaces interactivas");
}

TEST(OuiCodeHelpTests, TheTopicsAreThePartsAndThePartsWithinThem)
{
    const std::string markdown{"# OUI Code\n\nIntro.\n\n## Scripts\n\nAbout scripts.\n\n### Running a `script`\n\n"
                               "Press F5.\n\n```\n### not a heading\n```\n\n## Terminals\n\nKeys.\n"};
    const auto topics{help_topics(markdown)};
    ASSERT_EQ(topics.size(), 4u);
    EXPECT_EQ(topics[0].title, "OUI Code");
    EXPECT_EQ(topics[1].title, "Scripts");
    EXPECT_EQ(topics[1].depth, 0);
    EXPECT_EQ(topics[2].title, "Running a script") << "the marks of code gone";
    EXPECT_EQ(topics[2].depth, 1);
    EXPECT_EQ(topics[2].section, 1u) << "in the part it is in";
    EXPECT_EQ(topics[2].anchor, "running-a-script");
    EXPECT_NE(topics[2].text.find("Press F5."), std::string::npos);
    EXPECT_EQ(topics[1].text.find("Press F5."), std::string::npos) << "each topic says what is up to the next";
    EXPECT_EQ(topics[3].section, 2u);
}

TEST(OuiCodeHelpTests, TheSearchFindsTopicsLooselyTheirTitlesFirst)
{
    const std::vector<HelpTopicEntry> topics{
        {"Getting around", 0, 0, {}, "Ctrl+Tab goes between tabs.\n"},
        {"A shell in a tab", 0, 1, {}, "Opens your shell.\n"},
        {"Búsqueda", 0, 2, {}, "Encuentra texto.\n"},
        {"Terminals", 0, 3, {}, "Colours and keys.\n"}};
    EXPECT_EQ(find_help_topics(topics, ""), (std::vector<std::size_t>{0, 1, 2, 3})) << "nothing typed, every one";
    EXPECT_EQ(find_help_topics(topics, "tab"), (std::vector<std::size_t>{1, 0})) << "in a title first";
    EXPECT_EQ(find_help_topics(topics, "shl"), (std::vector<std::size_t>{1})) << "its letters in order";
    EXPECT_EQ(find_help_topics(topics, "BUSQUEDA"), (std::vector<std::size_t>{2})) << "whatever the case or accents";
    EXPECT_EQ(find_help_topics(topics, "keys colours"), (std::vector<std::size_t>{3})) << "every word";
    EXPECT_TRUE(find_help_topics(topics, "keys nowhere").empty());
}
