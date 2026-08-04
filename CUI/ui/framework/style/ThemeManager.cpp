#include "ThemeManager.h"
#include <algorithm>
#include <cassert>
#include <vector>

namespace CUI {

namespace {
D2D1_COLOR_F Rgb(int r, int g, int b, float a = 1.0f) {
    return D2D1::ColorF(r / 255.0f, g / 255.0f, b / 255.0f, a);
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
        m_tokens.windowBackground = Rgb(0x1E, 0x1E, 0x1E);
        m_tokens.cardBackground = Rgb(0x25, 0x25, 0x26);
        m_tokens.cardBorder = Rgb(0x3E, 0x3E, 0x42);
        m_tokens.textPrimary = Rgb(0xFF, 0xFF, 0xFF);
        m_tokens.textSecondary = Rgb(0xCC, 0xCC, 0xCC);
        m_tokens.textMuted = Rgb(0x88, 0x88, 0x88);
        m_tokens.titleBarBackground = Rgb(0x1F, 0x1F, 0x1F);
        m_tokens.titleBarText = Rgb(0xD8, 0xD8, 0xD8);
        m_tokens.accentColor = Rgb(0x00, 0x86, 0xF0);
        m_tokens.accentForeground = Rgb(0xFF, 0xFF, 0xFF);
        m_tokens.dangerColor = Rgb(0xC4, 0x2B, 0x1C);
        m_tokens.paneBackground = Rgb(0x25, 0x25, 0x26);
        m_tokens.inputBackground = Rgb(0x2D, 0x2D, 0x2D);
        m_tokens.inputBorder = Rgb(0x45, 0x45, 0x45);
        m_tokens.hoverBackground = Rgb(0x2A, 0x2D, 0x2E);
        m_tokens.pressedBackground = Rgb(0x32, 0x36, 0x38);
        m_tokens.selectedBackground = Rgb(0x09, 0x47, 0x71, 0.85f);
        m_tokens.focusedBorder = Rgb(0x00, 0x86, 0xF0);
        m_tokens.activityBarBackground = Rgb(0x33, 0x33, 0x33);
    } else {
        m_tokens.windowBackground = Rgb(0xF3, 0xF3, 0xF3);
        m_tokens.cardBackground = Rgb(0xFF, 0xFF, 0xFF);
        m_tokens.cardBorder = Rgb(0xE0, 0xE0, 0xE0);
        m_tokens.textPrimary = Rgb(0x1A, 0x1A, 0x1A);
        m_tokens.textSecondary = Rgb(0x50, 0x50, 0x50);
        m_tokens.textMuted = Rgb(0x70, 0x70, 0x70);
        m_tokens.titleBarBackground = Rgb(0xF9, 0xF9, 0xF9);
        m_tokens.titleBarText = Rgb(0x20, 0x20, 0x20);
        m_tokens.accentColor = Rgb(0x00, 0x5F, 0xB8);
        m_tokens.accentForeground = Rgb(0xFF, 0xFF, 0xFF);
        m_tokens.dangerColor = Rgb(0xC4, 0x2B, 0x1C);
        m_tokens.paneBackground = Rgb(0xF8, 0xF8, 0xF8);
        m_tokens.inputBackground = Rgb(0xFF, 0xFF, 0xFF);
        m_tokens.inputBorder = Rgb(0xD1, 0xD1, 0xD1);
        m_tokens.hoverBackground = Rgb(0xEE, 0xEE, 0xEE);
        m_tokens.pressedBackground = Rgb(0xE0, 0xE0, 0xE0);
        m_tokens.selectedBackground = Rgb(0x00, 0x60, 0xC0, 0.22f);
        m_tokens.focusedBorder = Rgb(0x00, 0x5F, 0xB8);
        m_tokens.activityBarBackground = Rgb(0xE8, 0xE8, 0xE8);
    }
}

MaterialRole ThemeManager::GetMaterialRole(const std::string& tokenName) const {
    // Chrome: structural shell that should reveal SystemBackdrop.
    if (tokenName == "windowBackground" ||
        tokenName == "editorBackground" ||
        tokenName == "titleBarBackground" ||
        tokenName == "paneBackground" ||
        tokenName == "sideBarBackground" ||
        tokenName == "activityBarBackground" ||
        tokenName == "tabBarBackground") {
        return MaterialRole::Chrome;
    }

    // Surface: content containers — readable, slightly translucent over chrome.
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
    if (!m_backdropActive) {
        return base;
    }

    const MaterialRole role = GetMaterialRole(tokenName);
    const bool light = (m_mode == ThemeMode::Light);

    // Distinct tints so Mica / MicaAlt / Acrylic are obvious even before blur settles.
    if (role == MaterialRole::Chrome || role == MaterialRole::Surface) {
        switch (m_backdropType) {
        case BackdropType::Mica:
            base.r = light ? base.r * 0.92f + 0.05f : base.r * 0.85f + 0.04f;
            base.g = light ? base.g * 0.94f + 0.04f : base.g * 0.88f + 0.05f;
            base.b = light ? base.b * 0.96f + 0.06f : base.b * 0.92f + 0.08f;
            break;
        case BackdropType::MicaAlt:
            base.r = light ? base.r * 0.95f + 0.04f : base.r * 0.90f + 0.06f;
            base.g = light ? base.g * 0.93f + 0.03f : base.g * 0.86f + 0.04f;
            base.b = light ? base.b * 0.92f + 0.05f : base.b * 0.84f + 0.05f;
            break;
        case BackdropType::Acrylic:
            base.r = light ? base.r * 0.90f + 0.06f : base.r * 0.78f + 0.05f;
            base.g = light ? base.g * 0.92f + 0.05f : base.g * 0.80f + 0.06f;
            base.b = light ? base.b * 0.94f + 0.08f : base.b * 0.88f + 0.10f;
            break;
        default:
            break;
        }
    }

    if (role == MaterialRole::Chrome) {
        // Host canvas stays fully clear so DWM mica fills the gaps.
        if (tokenName == "windowBackground" || tokenName == "editorBackground") {
            base.a = 0.0f;
            return base;
        }
        // Title / pane / activity: tinted glass over SystemBackdrop.
        if (tokenName == "titleBarBackground") {
            base.a = light ? 0.35f : 0.22f;
        } else if (tokenName == "paneBackground" || tokenName == "sideBarBackground") {
            base.a = light ? 0.42f : 0.26f;
        } else {
            base.a = light ? 0.40f : 0.24f;
        }
        return base;
    }

    if (role == MaterialRole::Surface) {
        // Cards/inputs keep readability while still picking up backdrop.
        if (tokenName == "selectedBackground") {
            base.a = (std::min)(base.a, light ? 0.55f : 0.70f);
        } else if (tokenName == "hoverBackground" || tokenName == "pressedBackground") {
            base.a = light ? 0.45f : 0.35f;
        } else if (tokenName == "statusBarBackground") {
            base.a = light ? 0.75f : 0.65f;
        } else {
            base.a = light ? 0.62f : 0.48f;
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
    char buf[16];
    sprintf_s(buf, "#%02X%02X%02X", static_cast<int>(c.r * 255), static_cast<int>(c.g * 255), static_cast<int>(c.b * 255));
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
