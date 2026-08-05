#pragma once

#include "../core/Value.h"
#include "../window/WindowBackdrop.h"
#include "ThemeTokenId.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace CUI {

// WinUI-like material contract:
// - Chrome: title bar / nav pane / window host — lets SystemBackdrop show through
// - Surface: page cards / inputs in the document — readable but slightly translucent
// - Solid: text / accents / borders — always opaque
// Popups (menu / flyout / combobox dropdown / tooltip) must NOT use Surface —
// call GetFlatColor() so they stay fully opaque over the backdrop.
enum class MaterialRole {
    Chrome,
    Surface,
    Solid
};

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
    D2D1_COLOR_F selectedBackground;
    D2D1_COLOR_F focusedBorder;
    D2D1_COLOR_F activityBarBackground;
};

class ThemeManager {
public:
    static ThemeManager& Instance();

    ThemeMode GetThemeMode() const { return m_mode; }
    void SetThemeMode(ThemeMode mode);

    // When true, GetColor() applies chrome/surface alphas so Mica/Acrylic can show.
    bool IsBackdropActive() const { return m_backdropActive; }
    void SetBackdropActive(bool active);
    void SetBackdropType(BackdropType type);
    BackdropType GetBackdropType() const { return m_backdropType; }

    const ThemeTokens& GetTokens() const { return m_tokens; }

    // Preferred paint path — applies material role alpha when backdrop is active.
    D2D1_COLOR_F GetColor(ThemeTokenId id) const;
    // Deprecated: prefer GetColor(ThemeTokenId). Kept for markup / ThemeTokenIdFromName bridges.
    D2D1_COLOR_F GetColor(const std::string& tokenName) const;
    // Menus / flyouts / dropdowns / tooltips — never translucent under SystemBackdrop.
    D2D1_COLOR_F GetFlatColor(ThemeTokenId id) const;
    D2D1_COLOR_F GetFlatColor(const std::string& tokenName) const;
    MaterialRole GetMaterialRole(const std::string& tokenName) const;
    MaterialRole GetMaterialRole(ThemeTokenId id) const;

    std::string GetColorHex(const std::string& tokenName) const;
    static const std::vector<std::string>& GetTokenNames();

private:
    ThemeManager();
    void UpdateTokens();
    D2D1_COLOR_F LookupBaseColor(const std::string& tokenName) const;
    D2D1_COLOR_F LookupBaseColor(ThemeTokenId id) const;
    D2D1_COLOR_F ApplyMaterialRole(const std::string& tokenName, D2D1_COLOR_F base) const;
    D2D1_COLOR_F ApplyMaterialRole(ThemeTokenId id, D2D1_COLOR_F base) const;

    ThemeMode m_mode = ThemeMode::Dark;
    ThemeTokens m_tokens{};
    bool m_backdropActive = false;
    BackdropType m_backdropType = BackdropType::None;
};

} // namespace CUI
