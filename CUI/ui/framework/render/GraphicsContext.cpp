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
#pragma comment(lib, "dcomp.lib")
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

HRESULT GraphicsContext::BindSwapChainTarget(float dpiX, float dpiY) {
    ComPtr<IDXGISurface> dxgiBackBuffer;
    HRESULT hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer));
    if (FAILED(hr)) return hr;

    D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        dpiX,
        dpiY
    );

    ComPtr<ID2D1Bitmap1> d2dTargetBitmap;
    hr = m_d2dContext->CreateBitmapFromDxgiSurface(
        dxgiBackBuffer.Get(),
        &bitmapProperties,
        &d2dTargetBitmap
    );
    if (FAILED(hr)) return hr;

    m_d2dContext->SetTarget(d2dTargetBitmap.Get());
    m_dpiScale = dpiX / 96.0f;
    m_d2dContext->SetDpi(dpiX, dpiY);
    return S_OK;
}

HRESULT GraphicsContext::CreateDeviceResources() {
    if (!m_hwnd) return E_POINTER;

    RECT rc;
    GetClientRect(m_hwnd, &rc);
    UINT width = static_cast<UINT>(std::max<LONG>(1, rc.right - rc.left));
    UINT height = static_cast<UINT>(std::max<LONG>(1, rc.bottom - rc.top));

    float dpiX = 96.0f;
    float dpiY = 96.0f;
    UINT dpi = GetDpiForWindow(m_hwnd);
    if (dpi > 0) {
        dpiX = static_cast<float>(dpi);
        dpiY = static_cast<float>(dpi);
    }

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

    hr = m_d2dFactory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice);
    if (FAILED(hr)) return hr;

    hr = m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_d2dContext);
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
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

    m_usesCompositionSwapChain = false;
    m_supportsPerPixelAlpha = false;

#ifndef WS_EX_NOREDIRECTIONBITMAP
#define WS_EX_NOREDIRECTIONBITMAP 0x00200000L
#endif

    // HWND flip swap chains do NOT composite per-pixel alpha (PREMULTIPLIED is
    // unsupported / ignored). Always use Composition + WS_EX_NOREDIRECTIONBITMAP
    // so translucent chrome can reveal DWM Mica/Acrylic.
    const LONG_PTR ex = GetWindowLongPtr(m_hwnd, GWL_EXSTYLE);
    if ((ex & WS_EX_NOREDIRECTIONBITMAP) == 0) {
        SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, ex | WS_EX_NOREDIRECTIONBITMAP);
        SetWindowPos(
            m_hwnd, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE
        );
    }

    hr = dxgiFactory->CreateSwapChainForComposition(
        d3dDevice.Get(),
        &swapChainDesc,
        nullptr,
        &m_swapChain
    );
    if (SUCCEEDED(hr)) {
        hr = DCompositionCreateDevice(dxgiDevice.Get(), IID_PPV_ARGS(&m_dcompDevice));
        if (SUCCEEDED(hr)) {
            hr = m_dcompDevice->CreateTargetForHwnd(m_hwnd, TRUE, &m_dcompTarget);
        }
        if (SUCCEEDED(hr)) {
            hr = m_dcompDevice->CreateVisual(&m_dcompVisual);
        }
        if (SUCCEEDED(hr)) {
            hr = m_dcompVisual->SetContent(m_swapChain.Get());
        }
        if (SUCCEEDED(hr)) {
            hr = m_dcompTarget->SetRoot(m_dcompVisual.Get());
        }
        if (SUCCEEDED(hr)) {
            hr = m_dcompDevice->Commit();
        }
        if (SUCCEEDED(hr)) {
            m_usesCompositionSwapChain = true;
            m_supportsPerPixelAlpha = true;
        } else {
            m_dcompVisual.Reset();
            m_dcompTarget.Reset();
            m_dcompDevice.Reset();
            m_swapChain.Reset();
        }
    }

    if (!m_swapChain) {
        // Opaque fallback — material will not show through (label shows 无透).
        swapChainDesc.Scaling = DXGI_SCALING_NONE;
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
        m_supportsPerPixelAlpha = false;
    }

    dxgiFactory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER);

    hr = BindSwapChainTarget(dpiX, dpiY);
    if (FAILED(hr)) return hr;

    m_resources.Initialize(m_d2dContext.Get(), m_dwriteFactory.Get());
    return S_OK;
}

