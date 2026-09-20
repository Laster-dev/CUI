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

    auto btnDark = CUI::Widgets::Button("深色").Shared();
    auto btnLight = CUI::Widgets::Button("浅色").Shared();
    btnDark->OnClick().Connect([window](UIElement*) {
        if (window) {
                        window->SetThemeMode(ThemeMode::Dark);
        }
    });
    btnLight->OnClick().Connect([window](UIElement*) {
        if (window) {
                        window->SetThemeMode(ThemeMode::Light);
        }
    });

    auto backdrop = std::make_shared<ComboBox>();
        backdrop->SetWidth(200.0f);
    backdrop->AddItem("关闭");
    backdrop->AddItem("自动材质");
    backdrop->AddItem("纯色");
    backdrop->AddItem("云母");
    backdrop->AddItem("云母(Alt)");
    backdrop->AddItem("亚克力");
    backdrop->AddItem("兼容模糊");
    if (window) {
                backdrop->SetSelectedIndex(IndexForBackdrop(window->GetBackdropType()));
    }
    backdrop->OnSelectionChanged().Connect([window](ComboBox*, int index, const std::string&) {
        if (window) {
                        window->SetBackdropType(BackdropFromIndex(index));
        }
    });

    auto anim = std::make_shared<ToggleSwitch>();
        anim->SetHeader("动效");
        anim->SetIsOn(UIElement::AreAnimationsEnabled());
    anim->OnToggled().Connect([](ToggleSwitch*, bool on) {
        UIElement::SetAnimationsEnabled(on);
    });

    auto stats = std::make_shared<CheckBox>("显示渲染统计叠加层");
    if (window && window->IsRenderStatsOverlayVisible()) {
                stats->SetState(CheckState::Checked);
    }
    stats->OnCheckStateChanged().Connect([window](CheckBox*, CheckState state) {
        if (window) {
                        window->SetRenderStatsOverlayVisible(state == CheckState::Checked);
        }
    });

    auto body = Column(20, {
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
        body->SetBackgroundToken(ThemeTokenId::WindowBackground);

    auto scroll = std::make_shared<ScrollViewer>();
        scroll->SetAlign(Alignment::Stretch);
        scroll->SetFlexGrow(1.0f);
        scroll->SetBackgroundToken(ThemeTokenId::WindowBackground);
        scroll->AddChild(body);
    return scroll;
}

} // namespace Gallery




