#pragma once
#include <d2d1_1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace CUI {

using Microsoft::WRL::ComPtr;

class RenderResources {
public:
    RenderResources() = default;
    ~RenderResources() = default;

    void Initialize(ID2D1DeviceContext* context, IDWriteFactory* dwriteFactory);
    void ReleaseDeviceResources();
    void ReleaseContextBrushes(ID2D1DeviceContext* context);

    ID2D1SolidColorBrush* GetSolidBrush(D2D1_COLOR_F color);
    IDWriteTextFormat* GetTextFormat(const std::string& fontFamily, float fontSize, DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL);

private:
    static uint64_t ColorToKey(D2D1_COLOR_F color);
    static std::string TextFormatToKey(const std::string& font, float size, DWRITE_FONT_WEIGHT weight, DWRITE_FONT_STYLE style);

    ComPtr<ID2D1DeviceContext> m_context;
    ComPtr<IDWriteFactory> m_dwriteFactory;

    // Per-device-context brush caches so swapchain <-> scene-layer switches do not thrash.
    std::unordered_map<ID2D1DeviceContext*, std::unordered_map<uint64_t, ComPtr<ID2D1SolidColorBrush>>> m_brushCaches;
    std::unordered_map<std::string, ComPtr<IDWriteTextFormat>> m_textFormatCache;
};

} // namespace CUI