void GraphicsContext::Resize(UINT width, UINT height) {
    if (!m_d2dContext || !m_swapChain) return;

    m_d2dContext->SetTarget(nullptr);
    m_d2dContext->Flush();
    m_resources.ReleaseDeviceResources();

    if (width == 0) width = 1;
    if (height == 0) height = 1;

    HRESULT hr = m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) {
        ReleaseDeviceResources();
        CreateDeviceResources();
        return;
    }

    float dpiX = 96.0f;
    float dpiY = 96.0f;
    UINT dpi = GetDpiForWindow(m_hwnd);
    if (dpi > 0) {
        dpiX = static_cast<float>(dpi);
        dpiY = static_cast<float>(dpi);
    }

    hr = BindSwapChainTarget(dpiX, dpiY);
    if (FAILED(hr)) {
        ReleaseDeviceResources();
        CreateDeviceResources();
        return;
    }

    if (m_usesCompositionSwapChain && m_dcompDevice) {
        m_dcompDevice->Commit();
    }

    m_resources.Initialize(m_d2dContext.Get(), m_dwriteFactory.Get());
}

void GraphicsContext::ReleaseDeviceResources() {
    m_resources.ReleaseDeviceResources();
    if (m_d2dContext) {
        m_d2dContext->SetTarget(nullptr);
    }
    m_dcompVisual.Reset();
    m_dcompTarget.Reset();
    m_dcompDevice.Reset();
    m_swapChain.Reset();
    m_d2dContext.Reset();
    m_d2dDevice.Reset();
    m_usesCompositionSwapChain = false;
    m_supportsPerPixelAlpha = false;
}

void GraphicsContext::BeginDraw() {
    if (m_d2dContext) {
        m_d2dContext->BeginDraw();
        m_d2dContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        // ClearType requires an opaque backdrop. Layer caches and material clears
        // are often transparent — grayscale keeps glyphs readable everywhere.
        m_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
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
        if (m_usesCompositionSwapChain && m_dcompDevice) {
            m_dcompDevice->Commit();
        }
    }
    return hr;
}

inline float SnapPixel(float val) {
    return std::floor(val + 0.5f);
}

inline D2D1_RECT_F SnapRectForStroke(const Rect& rect, float strokeWidth) {
    float halfStroke = strokeWidth * 0.5f;
    return D2D1::RectF(
        std::floor(rect.x) + halfStroke,
        std::floor(rect.y) + halfStroke,
        std::floor(rect.x + rect.width) - halfStroke,
        std::floor(rect.y + rect.height) - halfStroke
    );
}

inline D2D1_RECT_F SnapRectForFill(const Rect& rect) {
    return D2D1::RectF(
        std::floor(rect.x),
        std::floor(rect.y),
        std::floor(rect.x + rect.width),
        std::floor(rect.y + rect.height)
    );
}

