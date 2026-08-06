#pragma once
#include "RenderResources.h"
#include "RenderLayer.h"
#include "../core/Value.h"
#include <d2d1_1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <dxgi1_2.h>
#include <dcomp.h>
#include <wrl/client.h>
#include <vector>

namespace CUI {

using Microsoft::WRL::ComPtr;
class CompositionContext;

class GraphicsContext {
public:
    GraphicsContext();
    ~GraphicsContext();

    bool Initialize(HWND hwnd);
    void Resize(UINT width, UINT height);
    void ReleaseDeviceResources();

    void BeginDraw();
    HRESULT EndDraw();

    ID2D1DeviceContext* GetD2DContext() const { return m_d2dContext.Get(); }
    IDWriteFactory* GetDWriteFactory() const { return m_dwriteFactory.Get(); }
    RenderResources& GetResources() { return m_resources; }
    HWND GetHwnd() const { return m_hwnd; }
    bool UsesCompositionSwapChain() const { return m_usesCompositionSwapChain; }
    // True when the present path keeps per-pixel alpha for DWM Mica/Acrylic.
    bool SupportsPerPixelAlpha() const { return m_supportsPerPixelAlpha; }

    void PushClip(const Rect& rect);
    // Geometry clip matching FillRoundedRect corners (for ripples, etc.).
    void PushRoundedClip(const Rect& rect, float radius);
    void PopClip();
    void PushOpacity(float opacity);
    void PopOpacity();
    bool EnsureLayerCache(RenderLayer& layer, Size sizeInDips);
    ID2D1DeviceContext* BeginLayerDraw(RenderLayer& layer);
    void EndLayerDraw(RenderLayer& layer);
    void DrawLayer(const RenderLayer& layer, const Rect& destRect, const Rect* sourceRect = nullptr, float opacity = 1.0f);
    // clearTarget=false keeps existing layer pixels (dirty-rect patching).
    bool PushLayerTarget(RenderLayer& layer, Size sizeInDips, const Rect& paintBounds, D2D1_COLOR_F clearColor, bool clearTarget = true);
    void PopLayerTarget(RenderLayer& layer);
    // Erase pixels in rect (SOURCE_COPY). Needed for transparent material clears under clip.
    void ClearRect(const Rect& rect);

    void DrawRect(const Rect& rect, D2D1_COLOR_F color, float strokeWidth = 1.0f);
    void FillRect(const Rect& rect, D2D1_COLOR_F color);
    void FillRoundedRect(const Rect& rect, float radius, D2D1_COLOR_F color);
    void DrawRoundedRect(const Rect& rect, float radius, D2D1_COLOR_F color, float strokeWidth = 1.0f);
    void DrawLine(Point p1, Point p2, D2D1_COLOR_F color, float strokeWidth = 1.0f);

    enum class ChevronDirection { Down, Up, Left, Right };
    // Vector chevron (two strokes). Prefer this over glyph characters under DPI scaling.
    void DrawChevron(const Rect& bounds, D2D1_COLOR_F color, ChevronDirection direction, float strokeWidth = 1.6f);
    void DrawText(const std::string& text, const Rect& rect, D2D1_COLOR_F color,
                  const std::string& fontName = "微软雅黑", float fontSize = 13.0f,
                  DWRITE_TEXT_ALIGNMENT align = DWRITE_TEXT_ALIGNMENT_LEADING,
                  DWRITE_PARAGRAPH_ALIGNMENT vAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
                  DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL);

    Size MeasureText(const std::string& text, const std::string& fontName = "微软雅黑",
                     float fontSize = 13.0f, DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL);

    struct TextLayoutOptions {
        float maxWidth = 10000.0f;
        float maxHeight = 10000.0f;
        DWRITE_WORD_WRAPPING wrapping = DWRITE_WORD_WRAPPING_NO_WRAP;
        DWRITE_PARAGRAPH_ALIGNMENT paragraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
        float lineSpacing = 1.0f;
        float lineHeight = 0.0f;
    };

