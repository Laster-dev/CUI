#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/core/Value.h"
#include "framework/controls/ComboBox.h"
#include "framework/controls/Slider.h"
#include <format>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

std::shared_ptr<Button> MakeChip(const std::string& text, D2D1_COLOR_F color, float flexGrow = 0.0f) {
    auto b = ElevatedButton(text).Background(color).Padding(14, 8, 14, 8).Build();
    if (flexGrow > 0.0f) {
        b->SetFlexGrow(flexGrow);
    }
    return b;
}

} // namespace

std::shared_ptr<UIElement> BuildStackPanelPage() {
    // —— 水平排列 ——
    auto row = Row(12).Padding(12).Build();
    row->SetBackgroundToken(ThemeTokenId::CardBackground);
    row->SetBorderToken(ThemeTokenId::CardBorder);
    row->SetBorderThickness(1.0f);
    row->SetCornerRadius(6.0f);
    row->AddChild(MakeChip("按钮 1", Rgb(0x007ACC)));
    row->AddChild(MakeChip("按钮 2", Rgb(0x10B981)));
    row->AddChild(MakeChip("弹性填充", Rgb(0x845EF7), 1.0f));
    row->AddChild(MakeChip("按钮 4", Rgb(0xD13438)));

    // —— 垂直排列 ——
    auto col = Column(10).Padding(12).Build();
    col->SetBackgroundToken(ThemeTokenId::CardBackground);
    col->SetBorderToken(ThemeTokenId::CardBorder);
    col->SetBorderThickness(1.0f);
    col->SetCornerRadius(6.0f);
    col->AddChild(MakeLabel("标题一", 14.0f, ThemeTokenId::TextPrimary, true));
    col->AddChild(MakeLabel("说明文字：StackPanel 按添加顺序自上而下堆叠，每个子元素独占一行。", 12.0f, ThemeTokenId::TextMuted, false));
    col->AddChild(TextField("输入框也按顺序排列").Build());
    col->AddChild(WrapPanelWidget("Horizontal").Gap(8).Children({
        ElevatedButton("确定").Background(Rgb(0x007ACC)).Padding(14, 8, 14, 8).Build(),
        ElevatedButton("取消").Padding(14, 8, 14, 8).Build(),
    }).Build());

    // —— 方向与间距（实时调节）——
    auto livePanel = Column(12).Padding(12).Build();
    livePanel->SetBackgroundToken(ThemeTokenId::CardBackground);
    livePanel->SetBorderToken(ThemeTokenId::CardBorder);
    livePanel->SetBorderThickness(1.0f);
    livePanel->SetCornerRadius(6.0f);
    livePanel->AddChild(MakeChip("元素 A", Rgb(0x007ACC)));
    livePanel->AddChild(MakeChip("元素 B", Rgb(0x10B981)));
    livePanel->AddChild(MakeChip("元素 C", Rgb(0x845EF7)));

    auto dirCombo = Make<ComboBox>();
    dirCombo->AddItem("Vertical(垂直)");
    dirCombo->AddItem("Horizontal(水平)");
    dirCombo->SetSelectedIndex(0);

    State<int> dirIndex{ 0 };
    dirCombo->SelectedIndex.Bind(dirIndex);
    // 选择索引通过转换器驱动容器的 Orientation 属性，无需手动事件转发。
    livePanel->Orientation.Bind(dirIndex,
        MakeConverter<int, Orientation>([](int index) {
            return index == 0 ? Orientation::Vertical : Orientation::Horizontal;
        }),
        BindingMode::OneWay);

    auto gapSlider = SliderWidget(12.0f, 0.0f, 40.0f).Build();
    gapSlider->SetFlexGrow(1.0f);
    State<float> gapValue{ 12.0f };
    gapSlider->ValueProperty.Bind(gapValue);
    // 滑块值直接同步到容器的 Gap 属性，双向绑定双向联通。
    livePanel->Gap.Bind(gapValue, BindingMode::OneWay);

    auto configValue = MakeComputed<std::string>([](int dir, float gap) {
        return std::format("方向：{}　间距：{:.0f}px",
                           dir == 0 ? "Vertical" : "Horizontal", gap);
    }, dirIndex, gapValue);
    auto configStatus = MakeStatus("");
    configStatus->Text.Bind(configValue, BindingMode::OneWay);

    SamplePageSpec spec;
    spec.title = "StackPanel(堆栈面板)";
    spec.subtitle = "将子元素排成单行的水平或垂直序列，是 CUI 中最常用的布局容器。";
    spec.sections = {
        {
            "水平排列",
            "Row(gap) 快捷创建水平 StackPanel；子元素按顺序从左到右排列，超出部分可配合 FlexGrow 弹性伸缩。",
            Column(12).Children({
                row,
                MakeStatus("紫色按钮设了 SetFlexGrow(1)，自动占满剩余宽度。"),
            }).Build(),
        },
        {
            "垂直排列",
            "Column(gap) 快捷创建垂直 StackPanel；每个子元素独占一行，宽度默认由自身内容决定。",
            Column(12).Children({ col }).Build(),
        },
        {
            "方向与间距（实时调节）",
            "在运行时通过 SetOrientation / SetGap 改变排列方向与间距，布局即时重排。",
            Column(12).Children({
                livePanel,
                WrapPanelWidget("Horizontal").Gap(16).Children({ dirCombo, gapSlider, configStatus }).Build(),
            }).Build(),
        },
    };
    spec.source =
        "auto row = Row(12).Build();            // 水平 StackPanel，间距 12px\n"
        "row->AddChild(ElevatedButton(\"A\").Build());\n"
        "\n"
        "auto col = Column(10).Build();         // 垂直 StackPanel，间距 10px\n"
        "col->AddChild(Text(\"标题\").Build());\n"
        "\n"
        "// 运行时切换方向与间距：状态经转换器驱动属性，无需事件\n"
        "State<int> dirIndex{ 0 };\n"
        "combo->SelectedIndex.Bind(dirIndex);\n"
        "panel->Orientation.Bind(dirIndex, MakeConverter<int, Orientation>(\n"
        "    [](int i) { return i == 0 ? Orientation::Vertical : Orientation::Horizontal; }));\n"
        "panel->Gap.Bind(gapState, BindingMode::OneWay);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
