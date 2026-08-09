#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/style/ThemeManager.h"
#include "framework/style/ThemeTokenId.h"

using namespace CUI;
using namespace CUI::DSL;

namespace {
template <typename T>
std::shared_ptr<T> BindThemeToken(const std::shared_ptr<T>& element, const std::string& tokenProp, const std::string& tokenName) {
    if (!element) {
        return element;
    }
    ThemeTokenId id = ThemeTokenIdFromName(tokenName);
    if (tokenProp == "theme.backgroundToken") {
        element->SetBackgroundToken(id);
        element->SetBackground(ThemeManager::Instance().GetColor(tokenName));
    } else if (tokenProp == "theme.borderToken") {
        element->SetBorderToken(id);
        element->SetBorderBrush(ThemeManager::Instance().GetColor(tokenName));
    } else if (tokenProp == "theme.colorToken") {
        element->SetColorToken(id);
    }
    return element;
}
}

ShowcasePage BuildSplitterPage(const ShowcaseContext& ctx) {
    auto splitterLR = SplitterWidget(Orientation::Vertical).Build();
    auto splitterTB = SplitterWidget(Orientation::Horizontal).Build();

    auto leftPane = Column(6).Width(140).MinWidth(72).Padding(10).Children({
        CreateShowcaseText("导航", 12.0f, "", true),
        CreateShowcaseText("• Home", 11.0f, ""),
        CreateShowcaseText("• Documents", 11.0f, ""),
        CreateShowcaseText("• Settings", 11.0f, "")
    }).Build();
    BindThemeToken(leftPane, "theme.backgroundToken", "paneBackground");

    auto rightPane = Column(6).FlexGrow(1.0f).MinWidth(96).Padding(10).Children({
        CreateShowcaseText("内容区", 12.0f, "", true),
        CreateShowcaseText("拖拽中间细分割条调整左右宽度。", 11.0f, "")
    }).Build();
    BindThemeToken(rightPane, "theme.backgroundToken", "cardBackground");

    auto topPane = Column(4).Height(88).MinHeight(48).Padding(10).Children({
        CreateShowcaseText("编辑器", 12.0f, "", true),
        CreateShowcaseText("int main() { return 0; }", 11.0f, "")
    }).Build();
    BindThemeToken(topPane, "theme.backgroundToken", "paneBackground");

    auto bottomPane = Column(4).FlexGrow(1.0f).MinHeight(48).Padding(10).Children({
        CreateShowcaseText("输出", 12.0f, "", true),
        CreateShowcaseText("Build succeeded.", 11.0f, "")
    }).Build();
    BindThemeToken(bottomPane, "theme.backgroundToken", "cardBackground");

    auto rowSplit = Row().Height(150).CornerRadius(6).BorderToken(ThemeTokenId::CardBorder, 1).Children({
        leftPane,
        splitterLR,
        rightPane
    }).Build();
    BindThemeToken(rowSplit, "theme.backgroundToken", "cardBackground");
    BindThemeToken(rowSplit, "theme.borderToken", "cardBorder");

    auto colSplit = Column(0).Height(170).CornerRadius(6).BorderToken(ThemeTokenId::CardBorder, 1).Children({
        topPane,
        splitterTB,
        bottomPane
    }).Build();
    BindThemeToken(colSplit, "theme.backgroundToken", "cardBackground");
    BindThemeToken(colSplit, "theme.borderToken", "cardBorder");

    auto demo = CreateDemoSurface({
        CreateShowcaseText("水平拆分（左右）", 12.0f, ""),
        rowSplit,
        CreateShowcaseText("垂直拆分（上下）", 12.0f, ""),
        colSplit
    });

    return { "Splitter 拆分条", CreatePage(
        "Splitter / GridSplitter",
        "WinUI 风格细分割条：悬停/拖拽时显示强调色指示线，拖拽即可联动面板尺寸。",
        demo,
        CreatePropertyGrid(ctx, splitterLR),
        splitterLR) };
}
