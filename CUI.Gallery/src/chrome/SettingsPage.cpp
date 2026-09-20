#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "Gallery.h"
#include "framework/core/Widgets.h"
#include "chrome/SettingsPage.h"


using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {
namespace {

int IndexForBackdrop(BackdropType type) {
    switch (type) {
    case BackdropType::Auto: return 1;
    case BackdropType::Solid: return 2;
    case BackdropType::Mica: return 3;
    case BackdropType::MicaAlt: return 4;
    case BackdropType::Acrylic: return 5;
    case BackdropType::Blur: return 6;
    default: return 0;
    }
}

BackdropType BackdropFromIndex(int index) {
    switch (index) {
    case 1: return BackdropType::Auto;
    case 2: return BackdropType::Solid;
    case 3: return BackdropType::Mica;
    case 4: return BackdropType::MicaAlt;
    case 5: return BackdropType::Acrylic;
    case 6: return BackdropType::Blur;
    default: return BackdropType::None;
    }
}

} // namespace

Element BuildSettingsPage() {
    Window* window = Window::Current();

    CUI::Widgets::Ref btnDark = CUI::Widgets::Button("深色").Shared();
    CUI::Widgets::Ref btnLight = CUI::Widgets::Button("浅色").Shared();
    btnDark->OnClick().Connect([window](UIElement*) {
        if (window) {
                        window->ApplyThemeMode(ThemeMode::Dark);
        }
    });
    btnLight->OnClick().Connect([window](UIElement*) {
        if (window) {
                        window->ApplyThemeMode(ThemeMode::Light);
        }
    });

    CUI::Widgets::Ref backdrop = CUI::Widgets::ComboBox().Shared();
        backdrop.Width(200.0f);
    backdrop->AddItem("关闭");
    backdrop->AddItem("自动材质");
    backdrop->AddItem("纯色");
    backdrop->AddItem("云母");
    backdrop->AddItem("云母(Alt)");
    backdrop->AddItem("亚克力");
    backdrop->AddItem("兼容模糊");
    if (window) {
                backdrop.SelectedIndex(IndexForBackdrop(window->GetBackdropType()));
    }
    backdrop->OnSelectionChanged().Connect([window](ComboBox*, int index, const std::string&) {
        if (window) {
                        window->ApplyBackdropType(BackdropFromIndex(index));
        }
    });

    CUI::Widgets::Ref anim = CUI::Widgets::ToggleSwitch().Shared();
        anim.Header("动效");
        anim.IsOn(UIElement::AreAnimationsEnabled());
    anim->OnToggled().Connect([](ToggleSwitch*, bool on) {
        UIElement::ApplyAnimationsEnabled(on);
    });

    CUI::Widgets::Ref stats = CUI::Widgets::CheckBox("显示渲染统计叠加层").Shared();
    if (window && window->IsRenderStatsOverlayVisible()) {
                stats.State(CheckState::Checked);
    }
    stats->OnCheckStateChanged().Connect([window](CheckBox*, CheckState state) {
        if (window) {
                        window->ApplyRenderStatsOverlayVisible(state == CheckState::Checked);
        }
    });

    CUI::Widgets::Ref body =Column(20, {
        Column(6, {
            MakeLabel("设置", 28.0f, ThemeTokenId::TextPrimary, true),
            MakeLabel("主题、窗口背景、动效与诊断。", 14.0f, ThemeTokenId::TextMuted, false),
        }),
        MakeCard({
            MakeLabel("外观", 15.0f, ThemeTokenId::TextPrimary, true),
            Row(12, { btnDark, btnLight }),
            MakeLabel("背景材质", 13.0f, ThemeTokenId::TextSecondary, false),
            backdrop,
        }, 12.0f),
        MakeCard({
            MakeLabel("动效", 15.0f, ThemeTokenId::TextPrimary, true),
            anim,
        }, 12.0f),
        MakeCard({
            MakeLabel("诊断", 15.0f, ThemeTokenId::TextPrimary, true),
            stats,
        }, 12.0f),
    }).Build();
        body.BackgroundToken(ThemeTokenId::WindowBackground);

    CUI::Widgets::Ref scroll = CUI::Widgets::ScrollViewer().Shared();
        scroll.Align(Alignment::Stretch);
        scroll.FlexGrow(1.0f);
        scroll.BackgroundToken(ThemeTokenId::WindowBackground);
        scroll->AddChild(body);
    return scroll;
}

} // namespace Gallery




