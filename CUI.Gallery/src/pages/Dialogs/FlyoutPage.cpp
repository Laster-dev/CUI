#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/TextBox.h"
#include "framework/controls/ToggleSwitch.h"
#include "framework/controls/Flyout.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildFlyoutPage() {
    auto status = MakeStatus("点击按钮弹出浮出层，此处显示操作提示。");

    // ── 1. 简单文本浮出层 ────────────────────────────────────────────────
    auto btnSimple = Make<Button>("简单文本浮出层");
    btnSimple->OnClick().Connect([](UIElement* src) {
        auto flyout = Make<Flyout>();
        flyout->SetPlacement(FlyoutPlacement::Bottom);

        auto content = Column(8).Children({
            MakeLabel("这是一个 Flyout 浮出层", 13.0f, ThemeTokenId::TextPrimary, true),
            MakeLabel("点击此区域外的任意位置即可关闭。", 12.0f, ThemeTokenId::TextSecondary),
        }).Build();
        flyout->SetContent(content);
        src->AddChild(flyout);
        flyout->ShowAt(src);
    });

    // ── 2. 多方向放置 ────────────────────────────────────────────────────
    auto btnTop    = Make<Button>("↑ Top");
    auto btnBottom = Make<Button>("↓ Bottom");
    auto btnLeft   = Make<Button>("← Left");
    auto btnRight  = Make<Button>("→ Right");

    btnTop->SetWidth(90.0f);
    btnBottom->SetWidth(90.0f);
    btnLeft->SetWidth(90.0f);
    btnRight->SetWidth(90.0f);

    auto makePlacementFlyout = [](UIElement* src, FlyoutPlacement p, const std::string& label) {
        auto flyout = Make<Flyout>();
        flyout->SetPlacement(p);
        auto content = Column(6).Children({
            MakeLabel(label, 12.0f, ThemeTokenId::TextPrimary),
            MakeLabel("Placement 演示内容。", 12.0f, ThemeTokenId::TextSecondary),
        }).Build();
        flyout->SetContent(content);
        src->AddChild(flyout);
        flyout->ShowAt(src);
    };

    btnTop->OnClick().Connect([makePlacementFlyout](UIElement* src) {
        makePlacementFlyout(src, FlyoutPlacement::Top, "放置于上方（Top）");
    });
    btnBottom->OnClick().Connect([makePlacementFlyout](UIElement* src) {
        makePlacementFlyout(src, FlyoutPlacement::Bottom, "放置于下方（Bottom）");
    });
    btnLeft->OnClick().Connect([makePlacementFlyout](UIElement* src) {
        makePlacementFlyout(src, FlyoutPlacement::Left, "放置于左侧（Left）");
    });
    btnRight->OnClick().Connect([makePlacementFlyout](UIElement* src) {
        makePlacementFlyout(src, FlyoutPlacement::Right, "放置于右侧（Right）");
    });

    // ── 3. 带操作按钮的浮出层 ───────────────────────────────────────────
    auto btnAction = Make<Button>("带操作的浮出层");
    btnAction->SetBackgroundToken(ThemeTokenId::CardBackground);
    btnAction->SetColorToken(ThemeTokenId::TextPrimary);
    btnAction->SetBorderToken(ThemeTokenId::CardBorder);
    btnAction->SetBorderThickness(1.0f);
    btnAction->OnClick().Connect([status](UIElement* src) {
        auto flyout = Make<Flyout>();
        flyout->SetPlacement(FlyoutPlacement::Bottom);

        auto confirmBtn = Make<Button>("删除");
        confirmBtn->SetBackground(Color::Hex("#C62828"));
        confirmBtn->SetHoverBackground(Color::Hex("#B71C1C"));
        confirmBtn->SetColor(Color::White);
        confirmBtn->SetWidth(72.0f);

        auto cancelBtn = Make<Button>("取消");
        cancelBtn->SetBackgroundToken(ThemeTokenId::CardBackground);
        cancelBtn->SetColorToken(ThemeTokenId::TextPrimary);
        cancelBtn->SetBorderToken(ThemeTokenId::CardBorder);
        cancelBtn->SetBorderThickness(1.0f);
        cancelBtn->SetWidth(72.0f);

        auto content = Column(12).Children({
            MakeLabel("确认删除？", 13.0f, ThemeTokenId::TextPrimary, true),
            MakeLabel("此操作将永久移除所选项目。", 12.0f, ThemeTokenId::TextSecondary),
            Row(8).Children({ confirmBtn, cancelBtn }).Build(),
        }).Build();
        flyout->SetContent(content);

        confirmBtn->OnClick().Connect([status, flyout](UIElement*) {
            status->SetText("已确认删除操作。");
            flyout->Hide();
        });
        cancelBtn->OnClick().Connect([status, flyout](UIElement*) {
            status->SetText("已取消删除操作。");
            flyout->Hide();
        });

        src->AddChild(flyout);
        flyout->ShowAt(src);
    });

    // ── 4. 包含输入框的浮出层 ───────────────────────────────────────────
    auto btnInputFlyout = Make<Button>("输入浮出层");
    btnInputFlyout->SetBackgroundToken(ThemeTokenId::CardBackground);
    btnInputFlyout->SetColorToken(ThemeTokenId::TextPrimary);
    btnInputFlyout->SetBorderToken(ThemeTokenId::CardBorder);
    btnInputFlyout->SetBorderThickness(1.0f);
    btnInputFlyout->OnClick().Connect([status](UIElement* src) {
        auto flyout = Make<Flyout>();
        flyout->SetPlacement(FlyoutPlacement::Bottom);

        auto input = Make<TextBox>();
        input->SetPlaceholder("输入新项目名称");
        input->SetWidth(200.0f);

        auto applyBtn = Make<Button>("应用");
        applyBtn->SetWidth(60.0f);

        auto content = Column(10).Children({
            MakeLabel("重命名项目", 13.0f, ThemeTokenId::TextPrimary, true),
            input,
            applyBtn,
        }).Build();
        flyout->SetContent(content);

        applyBtn->OnClick().Connect([status, flyout, input](UIElement*) {
            std::string name = input->GetText();
            status->SetText("已应用名称：「" + (name.empty() ? "（空）" : name) + "」。");
            flyout->Hide();
        });

        src->AddChild(flyout);
        flyout->ShowAt(src);
    });

    SamplePageSpec spec;
    spec.title    = "Flyout（浮出层）";
    spec.subtitle = "轻量弹出面板，附着于目标控件旁，点击外部区域自动收拢，不阻断背景操作。";
    spec.sections = {
        {
            "简单浮出层",
            "调用 ShowAt(target) 将 Flyout 显示在指定控件附近，SetPlacement() 控制停靠方向，默认为 Bottom。",
            Column(12).Children({
                btnSimple,
                status,
            }).Build(),
        },
        {
            "停靠方向",
            "支持 Top、Bottom、Left、Right 四个方向，框架会在边界受限时自动调整位置。",
            Row(10).Children({ btnTop, btnBottom, btnLeft, btnRight }).Build(),
        },
        {
            "带操作按钮 & 输入框",
            "Flyout 内容为任意 UIElement，可嵌入按钮、输入框等控件实现轻量交互，无需打开完整对话框。",
            Row(12).Children({ btnAction, btnInputFlyout }).Build(),
        },
    };
    spec.source =
        "auto flyout = Make<Flyout>();\n"
        "flyout->SetPlacement(FlyoutPlacement::Bottom);\n"
        "flyout->SetContent(/* 任意 UIElement */);\n"
        "parent->AddChild(flyout); // 先挂载到活动 UI 树\n"
        "flyout->ShowAt(targetElement);\n"
        "\n"
        "// 手动关闭\n"
        "flyout->Hide();\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
