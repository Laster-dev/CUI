#pragma once
#include "RenderResources.h"
#include "../core/Value.h"
#include <d2d1_1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <vector>

namespace CUI {

using Microsoft::WRL::ComPtr;

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

    void PushClip(const Rect& rect);
    void PopClip();

    void DrawRect(const Rect& rect, D2D1_COLOR_F color, float strokeWidth = 1.0f);
    void FillRect(const Rect& rect, D2D1_COLOR_F color);
    void FillRoundedRect(const Rect& rect, float radius, D2D1_COLOR_F color);
    void DrawRoundedRect(const Rect& rect, float radius, D2D1_COLOR_F color, float strokeWidth = 1.0f);
    void DrawLine(Point p1, Point p2, D2D1_COLOR_F color, float strokeWidth = 1.0f);
    void DrawText(const std::string& text, const Rect& rect, D2D1_COLOR_F color,
                  const std::string& fontName = "Segoe UI", float fontSize = 13.0f,
                  DWRITE_TEXT_ALIGNMENT align = DWRITE_TEXT_ALIGNMENT_LEADING,
                  DWRITE_PARAGRAPH_ALIGNMENT vAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
                  DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL);

    Size MeasureText(const std::string& text, const std::string& fontName = "Segoe UI",
                     float fontSize = 13.0f, DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL);

    float GetDpiScale() const { return m_dpiScale; }

private:
    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceResources();

    HWND m_hwnd = nullptr;
    float m_dpiScale = 1.0f;

    ComPtr<ID2D1Factory1> m_d2dFactory;
    ComPtr<IDWriteFactory> m_dwriteFactory;
    ComPtr<IWICImagingFactory2> m_wicFactory;

    ComPtr<ID2D1DeviceContext> m_d2dContext;
    ComPtr<IDXGISwapChain1> m_swapChain;

    RenderResources m_resources;
    std::vector<D2D1_RECT_F> m_clipStack;
};

} // namespace CUI
