#include "SettingsView.h"
#include "framework/controls/Button.h"
#include "framework/controls/ToggleSwitch.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/Panel.h"
#include "framework/controls/ScrollViewer.h"
#include "framework/style/ThemeTokenId.h"

using namespace CUI;
using namespace CUI::DSL;

namespace AutoGuard {

SettingsView::SettingsView(Window* window, std::function<void(const std::string&)> onShowToast)
    : m_window(window)
    , m_onShowToast(std::move(onShowToast)) {
}

std::shared_ptr<UIElement> SettingsView::Build() {
    auto window =m_window;
    auto onShowToast = m_onShowToast;

    auto makeSettingCard = [](const std::string& t, const std::string& d, const std::shared_ptr<UIElement>& r) {
        return Row(10.0f, {
            Column(2.0f, {
                Text(t).FontSize(13.0f).FontWeight(FontWeight::Medium).ForegroundToken(ThemeTokenId::TextPrimary),
                Text(d).FontSize(11.0f).ForegroundToken(ThemeTokenId::TextSecondary)
            }).Build(),
            Expanded(Row(0, {}).Build()),
            r
        })
        .Padding(10.0f)
        .CornerRadius(4.0f)
        .BackgroundToken(ThemeTokenId::CardBackground)
        .Build();
    };

    CUI::Widgets::Ref btnDark = CUI::Widgets::Button("🌙 深色模式").Shared();
        btnDark.Height(28.0f);
    btnDark->OnClick().Connect([window](UIElement*) {
        if (window) {
            window->ApplyThemeMode(ThemeMode::Dark);
        }
    });

    CUI::Widgets::Ref btnLight = CUI::Widgets::Button("☀ 浅色模式").Shared();
        btnLight.Height(28.0f);
    btnLight->OnClick().Connect([window](UIElement*) {
        if (window) {
            window->ApplyThemeMode(ThemeMode::Light);
        }
    });

    auto themeRow = Row(6.0f, { btnDark, btnLight }).Build();
    auto themeCard = makeSettingCard("界面外观", "切换深色 / 浅色模式", themeRow);

    CUI::Widgets::Ref lowPerfSwitch = CUI::Widgets::ToggleSwitch().Shared();
    if (window) lowPerfSwitch.IsOn(window->IsLowPerformanceMode());
    lowPerfSwitch->OnToggled().Connect([window, onShowToast](ToggleSwitch*, bool on) {
        if (window) {
                        window->ApplyLowPerformanceMode(on);
            if (onShowToast) onShowToast(on ? "已开启低功耗渲染模式。" : "已关闭低功耗模式。");
        }
    });
    auto lowPerfCard = makeSettingCard("低功耗模式", "降低空闲渲染帧率", lowPerfSwitch);

    auto aboutCard = Column(4.0f, {
        Text("🛡 AutoGuard 启动项安全管理专家 v1.2 Release").FontSize(14.0f).FontWeight(FontWeight::SemiBold).ForegroundToken(ThemeTokenId::TextPrimary),
        Text("基于 CUI Direct2D 纯 C++ 声明式 UI 引擎研发，面向 Windows 10/11 打造。").FontSize(12.0f).ForegroundToken(ThemeTokenId::TextSecondary)
    })
    .Padding(12.0f)
    .CornerRadius(4.0f)
    .BackgroundToken(ThemeTokenId::CardBackground)
    .Build();

    auto content = Column(10.0f, {
        Text("设置与关于").FontSize(18.0f).FontWeight(FontWeight::SemiBold).ForegroundToken(ThemeTokenId::TextPrimary),
        themeCard,
        lowPerfCard,
        aboutCard
    })
    .Padding(12.0f)
    .BackgroundToken(ThemeTokenId::WindowBackground)
    .Build();

    CUI::Widgets::Ref scroll = CUI::Widgets::ScrollViewer().Shared();
        scroll.Align(Alignment::Stretch);
        scroll.FlexGrow(1.0f);
        scroll->AddChild(content);
    return scroll;
}

} // namespace AutoGuard
