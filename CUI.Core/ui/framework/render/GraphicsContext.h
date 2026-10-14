#pragma once
#include <windows.h>
#include <unknwn.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <d2d1_3.h>
#include <d3d11.h>
#include <dwrite.h>
#include <wincodec.h>
#include <dxgi1_2.h>
#include <dcomp.h>
#include <wrl/client.h>
#include "RenderResources.h"
#include "RenderLayer.h"
#include "../core/Value.h"
#include "../text/TextTypes.h"
#include "../text/TextLayout.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace CUI {

using Microsoft::WRL::ComPtr;
class CompositionContext;
class DisplayList;

class GraphicsContext {
public:
    GraphicsContext();
    ~GraphicsContext();

    bool Initialize(HWND hwnd);
    void ApplyRequirePerPixelAlpha(bool enabled);
    bool RequiresPerPixelAlpha() const { return m_requirePerPixelAlpha; }
    void Resize(UINT width, UINT height);
    void ReleaseDeviceResources();
    void Shutdown();

    void BeginDraw();
    HRESULT EndDraw();
    // Partial present (FLIP_SEQUENTIAL). dirtyRectPx is in physical pixels; null = full present.
    HRESULT EndDraw(const RECT* dirtyRectPx);

    ID2D1DeviceContext* GetD2DContext() const { return m_d2dContext.Get(); }
    ID2D1Device* GetD2DDevice() const { return m_d2dDevice.Get(); }
    ID3D11Device* GetD3DDevice() const { return m_d3dDevice.Get(); }
    IDWriteFactory* GetDWriteFactory() const { return m_dwriteFactory.Get(); }
    IWICImagingFactory2* GetWicFactory() const { return m_wicFactory.Get(); }
    RenderResources& GetResources() { return m_resources; }
    HWND GetHwnd() const { return m_hwnd; }
    bool UsesCompositionSwapChain() const { return m_usesCompositionSwapChain; }
    // True when the present path keeps per-pixel alpha for DWM Mica/Acrylic.
    bool SupportsPerPixelAlpha() const { return m_supportsPerPixelAlpha; }
    float GetGpuVideoMemoryUsage01() const;
    IDCompositionDevice* GetDCompDevice() const { return m_dcompDevice.Get(); }
    IDCompositionVisual* GetDCompRootVisual() const { return m_dcompRootVisual.Get(); }
    // Commit the DComp tree (used after a normal swapchain Present).
    HRESULT CommitComposition();

    // Create or resize a premultiplied BGRA composition surface (pixel size).
    bool EnsureCompositionSurface(
        ComPtr<IDCompositionSurface>& surface,
        UINT widthPx,
        UINT heightPx);
    // Draw into a composition surface with a temporary D2D context (DIPs, origin 0,0).
    // draw() may call FillRoundedRect/etc. on this GraphicsContext.
    bool DrawCompositionSurface(
        IDCompositionSurface* surface,
        float widthDips,
        float heightDips,
        const std::function<void()>& draw);
    bool AttachCompositionOverlay(IDCompositionVisual* visual);
    void DetachCompositionOverlay(IDCompositionVisual* visual);

    void PushClip(const Rect& rect);
    // Geometry clip matching FillRoundedRect corners (for ripples, etc.).
    void PushRoundedClip(const Rect& rect, float radius);
    void PushEllipseClip(Point center, float radiusX, float radiusY);
    void PopClip();
    void PushOpacity(float opacity);
    void PopOpacity();
    void PushTransform(const Matrix3x2& transform);
    void PushTranslation(float dx, float dy);
    void PopTransform();
    // WinUI3-style popup enter: opacity + scale around origin. Pair with PopPopupReveal.
    void PushPopupReveal(const Rect& bounds, float progress, Point origin);
    void PopPopupReveal();
    bool EnsureLayerCache(RenderLayer& layer, Size sizeInDips);
    // Lazily allocate the twin scratch bitmap (ScrollViewer scroll-patch path only).
    bool EnsureLayerScratch(RenderLayer& layer);
    bool SnapshotLayerToScratch(RenderLayer& layer);
    ID2D1DeviceContext* BeginLayerDraw(RenderLayer& layer);
    void EndLayerDraw(RenderLayer& layer);
    void DrawLayer(const RenderLayer& layer, const Rect& destRect, const Rect* sourceRect = nullptr, float opacity = 1.0f);
    void DrawLayerScratch(const RenderLayer& layer, const Rect& destRect, float opacity = 1.0f);
    void DrawDisplayList(const DisplayList& list, Point offset = {0.0f, 0.0f}, float opacity = 1.0f);

