#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/ToggleSwitch.h"
#include "framework/controls/TeachingTip.h"
#include "framework/window/BubbleChrome.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildTeachingTipPage() {
    auto status = MakeStatus("点击按钮显示指引气泡，此处显示操作结果。");

    // ── 1. 基础气泡 ─────────────────────────────────────────────────────
    auto btnBasic = Make<Button>("显示基础 TeachingTip");
    btnBasic->OnClick().Connect([status](UIElement* src) {
        auto tip = Make<TeachingTip>();
        tip->SetTitle("欢迎使用 CUI 框架");
        tip->SetMessage("这是一个 TeachingTip 气泡指引示例。它会自动停靠在目标控件旁边，并带有小三角箭头指示来源。");
        tip->SetActionText("我知道了");
        tip->OnAction().Connect([status, tip]() {
            status->Text = "已点击【我知道了】，气泡已关闭。";
            tip->Close();
        });
        tip->OnClosed().Connect([status]() {
            status->Text = "气泡已关闭。";
        });
        src->AddChild(tip);
        tip->ShowAround(src);
    });

    // ── 2. 无操作按钮、仅关闭叉 ────────────────────────────────────────
    auto btnCloseOnly = Make<Button>("仅关闭按钮");
    btnCloseOnly->BackgroundToken = ThemeTokenId::CardBackground;
    btnCloseOnly->ColorToken = ThemeTokenId::TextPrimary;
    btnCloseOnly->BorderToken = ThemeTokenId::CardBorder;
    btnCloseOnly->BorderThickness = 1.0f;
    btnCloseOnly->OnClick().Connect([status](UIElement* src) {
        auto tip = Make<TeachingTip>();
        tip->SetTitle("提示");
        tip->SetMessage("此提示没有操作按钮，只有右上角的关闭叉可以消退。");
        tip->SetIsCloseVisible(true);
        tip->OnClosed().Connect([status]() {
            status->Text = "已通过关闭按钮消退气泡。";
        });
        src->AddChild(tip);
        tip->ShowAround(src);
    });

    // ── 3. 不同停靠方向 ─────────────────────────────────────────────────
    auto btnPlacementTop    = Make<Button>("↑ Top");
    auto btnPlacementBottom = Make<Button>("↓ Bottom");
    auto btnPlacementLeft   = Make<Button>("← Left");
    auto btnPlacementRight  = Make<Button>("→ Right");

    btnPlacementTop->Width = 90.0f;
    btnPlacementBottom->Width = 90.0f;
    btnPlacementLeft->Width = 90.0f;
    btnPlacementRight->Width = 90.0f;

    auto makeDirectionTip = [](UIElement* src, BubblePlacement p, const std::string& label) {
        auto tip = Make<TeachingTip>();
        tip->SetTitle(label);
        tip->SetMessage("气泡将优先显示在此方向，边界受限时自动回退。");
        tip->SetActionText("关闭");
        tip->SetPreferredPlacement(p);
        tip->OnAction().Connect([tip]() { tip->Close(); });
        src->AddChild(tip);
        tip->ShowAround(src);
    };

    btnPlacementTop->OnClick().Connect([makeDirectionTip](UIElement* src) {
        makeDirectionTip(src, BubblePlacement::Top, "停靠于上方（Top）");
    });
    btnPlacementBottom->OnClick().Connect([makeDirectionTip](UIElement* src) {
        makeDirectionTip(src, BubblePlacement::Bottom, "停靠于下方（Bottom）");
    });
    btnPlacementLeft->OnClick().Connect([makeDirectionTip](UIElement* src) {
        makeDirectionTip(src, BubblePlacement::Left, "停靠于左侧（Left）");
    });
    btnPlacementRight->OnClick().Connect([makeDirectionTip](UIElement* src) {
        makeDirectionTip(src, BubblePlacement::Right, "停靠于右侧（Right）");
    });

    // ── 4. 模态气泡 ─────────────────────────────────────────────────────
    auto btnModal = Make<Button>("模态指引气泡");
    btnModal->Background = Color::Hex("#1565C0");
    btnModal->HoverBackground = Color::Hex("#0D47A1");
    btnModal->PressedBackground = Color::Hex("#082C6E");
    btnModal->Foreground = Color::White;
    btnModal->OnClick().Connect([status](UIElement* src) {
        auto tip = Make<TeachingTip>();
        tip->SetTitle("新功能介绍：智能搜索");
        tip->SetMessage("使用顶部搜索栏可快速定位任意控件或页面。\n支持拼音首字母检索，按 Ctrl+K 随时唤出。");
        tip->SetActionText("立即体验");
        tip->SetIsModal(true);
        tip->SetPreferredPlacement(BubblePlacement::Auto);
        tip->OnAction().Connect([status, tip]() {
            status->Text = "已点击【立即体验】，功能引导完成。";
            tip->Close();
        });
        tip->OnClosed().Connect([status]() {
            status->Text = "模态气泡已关闭。";
        });
        src->AddChild(tip);
        tip->ShowAround(src);
    });

    // ── 5. 自动停靠（Auto） ─────────────────────────────────────────────
    auto btnAuto = Make<Button>("自动停靠（Auto）");
    btnAuto->BackgroundToken = ThemeTokenId::CardBackground;
    btnAuto->ColorToken = ThemeTokenId::TextPrimary;
    btnAuto->BorderToken = ThemeTokenId::CardBorder;
    btnAuto->BorderThickness = 1.0f;
    btnAuto->OnClick().Connect([status](UIElement* src) {
        auto tip = Make<TeachingTip>();
        tip->SetTitle("自动方位（Auto）");
        tip->SetMessage("当 PreferredPlacement 设置为 Auto 时，框架根据目标控件的位置和视口空间自动选择最优停靠方向，确保气泡始终可见。");
        tip->SetActionText("明白了");
        tip->SetPreferredPlacement(BubblePlacement::Auto);
        tip->OnAction().Connect([status, tip]() {
            status->Text = "Auto 停靠示例完成。";
            tip->Close();
        });
        src->AddChild(tip);
        tip->ShowAround(src);
    });

    SamplePageSpec spec;
    spec.title    = "TeachingTip（气泡指引）";
    spec.subtitle = "带三角箭头指示的悬浮气泡，用于新功能介绍、操作步骤引导。支持操作按钮、模态阻断、自动停靠等特性。";
    spec.sections = {
        {
            "基础示例",
            "调用 ShowAround(target) 将气泡显示在目标控件附近，SetActionText() 设置操作按钮文字，OnAction() 回调处理点击。",
            Column(12).Children({
                Row(10).Children({ btnBasic, btnCloseOnly }).Build(),
                status,
            }).Build(),
        },
        {
            "停靠方向 PreferredPlacement",
            "通过 SetPreferredPlacement() 指定箭头优先指向的方位；框架在视口边界受限时自动回退到最优位置。",
            Row(10).Children({
                btnPlacementTop, btnPlacementBottom, btnPlacementLeft, btnPlacementRight
            }).Build(),
        },
        {
            "模态 & 自动停靠",
            "SetIsModal(true) 开启模态：气泡显示期间背景区域无法响应鼠标点击，适合关键功能首次引导。\n"
            "SetPreferredPlacement(Auto) 让框架根据空间自动选择最佳停靠方向。",
            Row(12).Children({ btnModal, btnAuto }).Build(),
        },
    };
    spec.source =
        "auto tip = Make<TeachingTip>();\n"
        "tip->SetTitle(\"标题\");\n"
        "tip->SetMessage(\"详细说明文字。\");\n"
        "tip->SetActionText(\"我知道了\");\n"
        "tip->SetPreferredPlacement(BubblePlacement::Bottom);\n"
        "tip->OnAction().Connect([tip]() { tip->Close(); });\n"
        "parent->AddChild(tip); // 先挂载到活动 UI 树\n"
        "tip->ShowAround(targetElement);\n"
        "\n"
        "// 模态气泡（阻断背景点击）\n"
        "tip->SetIsModal(true);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
