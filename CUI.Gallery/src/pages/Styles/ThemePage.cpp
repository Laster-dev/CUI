#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/style/ThemeManager.h"
#include "framework/window/Window.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

int IndexForThemeSource(ThemeSource source) {
    switch (source) {
    case ThemeSource::Light: return 1;
    case ThemeSource::Dark: return 2;
    default: return 0;
    }
}

ThemeSource ThemeSourceFromIndex(int index) {
    switch (index) {
    case 1: return ThemeSource::Light;
    case 2: return ThemeSource::Dark;
    default: return ThemeSource::System;
    }
}

std::string DescribeTheme(ThemeManager& tm, ThemeSource source) {
    return std::string("当前主题：")
        + (tm.GetThemeMode() == ThemeMode::Dark ? "深色" : "浅色")
        + (source == ThemeSource::System ? "（跟随系统）" : "（手动锁定）");
}

Element MakeSwatch(const std::string& name, ThemeTokenId token) {
    auto chip = Container().Size(44.0f, 44.0f).CornerRadius(8.0f);
    chip->BackgroundToken = token;
    chip->BorderToken = ThemeTokenId::CardBorder;
    chip->BorderThickness = 1.0f;
    return Column(4, { chip, MakeLabel(name, 11.0f, ThemeTokenId::TextMuted, false) });
}

} // namespace

Element BuildThemePage() {
    Window* window = Window::Current();
    ThemeManager& tm = ThemeManager::Instance();

    auto status = MakeStatus(DescribeTheme(tm, tm.GetThemeSource()));

    auto mode = SegmentedWidget({ "跟随系统", "浅色", "深色" });
    mode->SetSelectedIndex(IndexForThemeSource(tm.GetThemeSource()));
    mode->OnSelectionChanged().Connect([window, &tm, status](SegmentedControl*, int index, const std::string&) {
        const ThemeSource source = ThemeSourceFromIndex(index);
        if (source == ThemeSource::System) {
            tm.SetThemeSource(ThemeSource::System);
        } else if (window) {
            window->SetThemeMode(source == ThemeSource::Dark ? ThemeMode::Dark : ThemeMode::Light);
        }
        status->Text = DescribeTheme(tm, source);
    });

    // 预览：全部通过 ThemeTokenId 取色，切换主题后随广播自动重绘。
    auto secondaryBtn = ElevatedButton("次要操作");
    secondaryBtn->SetBackgroundToken(ThemeTokenId::CardBackground);
    secondaryBtn->SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
    secondaryBtn->SetPressedBackgroundToken(ThemeTokenId::PressedBackground);
    secondaryBtn->SetBorderToken(ThemeTokenId::CardBorder);
    secondaryBtn->SetBorderThickness(1.0f);
    secondaryBtn->SetColorToken(ThemeTokenId::TextPrimary);

    auto input = TextField("输入内容");
    input->Width = 240.0f;

    auto preview = Column(14, {
        MakeLabel("主题预览", 18.0f, ThemeTokenId::TextPrimary, true),
        MakeLabel("同一棵树在深浅主题下的实时效果。", 13.0f, ThemeTokenId::TextSecondary, false),
        Row(12, { ElevatedButton("主要操作"), secondaryBtn, input }),
        MakeLabel("正文文本 textPrimary", 14.0f, ThemeTokenId::TextPrimary, false),
        MakeLabel("次要说明 textSecondary", 13.0f, ThemeTokenId::TextSecondary, false),
        MakeLabel("辅助信息 textMuted", 12.0f, ThemeTokenId::TextMuted, false),
        Row(16, {
            MakeSwatch("accentColor", ThemeTokenId::AccentColor),
            MakeSwatch("dangerColor", ThemeTokenId::DangerColor),
            MakeSwatch("hoverBackground", ThemeTokenId::HoverBackground),
            MakeSwatch("pressedBackground", ThemeTokenId::PressedBackground),
            MakeSwatch("selectedBackground", ThemeTokenId::SelectedBackground),
        }),
    });

    SamplePageSpec spec;
    spec.title = "Theme(主题样式)";
    spec.subtitle = "浅色 / 深色主题与跟随系统切换，所有配色均通过 ThemeTokenId 动态解析。";
    spec.sections = {
        {
            "主题切换",
            "跟随系统会监听 Windows 深浅模式广播；浅色 / 深色为手动锁定。",
            Column(12, { mode, status }),
        },
        {
            "主题预览",
            "窗口背景、卡片、文字与控件配色随主题整体切换。",
            preview,
        },
    };
    spec.source =
        "// 跟随系统（监听 Windows 广播）\n"
        "ThemeManager::Instance().SetThemeSource(ThemeSource::System);\n"
        "// 手动锁定深浅\n"
        "window->SetThemeMode(ThemeMode::Dark);\n"
        "window->SetThemeMode(ThemeMode::Light);\n"
        "// 控件只声明 Token，不写死颜色\n"
        "control->SetBackgroundToken(ThemeTokenId::CardBackground);\n"
        "control->SetColorToken(ThemeTokenId::TextPrimary);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
