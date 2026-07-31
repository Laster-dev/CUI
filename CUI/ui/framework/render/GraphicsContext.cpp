#define NOMINMAX
#include "GraphicsContext.h"
#include <d3d11.h>
#include <dxgi1_2.h>
#include <stdexcept>
#include <algorithm>
#include <cmath>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "ole32.lib")

namespace CUI {

GraphicsContext::GraphicsContext() {}

GraphicsContext::~GraphicsContext() {
    ReleaseDeviceResources();
}

bool GraphicsContext::Initialize(HWND hwnd) {
    m_hwnd = hwnd;

    HRESULT hr = CreateDeviceIndependentResources();
    if (FAILED(hr)) return false;

    hr = CreateDeviceResources();
    if (FAILED(hr)) return false;

    return true;
}

HRESULT GraphicsContext::CreateDeviceIndependentResources() {
    D2D1_FACTORY_OPTIONS options = {};
#ifdef _DEBUG
    options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &options, &m_d2dFactory);
    if (FAILED(hr)) return hr;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &m_dwriteFactory);
    if (FAILED(hr)) return hr;

    hr = CoCreateInstance(CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER, __uuidof(IWICImagingFactory2), &m_wicFactory);
    return hr;
}

HRESULT GraphicsContext::CreateDeviceResources() {
    if (!m_hwnd) return E_POINTER;

    RECT rc;
    GetClientRect(m_hwnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;
    if (width == 0) width = 1;
    if (height == 0) height = 1;

    // Create D3D11 Device
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    ComPtr<ID3D11Device> d3dDevice;
    ComPtr<ID3D11DeviceContext> d3dContext;
    D3D_FEATURE_LEVEL featureLevel;

    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        0,
        creationFlags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &d3dDevice,
        &featureLevel,
        &d3dContext
    );

    if (FAILED(hr)) {
        // Fallback to WARP if hardware fails
        hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            0,
            creationFlags,
            featureLevels,
            ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION,
            &d3dDevice,
            &featureLevel,
            &d3dContext
        );
    }
    if (FAILED(hr)) return hr;

    ComPtr<IDXGIDevice> dxgiDevice;
    hr = d3dDevice.As(&dxgiDevice);
    if (FAILED(hr)) return hr;

    ComPtr<ID2D1Device> d2dDevice;
    hr = m_d2dFactory->CreateDevice(dxgiDevice.Get(), &d2dDevice);
    if (FAILED(hr)) return hr;

    hr = d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_d2dContext);
    if (FAILED(hr)) return hr;

    ComPtr<IDXGIAdapter> dxgiAdapter;
    hr = dxgiDevice->GetAdapter(&dxgiAdapter);
    if (FAILED(hr)) return hr;

    ComPtr<IDXGIFactory2> dxgiFactory;
    hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(hr)) return hr;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    hr = dxgiFactory->CreateSwapChainForHwnd(
        d3dDevice.Get(),
        m_hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &m_swapChain
    );
    if (FAILED(hr)) return hr;

    ComPtr<IDXGISurface> dxgiBackBuffer;
    hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer));
    if (FAILED(hr)) return hr;

    D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );

    ComPtr<ID2D1Bitmap1> d2dTargetBitmap;
    hr = m_d2dContext->CreateBitmapFromDxgiSurface(
        dxgiBackBuffer.Get(),
        &bitmapProperties,
        &d2dTargetBitmap
    );
    if (FAILED(hr)) return hr;

    m_d2dContext->SetTarget(d2dTargetBitmap.Get());

    // DPI setting
    float dpiX = 96.0f, dpiY = 96.0f;
    UINT dpi = GetDpiForWindow(m_hwnd);
    if (dpi > 0) {
        dpiX = (float)dpi;
        dpiY = (float)dpi;
    }
    m_dpiScale = dpiX / 96.0f;
    m_d2dContext->SetDpi(dpiX, dpiY);

    m_resources.Initialize(m_d2dContext.Get(), m_dwriteFactory.Get());
    return S_OK;
}

void GraphicsContext::Resize(UINT width, UINT height) {
    if (!m_d2dContext || !m_swapChain) return;

    m_d2dContext->SetTarget(nullptr);
    m_resources.ReleaseDeviceResources();

    if (width == 0) width = 1;
    if (height == 0) height = 1;

    HRESULT hr = m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (SUCCEEDED(hr)) {
        ComPtr<IDXGISurface> dxgiBackBuffer;
        m_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer));

        D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
        );

        ComPtr<ID2D1Bitmap1> d2dTargetBitmap;
        m_d2dContext->CreateBitmapFromDxgiSurface(dxgiBackBuffer.Get(), &bitmapProperties, &d2dTargetBitmap);
        m_d2dContext->SetTarget(d2dTargetBitmap.Get());
        m_resources.Initialize(m_d2dContext.Get(), m_dwriteFactory.Get());
    }
}

void GraphicsContext::ReleaseDeviceResources() {
    m_resources.ReleaseDeviceResources();
    m_d2dContext.Reset();
    m_swapChain.Reset();
}

