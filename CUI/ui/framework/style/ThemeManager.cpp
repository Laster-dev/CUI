#include "ThemeManager.h"

namespace CUI {

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

void ThemeManager::UpdateTokens() {
    if (m_mode == ThemeMode::Dark) {
        m_tokens.windowBackground = D2D1::ColorF(0x1E / 255.0f, 0x1E / 255.0f, 0x1E / 255.0f, 1.0f);
        m_tokens.cardBackground   = D2D1::ColorF(0x25 / 255.0f, 0x25 / 255.0f, 0x26 / 255.0f, 1.0f);
        m_tokens.cardBorder       = D2D1::ColorF(0x3E / 255.0f, 0x3E / 255.0f, 0x42 / 255.0f, 1.0f);
        m_tokens.textPrimary      = D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f);
        m_tokens.textSecondary    = D2D1::ColorF(0xCC / 255.0f, 0xCC / 255.0f, 0xCC / 255.0f, 1.0f);
        m_tokens.textMuted        = D2D1::ColorF(0x88 / 255.0f, 0x88 / 255.0f, 0x88 / 255.0f, 1.0f);
        m_tokens.titleBarBackground = D2D1::ColorF(0x1F / 255.0f, 0x1F / 255.0f, 0x1F / 255.0f, 1.0f);
        m_tokens.titleBarText     = D2D1::ColorF(0xD8 / 255.0f, 0xD8 / 255.0f, 0xD8 / 255.0f, 1.0f);
        m_tokens.accentColor      = D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f);
        m_tokens.paneBackground   = D2D1::ColorF(0x25 / 255.0f, 0x25 / 255.0f, 0x26 / 255.0f, 1.0f);
        m_tokens.inputBackground  = D2D1::ColorF(0x2D / 255.0f, 0x2D / 255.0f, 0x2D / 255.0f, 1.0f);
        m_tokens.inputBorder     = D2D1::ColorF(0x45 / 255.0f, 0x45 / 255.0f, 0x45 / 255.0f, 1.0f);
    } else {
        m_tokens.windowBackground = D2D1::ColorF(0xF3 / 255.0f, 0xF3 / 255.0f, 0xF3 / 255.0f, 1.0f);
        m_tokens.cardBackground   = D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f);
        m_tokens.cardBorder       = D2D1::ColorF(0xE0 / 255.0f, 0xE0 / 255.0f, 0xE0 / 255.0f, 1.0f);
        m_tokens.textPrimary      = D2D1::ColorF(0x1A / 255.0f, 0x1A / 255.0f, 0x1A / 255.0f, 1.0f);
        m_tokens.textSecondary    = D2D1::ColorF(0x50 / 255.0f, 0x50 / 255.0f, 0x50 / 255.0f, 1.0f);
        m_tokens.textMuted        = D2D1::ColorF(0x70 / 255.0f, 0x70 / 255.0f, 0x70 / 255.0f, 1.0f);
        m_tokens.titleBarBackground = D2D1::ColorF(0xF9 / 255.0f, 0xF9 / 255.0f, 0xF9 / 255.0f, 1.0f);
        m_tokens.titleBarText     = D2D1::ColorF(0x20 / 255.0f, 0x20 / 255.0f, 0x20 / 255.0f, 1.0f);
        m_tokens.accentColor      = D2D1::ColorF(0x00 / 255.0f, 0x5F / 255.0f, 0xB8 / 255.0f, 1.0f);
        m_tokens.paneBackground   = D2D1::ColorF(0xF8 / 255.0f, 0xF8 / 255.0f, 0xF8 / 255.0f, 1.0f);
        m_tokens.inputBackground  = D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f);
        m_tokens.inputBorder     = D2D1::ColorF(0xD1 / 255.0f, 0xD1 / 255.0f, 0xD1 / 255.0f, 1.0f);
    }
}

D2D1_COLOR_F ThemeManager::GetColor(const std::string& tokenName) const {
    if (tokenName == "windowBackground") return m_tokens.windowBackground;
    if (tokenName == "cardBackground") return m_tokens.cardBackground;
    if (tokenName == "cardBorder") return m_tokens.cardBorder;
    if (tokenName == "textPrimary") return m_tokens.textPrimary;
    if (tokenName == "textSecondary") return m_tokens.textSecondary;
    if (tokenName == "textMuted") return m_tokens.textMuted;
    if (tokenName == "titleBarBackground") return m_tokens.titleBarBackground;
    if (tokenName == "titleBarText") return m_tokens.titleBarText;
    if (tokenName == "accentColor") return m_tokens.accentColor;
    if (tokenName == "paneBackground") return m_tokens.paneBackground;
    if (tokenName == "inputBackground") return m_tokens.inputBackground;
    if (tokenName == "inputBorder") return m_tokens.inputBorder;
    return m_tokens.textPrimary;
}

std::string ThemeManager::GetColorHex(const std::string& tokenName) const {
    D2D1_COLOR_F c = GetColor(tokenName);
    char buf[16];
    sprintf_s(buf, "#%02X%02X%02X", static_cast<int>(c.r * 255), static_cast<int>(c.g * 255), static_cast<int>(c.b * 255));
    return std::string(buf);
}

} // namespace CUI