    // 指令流录制模式 (DisplayList Recording Mode)
    void BeginRecording(std::shared_ptr<DisplayList> list);
    std::shared_ptr<DisplayList> EndRecording();
    bool IsRecording() const { return m_activeDisplayList != nullptr; }
    DisplayList* GetActiveDisplayList() const { return m_activeDisplayList.get(); }

    // clearTarget=false keeps existing layer pixels (dirty-rect patching).
    bool PushLayerTarget(RenderLayer& layer, Size sizeInDips, const Rect& paintBounds, Color clearColor, bool clearTarget = true);
    void PopLayerTarget(RenderLayer& layer);
    // Erase pixels in rect (SOURCE_COPY). Needed for transparent material clears under clip.
    // Prefer passing the scene clear color when patching opaque caches — transparent clears
    // leave holes that composite as black bars over WindowBackground.
    void ClearRect(const Rect& rect);
    void ClearRect(const Rect& rect, Color color);
    // Expand rect outward to whole device pixels (and optional DIP inflate). Clear+clip must share this.
    static Rect SnapExpandRect(const Rect& rect, float dpiScale, float inflateDips = 0.0f);

    void DrawRect(const Rect& rect, Color color, float strokeWidth = 1.0f);
    void FillRect(const Rect& rect, Color color);
    void FillRoundedRect(const Rect& rect, float radius, Color color);
    void DrawRoundedRect(const Rect& rect, float radius, Color color, float strokeWidth = 1.0f);
    void DrawLine(Point p1, Point p2, Color color, float strokeWidth = 1.0f);
    // Antialiased stroke with round caps — for icons. DrawLine pixel-snaps and looks jagged on diagonals.
    void DrawSmoothLine(Point p1, Point p2, Color color, float strokeWidth = 1.5f);
    // Circular arc with round caps (angles in radians, 0 = +X, increasing = clockwise in DIP space).
    void DrawSmoothArc(
        Point center,
        float radius,
        float startRad,
        float sweepRad,
        Color color,
        float strokeWidth = 1.5f);

    enum class ChevronDirection { Down, Up, Left, Right };
    // Theme-tinted SVG chevron (source points right; rotated for other directions).
    void DrawChevron(const Rect& bounds, Color color, ChevronDirection direction, float strokeWidth = 1.6f);
    void FillPolygon(const Point* points, int count, Color color);
    void DrawPolygon(const Point* points, int count, Color color, float strokeWidth = 1.0f);
    void FillRoundedGeometry(const std::vector<Point>& points, float radius, Color color);
    // Draw a native HICON (e.g. extracted from regedit.exe) into dest DIPs.
    void DrawHIcon(HICON icon, const Rect& dest, float opacity = 1.0f);

    // Inline `<svg>…</svg>` markup, or a filesystem path ending in `.svg`.
    static bool LooksLikeSvg(const std::string& source);
    // Native Direct2D SVG. `tint` recolors fill/stroke (monochrome theme icons).
    void DrawSvg(
        const std::string& source,
        const Rect& dest,
        const Color* tint = nullptr,
        float opacity = 1.0f,
        const Color* foreground = nullptr);
    // SVG markup/path, else emoji/glyph text centered in dest.
    void DrawIcon(
        const std::string& icon,
        const Rect& dest,
        Color color,
        float opacity = 1.0f,
        const std::string& glyphFont = "Segoe UI Emoji",
        float glyphSize = 0.0f);
    struct InheritedTextStyle {
        FontWeight weight = FontWeight::Normal;
        FontStyle style = FontStyle::Normal;
        FontStretch stretch = FontStretch::Normal;
        bool underline = false;
        bool strikethrough = false;
    };

    void PushInheritedTextStyle(const InheritedTextStyle& style);
    void PopInheritedTextStyle();

    // High-level typography API
    void DrawText(const std::string& text, const Rect& rect, Color color, const TextStyle& style);

    // Pure CUI DrawText overload
    void DrawText(const std::string& text, const Rect& rect, Color color,
                  const std::string& fontName = "微软雅黑", float fontSize = 13.0f,
                  TextAlignment align = TextAlignment::Left,
                  TextVerticalAlignment vAlign = TextVerticalAlignment::Center,
                  FontWeight weight = FontWeight::Normal,
                  bool truncateWithEllipsis = false,
                  FontStyle style = FontStyle::Normal,
                  FontStretch stretch = FontStretch::Normal,
                  bool underline = false,
                  bool strikethrough = false);

    // Compatibility DrawText overload with DWRITE enums
    void DrawText(const std::string& text, const Rect& rect, Color color,
                  const std::string& fontName, float fontSize,
                  DWRITE_TEXT_ALIGNMENT align,
                  DWRITE_PARAGRAPH_ALIGNMENT vAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
                  DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL,
                  bool truncateWithEllipsis = false,
                  DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL,
                  DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL,
                  bool underline = false,
                  bool strikethrough = false);

    // VS auto-hide tabs: rotate the whole run around `center` (ClearType cannot).
    void DrawTextRotated(
        const std::string& text,
        Point center,
        float degrees,
        Color color,
        const std::string& fontName = "Segoe UI",
        float fontSize = 11.0f,
        FontWeight weight = FontWeight::Normal);

    void DrawTextRotated(
        const std::string& text,
        Point center,
        float degrees,
        Color color,
        const std::string& fontName,
        float fontSize,
        DWRITE_FONT_WEIGHT weight);

    Size MeasureText(const std::string& text, const TextStyle& style, float maxWidth = 10000.0f);

    Size MeasureText(const std::string& text, const std::string& fontName = "微软雅黑",
                     float fontSize = 13.0f, FontWeight weight = FontWeight::Normal,
                     FontStyle style = FontStyle::Normal,
                     FontStretch stretch = FontStretch::Normal);

    Size MeasureText(const std::string& text, const std::string& fontName,
                     float fontSize, DWRITE_FONT_WEIGHT weight,
                     DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL,
                     DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL);

    static ComPtr<IDWriteFactory> GetSharedWriteFactory();
    static ComPtr<IDWriteTextLayout> CreateTextLayout(
        const std::wstring& text,
        const std::string& fontName,
        float fontSize,
        const TextLayoutOptions& options,
        DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL);

    void DrawTextLayout(const TextLayout& layout, const Rect& originRect, Color color);
    void DrawTextLayout(IDWriteTextLayout* layout, const Rect& originRect, Color color);
    static TextCaretInfo GetTextCaretInfo(IDWriteTextLayout* layout, UINT32 position, const Point& origin);
    static UINT32 HitTestTextLayout(IDWriteTextLayout* layout, float x, float y, const Point& origin);
    static bool GetTextSelectionBounds(IDWriteTextLayout* layout, UINT32 start, UINT32 end,
                                       const Point& origin, std::vector<D2D1_RECT_F>& rects);

    float GetDpiScale() const { return m_dpiScale; }
    void ApplyPaintBounds(const Rect& rect) { m_paintBounds = rect; }
    const Rect& GetPaintBounds() const { return m_paintBounds; }
    bool IntersectsPaintBounds(const Rect& rect) const { return m_paintBounds.IsEmpty() || rect.Intersects(m_paintBounds); }
    void ApplyCompositionContext(CompositionContext* context) { m_compositionContext = context; }
    CompositionContext* GetCompositionContext() const { return m_compositionContext; }

private:
    void PushTransform(const D2D1_MATRIX_3X2_F& transform);
    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceResources();
    HRESULT BindSwapChainTarget(float dpiX, float dpiY);
    void DrawTextOnTarget(
        ID2D1RenderTarget* target,
        const std::wstring& text,
        const Rect& rect,
        Color color,
        const std::string& fontName,
        float fontSize,
        DWRITE_TEXT_ALIGNMENT align,
        DWRITE_PARAGRAPH_ALIGNMENT vAlign,
        DWRITE_FONT_WEIGHT weight,
        DWRITE_FONT_STYLE style,
        DWRITE_FONT_STRETCH stretch,
        bool underline,
        bool strikethrough,
        D2D1_TEXT_ANTIALIAS_MODE antialiasMode,
        bool truncateWithEllipsis = false);
    void DrawTextLayoutOnTarget(
        ID2D1RenderTarget* target,
        IDWriteTextLayout* layout,
        const Rect& originRect,
        Color color,
        D2D1_TEXT_ANTIALIAS_MODE antialiasMode);

    HWND m_hwnd = nullptr;
    float m_dpiScale = 1.0f;
    bool m_usesCompositionSwapChain = false;
    bool m_supportsPerPixelAlpha = false;
    bool m_requirePerPixelAlpha = false;

    ComPtr<ID2D1Factory1> m_d2dFactory;
    ComPtr<IDWriteFactory> m_dwriteFactory;
    ComPtr<IWICImagingFactory2> m_wicFactory;

    ComPtr<ID2D1Device> m_d2dDevice;
    ComPtr<ID2D1DeviceContext> m_d2dContext;
    ComPtr<ID3D11Device> m_d3dDevice;
    ComPtr<IDXGISwapChain1> m_swapChain;
    ComPtr<IDCompositionDevice> m_dcompDevice;
    ComPtr<IDCompositionTarget> m_dcompTarget;
    ComPtr<IDCompositionVisual> m_dcompRootVisual;
    ComPtr<IDCompositionVisual> m_dcompVisual;
    // Reused for DComp overlay surface draws (ProgressBar indeterminate).
    ComPtr<ID2D1DeviceContext> m_overlayD2dContext;

    RenderResources m_resources;
    std::vector<D2D1_RECT_F> m_clipStack;
    std::vector<bool> m_clipIsLayer; // true = PushLayer (rounded), false = axis-aligned
    std::vector<float> m_opacityStack;
    std::vector<D2D1_MATRIX_3X2_F> m_transformStack;
    std::vector<InheritedTextStyle> m_inheritedTextStyleStack;
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

    std::shared_ptr<DisplayList> m_activeDisplayList;
    std::vector<std::shared_ptr<DisplayList>> m_displayListStack;

    // Cached HICON → D2D bitmap conversions (cleared with device resources).
    std::unordered_map<HICON, ComPtr<ID2D1Bitmap>> m_iconBitmapCache;
    ID2D1Bitmap* GetOrCreateIconBitmap(HICON icon);

    struct SvgCacheEntry {
        ComPtr<ID2D1SvgDocument> doc;
        D2D1_SIZE_F viewport{ 24.0f, 24.0f };
    };
    std::unordered_map<std::string, SvgCacheEntry> m_svgCache;
    ComPtr<ID2D1DeviceContext5> GetSvgContext();
    const SvgCacheEntry* GetOrCreateSvg(
        const std::string& source,
        const D2D1_COLOR_F* tint,
        const D2D1_COLOR_F* foreground);
};

float SnapFontSizeToDevicePixels(float fontSizeDips, float dpiScale);

} // namespace CUI