void GraphicsContext::BeginDraw() {
    if (m_d2dContext) {
        m_d2dContext->BeginDraw();
        m_d2dContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        m_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
    }
}

HRESULT GraphicsContext::EndDraw() {
    if (!m_d2dContext) return E_POINTER;

    HRESULT hr = m_d2dContext->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        ReleaseDeviceResources();
        CreateDeviceResources();
        return hr;
    }

    if (m_swapChain) {
        // Pace animation to the compositor refresh rate for smoother scrolling/toast motion.
        m_swapChain->Present(1, 0);
    }
    return hr;
}

void GraphicsContext::PushClip(const Rect& rect) {
    if (m_d2dContext) {
        D2D1_RECT_F d2dRect = rect.ToD2D();
        m_clipStack.push_back(d2dRect);
        m_d2dContext->PushAxisAlignedClip(d2dRect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    }
}

void GraphicsContext::PopClip() {
    if (m_d2dContext && !m_clipStack.empty()) {
        m_d2dContext->PopAxisAlignedClip();
        m_clipStack.pop_back();
    }
}

void GraphicsContext::DrawRect(const Rect& rect, D2D1_COLOR_F color, float strokeWidth) {
    if (auto brush = m_resources.GetSolidBrush(color)) {
        m_d2dContext->DrawRectangle(rect.ToD2D(), brush, strokeWidth);
    }
}

void GraphicsContext::FillRect(const Rect& rect, D2D1_COLOR_F color) {
    if (auto brush = m_resources.GetSolidBrush(color)) {
        m_d2dContext->FillRectangle(rect.ToD2D(), brush);
    }
}

void GraphicsContext::FillRoundedRect(const Rect& rect, float radius, D2D1_COLOR_F color) {
    if (auto brush = m_resources.GetSolidBrush(color)) {
        D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(rect.ToD2D(), radius, radius);
        m_d2dContext->FillRoundedRectangle(rr, brush);
    }
}

void GraphicsContext::DrawRoundedRect(const Rect& rect, float radius, D2D1_COLOR_F color, float strokeWidth) {
    if (auto brush = m_resources.GetSolidBrush(color)) {
        D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(rect.ToD2D(), radius, radius);
        m_d2dContext->DrawRoundedRectangle(rr, brush, strokeWidth);
    }
}

void GraphicsContext::DrawLine(Point p1, Point p2, D2D1_COLOR_F color, float strokeWidth) {
    if (auto brush = m_resources.GetSolidBrush(color)) {
        m_d2dContext->DrawLine(D2D1::Point2F(p1.x, p1.y), D2D1::Point2F(p2.x, p2.y), brush, strokeWidth);
    }
}

void GraphicsContext::DrawText(const std::string& text, const Rect& rect, D2D1_COLOR_F color,
                               const std::string& fontName, float fontSize,
                               DWRITE_TEXT_ALIGNMENT align, DWRITE_PARAGRAPH_ALIGNMENT vAlign,
                               DWRITE_FONT_WEIGHT weight) {
    if (text.empty()) return;

    auto brush = m_resources.GetSolidBrush(color);
    auto format = m_resources.GetTextFormat(fontName, fontSize, weight);
    if (brush && format) {
        format->SetTextAlignment(align);
        format->SetParagraphAlignment(vAlign);
        format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

        DWRITE_TRIMMING trimming = { DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0 };
        format->SetTrimming(&trimming, nullptr);

        std::wstring wText = Utf8ToUtf16(text);
        m_d2dContext->DrawText(
            wText.c_str(),
            static_cast<UINT32>(wText.length()),
            format,
            rect.ToD2D(),
            brush,
            D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
        );
    }
}

Size GraphicsContext::MeasureText(const std::string& text, const std::string& fontName, float fontSize, DWRITE_FONT_WEIGHT weight) {
    if (text.empty()) return Size(0, fontSize + 4);

    ComPtr<IDWriteFactory> factory = m_dwriteFactory;
    if (!factory) {
        static ComPtr<IDWriteFactory> s_sharedDWriteFactory;
        if (!s_sharedDWriteFactory) {
            DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &s_sharedDWriteFactory);
        }
        factory = s_sharedDWriteFactory;
    }
    if (!factory) return Size(text.length() * fontSize * 0.65f, fontSize + 4);

    std::wstring wFont = Utf8ToUtf16(fontName);
    ComPtr<IDWriteTextFormat> format;
    HRESULT hr = factory->CreateTextFormat(
        wFont.c_str(),
        nullptr,
        weight,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        fontSize,
        L"en-US",
        &format
    );
    if (FAILED(hr)) return Size(text.length() * fontSize * 0.65f, fontSize + 4);

    std::wstring wText = Utf8ToUtf16(text);
    ComPtr<IDWriteTextLayout> layout;
    hr = factory->CreateTextLayout(
        wText.c_str(),
        static_cast<UINT32>(wText.length()),
        format.Get(),
        10000.0f,
        10000.0f,
        &layout
    );

    if (SUCCEEDED(hr)) {
        DWRITE_TEXT_METRICS metrics;
        layout->GetMetrics(&metrics);
        return Size(metrics.width, metrics.height);
    }

    return Size(text.length() * fontSize * 0.65f, fontSize + 4);
}

ComPtr<IDWriteFactory> GraphicsContext::GetSharedWriteFactory() {
    static ComPtr<IDWriteFactory> s_sharedDWriteFactory;
    if (!s_sharedDWriteFactory) {
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &s_sharedDWriteFactory);
    }
    return s_sharedDWriteFactory;
}

ComPtr<IDWriteTextLayout> GraphicsContext::CreateTextLayout(
    const std::wstring& text,
    const std::string& fontName,
    float fontSize,
    const TextLayoutOptions& options,
    DWRITE_FONT_WEIGHT weight) {

    ComPtr<IDWriteFactory> factory = GetSharedWriteFactory();
    if (!factory) return nullptr;

    const std::wstring& targetText = text.empty() ? L" " : text;

    std::wstring wFont = Utf8ToUtf16(fontName);
    ComPtr<IDWriteTextFormat> format;
    HRESULT hr = factory->CreateTextFormat(
        wFont.c_str(),
        nullptr,
        weight,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        fontSize,
        L"zh-CN",
        &format
    );
    if (FAILED(hr)) return nullptr;

    format->SetWordWrapping(options.wrapping);
    format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    format->SetParagraphAlignment(options.paragraphAlignment);

    ComPtr<IDWriteTextLayout> layout;
    hr = factory->CreateTextLayout(
        targetText.c_str(),
        static_cast<UINT32>(targetText.length()),
        format.Get(),
        options.maxWidth,
        options.maxHeight,
        &layout
    );
    if (FAILED(hr)) return nullptr;

    if (options.lineHeight > 0.0f) {
        float baseline = options.lineHeight * 0.8f;
        layout->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, options.lineHeight, baseline);
    } else if (options.lineSpacing > 0.0f && std::abs(options.lineSpacing - 1.0f) > 0.001f) {
        float baseline = options.lineSpacing * 0.8f;
        layout->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_PROPORTIONAL, options.lineSpacing, baseline);
    }

    return layout;
}

void GraphicsContext::DrawTextLayout(IDWriteTextLayout* layout, const Rect& originRect, D2D1_COLOR_F color) {
    if (!layout || !m_d2dContext) return;

    auto brush = m_resources.GetSolidBrush(color);
    if (!brush) return;

    m_d2dContext->DrawTextLayout(
        D2D1::Point2F(originRect.x, originRect.y),
        layout,
        brush,
        D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
    );
}

GraphicsContext::TextCaretInfo GraphicsContext::GetTextCaretInfo(IDWriteTextLayout* layout, UINT32 position, const Point& origin) {
    TextCaretInfo info;
    if (!layout) return info;

    DWRITE_HIT_TEST_METRICS metrics = {};
    float x = 0.0f;
    float y = 0.0f;
    if (SUCCEEDED(layout->HitTestTextPosition(position, FALSE, &x, &y, &metrics))) {
        info.x = origin.x + x;
        info.y = origin.y + y;
        info.height = metrics.height;
        if (info.height <= 0.0f) {
            DWRITE_TEXT_METRICS textMetrics = {};
            layout->GetMetrics(&textMetrics);
            info.height = std::max(16.0f, textMetrics.height);
        }
    }
    return info;
}

UINT32 GraphicsContext::HitTestTextLayout(IDWriteTextLayout* layout, float x, float y, const Point& origin) {
    if (!layout) return 0;

    float relX = x - origin.x;
    float relY = y - origin.y;
    BOOL trailing = FALSE;
    BOOL inside = FALSE;
    DWRITE_HIT_TEST_METRICS metrics = {};
    if (SUCCEEDED(layout->HitTestPoint(relX, relY, &trailing, &inside, &metrics))) {
        UINT32 pos = metrics.textPosition;
        if (trailing && pos < UINT32_MAX) {
            pos++;
        }
        return pos;
    }
    return 0;
}

bool GraphicsContext::GetTextSelectionBounds(IDWriteTextLayout* layout, UINT32 start, UINT32 end,
                                             const Point& origin, std::vector<D2D1_RECT_F>& rects) {
    rects.clear();
    if (!layout || start == end) return false;

    UINT32 selMin = (std::min)(start, end);
    UINT32 selMax = (std::max)(start, end);
    UINT32 length = selMax - selMin;

    UINT32 count = 0;
    layout->HitTestTextRange(selMin, length, origin.x, origin.y, nullptr, 0, &count);
    if (count == 0) return false;

    std::vector<DWRITE_HIT_TEST_METRICS> metrics(count);
    layout->HitTestTextRange(selMin, length, origin.x, origin.y, metrics.data(), count, &count);

    rects.reserve(count);
    for (UINT32 i = 0; i < count; ++i) {
        const DWRITE_HIT_TEST_METRICS& m = metrics[i];
        rects.push_back(D2D1::RectF(m.left, m.top, m.left + m.width, m.top + m.height));
    }
    return !rects.empty();
}

} // namespace CUI