void GraphicsContext::PushClip(const Rect& rect) {
    if (m_d2dContext) {
        D2D1_RECT_F d2dRect = SnapRectForFill(rect);
        m_clipStack.push_back(d2dRect);
        m_d2dContext->PushAxisAlignedClip(d2dRect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    }
}

void GraphicsContext::PushOpacity(float opacity) {
    if (!m_d2dContext) {
        return;
    }
    opacity = std::clamp(opacity, 0.0f, 1.0f);
    if (opacity >= 0.999f) {
        return;
    }
    D2D1_LAYER_PARAMETERS layerParams = D2D1::LayerParameters(
        D2D1::InfiniteRect(),
        nullptr,
        D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
        D2D1::IdentityMatrix(),
        opacity,
        nullptr,
        D2D1_LAYER_OPTIONS_NONE
    );
    m_d2dContext->PushLayer(layerParams, nullptr);
    m_opacityStack.push_back(opacity);
}

void GraphicsContext::PopOpacity() {
    if (!m_d2dContext || m_opacityStack.empty()) {
        return;
    }
    m_opacityStack.pop_back();
    m_d2dContext->PopLayer();
}

bool GraphicsContext::EnsureLayerCache(RenderLayer& layer, Size sizeInDips) {
    if (!m_d2dDevice) {
        return false;
    }

    const float width = (std::max)(1.0f, std::ceil(sizeInDips.width));
    const float height = (std::max)(1.0f, std::ceil(sizeInDips.height));

    const bool recreate = !layer.m_cacheContext
        || !layer.m_cacheBitmap
        || std::abs(layer.m_cacheSurfaceSize.width - width) > 0.5f
        || std::abs(layer.m_cacheSurfaceSize.height - height) > 0.5f;

    if (!recreate) {
        return true;
    }

    layer.ResetCache();

    HRESULT hr = m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &layer.m_cacheContext);
    if (FAILED(hr) || !layer.m_cacheContext) {
        layer.ResetCache();
        return false;
    }

    const float dpi = 96.0f * m_dpiScale;
    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        dpi,
        dpi
    );

    hr = layer.m_cacheContext->CreateBitmap(
        D2D1::SizeU(static_cast<UINT32>(width), static_cast<UINT32>(height)),
        nullptr,
        0,
        &props,
        &layer.m_cacheBitmap
    );
    if (FAILED(hr) || !layer.m_cacheBitmap) {
        layer.ResetCache();
        return false;
    }

    hr = layer.m_cacheContext->CreateBitmap(
        D2D1::SizeU(static_cast<UINT32>(width), static_cast<UINT32>(height)),
        nullptr,
        0,
        &props,
        &layer.m_scratchBitmap
    );
    if (FAILED(hr) || !layer.m_scratchBitmap) {
        layer.ResetCache();
        return false;
    }

    layer.m_cacheContext->SetTarget(layer.m_cacheBitmap.Get());
    layer.SetCacheSurfaceSize(Size(width, height));
    layer.Invalidate(RenderLayer::SizeDirty | RenderLayer::ContentDirty);
    return true;
}

ID2D1DeviceContext* GraphicsContext::BeginLayerDraw(RenderLayer& layer) {
    if (!layer.m_cacheContext || !layer.m_cacheBitmap) {
        return nullptr;
    }

    layer.m_cacheContext->BeginDraw();
    layer.m_cacheContext->SetTarget(layer.m_cacheBitmap.Get());
    layer.m_cacheContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    layer.m_cacheContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    return layer.m_cacheContext.Get();
}

void GraphicsContext::EndLayerDraw(RenderLayer& layer) {
    if (!layer.m_cacheContext) {
        return;
    }

    HRESULT hr = layer.m_cacheContext->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        layer.ResetCache();
    }
}

void GraphicsContext::DrawLayer(const RenderLayer& layer, const Rect& destRect, const Rect* sourceRect, float opacity) {
    if (!m_d2dContext || !layer.m_cacheBitmap) {
        return;
    }

    D2D1_RECT_F dest = destRect.ToD2D();
    const D2D1_RECT_F* src = nullptr;
    D2D1_RECT_F srcRect = {};
    if (sourceRect && !sourceRect->IsEmpty()) {
        srcRect = sourceRect->ToD2D();
        src = &srcRect;
    }

    m_d2dContext->DrawBitmap(
        layer.m_cacheBitmap.Get(),
        &dest,
        opacity,
        D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
        src
    );
}

bool GraphicsContext::PushLayerTarget(RenderLayer& layer, Size sizeInDips, const Rect& paintBounds, D2D1_COLOR_F clearColor, bool clearTarget) {
    if (!EnsureLayerCache(layer, sizeInDips)) {
        return false;
    }

    TargetState state;
    state.context = m_d2dContext;
    state.paintBounds = m_paintBounds;
    state.clipStack = m_clipStack;
    state.opacityStack = m_opacityStack;
    m_targetStack.push_back(std::move(state));

    m_d2dContext = layer.m_cacheContext;
    m_paintBounds = paintBounds;
    m_clipStack.clear();
    m_opacityStack.clear();
    m_resources.Initialize(m_d2dContext.Get(), m_dwriteFactory.Get());

    auto* layerContext = BeginLayerDraw(layer);
    if (!layerContext) {
        PopLayerTarget(layer);
        return false;
    }

    if (clearTarget) {
        layerContext->Clear(clearColor);
    }
    return true;
}

