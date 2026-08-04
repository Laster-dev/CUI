#include "ThemeManager.h"
#include <algorithm>
#include <cassert>
#include <vector>

namespace CUI {

namespace {
D2D1_COLOR_F Rgb(int r, int g, int b, float a = 1.0f) {
    return D2D1::ColorF(r / 255.0f, g / 255.0f, b / 255.0f, a);
}

// WinUI Color is often #AARRGGBB. Helpers match Common_themeresources Light/Default.
D2D1_COLOR_F Argb(int a, int r, int g, int b) {
    return D2D1::ColorF(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}
} // namespace

ThemeManager& ThemeManager::Instance() {
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager() {
    UpdateTokens();
}

void ThemeManager::SetThemeMode(ThemeMode mode) {
    if (m_mode != mode) {
        m_mode = mode;
        UpdateTokens();
    }
}

void ThemeManager::SetBackdropActive(bool active) {
    m_backdropActive = active;
    if (!active) {
        m_backdropType = BackdropType::None;
    }
}

void ThemeManager::SetBackdropType(BackdropType type) {
    m_backdropType = type;
    m_backdropActive = (type != BackdropType::None);
}

void ThemeManager::UpdateTokens() {
    if (m_mode == ThemeMode::Dark) {
        // WinUI Default (Dark)
        m_tokens.windowBackground = Rgb(0x20, 0x20, 0x20);       // SolidBackgroundFillColorBase
        m_tokens.cardBackground = Rgb(0x2C, 0x2C, 0x2C);
        m_tokens.cardBorder = Rgb(0x3F, 0x3F, 0x3F);
        m_tokens.textPrimary = Rgb(0xFF, 0xFF, 0xFF);            // TextFillColorPrimary
        m_tokens.textSecondary = Rgb(0xC5, 0xC5, 0xC5);
        m_tokens.textMuted = Rgb(0x87, 0x87, 0x87);
        m_tokens.titleBarBackground = Rgb(0x20, 0x20, 0x20);
        m_tokens.titleBarText = Rgb(0xFF, 0xFF, 0xFF);
        m_tokens.accentColor = Rgb(0x00, 0x86, 0xF0);
        m_tokens.accentForeground = Rgb(0xFF, 0xFF, 0xFF);
        m_tokens.dangerColor = Rgb(0xC4, 0x2B, 0x1C);
        m_tokens.paneBackground = Rgb(0x2C, 0x2C, 0x2C);
        m_tokens.inputBackground = Rgb(0x2D, 0x2D, 0x2D);
        m_tokens.inputBorder = Rgb(0x45, 0x45, 0x45);
        m_tokens.hoverBackground = Rgb(0x2A, 0x2D, 0x2E);
        m_tokens.pressedBackground = Rgb(0x32, 0x36, 0x38);
        m_tokens.selectedBackground = Rgb(0x09, 0x47, 0x71, 0.85f);
        m_tokens.focusedBorder = Rgb(0x00, 0x86, 0xF0);
        m_tokens.activityBarBackground = Rgb(0x1C, 0x1C, 0x1C);
    } else {
        // WinUI Light — Common_themeresources_any.xaml Light dictionary
        m_tokens.windowBackground = Rgb(0xF3, 0xF3, 0xF3);       // SolidBackgroundFillColorBase
        m_tokens.cardBackground = Rgb(0xFF, 0xFF, 0xFF);         // Card / quarternary surface
        m_tokens.cardBorder = Rgb(0xEB, 0xEB, 0xEB);             // CardStrokeColorDefaultSolid
        m_tokens.textPrimary = Rgb(0x1A, 0x1A, 0x1A);           // TextFillColorPrimary (opaque for D2D)
        m_tokens.textSecondary = Rgb(0x5C, 0x5C, 0x5C);         // ~TextFillColorSecondary #9E000000 on white
        m_tokens.textMuted = Rgb(0x75, 0x75, 0x75);             // ~TextFillColorTertiary
        m_tokens.titleBarBackground = Rgb(0xF9, 0xF9, 0xF9);     // SolidBackgroundFillColorTertiary
        m_tokens.titleBarText = Rgb(0x1A, 0x1A, 0x1A);
        m_tokens.accentColor = Rgb(0x00, 0x5F, 0xB8);            // SystemAccentColorDark1
        m_tokens.accentForeground = Rgb(0xFF, 0xFF, 0xFF);       // TextOnAccentFillColorPrimary
        m_tokens.dangerColor = Rgb(0xC4, 0x2B, 0x1C);             // SystemFillColorCritical
        m_tokens.paneBackground = Rgb(0xF3, 0xF3, 0xF3);
        m_tokens.inputBackground = Rgb(0xFF, 0xFF, 0xFF);
        m_tokens.inputBorder = Rgb(0xD1, 0xD1, 0xD1);
        m_tokens.hoverBackground = Rgb(0xF5, 0xF5, 0xF5);        // SubtleFill ≈ black@3.5% on white
        m_tokens.pressedBackground = Rgb(0xF0, 0xF0, 0xF0);
        // Nav selection: light gray fill (WinUI SubtleFill), keeps primary text dark
        m_tokens.selectedBackground = Rgb(0xE8, 0xE8, 0xE8);
        m_tokens.focusedBorder = Rgb(0x00, 0x5F, 0xB8);
        m_tokens.activityBarBackground = Rgb(0xEE, 0xEE, 0xEE);
    }
}

MaterialRole ThemeManager::GetMaterialRole(const std::string& tokenName) const {
    if (tokenName == "windowBackground" ||
        tokenName == "editorBackground" ||
        tokenName == "titleBarBackground" ||
        tokenName == "paneBackground" ||
        tokenName == "sideBarBackground" ||
        tokenName == "activityBarBackground" ||
        tokenName == "tabBarBackground") {
        return MaterialRole::Chrome;
    }

    if (tokenName == "cardBackground" ||
        tokenName == "inputBackground" ||
        tokenName == "hoverBackground" ||
        tokenName == "pressedBackground" ||
        tokenName == "selectedBackground" ||
        tokenName == "statusBarBackground") {
        return MaterialRole::Surface;
    }

    return MaterialRole::Solid;
}

D2D1_COLOR_F ThemeManager::ApplyMaterialRole(const std::string& tokenName, D2D1_COLOR_F base) const {
    if (!m_backdropActive || m_backdropType == BackdropType::None) {
        return base;
    }

    const MaterialRole role = GetMaterialRole(tokenName);
    const bool light = (m_mode == ThemeMode::Light);
    const bool acrylic = (m_backdropType == BackdropType::Acrylic);
    const bool micaAlt = (m_backdropType == BackdropType::MicaAlt);

    if (role == MaterialRole::Chrome) {
        // 宿主画布全透明 —— 页面/编辑器空隙必须露出 DWM，否则材质永远看不见。
        if (tokenName == "windowBackground" || tokenName == "editorBackground") {
            base.a = 0.0f;
            return base;
        }

        if (light) {
            base.r = 1.0f;
            base.g = 1.0f;
            base.b = 1.0f;
            if (micaAlt) {
                base.r = 0.94f; base.g = 0.96f; base.b = 1.0f;
            } else if (acrylic) {
                base.r = 0.90f; base.g = 0.94f; base.b = 0.99f;
            }
            const float titleA = acrylic ? 0.12f : (micaAlt ? 0.16f : 0.22f);
            const float paneA = acrylic ? 0.14f : (micaAlt ? 0.18f : 0.24f);
            base.a = (tokenName == "titleBarBackground") ? titleA : paneA;
            return base;
        }

        // 深色玻璃：必须足够透，否则在深色壁纸上像「死黑底」
        if (micaAlt) {
            base.r = 0.10f; base.g = 0.10f; base.b = 0.12f;
        } else if (acrylic) {
            base.r = 0.08f; base.g = 0.10f; base.b = 0.14f;
        } else {
            base.r = 0.12f; base.g = 0.12f; base.b = 0.12f;
        }
        const float titleA = acrylic ? 0.16f : (micaAlt ? 0.20f : 0.26f);
        const float paneA = acrylic ? 0.18f : (micaAlt ? 0.22f : 0.28f);
        base.a = (tokenName == "titleBarBackground") ? titleA : paneA;
        return base;
    }

    if (role == MaterialRole::Surface) {
        if (tokenName == "selectedBackground" ||
            tokenName == "hoverBackground" ||
            tokenName == "pressedBackground") {
            return base;
        }
        if (light) {
            base.r = 1.0f;
            base.g = 1.0f;
            base.b = 1.0f;
            if (tokenName == "statusBarBackground") {
                base.a = 0.72f;
            } else if (tokenName == "inputBackground") {
                base.a = acrylic ? 0.55f : 0.70f;
            } else {
                // Demo / 卡片：能透出底下云母色调
                base.a = acrylic ? 0.55f : 0.68f;
            }
            return base;
        }

        if (tokenName == "statusBarBackground") {
            base.a = 0.65f;
        } else if (tokenName == "inputBackground") {
            base.r = 0.18f; base.g = 0.18f; base.b = 0.18f;
            base.a = acrylic ? 0.45f : 0.58f;
        } else {
            base.r = 0.16f; base.g = 0.16f; base.b = 0.16f;
            base.a = acrylic ? 0.42f : 0.55f;
        }
        return base;
    }

    return base;
}

D2D1_COLOR_F ThemeManager::LookupBaseColor(const std::string& tokenName) const {
    if (tokenName == "windowBackground") return m_tokens.windowBackground;
    if (tokenName == "cardBackground") return m_tokens.cardBackground;
    if (tokenName == "cardBorder") return m_tokens.cardBorder;
    if (tokenName == "textPrimary") return m_tokens.textPrimary;
    if (tokenName == "textSecondary") return m_tokens.textSecondary;
    if (tokenName == "textMuted") return m_tokens.textMuted;
    if (tokenName == "titleBarBackground") return m_tokens.titleBarBackground;
    if (tokenName == "titleBarText") return m_tokens.titleBarText;
    if (tokenName == "accentColor") return m_tokens.accentColor;
    if (tokenName == "accentForeground") return m_tokens.accentForeground;
    if (tokenName == "selectedBackground") return m_tokens.selectedBackground;
    if (tokenName == "dangerColor") return m_tokens.dangerColor;
    if (tokenName == "paneBackground") return m_tokens.paneBackground;
    if (tokenName == "inputBackground") return m_tokens.inputBackground;
    if (tokenName == "inputBorder") return m_tokens.inputBorder;
    if (tokenName == "hoverBackground") return m_tokens.hoverBackground;
    if (tokenName == "pressedBackground") return m_tokens.pressedBackground;
    if (tokenName == "focusedBorder") return m_tokens.focusedBorder;
    if (tokenName == "activityBarBackground") return m_tokens.activityBarBackground;
    if (tokenName == "sideBarBackground") return m_tokens.paneBackground;
    if (tokenName == "editorBackground") return m_tokens.windowBackground;
    if (tokenName == "statusBarBackground") return m_tokens.accentColor;
    if (tokenName == "tabBarBackground") return m_tokens.paneBackground;

    assert(!"Unknown theme color token");
    return D2D1::ColorF(1.0f, 0.0f, 1.0f, 1.0f);
}

D2D1_COLOR_F ThemeManager::GetColor(const std::string& tokenName) const {
    return ApplyMaterialRole(tokenName, LookupBaseColor(tokenName));
}

std::string ThemeManager::GetColorHex(const std::string& tokenName) const {
    D2D1_COLOR_F c = GetColor(tokenName);
    // Premul-aware opaque hex for CSS-like APIs (ignore alpha).
    const float a = (std::max)(c.a, 0.001f);
    const int r = static_cast<int>(std::clamp(c.r / a, 0.0f, 1.0f) * 255.0f + 0.5f);
    const int g = static_cast<int>(std::clamp(c.g / a, 0.0f, 1.0f) * 255.0f + 0.5f);
    const int b = static_cast<int>(std::clamp(c.b / a, 0.0f, 1.0f) * 255.0f + 0.5f);
    char buf[16];
    sprintf_s(buf, "#%02X%02X%02X", r, g, b);
    return std::string(buf);
}

const std::vector<std::string>& ThemeManager::GetTokenNames() {
    static const std::vector<std::string> names = {
        "windowBackground",
        "cardBackground",
        "cardBorder",
        "textPrimary",
        "textSecondary",
        "textMuted",
        "titleBarBackground",
        "titleBarText",
        "accentColor",
        "accentForeground",
        "dangerColor",
        "paneBackground",
        "inputBackground",
        "inputBorder",
        "hoverBackground",
        "pressedBackground",
        "focusedBorder",
        "activityBarBackground",
        "sideBarBackground",
        "editorBackground",
        "statusBarBackground",
        "tabBarBackground",
        "selectedBackground"
    };
    return names;
}

} // namespace CUI
