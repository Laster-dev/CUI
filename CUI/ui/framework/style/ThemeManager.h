#pragma once

#include "../core/Value.h"
#include "../window/WindowBackdrop.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace CUI {

struct ThemeTokens {
    D2D1_COLOR_F windowBackground;
    D2D1_COLOR_F cardBackground;
    D2D1_COLOR_F cardBorder;
    D2D1_COLOR_F textPrimary;
    D2D1_COLOR_F textSecondary;
    D2D1_COLOR_F textMuted;
    D2D1_COLOR_F titleBarBackground;
    D2D1_COLOR_F titleBarText;
    D2D1_COLOR_F accentColor;
    D2D1_COLOR_F accentForeground;
    D2D1_COLOR_F dangerColor;
    D2D1_COLOR_F paneBackground;
    D2D1_COLOR_F inputBackground;
    D2D1_COLOR_F inputBorder;
    D2D1_COLOR_F hoverBackground;
    D2D1_COLOR_F pressedBackground;
    D2D1_COLOR_F focusedBorder;
    D2D1_COLOR_F activityBarBackground;
};

class ThemeManager {
public:
    static ThemeManager& Instance();

    ThemeMode GetThemeMode() const { return m_mode; }
    void SetThemeMode(ThemeMode mode);

    const ThemeTokens& GetTokens() const { return m_tokens; }

    D2D1_COLOR_F GetColor(const std::string& tokenName) const;
    std::string GetColorHex(const std::string& tokenName) const;
    static const std::vector<std::string>& GetTokenNames();

private:
    ThemeManager();
    void UpdateTokens();

    ThemeMode m_mode = ThemeMode::Dark;
    ThemeTokens m_tokens{};
};

} // namespace CUI
