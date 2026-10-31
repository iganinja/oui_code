#include "documents.h"

#include <oui/text_layout.h>
#include <oui/text_metrics.h>

#include <gtest/gtest.h>

using namespace olagarro;

namespace
{

// A window's metrics as far as a label goes: each character half a cell, with a cell of margins, to the step.
class HalfCellLabels : public oui::TextMetrics
{
  public:
    [[nodiscard]] double label_width(std::string_view text) const override
    {
        return text.empty() ? 0.0 : oui::snap_up(0.5 * oui::text_width(text) + 1, 0.125);
    }
};

class MetricsInPlace
{
  public:
    explicit MetricsInPlace(const oui::TextMetrics& metrics) { oui::set_text_metrics(&metrics); }
    ~MetricsInPlace() { oui::set_text_metrics(nullptr); }
    MetricsInPlace(const MetricsInPlace&) = delete;
    MetricsInPlace& operator=(const MetricsInPlace&) = delete;
};

} // namespace

TEST(OuiCodeDocumentsTests, InATerminalAPathIsCutFromItsStartToItsColumns)
{
    EXPECT_EQ(fit_from_start("~/src/main.cpp", 14), "~/src/main.cpp");
    EXPECT_EQ(fit_from_start("~/src/main.cpp", 9), "…main.cpp");
}

TEST(OuiCodeDocumentsTests, InAWindowAPathIsCutToTheRoomItsLabelTakes)
{
    const HalfCellLabels metrics;
    const MetricsInPlace in_place{metrics};
    // Fourteen characters in eight cells: whole as the window shows it, though its columns are more.
    EXPECT_EQ(fit_from_start("~/src/main.cpp", 8), "~/src/main.cpp");
    // In five cells and a quarter, "…" and seven characters with the margins.
    EXPECT_EQ(fit_from_start("~/src/main.cpp", 5.25), "…ain.cpp");
}
