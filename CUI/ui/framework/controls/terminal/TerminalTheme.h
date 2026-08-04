#pragma once
#include <cstdint>
#include <d2d1.h>

namespace CUI {
namespace Term {

// Byte-accurate ARGB color so palette values and OSC color reports stay
// bit-identical to the WPF source; converted to D2D1_COLOR_F only when painting.
struct TermColor {
    uint8_t a = 255;
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    TermColor() = default;
    TermColor(uint8_t a_, uint8_t r_, uint8_t g_, uint8_t b_) : a(a_), r(r_), g(g_), b(b_) {}

    static TermColor FromRgb(uint8_t r, uint8_t g, uint8_t b) { return TermColor(255, r, g, b); }
    static TermColor FromArgb(uint8_t a, uint8_t r, uint8_t g, uint8_t b) { return TermColor(a, r, g, b); }

    D2D1_COLOR_F ToD2D() const {
        return D2D1::ColorF(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
    }

    uint32_t Key() const {
        return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(r) << 16)
            | (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
    }

    bool operator==(const TermColor& other) const { return Key() == other.Key(); }
    bool operator!=(const TermColor& other) const { return Key() != other.Key(); }
};

// ANSI / UI colors for the terminal surface.
struct TerminalTheme {
    TermColor Background = TermColor::FromRgb(0x0C, 0x0C, 0x0C);
    TermColor Foreground = TermColor::FromRgb(0xCC, 0xCC, 0xCC);
    TermColor Cursor = TermColor::FromRgb(0xCC, 0xCC, 0xCC);
    TermColor Selection = TermColor::FromArgb(0xB3, 0x26, 0x4F, 0x78);
    // Selected text color; A=0 uses auto-contrast against Selection.
    TermColor SelectionForeground = TermColor::FromArgb(0, 0, 0, 0);
    TermColor Ansi16[16];

    TerminalTheme();

    static TerminalTheme Dark();
    static TerminalTheme Light();
};

} // namespace Term
} // namespace CUI
