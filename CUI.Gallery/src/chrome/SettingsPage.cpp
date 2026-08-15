#include "chrome/SettingsPage.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/Button.h"
#include "framework/controls/CheckBox.h"
#include "framework/controls/ComboBox.h"
#include "framework/controls/ScrollViewer.h"
#include "framework/controls/ToggleSwitch.h"
#include "framework/style/ThemeTokenId.h"
#include "framework/window/Window.h"
#include "framework/window/WindowBackdrop.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {
namespace {

int IndexForBackdrop(BackdropType type) {
    switch (type) {
    case BackdropType::Mica: return 1;
    case BackdropType::MicaAlt: return 2;
    case BackdropType::Acrylic: return 3;
    default: return 0;
    }
}

BackdropType BackdropFromIndex(int index) {
    switch (index) {
    case 1: return BackdropType::Mica;
    case 2: return BackdropType::MicaAlt;
    case 3: return BackdropType::Acrylic;
    default: return BackdropType::None;
    }
}

} // namespace

Element BuildSettingsPage() {
    Window* window = Window::Current();

    auto btnDark = std::make_shared<Button>("深色");
    auto btnLight = std::make_shared<Button>("浅色");
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
    backdrop->Width = 200.0f;
    backdrop->AddItem("无");
    backdrop->AddItem("云母");
    backdrop->AddItem("云母(Alt)");
    backdrop->AddItem("亚克力");
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

    auto body = Column(20).Padding(24).Children({
        Column(6).Children({
            MakeLabel("设置", 28.0f, ThemeTokenId::TextPrimary, true),
            MakeLabel("主题、窗口背景、动效与诊断。", 14.0f, ThemeTokenId::TextMuted, false),
        }).Build(),
        MakeCard({
            MakeLabel("外观", 15.0f, ThemeTokenId::TextPrimary, true),
            Row(12).Children({ btnDark, btnLight }).Build(),
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
    body->BackgroundToken = ThemeTokenId::WindowBackground;

    auto scroll = std::make_shared<ScrollViewer>();
    scroll->Align = Alignment::Stretch;
    scroll->FlexGrow = 1.0f;
    scroll->BackgroundToken = ThemeTokenId::WindowBackground;
    scroll->AddChild(body);
    return scroll;
}

} // namespace Gallery
