#pragma once
#include <cstdint>

namespace CUI {
namespace Term {

// Cell content aligned with xterm.js CellData attribute packing.
// Fg/Bg: 0-255 = ANSI/256 palette index; bit 24 set = RGB (0xRRGGBB in low 24 bits).
struct CellData {
    static const int ContentMask = 0x1FFFFF;
    static const int WidthMask = 0xC00000;
    static const int WidthShift = 22;

    static const uint32_t AttrBold = 1u << 0;
    static const uint32_t AttrDim = 1u << 1;
    static const uint32_t AttrItalic = 1u << 2;
    static const uint32_t AttrUnderline = 1u << 3;
    static const uint32_t AttrBlink = 1u << 4;
    static const uint32_t AttrInverse = 1u << 5;
    static const uint32_t AttrInvisible = 1u << 6;
    static const uint32_t AttrStrikethrough = 1u << 7;

    static const int ColorModePalette = 0;
    static const int ColorModeRgb = 1 << 24;
    static const int DefaultColor = 0x100; // sentinel for default fg/bg

    int Content = ' ' | (1 << WidthShift);
    int Fg = DefaultColor;
    int Bg = DefaultColor;
    uint32_t Attrs = 0;
    // OSC 8 hyperlink id (0 = none).
    int LinkId = 0;

    static CellData Empty() {
        CellData c;
        c.Content = ' ' | (1 << WidthShift);
        c.Fg = DefaultColor;
        c.Bg = DefaultColor;
        c.Attrs = 0;
        c.LinkId = 0;
        return c;
    }

    int GetCodePoint() const { return Content & ContentMask; }
    void SetCodePoint(int value) { Content = (Content & ~ContentMask) | (value & ContentMask); }

    int GetWidth() const { return (Content & WidthMask) >> WidthShift; }
    void SetWidth(int value) { Content = (Content & ~WidthMask) | ((value << WidthShift) & WidthMask); }

    // Mirrors CellData.Char in the C# source: code points above U+FFFF collapse to '?'.
    wchar_t GetChar() const {
        const int cp = GetCodePoint();
        return static_cast<wchar_t>(cp <= 0xFFFF ? cp : '?');
    }

    void SetChar(wchar_t value) {
        SetCodePoint(static_cast<int>(value));
        SetWidth(value == 0 ? 0 : 1);
    }

    bool IsEmpty() const {
        const int cp = GetCodePoint();
        return cp == 0 || cp == ' ';
    }

    void SetFrom(const CellData& other) {
        Content = other.Content;
        Fg = other.Fg;
        Bg = other.Bg;
        Attrs = other.Attrs;
        LinkId = other.LinkId;
    }

    void Reset() {
        Content = ' ' | (1 << WidthShift);
        Fg = DefaultColor;
        Bg = DefaultColor;
        Attrs = 0;
        LinkId = 0;
    }
};

} // namespace Term
} // namespace CUI
