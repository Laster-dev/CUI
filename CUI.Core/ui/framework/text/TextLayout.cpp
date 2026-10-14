#include "TextLayout.h"
#include "../render/GraphicsContext.h"
#include <dwrite.h>
#include <wrl/client.h>
#include <algorithm>

namespace CUI {

struct TextLayout::Impl {
    Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
};

TextLayout::TextLayout() = default;
TextLayout::~TextLayout() = default;
TextLayout::TextLayout(const TextLayout& other) = default;
TextLayout& TextLayout::operator=(const TextLayout& other) = default;
TextLayout::TextLayout(TextLayout&& other) noexcept = default;
TextLayout& TextLayout::operator=(TextLayout&& other) noexcept = default;

bool TextLayout::IsValid() const {
    return m_impl && m_impl->layout != nullptr;
}

Size TextLayout::GetSize() const {
    if (!IsValid()) return Size(0.0f, 0.0f);
    DWRITE_TEXT_METRICS metrics = {};
    m_impl->layout->GetMetrics(&metrics);
    const float w = (std::max)(metrics.widthIncludingTrailingWhitespace, metrics.width) + 1.0f;
    const float h = metrics.height;
    return Size(w, h);
}

float TextLayout::GetWidth() const {
    return GetSize().width;
}

float TextLayout::GetHeight() const {
    return GetSize().height;
}

uint32_t TextLayout::GetLineCount() const {
    if (!IsValid()) return 0;
    DWRITE_TEXT_METRICS metrics = {};
    m_impl->layout->GetMetrics(&metrics);
    return metrics.lineCount;
}

TextMetrics TextLayout::GetMetrics() const {
    TextMetrics result = {};
    if (!IsValid()) return result;
    DWRITE_TEXT_METRICS m = {};
    m_impl->layout->GetMetrics(&m);
    result.width = m.width;
    result.widthIncludingTrailingWhitespace = m.widthIncludingTrailingWhitespace;
    result.height = m.height;
    result.layoutWidth = m.layoutWidth;
    result.layoutHeight = m.layoutHeight;
    result.maxBidiReorderingDepth = m.maxBidiReorderingDepth;
    result.lineCount = m.lineCount;
    return result;
}

TextCaretInfo TextLayout::GetCaretInfo(uint32_t position, const Point& origin) const {
    TextCaretInfo info;
    if (!IsValid()) return info;

    DWRITE_HIT_TEST_METRICS metrics = {};
    float x = 0.0f;
    float y = 0.0f;
    if (SUCCEEDED(m_impl->layout->HitTestTextPosition(position, FALSE, &x, &y, &metrics))) {
        info.x = origin.x + x;
        info.y = origin.y + y;
        info.height = metrics.height;
        if (info.height <= 0.0f) {
            DWRITE_TEXT_METRICS textMetrics = {};
            m_impl->layout->GetMetrics(&textMetrics);
            info.height = (std::max)(16.0f, textMetrics.height);
        }
    }
    return info;
}

uint32_t TextLayout::HitTest(float x, float y, const Point& origin) const {
    if (!IsValid()) return 0;

    float relX = x - origin.x;
    float relY = y - origin.y;
    BOOL trailing = FALSE;
    BOOL inside = FALSE;
    DWRITE_HIT_TEST_METRICS metrics = {};
    if (SUCCEEDED(m_impl->layout->HitTestPoint(relX, relY, &trailing, &inside, &metrics))) {
        uint32_t pos = metrics.textPosition;
        if (trailing && pos < UINT32_MAX) {
            pos++;
        }
        return pos;
    }
    return 0;
}

bool TextLayout::GetSelectionBounds(uint32_t start, uint32_t end, const Point& origin, std::vector<Rect>& rects) const {
    rects.clear();
    if (!IsValid() || start == end) return false;

    uint32_t selMin = (std::min)(start, end);
    uint32_t selMax = (std::max)(start, end);
    uint32_t length = selMax - selMin;

    uint32_t count = 0;
    m_impl->layout->HitTestTextRange(selMin, length, origin.x, origin.y, nullptr, 0, &count);
    if (count == 0) return false;

    std::vector<DWRITE_HIT_TEST_METRICS> metrics(count);
    m_impl->layout->HitTestTextRange(selMin, length, origin.x, origin.y, metrics.data(), count, &count);

    rects.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        const DWRITE_HIT_TEST_METRICS& m = metrics[i];
        rects.push_back(Rect(m.left, m.top, m.width, m.height));
    }
    return true;
}

static DWRITE_WORD_WRAPPING ToDWriteWrapping(TextWrapping wrapping) {
    switch (wrapping) {
    case TextWrapping::Wrap:
    case TextWrapping::EmergencyBreak:
        return DWRITE_WORD_WRAPPING_WRAP;
    case TextWrapping::NoWrap:
    default:
        return DWRITE_WORD_WRAPPING_NO_WRAP;
    }
}

static DWRITE_PARAGRAPH_ALIGNMENT ToDWriteParagraphAlignment(TextVerticalAlignment align) {
    switch (align) {
    case TextVerticalAlignment::Center:
        return DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
    case TextVerticalAlignment::Bottom:
        return DWRITE_PARAGRAPH_ALIGNMENT_FAR;
    case TextVerticalAlignment::Top:
    default:
        return DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
    }
}

TextLayout TextLayout::Create(
    const std::wstring& text,
    const std::string& fontName,
    float fontSize,
    const TextLayoutOptions& options,
    FontWeight weight,
    FontStyle style,
    FontStretch stretch)
{
    auto nativeLayout = GraphicsContext::CreateTextLayout(
        text,
        fontName,
        fontSize,
        options,
        static_cast<DWRITE_FONT_WEIGHT>(weight),
        static_cast<DWRITE_FONT_STYLE>(style),
        static_cast<DWRITE_FONT_STRETCH>(stretch)
    );

    TextLayout result;
    if (nativeLayout) {
        result.m_impl = std::make_shared<Impl>();
        result.m_impl->layout = nativeLayout;
    }
    return result;
}

TextLayout TextLayout::Create(
    const std::string& utf8Text,
    const std::string& fontName,
    float fontSize,
    const TextLayoutOptions& options,
    FontWeight weight,
    FontStyle style,
    FontStretch stretch)
{
    return Create(Utf8ToUtf16(utf8Text), fontName, fontSize, options, weight, style, stretch);
}

TextLayout TextLayout::Create(
    const std::string& utf8Text,
    const TextStyle& style,
    float maxWidth,
    float maxHeight)
{
    TextLayoutOptions opt;
    opt.maxWidth = maxWidth;
    opt.maxHeight = maxHeight;
    opt.wrapping = style.wrapping;
    opt.paragraphAlignment = style.verticalAlignment;
    opt.lineSpacing = style.lineSpacing;
    opt.lineHeight = style.lineHeight;

    return Create(Utf8ToUtf16(utf8Text), style.fontFamily, style.fontSize, opt, style.weight, style.style, style.stretch);
}

void* TextLayout::GetNativeLayout() const {
    return m_impl ? m_impl->layout.Get() : nullptr;
}

} // namespace CUI
