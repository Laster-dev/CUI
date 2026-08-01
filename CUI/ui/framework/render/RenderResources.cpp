#include "RenderResources.h"
#include <sstream>

namespace CUI {

void RenderResources::Initialize(ID2D1DeviceContext* context, IDWriteFactory* dwriteFactory) {
    if (m_context.Get() != context) {
        m_brushCache.clear();
    }
    m_context = context;
    m_dwriteFactory = dwriteFactory;
}

void RenderResources::ReleaseDeviceResources() {
    m_brushCache.clear();
}

std::string RenderResources::ColorToKey(D2D1_COLOR_F color) {
    std::stringstream ss;
    ss << color.r << "_" << color.g << "_" << color.b << "_" << color.a;
    return ss.str();
}

std::string RenderResources::TextFormatToKey(const std::string& font, float size, DWRITE_FONT_WEIGHT weight, DWRITE_FONT_STYLE style) {
    std::stringstream ss;
    ss << font << "_" << size << "_" << static_cast<int>(weight) << "_" << static_cast<int>(style);
    return ss.str();
}

ID2D1SolidColorBrush* RenderResources::GetSolidBrush(D2D1_COLOR_F color) {
    if (!m_context) return nullptr;

    std::string key = ColorToKey(color);
    auto it = m_brushCache.find(key);
    if (it != m_brushCache.end()) {
        return it->second.Get();
    }

    ComPtr<ID2D1SolidColorBrush> brush;
    HRESULT hr = m_context->CreateSolidColorBrush(color, &brush);
    if (SUCCEEDED(hr)) {
        m_brushCache[key] = brush;
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
