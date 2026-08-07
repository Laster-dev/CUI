#include "RenderResources.h"
#include <algorithm>
#include <cmath>

namespace CUI {

void RenderResources::Initialize(ID2D1DeviceContext* context, IDWriteFactory* dwriteFactory) {
    m_context = context;
    m_dwriteFactory = dwriteFactory;
}

void RenderResources::ReleaseDeviceResources() {
    m_brushCaches.clear();
    m_context.Reset();
}

void RenderResources::ReleaseContextBrushes(ID2D1DeviceContext* context) {
    if (!context) {
        return;
    }
    m_brushCaches.erase(context);
}

uint64_t RenderResources::ColorToKey(D2D1_COLOR_F color) {
    auto quantize = [](float v) -> uint64_t {
        const float clamped = std::clamp(v, 0.0f, 1.0f);
        return static_cast<uint64_t>(clamped * 65535.0f + 0.5f);
    };
    return quantize(color.r)
        | (quantize(color.g) << 16)
        | (quantize(color.b) << 32)
        | (quantize(color.a) << 48);
}

std::string RenderResources::TextFormatToKey(const std::string& font, float size, DWRITE_FONT_WEIGHT weight, DWRITE_FONT_STYLE style) {
    std::string key;
    key.reserve(font.size() + 32);
    key.append(font);
    key.push_back('|');
    key.append(std::to_string(size));
    key.push_back('|');
    key.append(std::to_string(static_cast<int>(weight)));
    key.push_back('|');
    key.append(std::to_string(static_cast<int>(style)));
    return key;
}

ID2D1SolidColorBrush* RenderResources::GetSolidBrush(D2D1_COLOR_F color) {
    if (!m_context) return nullptr;

    ID2D1DeviceContext* ctx = m_context.Get();
    auto& cache = m_brushCaches[ctx];
    const uint64_t key = ColorToKey(color);
    auto it = cache.find(key);
    if (it != cache.end()) {
        return it->second.Get();
    }

    ComPtr<ID2D1SolidColorBrush> brush;
    HRESULT hr = ctx->CreateSolidColorBrush(color, &brush);
    if (SUCCEEDED(hr)) {
        cache[key] = brush;
        return brush.Get();
    }

    return nullptr;
}

IDWriteTextFormat* RenderResources::GetTextFormat(const std::string& fontFamily, float fontSize, DWRITE_FONT_WEIGHT weight, DWRITE_FONT_STYLE style) {
    if (!m_dwriteFactory) return nullptr;

    std::string key = TextFormatToKey(fontFamily, fontSize, weight, style);
    auto it = m_textFormatCache.find(key);
    if (it != m_textFormatCache.end()) {
        return it->second.Get();
    }

    std::wstring wFont(fontFamily.begin(), fontFamily.end());
    ComPtr<IDWriteTextFormat> format;
    HRESULT hr = m_dwriteFactory->CreateTextFormat(
        wFont.c_str(),
        nullptr,
        weight,
        style,
        DWRITE_FONT_STRETCH_NORMAL,
        fontSize,
        L"en-US",
        &format
    );

    if (SUCCEEDED(hr)) {
        m_textFormatCache[key] = format;
        return format.Get();
    }

    return nullptr;
}

} // namespace CUI
