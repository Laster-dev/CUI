#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "Gallery.h"
#include "framework/core/Widgets.h"
#include <format>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

std::shared_ptr<CUI::StackPanel> MakePane(
    const std::string& title,
    const std::string& hint,
    float flexGrow,
    float minMain) {
    CUI::Widgets::Ref pane =Column(6).Padding(10).Build();
        pane->AddChild(MakeLabel(title, 12.0f, ThemeTokenId::TextPrimary, true));
        pane->AddChild(MakeLabel(hint, 11.0f, ThemeTokenId::TextSecondary, false));
    if (flexGrow > 0.0f) {
                pane.FlexGrow(flexGrow);
    }
    if (minMain > 0.0f) {
                pane.MinWidth(minMain);
                pane.MinHeight(minMain);
    }
    return pane;
}

void StyleContainer(const std::shared_ptr<CUI::UIElement>& container) {
        container->ApplyBackgroundToken(ThemeTokenId::CardBackground);
        container->ApplyBorderToken(ThemeTokenId::CardBorder);
        container->ApplyBorderThickness(1.0f);
        container->ApplyClipToBounds(true);
}

} // namespace

std::shared_ptr<UIElement> BuildSplitterPage() {
    // —— 水平拆分（左右分栏）——
    CUI::Widgets::Ref leftPane =Column(6).Width(150).MinWidth(80).Padding(10).Build();
        leftPane.BackgroundToken(ThemeTokenId::PaneBackground);
        leftPane->AddChild(MakeLabel("导航", 12.0f, ThemeTokenId::TextPrimary, true));
        leftPane->AddChild(MakeLabel("• 项目", 11.0f, ThemeTokenId::TextSecondary, false));
        leftPane->AddChild(MakeLabel("• 文档", 11.0f, ThemeTokenId::TextSecondary, false));
        leftPane->AddChild(MakeLabel("• 设置", 11.0f, ThemeTokenId::TextSecondary, false));

    CUI::Widgets::Ref rightPane =Column(6).FlexGrow(1.0f).MinWidth(120).Padding(10).Build();
        rightPane.BackgroundToken(ThemeTokenId::CardBackground);
        rightPane->AddChild(MakeLabel("内容区", 12.0f, ThemeTokenId::TextPrimary, true));
        rightPane->AddChild(MakeLabel("拖拽中间的分隔条调整左右宽度。", 11.0f, ThemeTokenId::TextSecondary, false));

    CUI::Widgets::Ref splitLR = Widgets::Splitter().Shared();
    splitLR.Orientation(Orientation::Vertical);

    auto rowSplit = Row(0).Height(170).CornerRadius(6).Children({ leftPane, splitLR, rightPane }).Build();
    StyleContainer(rowSplit);

    // —— 垂直拆分（上下分栏）——
    CUI::Widgets::Ref topPane =Column(4).Height(90).MinHeight(50).Padding(10).Build();
        topPane.BackgroundToken(ThemeTokenId::PaneBackground);
        topPane->AddChild(MakeLabel("编辑器", 12.0f, ThemeTokenId::TextPrimary, true));
    topPane->AddChild(MakeLabel("int main() { return 0; }", 11.0f, ThemeTokenId::TextSecondary, false));

    CUI::Widgets::Ref bottomPane =Column(4).FlexGrow(1.0f).MinHeight(50).Padding(10).Build();
        bottomPane.BackgroundToken(ThemeTokenId::CardBackground);
        bottomPane->AddChild(MakeLabel("输出", 12.0f, ThemeTokenId::TextPrimary, true));
        bottomPane->AddChild(MakeLabel("构建成功。", 11.0f, ThemeTokenId::TextSecondary, false));

    CUI::Widgets::Ref splitTB = Widgets::Splitter().Shared();
    splitTB.Orientation(Orientation::Horizontal);

    auto colSplit = Column(0).Height(210).CornerRadius(6).Children({ topPane, splitTB, bottomPane }).Build();
    StyleContainer(colSplit);

    // —— 三栏双分隔条 ——
    CUI::Widgets::Ref p1 =MakePane("栏 A", "宽 110px，最小 60px。", 0.0f, 60.0f);
        p1.Width(110.0f);
        p1.BackgroundToken(ThemeTokenId::PaneBackground);
    CUI::Widgets::Ref p2 =MakePane("栏 B", "弹性伸缩，最小 80px。", 1.0f, 80.0f);
        p2.BackgroundToken(ThemeTokenId::CardBackground);
    CUI::Widgets::Ref p3 =MakePane("栏 C", "宽 130px，最小 60px。", 0.0f, 60.0f);
        p3.Width(130.0f);
        p3.BackgroundToken(ThemeTokenId::PaneBackground);

    CUI::Widgets::Ref s1 = Widgets::Splitter().Shared();
    s1.Orientation(Orientation::Vertical);
    CUI::Widgets::Ref s2 = Widgets::Splitter().Shared();
    s2.Orientation(Orientation::Vertical);

    auto threeCol = Row(0).Height(150).CornerRadius(6).Children({ p1, s1, p2, s2, p3 }).Build();
    StyleContainer(threeCol);

    // —— 拖拽事件与状态 ——
    CUI::Widgets::Ref eventStatus =MakeStatus("拖拽上方的分隔条，观察 SplitterMoved 事件输出。");
    splitLR->OnSplitterMoved().Connect(
        [leftPane, rightPane, eventStatus](Splitter*, float delta) {
                        eventStatus.Text(std::format(
                "SplitterMoved：位移 {:+0.1f}px → 左栏 {:.0f}px / 右栏 {:.0f}px",
                delta, leftPane->GetBounds().width, rightPane->GetBounds().width));
        });

    SamplePageSpec spec;
    spec.title = "Splitter(分隔条)";
    spec.subtitle = "夹在两个同级控件之间的拖拽分隔条，调整相邻区域的尺寸分配；悬停与拖拽时高亮强调色指示线。";
    spec.sections = {
        {
            "水平拆分（左右分栏）",
            "Widgets::Splitter(Orientation::Vertical) 竖条夹在左右两个面板之间；左栏固定初始宽度，右栏弹性伸缩，拖拽实时改宽。",
            Column(12, {
                rowSplit,
                MakeStatus("拖拽会同时改写左右两栏宽度，并受 MinWidth/MinHeight 约束（本例不小于 80/120px）。"),
            }),
        },
        {
            "垂直拆分（上下分栏）",
            "Widgets::Splitter(Orientation::Horizontal) 横条夹在上下两个面板之间，拖拽调整上下高度。",
            Column(12, {
                colSplit,
                MakeStatus("上栏固定初始高度，下栏弹性伸缩；最小高度约束 50px。"),
            }),
        },
        {
            "三栏双分隔条",
            "在同一行内放置两条分隔条即可实现三栏可调布局，各栏独立调整互不干扰。",
            Column(12, {
                threeCol,
                MakeStatus("左右栏固定宽度，中间栏弹性伸缩；两条分隔条分别调整自己的左右邻栏。"),
            }),
        },
        {
            "拖拽事件与状态",
            "通过 OnSplitterMoved() 订阅拖拽位移，事件参数为本次位移增量（可正可负）。",
            Column(12, {
                eventStatus,
            }),
        },
    };
    spec.source =
        "auto left = Column(6).Width(150).MinWidth(80).Padding(10).Build();\n"
        "auto right = Column(6).FlexGrow(1.0f).MinWidth(120).Padding(10).Build();\n"
        "\n"
        "auto splitter = Widgets::Splitter(Orientation::Vertical).Shared();\n"
        "// 竖条夹在左右面板之间，形成可拖拽的水平拆分\n"
        "auto row = Row(0).Height(170).Children( left, splitter, right ).Build();\n"
        "\n"
        "// 上下拆分：Widgets::Splitter(Orientation::Horizontal)\n"
        "auto splitterTB = Widgets::Splitter(Orientation::Horizontal).Shared();\n"
        "\n"
        "// 订阅拖拽事件：参数为本次位移增量\n"
        "splitter->OnSplitterMoved().Connect(\n"
        "    [](Splitter*, float delta) { /* 处理位移 */ });\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery



