#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 CUI::Widgets:: 句柄同名冲突
#endif
#include "Gallery.h"
#include "framework/core/Widgets.h"
using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildTeachingTipPage() {
    auto status = MakeStatus("点击按钮显示指引气泡，此处显示操作结果。");

    // ── 1. 基础气泡 ─────────────────────────────────────────────────────
    auto btnBasic = Button("显示基础 TeachingTip")
        .OnClick([status](UIElement* src) {
            CUI::Widgets::Ref tip = CUI::Widgets::TeachingTip().Shared();
            tip.Title("欢迎使用 CUI 框架");
            tip.Message("这是一个 TeachingTip 气泡指引示例。它会自动停靠在目标控件旁边，并带有小三角箭头指示来源。");
            tip.ActionText("我知道了");
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
    auto btnCloseOnly = Button("仅关闭按钮");
        btnCloseOnly->SetBackgroundToken(ThemeTokenId::CardBackground);
    
        btnCloseOnly->SetBorderToken(ThemeTokenId::CardBorder);
        btnCloseOnly->SetBorderThickness(1.0f);
    btnCloseOnly->OnClick().Connect([status](UIElement* src) {
        CUI::Widgets::Ref tip = CUI::Widgets::TeachingTip().Shared();
        tip.Title("提示");
        tip.Message("此提示没有操作按钮，只有右上角的关闭叉可以消退。");
        tip.IsCloseVisible(true);
        tip->OnClosed().Connect([status]() {
            status->Text = "已通过关闭按钮消退气泡。";
        });
                src->AddChild(tip);
        tip->ShowAround(src);
    });

    // ── 3. 不同停靠方向 ─────────────────────────────────────────────────
    auto btnPlacementTop    = Button("↑ Top");
    auto btnPlacementBottom = Button("↓ Bottom");
    auto btnPlacementLeft   = Button("← Left");
    auto btnPlacementRight  = Button("→ Right");

        btnPlacementTop->SetWidth(90.0f);
        btnPlacementBottom->SetWidth(90.0f);
        btnPlacementLeft->SetWidth(90.0f);
        btnPlacementRight->SetWidth(90.0f);

    auto makeDirectionTip = [](UIElement* src, BubblePlacement p, const std::string& label) {
        CUI::Widgets::Ref tip = CUI::Widgets::TeachingTip().Shared();
                tip.Title(label);
        tip.Message("气泡将优先显示在此方向，边界受限时自动回退。");
        tip.ActionText("关闭");
        tip.PreferredPlacement(p);
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
    auto btnModal = Button("模态指引气泡")
        .Background(Color::Hex("#1565C0"))
        .HoverBackground(Color::Hex("#0D47A1"))
        .PressedBackground(Color::Hex("#082C6E"))
        .Foreground(Color::White)
        .OnClick([status](UIElement* src) {
            CUI::Widgets::Ref tip = CUI::Widgets::TeachingTip().Shared();
            tip.Title("新功能介绍：智能搜索");
            tip.Message("使用顶部搜索栏可快速定位任意控件或页面。\n支持拼音首字母检索，按 Ctrl+K 随时唤出。");
            tip.ActionText("立即体验");
            tip.IsModal(true);
            tip.PreferredPlacement(BubblePlacement::Auto);
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
    auto btnAuto = Button("自动停靠（Auto）");
        btnAuto->SetBackgroundToken(ThemeTokenId::CardBackground);
    
        btnAuto->SetBorderToken(ThemeTokenId::CardBorder);
        btnAuto->SetBorderThickness(1.0f);
    btnAuto->OnClick().Connect([status](UIElement* src) {
        CUI::Widgets::Ref tip = CUI::Widgets::TeachingTip().Shared();
        tip.Title("自动方位（Auto）");
        tip.Message("当 PreferredPlacement 设置为 Auto 时，框架根据目标控件的位置和视口空间自动选择最优停靠方向，确保气泡始终可见。");
        tip.ActionText("明白了");
        tip.PreferredPlacement(BubblePlacement::Auto);
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
            "调用 ShowAround(target) 将气泡显示在目标控件附近，.ActionText() 设置操作按钮文字，OnAction() 回调处理点击。",
            Column(12, {
                Row(10, {btnBasic, btnCloseOnly }),
                status,
            }),
        },
        {
            "停靠方向 PreferredPlacement",
            "通过 .PreferredPlacement() 指定箭头优先指向的方位；框架在视口边界受限时自动回退到最优位置。",
            Row(10, {
                btnPlacementTop, btnPlacementBottom, btnPlacementLeft, btnPlacementRight
            }),
        },
        {
            "模态 & 自动停靠",
            ".IsModal(true) 开启模态：气泡显示期间背景区域无法响应鼠标点击，适合关键功能首次引导。\n"
            ".PreferredPlacement(Auto) 让框架根据空间自动选择最佳停靠方向。",
            Row(12, { btnModal, btnAuto }),
        },
    };
    spec.source =
        "auto tip = CUI::Widgets::TeachingTip().Shared();\n"
        "tip.Title(\"标题\");\n"
        "tip.Message(\"详细说明文字。\");\n"
        "tip.ActionText(\"我知道了\");\n"
        "tip.PreferredPlacement(BubblePlacement::Bottom);\n"
        "tip->OnAction().Connect([tip]() { tip->Close(); });\n"
        "parent->AddChild(tip); // 先挂载到活动 UI 树\n"
        "tip->ShowAround(targetElement);\n"
        "\n"
        "// 模态气泡（阻断背景点击）\n"
        "tip.IsModal(true);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery




