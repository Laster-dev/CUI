#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "Gallery.h"
#include "framework/core/Widgets.h"
#include <format>

using namespace CUI;
using namespace CUI::DSL;
using CUI::DSL::Fluent::Button;

namespace Gallery {

namespace {

Element MakeColorChip(const std::string& name, ThemeTokenId token) {
    auto chip =Container().Size(48.0f, 48.0f).CornerRadius(8.0f);
        chip.BackgroundToken(token);
        chip.BorderToken(ThemeTokenId::CardBorder);
        chip.BorderThickness(1.0f);
    return Column(4, {
        chip,
        MakeLabel(name, 11.0f, ThemeTokenId::TextMuted, false)
    }).Build();
}

} // namespace

Element BuildThemeTransitionPage() {
    ThemeManager& tm = ThemeManager::Instance();

    auto statusTheme = MakeStatus(
        std::string("当前系统主题：") + (tm.GetThemeMode() == ThemeMode::Dark ? "深色模式 (Dark)" : "浅色模式 (Light)"));

    auto triggerTransition = [statusTheme](Point origin) {
        if (Window* win = Window::Current()) {
            const ThemeMode nextMode = (ThemeManager::Instance().GetThemeMode() == ThemeMode::Dark)
                ? ThemeMode::Light
                : ThemeMode::Dark;
                        win->ApplyThemeModeWithRipple(nextMode, origin);
            statusTheme->Text = std::string("当前系统主题：")
                + (nextMode == ThemeMode::Dark ? "深色模式 (Dark)" : "浅色模式 (Light)")
                + " [波纹扩散已完成]";
        }
    };

    // 1. 多方位波纹触发按钮
    auto btnCenter = Button("从屏幕中心扩散切换 (Center)")
        .OnClick([triggerTransition](UIElement*) {
            if (Window* win = Window::Current()) {
                const Size sz = win->GetLogicalClientSize();
                triggerTransition(Point(sz.width * 0.5f, sz.height * 0.5f));
            } else {
                triggerTransition(Point(500.0f, 350.0f));
            }
        });

    auto btnTopLeft = Button("左上角 (Top-Left)")
        .BackgroundToken(ThemeTokenId::CardBackground)
        .ForegroundToken(ThemeTokenId::TextPrimary)
        .BorderToken(ThemeTokenId::CardBorder)
        .BorderThickness(1.0f)
        .OnClick([triggerTransition](UIElement*) {
            triggerTransition(Point(0.0f, 0.0f));
        });

    auto btnTopRight = Button("右上角 (Top-Right)")
        .BackgroundToken(ThemeTokenId::CardBackground)
        .ForegroundToken(ThemeTokenId::TextPrimary)
        .BorderToken(ThemeTokenId::CardBorder)
        .BorderThickness(1.0f)
        .OnClick([triggerTransition](UIElement*) {
            if (Window* win = Window::Current()) {
                triggerTransition(Point(win->GetLogicalClientSize().width, 0.0f));
            } else {
                triggerTransition(Point(1000.0f, 0.0f));
            }
        });

    auto btnBottomLeft = Button("左下角 (Bottom-Left)")
        .BackgroundToken(ThemeTokenId::CardBackground)
        .ForegroundToken(ThemeTokenId::TextPrimary)
        .BorderToken(ThemeTokenId::CardBorder)
        .BorderThickness(1.0f)
        .OnClick([triggerTransition](UIElement*) {
            if (Window* win = Window::Current()) {
                triggerTransition(Point(0.0f, win->GetLogicalClientSize().height));
            } else {
                triggerTransition(Point(0.0f, 700.0f));
            }
        });

    auto btnBottomRight = Button("右下角 (Bottom-Right)")
        .BackgroundToken(ThemeTokenId::CardBackground)
        .ForegroundToken(ThemeTokenId::TextPrimary)
        .BorderToken(ThemeTokenId::CardBorder)
        .BorderThickness(1.0f)
        .OnClick([triggerTransition](UIElement*) {
            if (Window* win = Window::Current()) {
                const Size sz = win->GetLogicalClientSize();
                triggerTransition(Point(sz.width, sz.height));
            } else {
                triggerTransition(Point(1000.0f, 700.0f));
            }
        });

    // 2. 混合组件沙盒
    auto sampleBtn = Button("主要操作 (Primary)");
    auto sampleSecondary = Button("次要操作 (Secondary)")
        .BackgroundToken(ThemeTokenId::CardBackground)
        .ForegroundToken(ThemeTokenId::TextPrimary)
        .BorderToken(ThemeTokenId::CardBorder)
        .BorderThickness(1.0f);
    CUI::Widgets::Ref sampleSlider = Widgets::Slider().Shared();
    sampleSlider.Minimum(0.0f);
    sampleSlider.Maximum(100.0f);
    sampleSlider.Value(65.0f);
        sampleSlider.Width(180.0f);
    CUI::Widgets::Ref sampleToggle = Widgets::ToggleSwitch().Header("开关状态").IsOn(true).Shared();
    CUI::Widgets::Ref sampleCheck = Widgets::CheckBox("记住配置 (Remember)").Shared();
    CUI::Widgets::Ref sampleInput = Widgets::TextBox().Text("输入测试文本..").Shared();
        sampleInput.Width(200.0f);
    CUI::Widgets::Ref sampleBar = Widgets::ProgressBar().Shared();
    sampleBar.Value(60.0f);
    sampleBar.IsIndeterminate(false);
        sampleBar.Height(6.0f);
        sampleBar.Align(Alignment::Stretch);

    auto sandboxCard =Container().Padding(16.0f).CornerRadius(12.0f);
        sandboxCard.BackgroundToken(ThemeTokenId::CardBackground);
        sandboxCard.BorderToken(ThemeTokenId::CardBorder);
        sandboxCard.BorderThickness(1.0f);
        sandboxCard->AddChild(Column(14, {
        MakeLabel("主题响应沙盒 (Theme Live Sandbox)", 16.0f, ThemeTokenId::TextPrimary, true),
        MakeLabel("波纹扫过时，所有 Token 配色将随 Direct2D 裁剪圆圈平滑刷新，无闪烁无重叠。", 12.0f, ThemeTokenId::TextSecondary, false),
        Row(12, { sampleBtn, sampleSecondary, sampleToggle, sampleCheck }),
        Row(14, { sampleInput, sampleSlider }),
        sampleBar,
    }).Build());

    // 3. 核心 Token 调色板预览
    auto paletteRow1 = Row(16, {
        MakeColorChip("TextPrimary", ThemeTokenId::TextPrimary),
        MakeColorChip("TextSecondary", ThemeTokenId::TextSecondary),
        MakeColorChip("TextMuted", ThemeTokenId::TextMuted),
        MakeColorChip("AccentColor", ThemeTokenId::AccentColor),
        MakeColorChip("DangerColor", ThemeTokenId::DangerColor),
    });

    auto paletteRow2 = Row(16, {
        MakeColorChip("CardBg", ThemeTokenId::CardBackground),
        MakeColorChip("CardBorder", ThemeTokenId::CardBorder),
        MakeColorChip("HoverBg", ThemeTokenId::HoverBackground),
        MakeColorChip("PressedBg", ThemeTokenId::PressedBackground),
        MakeColorChip("SelectedBg", ThemeTokenId::SelectedBackground),
    });

    SamplePageSpec spec;
    spec.title = "Theme Transition (主题平滑波纹过渡)";
    spec.subtitle = "基于 Direct2D 离屏快照与径向圆形遮罩剪裁（Radial Wave Ripple）的现代深浅主题平滑过渡动效。";
    spec.sections = {
        {
            "波纹扩散切换触发矩阵",
            "点击不同方位的按钮，观察主题切换波纹从该物理原点向全窗口优雅扩散开来的视觉动效（380ms EaseOutCubic 曲线）。",
            Column(12, {
                statusTheme,
                btnCenter,
                Row(12, { btnTopLeft, btnTopRight, btnBottomLeft, btnBottomRight }),
            }),
        },
        {
            "主题过渡沙盒 (Live Sandbox)",
            "沙盒内的所有控件均完全绑定 ThemeTokenId。波纹穿过界面瞬间，控件背景、文字、高亮及滑槽色彩无缝更新。",
            sandboxCard,
        },
        {
            "系统全局色彩 Token 实时看板",
            "展示当前主题模式下核心色彩 Token 的视觉呈现。",
            Column(12, {
                paletteRow1,
                paletteRow2,
            }),
        },
    };
    spec.source =
        "// 触发从指定点开始的径向波纹扩散主题切换\n"
        "Point clickOrigin(500.0f, 350.0f);\n"
        "Window::Current()->Fluent().Theme(ThemeMode::Dark).Apply();\n";

    return BuildSamplePage(spec);
}

} // namespace Gallery




