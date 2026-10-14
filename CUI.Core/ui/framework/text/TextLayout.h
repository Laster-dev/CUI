#pragma once

#include "TextTypes.h"
#include "../core/Value.h"
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace CUI {

struct TextCaretInfo {
    float x = 0.0f;
    float y = 0.0f;
    float height = 0.0f;
};

struct TextLayoutOptions {
    float maxWidth = 10000.0f;
    float maxHeight = 10000.0f;
    TextWrapping wrapping = TextWrapping::NoWrap;
    TextVerticalAlignment paragraphAlignment = TextVerticalAlignment::Top;
    float lineSpacing = 1.0f;
    float lineHeight = 0.0f;
};

struct TextMetrics {
    float width = 0.0f;
    float widthIncludingTrailingWhitespace = 0.0f;
    float height = 0.0f;
    float layoutWidth = 0.0f;
    float layoutHeight = 0.0f;
    uint32_t maxBidiReorderingDepth = 0;
    uint32_t lineCount = 0;
};

class TextLayout {
public:
    TextLayout();
    ~TextLayout();

    TextLayout(const TextLayout& other);
    TextLayout& operator=(const TextLayout& other);
    TextLayout(TextLayout&& other) noexcept;
    TextLayout& operator=(TextLayout&& other) noexcept;

    bool IsValid() const;
    explicit operator bool() const { return IsValid(); }

    Size GetSize() const;
    float GetWidth() const;
    float GetHeight() const;
    uint32_t GetLineCount() const;
    TextMetrics GetMetrics() const;

    TextCaretInfo GetCaretInfo(uint32_t position, const Point& origin = {}) const;
    uint32_t HitTest(float x, float y, const Point& origin = {}) const;
    bool GetSelectionBounds(uint32_t start, uint32_t end, const Point& origin, std::vector<Rect>& rects) const;

    // Factory methods
    static TextLayout Create(
        const std::wstring& text,
        const std::string& fontName,
        float fontSize,
        const TextLayoutOptions& options = {},
        FontWeight weight = FontWeight::Normal,
        FontStyle style = FontStyle::Normal,
        FontStretch stretch = FontStretch::Normal);

    static TextLayout Create(
        const std::string& utf8Text,
        const std::string& fontName,
        float fontSize,
        const TextLayoutOptions& options = {},
        FontWeight weight = FontWeight::Normal,
        FontStyle style = FontStyle::Normal,
        FontStretch stretch = FontStretch::Normal);

    static TextLayout Create(
        const std::string& utf8Text,
        const TextStyle& style,
        float maxWidth = 10000.0f,
        float maxHeight = 10000.0f);

    // Native access for renderer
    void* GetNativeLayout() const;

private:
    struct Impl;
    std::shared_ptr<Impl> m_impl;
};

} // namespace CUI