    struct TextCaretInfo {
        float x = 0.0f;
        float y = 0.0f;
        float height = 0.0f;
    };

    static ComPtr<IDWriteFactory> GetSharedWriteFactory();
    static ComPtr<IDWriteTextLayout> CreateTextLayout(
        const std::wstring& text,
        const std::string& fontName,
        float fontSize,
        const TextLayoutOptions& options,
        DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL);

    void DrawTextLayout(IDWriteTextLayout* layout, const Rect& originRect, D2D1_COLOR_F color);
    static TextCaretInfo GetTextCaretInfo(IDWriteTextLayout* layout, UINT32 position, const Point& origin);
    static UINT32 HitTestTextLayout(IDWriteTextLayout* layout, float x, float y, const Point& origin);
    static bool GetTextSelectionBounds(IDWriteTextLayout* layout, UINT32 start, UINT32 end,
                                       const Point& origin, std::vector<D2D1_RECT_F>& rects);

    float GetDpiScale() const { return m_dpiScale; }
    void SetPaintBounds(const Rect& rect) { m_paintBounds = rect; }
    const Rect& GetPaintBounds() const { return m_paintBounds; }
    bool IntersectsPaintBounds(const Rect& rect) const { return m_paintBounds.IsEmpty() || rect.Intersects(m_paintBounds); }
    void SetCompositionContext(CompositionContext* context) { m_compositionContext = context; }
    CompositionContext* GetCompositionContext() const { return m_compositionContext; }

private:
    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceResources();
    HRESULT BindSwapChainTarget(float dpiX, float dpiY);
    void DrawTextOnTarget(
        ID2D1RenderTarget* target,
        const std::wstring& text,
        const Rect& rect,
        D2D1_COLOR_F color,
        const std::string& fontName,
        float fontSize,
        DWRITE_TEXT_ALIGNMENT align,
        DWRITE_PARAGRAPH_ALIGNMENT vAlign,
        DWRITE_FONT_WEIGHT weight,
        D2D1_TEXT_ANTIALIAS_MODE antialiasMode);
    void DrawTextLayoutOnTarget(
        ID2D1RenderTarget* target,
        IDWriteTextLayout* layout,
        const Rect& originRect,
        D2D1_COLOR_F color,
        D2D1_TEXT_ANTIALIAS_MODE antialiasMode);

    HWND m_hwnd = nullptr;
    float m_dpiScale = 1.0f;
    bool m_usesCompositionSwapChain = false;
    bool m_supportsPerPixelAlpha = false;

    ComPtr<ID2D1Factory1> m_d2dFactory;
    ComPtr<IDWriteFactory> m_dwriteFactory;
    ComPtr<IWICImagingFactory2> m_wicFactory;

    ComPtr<ID2D1Device> m_d2dDevice;
    ComPtr<ID2D1DeviceContext> m_d2dContext;
    ComPtr<IDXGISwapChain1> m_swapChain;
    ComPtr<IDCompositionDevice> m_dcompDevice;
    ComPtr<IDCompositionTarget> m_dcompTarget;
    ComPtr<IDCompositionVisual> m_dcompVisual;

    RenderResources m_resources;
    std::vector<D2D1_RECT_F> m_clipStack;
    std::vector<bool> m_clipIsLayer; // true = PushLayer (rounded), false = axis-aligned
    std::vector<float> m_opacityStack;
    Rect m_paintBounds;

    struct TargetState {
        ComPtr<ID2D1DeviceContext> context;
        Rect paintBounds;
        std::vector<D2D1_RECT_F> clipStack;
        std::vector<bool> clipIsLayer;
        std::vector<float> opacityStack;
    };
    std::vector<TargetState> m_targetStack;
    CompositionContext* m_compositionContext = nullptr;
};

} // namespace CUI