void GraphicsContext::ClearRect(const Rect& rect) {
    if (!m_d2dContext || rect.IsEmpty()) {
        return;
    }
    // Expand outward to whole pixels so partial dirty clears never leave 1px stale seams.
    const D2D1_RECT_F clearRc = D2D1::RectF(
        std::floor(rect.x),
        std::floor(rect.y),
        std::ceil(rect.x + rect.width),
        std::ceil(rect.y + rect.height)
    );
    const D2D1_PRIMITIVE_BLEND oldBlend = m_d2dContext->GetPrimitiveBlend();
    m_d2dContext->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_COPY);
    if (auto brush = m_resources.GetSolidBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f))) {
        m_d2dContext->FillRectangle(clearRc, brush);
    }
    m_d2dContext->SetPrimitiveBlend(oldBlend);
}

void GraphicsContext::PopLayerTarget(RenderLayer& layer) {
    EndLayerDraw(layer);

    if (m_targetStack.empty()) {
        return;
    }

    TargetState state = std::move(m_targetStack.back());
    m_targetStack.pop_back();

    m_d2dContext = state.context;
    m_paintBounds = state.paintBounds;
    m_clipStack = std::move(state.clipStack);
    m_opacityStack = std::move(state.opacityStack);
    m_resources.Initialize(m_d2dContext.Get(), m_dwriteFactory.Get());
}

void GraphicsContext::PopClip() {
    if (m_d2dContext && !m_clipStack.empty()) {
        m_d2dContext->PopAxisAlignedClip();
        m_clipStack.pop_back();
    }
}

void GraphicsContext::DrawRect(const Rect& rect, D2D1_COLOR_F color, float strokeWidth) {
    if (auto brush = m_resources.GetSolidBrush(color)) {
        m_d2dContext->DrawRectangle(SnapRectForStroke(rect, strokeWidth), brush, strokeWidth);
    }
}

void GraphicsContext::FillRect(const Rect& rect, D2D1_COLOR_F color) {
    if (auto brush = m_resources.GetSolidBrush(color)) {
        m_d2dContext->FillRectangle(SnapRectForFill(rect), brush);
    }
}

void GraphicsContext::FillRoundedRect(const Rect& rect, float radius, D2D1_COLOR_F color) {
    if (auto brush = m_resources.GetSolidBrush(color)) {
        D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(SnapRectForFill(rect), radius, radius);
        m_d2dContext->FillRoundedRectangle(rr, brush);
    }
}

void GraphicsContext::DrawRoundedRect(const Rect& rect, float radius, D2D1_COLOR_F color, float strokeWidth) {
    if (auto brush = m_resources.GetSolidBrush(color)) {
        D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(SnapRectForStroke(rect, strokeWidth), radius, radius);
        m_d2dContext->DrawRoundedRectangle(rr, brush, strokeWidth);
    }
}

void GraphicsContext::DrawLine(Point p1, Point p2, D2D1_COLOR_F color, float strokeWidth) {
    if (auto brush = m_resources.GetSolidBrush(color)) {
        float offset = (static_cast<int>(strokeWidth) % 2 == 1) ? 0.5f : 0.0f;
        float x1 = std::floor(p1.x) + offset;
        float y1 = std::floor(p1.y) + offset;
        float x2 = std::floor(p2.x) + offset;
        float y2 = std::floor(p2.y) + offset;
        m_d2dContext->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), brush, strokeWidth);
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

        DWRITE_TRIMMING trimming = { DWRITE_TRIMMING_GRANULARITY_NONE, 0, 0 };
        format->SetTrimming(&trimming, nullptr);

        std::wstring wText = Utf8ToUtf16(text);
        // Ceil size so subpixel measure widths are not floored away — floor(width)
        // + CHARACTER trimming previously clipped the last glyph ("少最后一个字").
        Rect snappedRect(
            std::floor(rect.x),
            std::floor(rect.y),
            std::ceil(rect.width),
            std::ceil(rect.height)
        );
        m_d2dContext->DrawText(
            wText.c_str(),
            static_cast<UINT32>(wText.length()),
            format,
            snappedRect.ToD2D(),
            brush,
            D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT | D2D1_DRAW_TEXT_OPTIONS_NO_SNAP
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
        // Include trailing whitespace and a 1px overhang slack for glyph side bearings.
        const float w = (std::max)(metrics.widthIncludingTrailingWhitespace, metrics.width) + 1.0f;
        const float h = (std::max)(metrics.height, fontSize + 2.0f);
        return Size(w, h);
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
        D2D1::Point2F(std::floor(originRect.x), std::floor(originRect.y)),
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
