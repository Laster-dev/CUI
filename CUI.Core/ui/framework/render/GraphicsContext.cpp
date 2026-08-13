#define NOMINMAX
#include "GraphicsContext.h"
#include <d3d11.h>
#include <d3d11_4.h>
#include <dxgi1_2.h>
#include <shlwapi.h>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dcomp.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shlwapi.lib")

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

void GraphicsContext::SetRequirePerPixelAlpha(bool enabled) {
    if (m_requirePerPixelAlpha == enabled) {
        return;
    }
    m_requirePerPixelAlpha = enabled;
    if (m_hwnd && m_d2dContext) {
        ReleaseDeviceResources();
        CreateDeviceResources();
    }
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

    m_d3dDevice = d3dDevice;
    // Required before any secondary D2D device context / raster worker touches the GPU.
    {
        ComPtr<ID3D11Multithread> mt;
        if (SUCCEEDED(m_d3dDevice.As(&mt)) && mt) {
            mt->SetMultithreadProtected(TRUE);
        }
    }

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

    const LONG_PTR ex = GetWindowLongPtr(m_hwnd, GWL_EXSTYLE);
    if (m_requirePerPixelAlpha) {
        // HWND flip swap chains do NOT composite per-pixel alpha (PREMULTIPLIED is
        // unsupported / ignored). Use Composition only when the window actually
        // needs per-pixel alpha for transparent content or system materials.
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
                hr = m_dcompDevice->CreateVisual(&m_dcompRootVisual);
            }
            if (SUCCEEDED(hr)) {
                hr = m_dcompDevice->CreateVisual(&m_dcompVisual);
            }
            if (SUCCEEDED(hr)) {
                hr = m_dcompVisual->SetContent(m_swapChain.Get());
            }
            if (SUCCEEDED(hr)) {
                hr = m_dcompRootVisual->AddVisual(m_dcompVisual.Get(), FALSE, nullptr);
            }
            if (SUCCEEDED(hr)) {
                hr = m_dcompTarget->SetRoot(m_dcompRootVisual.Get());
            }
            if (SUCCEEDED(hr)) {
                hr = m_dcompDevice->Commit();
            }
            if (SUCCEEDED(hr)) {
                m_usesCompositionSwapChain = true;
                m_supportsPerPixelAlpha = true;
            } else {
                m_dcompVisual.Reset();
                m_dcompRootVisual.Reset();
                m_dcompTarget.Reset();
                m_dcompDevice.Reset();
                m_swapChain.Reset();
            }
        }
    }

    if (!m_swapChain) {
        if ((ex & WS_EX_NOREDIRECTIONBITMAP) != 0) {
            SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, ex & ~static_cast<LONG_PTR>(WS_EX_NOREDIRECTIONBITMAP));
            SetWindowPos(
                m_hwnd, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE
            );
        }
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
    m_svgCache.clear();
    m_iconBitmapCache.clear();
    m_resources.ReleaseDeviceResources();
    if (m_d2dContext) {
        m_d2dContext->SetTarget(nullptr);
    }
    m_dcompVisual.Reset();
    m_dcompRootVisual.Reset();
    m_dcompTarget.Reset();
    m_dcompDevice.Reset();
    m_overlayD2dContext.Reset();
    m_swapChain.Reset();
    m_d2dContext.Reset();
    m_d2dDevice.Reset();
    m_d3dDevice.Reset();
    m_usesCompositionSwapChain = false;
    m_supportsPerPixelAlpha = false;
}

HRESULT GraphicsContext::CommitComposition() {
    if (!m_usesCompositionSwapChain || !m_dcompDevice) {
        return E_FAIL;
    }
    return m_dcompDevice->Commit();
}

bool GraphicsContext::EnsureCompositionSurface(
    ComPtr<IDCompositionSurface>& surface,
    UINT widthPx,
    UINT heightPx) {
    if (!m_dcompDevice || widthPx == 0 || heightPx == 0) {
        return false;
    }
    widthPx = (std::max)(1u, widthPx);
    heightPx = (std::max)(1u, heightPx);

    // Recreate when missing; callers pass matching size each frame.
    if (surface) {
        // IDCompositionSurface has no size getter — ProgressBar tracks size and
        // resets the ComPtr when dimensions change.
        return true;
    }

    HRESULT hr = m_dcompDevice->CreateSurface(
        widthPx,
        heightPx,
        DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_ALPHA_MODE_PREMULTIPLIED,
        &surface);
    return SUCCEEDED(hr) && surface;
}

bool GraphicsContext::DrawCompositionSurface(
    IDCompositionSurface* surface,
    float widthDips,
    float heightDips,
    const std::function<void()>& draw) {
    if (!surface || !m_d2dDevice || !draw || widthDips <= 0.0f || heightDips <= 0.0f) {
        return false;
    }

    POINT updateOffset{};
    ComPtr<IDXGISurface> dxgiSurface;
    HRESULT hr = surface->BeginDraw(nullptr, IID_PPV_ARGS(&dxgiSurface), &updateOffset);
    if (FAILED(hr) || !dxgiSurface) {
        return false;
    }

    ComPtr<ID2D1DeviceContext> overlayDc = m_overlayD2dContext;
    if (!overlayDc) {
        hr = m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &overlayDc);
        if (FAILED(hr) || !overlayDc) {
            surface->EndDraw();
            return false;
        }
        m_overlayD2dContext = overlayDc;
    }

    const float dpi = m_dpiScale * 96.0f;
    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        dpi,
        dpi);

    ComPtr<ID2D1Bitmap1> targetBmp;
    hr = overlayDc->CreateBitmapFromDxgiSurface(dxgiSurface.Get(), &props, &targetBmp);
    if (FAILED(hr) || !targetBmp) {
        surface->EndDraw();
        return false;
    }

    TargetState state;
    state.context = m_d2dContext;
    state.paintBounds = m_paintBounds;
    state.clipStack = m_clipStack;
    state.clipIsLayer = m_clipIsLayer;
    state.opacityStack = m_opacityStack;
    m_targetStack.push_back(std::move(state));

    m_d2dContext = overlayDc;
    m_paintBounds = Rect();
    m_clipStack.clear();
    m_clipIsLayer.clear();
    m_opacityStack.clear();
    m_resources.Initialize(m_d2dContext.Get(), m_dwriteFactory.Get());

    m_d2dContext->SetTarget(targetBmp.Get());
    m_d2dContext->SetDpi(dpi, dpi);
    m_d2dContext->BeginDraw();
    m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(
        static_cast<float>(updateOffset.x) * 96.0f / dpi,
        static_cast<float>(updateOffset.y) * 96.0f / dpi));
    m_d2dContext->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
    m_d2dContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    draw();

    hr = m_d2dContext->EndDraw();
    m_d2dContext->SetTarget(nullptr);
    surface->EndDraw();

    TargetState restored = std::move(m_targetStack.back());
    m_targetStack.pop_back();
    m_d2dContext = restored.context;
    m_paintBounds = restored.paintBounds;
    m_clipStack = std::move(restored.clipStack);
    m_clipIsLayer = std::move(restored.clipIsLayer);
    m_opacityStack = std::move(restored.opacityStack);
    if (m_d2dContext) {
        m_resources.Initialize(m_d2dContext.Get(), m_dwriteFactory.Get());
    }

    return SUCCEEDED(hr);
}

bool GraphicsContext::AttachCompositionOverlay(IDCompositionVisual* visual) {
    if (!m_dcompRootVisual || !visual) {
        return false;
    }
    // Place above the swapchain visual (top of z-order).
    HRESULT hr = m_dcompRootVisual->AddVisual(visual, TRUE, nullptr);
    return SUCCEEDED(hr);
}

void GraphicsContext::DetachCompositionOverlay(IDCompositionVisual* visual) {
    if (!m_dcompRootVisual || !visual) {
        return;
    }
    m_dcompRootVisual->RemoveVisual(visual);
}

void GraphicsContext::BeginDraw() {
    if (m_d2dContext) {
        m_d2dContext->BeginDraw();
        m_d2dContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        // ClearType on opaque swap-chain presents; grayscale when alpha is preserved for Mica.
        m_d2dContext->SetTextAntialiasMode(
            m_supportsPerPixelAlpha
                ? D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE
                : D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
    }
}

HRESULT GraphicsContext::EndDraw() {
    return EndDraw(nullptr);
}

HRESULT GraphicsContext::EndDraw(const RECT* dirtyRectPx) {
    if (!m_d2dContext) return E_POINTER;

    HRESULT hr = m_d2dContext->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        ReleaseDeviceResources();
        CreateDeviceResources();
        return hr;
    }

    if (m_swapChain) {
        if (dirtyRectPx
            && dirtyRectPx->left < dirtyRectPx->right
            && dirtyRectPx->top < dirtyRectPx->bottom) {
            DXGI_PRESENT_PARAMETERS params = {};
            RECT dirty = *dirtyRectPx;
            params.DirtyRectsCount = 1;
            params.pDirtyRects = &dirty;
            m_swapChain->Present1(1, 0, &params);
        } else {
            m_swapChain->Present(1, 0);
        }
        if (m_usesCompositionSwapChain && m_dcompDevice) {
            m_dcompDevice->Commit();
        }
    }
    return hr;
}

namespace {

float SnapDipToDevicePixel(float value, float dpiScale) {
    const float scale = (dpiScale > 0.001f) ? dpiScale : 1.0f;
    return std::floor(value * scale + 0.5f) / scale;
}

Rect SnapRectToDevicePixels(const Rect& rect, float dpiScale) {
    const float scale = (dpiScale > 0.001f) ? dpiScale : 1.0f;
    const float x0 = std::floor(rect.x * scale + 0.5f);
    const float y0 = std::floor(rect.y * scale + 0.5f);
    const float x1 = std::floor((rect.x + rect.width) * scale + 0.5f);
    const float y1 = std::floor((rect.y + rect.height) * scale + 0.5f);
    return Rect(
        x0 / scale,
        y0 / scale,
        (std::max)(0.0f, x1 - x0) / scale,
        (std::max)(0.0f, y1 - y0) / scale
    );
}

Rect SnapTextRect(const Rect& rect, float dpiScale) {
    const float scale = (dpiScale > 0.001f) ? dpiScale : 1.0f;
    return Rect(
        SnapDipToDevicePixel(rect.x, dpiScale),
        SnapDipToDevicePixel(rect.y, dpiScale),
        std::ceil(rect.width * scale) / scale,
        std::ceil(rect.height * scale) / scale
    );
}

float SnapFontSizeToDevicePixels(float fontSizeDips, float dpiScale) {
    const float scale = (dpiScale > 0.001f) ? dpiScale : 1.0f;
    return std::round(fontSizeDips * scale) / scale;
}

D2D1_RECT_F SnapRectForStroke(const Rect& rect, float strokeWidth, float dpiScale) {
    const float scale = (dpiScale > 0.001f) ? dpiScale : 1.0f;
    const float snappedStroke = (std::max)(1.0f / scale, std::round(strokeWidth * scale) / scale);
    const float halfStroke = snappedStroke * 0.5f;
    return D2D1::RectF(
        SnapDipToDevicePixel(rect.x, dpiScale) + halfStroke,
        SnapDipToDevicePixel(rect.y, dpiScale) + halfStroke,
        SnapDipToDevicePixel(rect.x + rect.width, dpiScale) - halfStroke,
        SnapDipToDevicePixel(rect.y + rect.height, dpiScale) - halfStroke
    );
}

D2D1_RECT_F SnapRectForFill(const Rect& rect, float dpiScale) {
    return D2D1::RectF(
        SnapDipToDevicePixel(rect.x, dpiScale),
        SnapDipToDevicePixel(rect.y, dpiScale),
        SnapDipToDevicePixel(rect.x + rect.width, dpiScale),
        SnapDipToDevicePixel(rect.y + rect.height, dpiScale)
    );
}

} // namespace

inline float SnapPixel(float val) {
    return std::floor(val + 0.5f);
}

void GraphicsContext::PushClip(const Rect& rect) {
    if (m_d2dContext) {
        D2D1_RECT_F d2dRect = SnapRectForFill(rect, m_dpiScale);
        m_clipStack.push_back(d2dRect);
        m_clipIsLayer.push_back(false);
        // Scene patch / scroll patch clips must be hard pixel clips. Using
        // per-primitive AA here blends the clip edge with the clear color and
        // produces 1px light/dark seams during partial redraws.
        m_d2dContext->PushAxisAlignedClip(d2dRect, D2D1_ANTIALIAS_MODE_ALIASED);
    }
}

void GraphicsContext::PushRoundedClip(const Rect& rect, float radius) {
    if (!m_d2dContext) {
        return;
    }
    if (radius <= 0.01f || !m_d2dFactory) {
        PushClip(rect);
        return;
    }

    const D2D1_RECT_F d2dRect = SnapRectForFill(rect, m_dpiScale);
    const float maxR = (std::min)((d2dRect.right - d2dRect.left) * 0.5f, (d2dRect.bottom - d2dRect.top) * 0.5f);
    radius = (std::min)(radius, (std::max)(0.0f, maxR));

    ComPtr<ID2D1RoundedRectangleGeometry> geometry;
    const D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(d2dRect, radius, radius);
    if (FAILED(m_d2dFactory->CreateRoundedRectangleGeometry(rr, &geometry)) || !geometry) {
        PushClip(rect);
        return;
    }

    D2D1_LAYER_PARAMETERS layerParams = D2D1::LayerParameters(
        D2D1::InfiniteRect(),
        geometry.Get(),
        D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
        D2D1::IdentityMatrix(),
        1.0f,
        nullptr,
        D2D1_LAYER_OPTIONS_NONE
    );
    m_d2dContext->PushLayer(layerParams, nullptr);
    m_clipStack.push_back(d2dRect);
    m_clipIsLayer.push_back(true);
}

void GraphicsContext::PushEllipseClip(Point center, float radiusX, float radiusY) {
    if (!m_d2dContext || !m_d2dFactory) {
        return;
    }

    const D2D1_ELLIPSE ellipse = D2D1::Ellipse(
        D2D1::Point2F(center.x * m_dpiScale, center.y * m_dpiScale),
        (std::max)(0.1f, radiusX * m_dpiScale),
        (std::max)(0.1f, radiusY * m_dpiScale)
    );

    ComPtr<ID2D1EllipseGeometry> geometry;
    if (FAILED(m_d2dFactory->CreateEllipseGeometry(ellipse, &geometry)) || !geometry) {
        return;
    }

    D2D1_LAYER_PARAMETERS layerParams = D2D1::LayerParameters(
        D2D1::InfiniteRect(),
        geometry.Get(),
        D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
        D2D1::IdentityMatrix(),
        1.0f,
        nullptr,
        D2D1_LAYER_OPTIONS_NONE
    );
    m_d2dContext->PushLayer(layerParams, nullptr);
    m_clipStack.push_back(D2D1::RectF(center.x - radiusX, center.y - radiusY, center.x + radiusX, center.y + radiusY));
    m_clipIsLayer.push_back(true);
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

void GraphicsContext::PushTransform(const D2D1_MATRIX_3X2_F& transform) {
    if (!m_d2dContext) {
        return;
    }
    D2D1_MATRIX_3X2_F current = D2D1::IdentityMatrix();
    m_d2dContext->GetTransform(&current);
    m_transformStack.push_back(current);
    m_d2dContext->SetTransform(transform * current);
}

void GraphicsContext::PopTransform() {
    if (!m_d2dContext || m_transformStack.empty()) {
        return;
    }
    m_d2dContext->SetTransform(m_transformStack.back());
    m_transformStack.pop_back();
}

void GraphicsContext::PushPopupReveal(const Rect& bounds, float progress, Point origin) {
    (void)bounds;
    progress = std::clamp(progress, 0.0f, 1.0f);
    const float inv = 1.0f - progress;
    const float ease = 1.0f - inv * inv * inv;
    const float scale = 0.92f + 0.08f * ease;
    PushOpacity(ease);
    PushTransform(D2D1::Matrix3x2F::Scale(scale, scale, D2D1::Point2F(origin.x, origin.y)));
}

void GraphicsContext::PopPopupReveal() {
    PopTransform();
    PopOpacity();
}

bool GraphicsContext::EnsureLayerCache(RenderLayer& layer, Size sizeInDips) {
    if (!m_d2dDevice) {
        return false;
    }

    // Keep the layer size in the same DIP contract as the caller. Rounding to
    // whole DIPs here makes the cached bitmap subtly larger than the target at
    // non-100% DPI, and drawing it back with nearest-neighbor scaling can
    // manifest as 1px dark/light seams across unrelated controls.
    const float widthDips = (std::max)(1.0f, sizeInDips.width);
    const float heightDips = (std::max)(1.0f, sizeInDips.height);
    const float scale = (m_dpiScale > 0.001f) ? m_dpiScale : 1.0f;
    const UINT32 pixelW = static_cast<UINT32>((std::max)(1.0f, std::ceil(widthDips * scale)));
    const UINT32 pixelH = static_cast<UINT32>((std::max)(1.0f, std::ceil(heightDips * scale)));

    const bool recreate = !layer.m_cacheContext
        || !layer.m_cacheBitmap
        || std::abs(layer.m_cacheSurfaceSize.width - widthDips) > 0.5f
        || std::abs(layer.m_cacheSurfaceSize.height - heightDips) > 0.5f;

    if (!recreate) {
        return true;
    }

    if (layer.m_cacheContext) {
        m_resources.ReleaseContextBrushes(layer.m_cacheContext.Get());
    }
    layer.ResetCache();

    HRESULT hr = m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &layer.m_cacheContext);
    if (FAILED(hr) || !layer.m_cacheContext) {
        layer.ResetCache();
        return false;
    }

    const float dpi = 96.0f * scale;
    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        dpi,
        dpi
    );

    hr = layer.m_cacheContext->CreateBitmap(
        D2D1::SizeU(pixelW, pixelH),
        nullptr,
        0,
        &props,
        &layer.m_cacheBitmap
    );
    if (FAILED(hr) || !layer.m_cacheBitmap) {
        layer.ResetCache();
        return false;
    }

    layer.m_cacheContext->SetDpi(dpi, dpi);
    layer.m_cacheContext->SetTarget(layer.m_cacheBitmap.Get());
    layer.SetCacheSurfaceSize(Size(widthDips, heightDips));
    layer.Invalidate(RenderLayer::SizeDirty | RenderLayer::ContentDirty);
    return true;
}

bool GraphicsContext::EnsureLayerScratch(RenderLayer& layer) {
    if (layer.m_scratchBitmap) {
        return true;
    }
    if (!layer.m_cacheContext || !layer.m_cacheBitmap) {
        return false;
    }

    const float scale = (m_dpiScale > 0.001f) ? m_dpiScale : 1.0f;
    const float dpi = 96.0f * scale;
    const D2D1_SIZE_U pixels = layer.m_cacheBitmap->GetPixelSize();
    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        dpi,
        dpi
    );

    HRESULT hr = layer.m_cacheContext->CreateBitmap(
        pixels,
        nullptr,
        0,
        &props,
        &layer.m_scratchBitmap
    );
    return SUCCEEDED(hr) && layer.m_scratchBitmap;
}

ID2D1DeviceContext* GraphicsContext::BeginLayerDraw(RenderLayer& layer) {
    if (!layer.m_cacheContext || !layer.m_cacheBitmap) {
        return nullptr;
    }

    const float dpi = 96.0f * ((m_dpiScale > 0.001f) ? m_dpiScale : 1.0f);
    layer.m_cacheContext->BeginDraw();
    layer.m_cacheContext->SetTarget(layer.m_cacheBitmap.Get());
    layer.m_cacheContext->SetDpi(dpi, dpi);
    layer.m_cacheContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    // Layer bitmaps are always intermediate targets (often cleared transparent).
    // ClearType requires an opaque background — always use grayscale here.
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

    const Rect snappedDest = SnapRectToDevicePixels(destRect, m_dpiScale);
    D2D1_RECT_F dest = snappedDest.ToD2D();
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

ID2D1Bitmap* GraphicsContext::GetOrCreateIconBitmap(HICON icon) {
    if (!icon || !m_d2dContext) return nullptr;

    auto it = m_iconBitmapCache.find(icon);
    if (it != m_iconBitmapCache.end() && it->second) {
        return it->second.Get();
    }

    ICONINFO ii = {};
    if (!GetIconInfo(icon, &ii)) return nullptr;

    BITMAP bmp = {};
    if (ii.hbmColor) {
        GetObject(ii.hbmColor, sizeof(bmp), &bmp);
    } else if (ii.hbmMask) {
        GetObject(ii.hbmMask, sizeof(bmp), &bmp);
        bmp.bmHeight /= 2;
    }

    const int w = (std::max)(1, static_cast<int>(bmp.bmWidth));
    const int h = (std::max)(1, static_cast<int>(bmp.bmHeight > 0 ? bmp.bmHeight : bmp.bmWidth));

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<UINT32> pixels(static_cast<size_t>(w) * static_cast<size_t>(h), 0);
    HDC screenDc = GetDC(nullptr);
    HDC memDc = CreateCompatibleDC(screenDc);
    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(memDc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    bool ok = false;
    if (dib && memDc && bits) {
        HGDIOBJ old = SelectObject(memDc, dib);
        DrawIconEx(memDc, 0, 0, icon, w, h, 0, nullptr, DI_NORMAL);
        memcpy(pixels.data(), bits, pixels.size() * sizeof(UINT32));
        SelectObject(memDc, old);
        ok = true;
    }
    if (dib) DeleteObject(dib);
    if (memDc) DeleteDC(memDc);
    if (screenDc) ReleaseDC(nullptr, screenDc);
    if (ii.hbmColor) DeleteObject(ii.hbmColor);
    if (ii.hbmMask) DeleteObject(ii.hbmMask);
    if (!ok) return nullptr;

    for (UINT32& px : pixels) {
        const UINT32 a = (px >> 24) & 0xFFu;
        if (a == 0 || a == 255) continue;
        const UINT32 r = (px >> 16) & 0xFFu;
        const UINT32 g = (px >> 8) & 0xFFu;
        const UINT32 b = px & 0xFFu;
        px = (a << 24)
            | (((r * a) / 255u) << 16)
            | (((g * a) / 255u) << 8)
            | ((b * a) / 255u);
    }

    D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );
    ComPtr<ID2D1Bitmap> bitmap;
    if (FAILED(m_d2dContext->CreateBitmap(
            D2D1::SizeU(static_cast<UINT32>(w), static_cast<UINT32>(h)),
            pixels.data(),
            static_cast<UINT32>(w * 4),
            &props,
            &bitmap)) || !bitmap) {
        return nullptr;
    }

    ID2D1Bitmap* raw = bitmap.Get();
    m_iconBitmapCache[icon] = std::move(bitmap);
    return raw;
}

void GraphicsContext::DrawHIcon(HICON icon, const Rect& dest, float opacity) {
    if (!icon || !m_d2dContext || dest.width <= 0.0f || dest.height <= 0.0f) return;
    ID2D1Bitmap* bitmap = GetOrCreateIconBitmap(icon);
    if (!bitmap) return;

    const Rect snapped = SnapRectToDevicePixels(dest, m_dpiScale);
    const D2D1_RECT_F d2d = snapped.ToD2D();
    m_d2dContext->DrawBitmap(
        bitmap,
        &d2d,
        std::clamp(opacity, 0.0f, 1.0f),
        D2D1_INTERPOLATION_MODE_LINEAR,
        nullptr
    );
}

namespace {

bool PaintIsNoneOrUrl(ID2D1SvgElement* element, PCWSTR attr) {
    if (!element || !attr) {
        return false;
    }
    UINT32 len = 0;
    if (FAILED(element->GetAttributeValueLength(attr, D2D1_SVG_ATTRIBUTE_STRING_TYPE_SVG, &len)) || len == 0) {
        return false;
    }
    std::wstring value(static_cast<size_t>(len) + 1, L'\0');
    if (FAILED(element->GetAttributeValue(
            attr, D2D1_SVG_ATTRIBUTE_STRING_TYPE_SVG, value.data(), len + 1))) {
        return false;
    }
    const wchar_t* s = value.c_str();
    while (*s == L' ' || *s == L'\t') {
        ++s;
    }
    if (_wcsicmp(s, L"none") == 0) {
        return true;
    }
    return wcsncmp(s, L"url(", 4) == 0;
}

bool IsSvgShapeTag(ID2D1SvgElement* element) {
    if (!element || element->IsTextContent()) {
        return false;
    }
    const UINT32 len = element->GetTagNameLength();
    if (len == 0) {
        return false;
    }
    std::wstring tag(static_cast<size_t>(len) + 1, L'\0');
    if (FAILED(element->GetTagName(tag.data(), len + 1))) {
        return false;
    }
    static const wchar_t* kShapes[] = {
        L"path", L"rect", L"circle", L"ellipse", L"polygon", L"polyline", L"line", L"text", L"tspan"
    };
    for (const wchar_t* name : kShapes) {
        if (_wcsicmp(tag.c_str(), name) == 0) {
            return true;
        }
    }
    return false;
}

void TintSvgElement(ID2D1SvgElement* element, const D2D1_COLOR_F& color) {
    if (!element || element->IsTextContent()) {
        return;
    }
    auto tintAttr = [&](PCWSTR name, bool setIfMissing) {
        if (!element->IsAttributeSpecified(name, nullptr)) {
            if (setIfMissing) {
                element->SetAttributeValue(name, color);
            }
            return;
        }
        if (PaintIsNoneOrUrl(element, name)) {
            return;
        }
        element->SetAttributeValue(name, color);
    };
    tintAttr(L"fill", IsSvgShapeTag(element));
    tintAttr(L"stroke", false);
    element->SetAttributeValue(L"color", color);

    ComPtr<ID2D1SvgElement> child;
    element->GetFirstChild(&child);
    while (child) {
        TintSvgElement(child.Get(), color);
        ComPtr<ID2D1SvgElement> next;
        element->GetNextChild(child.Get(), &next);
        child = next;
    }
}

bool ReadFileUtf8(const std::string& path, std::string& out) {
    const std::wstring wpath = Utf8ToUtf16(path);
    FILE* file = nullptr;
    if (_wfopen_s(&file, wpath.c_str(), L"rb") != 0 || !file) {
        return false;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return false;
    }
    const long size = ftell(file);
    if (size < 0) {
        fclose(file);
        return false;
    }
    rewind(file);
    out.resize(static_cast<size_t>(size));
    const size_t read = fread(out.data(), 1, out.size(), file);
    fclose(file);
    out.resize(read);
    if (out.size() >= 3
        && static_cast<unsigned char>(out[0]) == 0xEF
        && static_cast<unsigned char>(out[1]) == 0xBB
        && static_cast<unsigned char>(out[2]) == 0xBF) {
        out.erase(0, 3);
    }
    return !out.empty();
}

const char* FindAttrValue(const std::string& svg, const char* name, char& quote, size_t& len) {
    quote = 0;
    len = 0;
    const size_t nameLen = std::strlen(name);
    size_t pos = 0;
    while (pos + nameLen + 2 < svg.size()) {
        const size_t found = svg.find(name, pos);
        if (found == std::string::npos) {
            return nullptr;
        }
        size_t i = found + nameLen;
        while (i < svg.size() && (svg[i] == ' ' || svg[i] == '\t')) {
            ++i;
        }
        if (i >= svg.size() || svg[i] != '=') {
            pos = found + 1;
            continue;
        }
        ++i;
        while (i < svg.size() && (svg[i] == ' ' || svg[i] == '\t')) {
            ++i;
        }
        if (i >= svg.size() || (svg[i] != '"' && svg[i] != '\'')) {
            pos = found + 1;
            continue;
        }
        quote = svg[i];
        const char* start = svg.c_str() + i + 1;
        const char* end = std::strchr(start, quote);
        if (!end) {
            return nullptr;
        }
        len = static_cast<size_t>(end - start);
        return start;
    }
    return nullptr;
}

float ParseSvgNumber(const char* s, const char* end, const char** next) {
    while (s < end && (*s == ' ' || *s == ',' || *s == '\t' || *s == '\n' || *s == '\r')) {
        ++s;
    }
    char* parsed = nullptr;
    const float v = std::strtof(s, &parsed);
    if (next) {
        *next = (parsed && parsed > s) ? parsed : s;
    }
    return v;
}

D2D1_SIZE_F ParseSvgViewport(const std::string& svg) {
    D2D1_SIZE_F size = D2D1::SizeF(24.0f, 24.0f);
    char quote = 0;
    size_t len = 0;
    if (const char* viewBox = FindAttrValue(svg, "viewBox", quote, len)) {
        const char* end = viewBox + len;
        const char* p = viewBox;
        ParseSvgNumber(p, end, &p);
        ParseSvgNumber(p, end, &p);
        const float w = ParseSvgNumber(p, end, &p);
        const float h = ParseSvgNumber(p, end, &p);
        if (w > 0.5f && h > 0.5f) {
            return D2D1::SizeF(w, h);
        }
    }
    if (const char* width = FindAttrValue(svg, "width", quote, len)) {
        const float w = ParseSvgNumber(width, width + len, nullptr);
        if (w > 0.5f) {
            size.width = w;
        }
    }
    if (const char* height = FindAttrValue(svg, "height", quote, len)) {
        const float h = ParseSvgNumber(height, height + len, nullptr);
        if (h > 0.5f) {
            size.height = h;
        }
    }
    return size;
}

std::string TintCacheKey(const D2D1_COLOR_F* tint) {
    if (!tint) {
        return "none";
    }
    char buf[16];
    const int r = std::clamp(static_cast<int>(tint->r * 255.0f + 0.5f), 0, 255);
    const int g = std::clamp(static_cast<int>(tint->g * 255.0f + 0.5f), 0, 255);
    const int b = std::clamp(static_cast<int>(tint->b * 255.0f + 0.5f), 0, 255);
    const int a = std::clamp(static_cast<int>(tint->a * 255.0f + 0.5f), 0, 255);
    std::snprintf(buf, sizeof(buf), "%02x%02x%02x%02x", r, g, b, a);
    return buf;
}

} // namespace

bool GraphicsContext::LooksLikeSvg(const std::string& source) {
    if (source.size() < 4) {
        return false;
    }
    size_t i = 0;
    if (source.size() >= 3
        && static_cast<unsigned char>(source[0]) == 0xEF
        && static_cast<unsigned char>(source[1]) == 0xBB
        && static_cast<unsigned char>(source[2]) == 0xBF) {
        i = 3;
    }
    while (i < source.size() && static_cast<unsigned char>(source[i]) <= 32) {
        ++i;
    }
    if (source.compare(i, 4, "<svg") == 0 || source.compare(i, 5, "<?xml") == 0) {
        return true;
    }
    if (source.size() >= 4) {
        const char* ext = source.c_str() + source.size() - 4;
        if (ext[0] == '.'
            && (ext[1] == 's' || ext[1] == 'S')
            && (ext[2] == 'v' || ext[2] == 'V')
            && (ext[3] == 'g' || ext[3] == 'G')) {
            return true;
        }
    }
    return false;
}

ComPtr<ID2D1DeviceContext5> GraphicsContext::GetSvgContext() {
    ComPtr<ID2D1DeviceContext5> ctx5;
    if (m_d2dContext) {
        m_d2dContext.As(&ctx5);
    }
    return ctx5;
}

const GraphicsContext::SvgCacheEntry* GraphicsContext::GetOrCreateSvg(
    const std::string& source,
    const D2D1_COLOR_F* tint) {
    ComPtr<ID2D1DeviceContext5> ctx5 = GetSvgContext();
    if (!ctx5 || source.empty()) {
        return nullptr;
    }

    const std::string key = source + "|" + TintCacheKey(tint);
    if (const auto it = m_svgCache.find(key); it != m_svgCache.end() && it->second.doc) {
        return &it->second;
    }

    std::string markup = source;
    const bool inlineSvg = (source.find("<svg") != std::string::npos) || (source.find("<SVG") != std::string::npos);
    if (!inlineSvg) {
        if (!ReadFileUtf8(source, markup)) {
            return nullptr;
        }
    }

    ComPtr<IStream> stream;
    stream.Attach(SHCreateMemStream(
        reinterpret_cast<const BYTE*>(markup.data()),
        static_cast<UINT>(markup.size())));
    if (!stream) {
        return nullptr;
    }

    const D2D1_SIZE_F viewport = ParseSvgViewport(markup);
    ComPtr<ID2D1SvgDocument> doc;
    if (FAILED(ctx5->CreateSvgDocument(stream.Get(), viewport, &doc)) || !doc) {
        return nullptr;
    }
    if (tint) {
        ComPtr<ID2D1SvgElement> root;
        doc->GetRoot(&root);
        TintSvgElement(root.Get(), *tint);
    }

    SvgCacheEntry entry;
    entry.doc = std::move(doc);
    entry.viewport = viewport;
    auto [it, _] = m_svgCache.emplace(key, std::move(entry));
    return &it->second;
}

void GraphicsContext::DrawSvg(
    const std::string& source,
    const Rect& dest,
    const D2D1_COLOR_F* tint,
    float opacity) {
    if (source.empty() || dest.width <= 0.5f || dest.height <= 0.5f) {
        return;
    }
    ComPtr<ID2D1DeviceContext5> ctx5 = GetSvgContext();
    const SvgCacheEntry* entry = GetOrCreateSvg(source, tint);
    if (!ctx5 || !entry || !entry->doc) {
        return;
    }

    const float vpW = (std::max)(1.0f, entry->viewport.width);
    const float vpH = (std::max)(1.0f, entry->viewport.height);
    const float scale = (std::min)(dest.width / vpW, dest.height / vpH);
    const float drawW = vpW * scale;
    const float drawH = vpH * scale;
    const float x = dest.x + (dest.width - drawW) * 0.5f;
    const float y = dest.y + (dest.height - drawH) * 0.5f;
    const D2D1_MATRIX_3X2_F xform =
        D2D1::Matrix3x2F::Scale(scale, scale) * D2D1::Matrix3x2F::Translation(x, y);

    opacity = std::clamp(opacity, 0.0f, 1.0f);
    const bool fade = opacity < 0.999f;
    if (fade) {
        PushOpacity(opacity);
    }
    PushTransform(xform);
    ctx5->DrawSvgDocument(entry->doc.Get());
    PopTransform();
    if (fade) {
        PopOpacity();
    }
}

void GraphicsContext::DrawIcon(
    const std::string& icon,
    const Rect& dest,
    D2D1_COLOR_F color,
    float opacity,
    const std::string& glyphFont,
    float glyphSize) {
    if (icon.empty() || dest.width <= 0.5f || dest.height <= 0.5f) {
        return;
    }
    if (LooksLikeSvg(icon)) {
        DrawSvg(icon, dest, &color, opacity);
        return;
    }
    const float size = (glyphSize > 0.5f) ? glyphSize : (std::max)(8.0f, dest.height * 0.78f);
    opacity = std::clamp(opacity, 0.0f, 1.0f);
    if (opacity < 0.999f) {
        color.a *= opacity;
    }
    DrawText(
        icon,
        dest,
        color,
        glyphFont,
        size,
        DWRITE_TEXT_ALIGNMENT_CENTER,
        DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

bool GraphicsContext::PushLayerTarget(RenderLayer& layer, Size sizeInDips, const Rect& paintBounds, D2D1_COLOR_F clearColor, bool clearTarget) {
    if (!EnsureLayerCache(layer, sizeInDips)) {
        return false;
    }

    TargetState state;
    state.context = m_d2dContext;
    state.paintBounds = m_paintBounds;
    state.clipStack = m_clipStack;
    state.clipIsLayer = m_clipIsLayer;
    state.opacityStack = m_opacityStack;
    m_targetStack.push_back(std::move(state));

    m_d2dContext = layer.m_cacheContext;
    m_paintBounds = paintBounds;
    m_clipStack.clear();
    m_clipIsLayer.clear();
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

Rect GraphicsContext::SnapExpandRect(const Rect& rect, float dpiScale, float inflateDips) {
    if (rect.IsEmpty()) {
        return rect;
    }
    const Rect inflated = inflateDips > 0.0f ? rect.Inflate(inflateDips) : rect;
    const float scale = (dpiScale > 0.001f) ? dpiScale : 1.0f;
    const float x0 = std::floor(inflated.x * scale) / scale;
    const float y0 = std::floor(inflated.y * scale) / scale;
    const float x1 = std::ceil((inflated.x + inflated.width) * scale) / scale;
    const float y1 = std::ceil((inflated.y + inflated.height) * scale) / scale;
    return Rect(x0, y0, (std::max)(0.0f, x1 - x0), (std::max)(0.0f, y1 - y0));
}

void GraphicsContext::ClearRect(const Rect& rect) {
    ClearRect(rect, D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
}

void GraphicsContext::ClearRect(const Rect& rect, D2D1_COLOR_F color) {
    if (!m_d2dContext || rect.IsEmpty()) {
        return;
    }
    // Caller should pass SnapExpandRect result so clear footprint == clip footprint.
    const Rect snapped = SnapExpandRect(rect, m_dpiScale, 0.0f);
    const D2D1_RECT_F clearRc = snapped.ToD2D();
    const D2D1_PRIMITIVE_BLEND oldBlend = m_d2dContext->GetPrimitiveBlend();
    m_d2dContext->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_COPY);
    if (auto brush = m_resources.GetSolidBrush(color)) {
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
    m_clipIsLayer = std::move(state.clipIsLayer);
    m_opacityStack = std::move(state.opacityStack);
    m_resources.Initialize(m_d2dContext.Get(), m_dwriteFactory.Get());
}

void GraphicsContext::PopClip() {
    if (!m_d2dContext || m_clipStack.empty()) {
        return;
    }
    const bool isLayer = !m_clipIsLayer.empty() && m_clipIsLayer.back();
    if (!m_clipIsLayer.empty()) {
        m_clipIsLayer.pop_back();
    }
    m_clipStack.pop_back();
    if (isLayer) {
        m_d2dContext->PopLayer();
    } else {
        m_d2dContext->PopAxisAlignedClip();
    }
}

void GraphicsContext::DrawRect(const Rect& rect, D2D1_COLOR_F color, float strokeWidth) {
    if (!m_d2dContext) {
        return;
    }
    if (auto brush = m_resources.GetSolidBrush(color)) {
        const float scale = (m_dpiScale > 0.001f) ? m_dpiScale : 1.0f;
        const float snappedStroke = (std::max)(1.0f / scale, std::round(strokeWidth * scale) / scale);
        m_d2dContext->DrawRectangle(SnapRectForStroke(rect, snappedStroke, m_dpiScale), brush, snappedStroke);
    }
}

void GraphicsContext::FillRect(const Rect& rect, D2D1_COLOR_F color) {
    if (auto brush = m_resources.GetSolidBrush(color)) {
        m_d2dContext->FillRectangle(SnapRectForFill(rect, m_dpiScale), brush);
    }
}

void GraphicsContext::FillRoundedRect(const Rect& rect, float radius, D2D1_COLOR_F color) {
    if (auto brush = m_resources.GetSolidBrush(color)) {
        D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(SnapRectForFill(rect, m_dpiScale), radius, radius);
        m_d2dContext->FillRoundedRectangle(rr, brush);
    }
}

void GraphicsContext::DrawRoundedRect(const Rect& rect, float radius, D2D1_COLOR_F color, float strokeWidth) {
    if (!m_d2dContext) {
        return;
    }
    if (auto brush = m_resources.GetSolidBrush(color)) {
        const float scale = (m_dpiScale > 0.001f) ? m_dpiScale : 1.0f;
        const float snappedStroke = (std::max)(1.0f / scale, std::round(strokeWidth * scale) / scale);
        D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(
            SnapRectForStroke(rect, snappedStroke, m_dpiScale),
            radius,
            radius);
        m_d2dContext->DrawRoundedRectangle(rr, brush, snappedStroke);
    }
}

void GraphicsContext::DrawLine(Point p1, Point p2, D2D1_COLOR_F color, float strokeWidth) {
    if (!m_d2dContext) {
        return;
    }
    if (auto brush = m_resources.GetSolidBrush(color)) {
        const float scale = (m_dpiScale > 0.001f) ? m_dpiScale : 1.0f;
        const float dx = std::abs(p2.x - p1.x);
        const float dy = std::abs(p2.y - p1.y);
        // Hairline = exactly one device pixel. The old center±halfStroke + floor/ceil
        // path often expanded a 1.0 DIP stroke into two device pixels.
        const float hairlineDip = 1.0f / scale;
        const float requested = (std::max)(hairlineDip, strokeWidth);
        const int devicePx = (std::max)(1, static_cast<int>(std::lround(requested * scale)));

        if (dy <= 0.01f) {
            const float x0 = std::min(p1.x, p2.x);
            const float x1 = std::max(p1.x, p2.x);
            const float py = std::floor(p1.y * scale + 0.5f);
            const float top = py / scale;
            const float bottom = (py + static_cast<float>(devicePx)) / scale;
            m_d2dContext->FillRectangle(
                D2D1::RectF(
                    std::floor(x0 * scale) / scale,
                    top,
                    std::ceil(x1 * scale) / scale,
                    bottom),
                brush);
            return;
        }
        if (dx <= 0.01f) {
            const float y0 = std::min(p1.y, p2.y);
            const float y1 = std::max(p1.y, p2.y);
            const float px = std::floor(p1.x * scale + 0.5f);
            const float left = px / scale;
            const float right = (px + static_cast<float>(devicePx)) / scale;
            m_d2dContext->FillRectangle(
                D2D1::RectF(
                    left,
                    std::floor(y0 * scale) / scale,
                    right,
                    std::ceil(y1 * scale) / scale),
                brush);
            return;
        }

        float offset = (devicePx % 2 == 1) ? 0.5f / scale : 0.0f;
        float x1 = std::floor(p1.x * scale) / scale + offset;
        float y1 = std::floor(p1.y * scale) / scale + offset;
        float x2 = std::floor(p2.x * scale) / scale + offset;
        float y2 = std::floor(p2.y * scale) / scale + offset;
        m_d2dContext->DrawLine(
            D2D1::Point2F(x1, y1),
            D2D1::Point2F(x2, y2),
            brush,
            static_cast<float>(devicePx) / scale);
    }
}

void GraphicsContext::DrawSmoothLine(Point p1, Point p2, D2D1_COLOR_F color, float strokeWidth) {
    if (!m_d2dContext) {
        return;
    }
    auto brush = m_resources.GetSolidBrush(color);
    if (!brush) {
        return;
    }
    ComPtr<ID2D1Factory> factory;
    m_d2dContext->GetFactory(&factory);
    if (!factory) {
        return;
    }
    ComPtr<ID2D1StrokeStyle> stroke;
    const HRESULT hr = factory->CreateStrokeStyle(
        D2D1::StrokeStyleProperties(
            D2D1_CAP_STYLE_ROUND,
            D2D1_CAP_STYLE_ROUND,
            D2D1_CAP_STYLE_ROUND,
            D2D1_LINE_JOIN_ROUND,
            10.0f,
            D2D1_DASH_STYLE_SOLID,
            0.0f),
        nullptr,
        0,
        &stroke);
    if (FAILED(hr) || !stroke) {
        m_d2dContext->DrawLine(D2D1::Point2F(p1.x, p1.y), D2D1::Point2F(p2.x, p2.y), brush, strokeWidth);
        return;
    }
    m_d2dContext->DrawLine(
        D2D1::Point2F(p1.x, p1.y),
        D2D1::Point2F(p2.x, p2.y),
        brush,
        (std::max)(0.75f, strokeWidth),
        stroke.Get());
}

void GraphicsContext::DrawSmoothArc(
    Point center,
    float radius,
    float startRad,
    float sweepRad,
    D2D1_COLOR_F color,
    float strokeWidth) {
    if (!m_d2dContext || radius <= 0.5f || std::abs(sweepRad) < 0.0005f || color.a <= 0.001f) {
        return;
    }

    auto brush = m_resources.GetSolidBrush(color);
    if (!brush) {
        return;
    }

    ComPtr<ID2D1Factory> factory;
    m_d2dContext->GetFactory(&factory);
    if (!factory) {
        return;
    }

    // D2D arc uses counter-clockwise for positive sweep in mathematical angles;
    // our ProgressiveRing convention is clockwise in DIP space (Y down), so flip.
    const float absSweep = std::abs(sweepRad);
    const bool clockwise = sweepRad >= 0.0f;
    const float endRad = startRad + sweepRad;

    const D2D1_POINT_2F startPt = D2D1::Point2F(
        center.x + std::cos(startRad) * radius,
        center.y + std::sin(startRad) * radius);
    const D2D1_POINT_2F endPt = D2D1::Point2F(
        center.x + std::cos(endRad) * radius,
        center.y + std::sin(endRad) * radius);

    ComPtr<ID2D1PathGeometry> path;
    if (FAILED(factory->CreatePathGeometry(&path)) || !path) {
        return;
    }
    ComPtr<ID2D1GeometrySink> sink;
    if (FAILED(path->Open(&sink)) || !sink) {
        return;
    }

    sink->BeginFigure(startPt, D2D1_FIGURE_BEGIN_HOLLOW);
    // Full circle needs two semicircle arcs (D2D rejects a full 360° single arc).
    if (absSweep >= 6.28318530718f - 0.001f) {
        const float midRad = startRad + (clockwise ? 3.14159265359f : -3.14159265359f);
        const D2D1_POINT_2F midPt = D2D1::Point2F(
            center.x + std::cos(midRad) * radius,
            center.y + std::sin(midRad) * radius);
        sink->AddArc(D2D1::ArcSegment(
            midPt,
            D2D1::SizeF(radius, radius),
            0.0f,
            clockwise ? D2D1_SWEEP_DIRECTION_CLOCKWISE : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE,
            D2D1_ARC_SIZE_SMALL));
        sink->AddArc(D2D1::ArcSegment(
            endPt,
            D2D1::SizeF(radius, radius),
            0.0f,
            clockwise ? D2D1_SWEEP_DIRECTION_CLOCKWISE : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE,
            D2D1_ARC_SIZE_SMALL));
    } else {
        sink->AddArc(D2D1::ArcSegment(
            endPt,
            D2D1::SizeF(radius, radius),
            0.0f,
            clockwise ? D2D1_SWEEP_DIRECTION_CLOCKWISE : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE,
            absSweep > 3.14159265359f ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL));
    }
    sink->EndFigure(D2D1_FIGURE_END_OPEN);
    if (FAILED(sink->Close())) {
        return;
    }

    ComPtr<ID2D1StrokeStyle> stroke;
    factory->CreateStrokeStyle(
        D2D1::StrokeStyleProperties(
            D2D1_CAP_STYLE_ROUND,
            D2D1_CAP_STYLE_ROUND,
            D2D1_CAP_STYLE_ROUND,
            D2D1_LINE_JOIN_ROUND,
            10.0f,
            D2D1_DASH_STYLE_SOLID,
            0.0f),
        nullptr,
        0,
        &stroke);

    m_d2dContext->DrawGeometry(
        path.Get(),
        brush,
        (std::max)(0.75f, strokeWidth),
        stroke.Get());
}

void GraphicsContext::DrawChevron(const Rect& bounds, D2D1_COLOR_F color, ChevronDirection direction, float strokeWidth) {
    (void)strokeWidth;
    if (bounds.IsEmpty() || bounds.width < 2.0f || bounds.height < 2.0f) {
        return;
    }

    // Fit a square glyph into whatever slot the control passed (ComboBox used a
    // tall 20×height strip, which made the SVG look like the old two-stroke V).
    const float side = (std::min)(bounds.width, bounds.height);
    const float cx = bounds.x + bounds.width * 0.5f;
    const float cy = bounds.y + bounds.height * 0.5f;
    const float originX = cx - side * 0.5f;
    const float originY = cy - side * 0.5f;

    // Right-pointing chevron from the provided 1024 SVG, normalized, no arcs
    // (D2D SVG often drops elliptical `a` commands → empty glyph).
    const Point local[6] = {
        Point(0.382f, 0.172f),
        Point(0.739f, 0.500f),
        Point(0.382f, 0.828f),
        Point(0.306f, 0.752f),
        Point(0.586f, 0.500f),
        Point(0.306f, 0.248f),
    };

    // y-down clockwise: Right=0, Down=90, Left=180, Up=270.
    float c = 1.0f;
    float s = 0.0f;
    switch (direction) {
    case ChevronDirection::Down:
        c = 0.0f;
        s = 1.0f;
        break;
    case ChevronDirection::Left:
        c = -1.0f;
        s = 0.0f;
        break;
    case ChevronDirection::Up:
        c = 0.0f;
        s = -1.0f;
        break;
    case ChevronDirection::Right:
    default:
        break;
    }

    Point pts[6];
    for (int i = 0; i < 6; ++i) {
        const float u = local[i].x - 0.5f;
        const float v = local[i].y - 0.5f;
        const float ru = u * c - v * s;
        const float rv = u * s + v * c;
        pts[i] = Point(originX + (ru + 0.5f) * side, originY + (rv + 0.5f) * side);
    }
    FillPolygon(pts, 6, color);
}

void GraphicsContext::FillPolygon(const Point* points, int count, D2D1_COLOR_F color) {
    if (!m_d2dContext || !points || count < 3) {
        return;
    }
    ComPtr<ID2D1Factory> factory;
    m_d2dContext->GetFactory(&factory);
    if (!factory) {
        return;
    }
    ComPtr<ID2D1PathGeometry> path;
    if (FAILED(factory->CreatePathGeometry(&path)) || !path) {
        return;
    }
    ComPtr<ID2D1GeometrySink> sink;
    if (FAILED(path->Open(&sink)) || !sink) {
        return;
    }
    sink->BeginFigure(D2D1::Point2F(points[0].x, points[0].y), D2D1_FIGURE_BEGIN_FILLED);
    for (int i = 1; i < count; ++i) {
        sink->AddLine(D2D1::Point2F(points[i].x, points[i].y));
    }
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    sink->Close();
    if (auto brush = m_resources.GetSolidBrush(color)) {
        m_d2dContext->FillGeometry(path.Get(), brush);
    }
}

void GraphicsContext::DrawPolygon(const Point* points, int count, D2D1_COLOR_F color, float strokeWidth) {
    if (!m_d2dContext || !points || count < 2) {
        return;
    }
    ComPtr<ID2D1Factory> factory;
    m_d2dContext->GetFactory(&factory);
    if (!factory) {
        return;
    }
    ComPtr<ID2D1PathGeometry> path;
    if (FAILED(factory->CreatePathGeometry(&path)) || !path) {
        return;
    }
    ComPtr<ID2D1GeometrySink> sink;
    if (FAILED(path->Open(&sink)) || !sink) {
        return;
    }
    sink->BeginFigure(D2D1::Point2F(points[0].x, points[0].y), D2D1_FIGURE_BEGIN_HOLLOW);
    for (int i = 1; i < count; ++i) {
        sink->AddLine(D2D1::Point2F(points[i].x, points[i].y));
    }
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    sink->Close();
    if (auto brush = m_resources.GetSolidBrush(color)) {
        const float scale = (m_dpiScale > 0.001f) ? m_dpiScale : 1.0f;
        const float snappedStroke = (std::max)(1.0f / scale, std::round(strokeWidth * scale) / scale);
        m_d2dContext->DrawGeometry(path.Get(), brush, snappedStroke);
    }
}

void GraphicsContext::DrawTextOnTarget(
    ID2D1RenderTarget* target,
    const std::wstring& text,
    const Rect& rect,
    D2D1_COLOR_F color,
    const std::string& fontName,
    float fontSize,
    DWRITE_TEXT_ALIGNMENT align,
    DWRITE_PARAGRAPH_ALIGNMENT vAlign,
    DWRITE_FONT_WEIGHT weight,
    D2D1_TEXT_ANTIALIAS_MODE antialiasMode,
    bool truncateWithEllipsis) {

    if (!target || text.empty()) {
        return;
    }

    auto format = m_resources.GetTextFormat(fontName, fontSize, weight);
    if (!format) {
        return;
    }

    ID2D1SolidColorBrush* brush = nullptr;
    if (m_d2dContext.Get() == target) {
        brush = m_resources.GetSolidBrush(color);
    }
    ComPtr<ID2D1SolidColorBrush> ownedBrush;
    if (!brush) {
        if (FAILED(target->CreateSolidColorBrush(color, &ownedBrush)) || !ownedBrush) {
            return;
        }
        brush = ownedBrush.Get();
    }

    format->SetTextAlignment(align);
    format->SetParagraphAlignment(vAlign);
    format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

    ComPtr<IDWriteInlineObject> ellipsis;
    if (truncateWithEllipsis && m_dwriteFactory) {
        DWRITE_TRIMMING trimming = { DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0 };
        if (SUCCEEDED(m_dwriteFactory->CreateEllipsisTrimmingSign(format, &ellipsis))) {
            format->SetTrimming(&trimming, ellipsis.Get());
        } else {
            format->SetTrimming(&trimming, nullptr);
        }
    } else {
        DWRITE_TRIMMING trimming = { DWRITE_TRIMMING_GRANULARITY_NONE, 0, 0 };
        format->SetTrimming(&trimming, nullptr);
    }

    const D2D1_TEXT_ANTIALIAS_MODE previousMode = target->GetTextAntialiasMode();
    target->SetTextAntialiasMode(antialiasMode);
    target->DrawText(
        text.c_str(),
        static_cast<UINT32>(text.length()),
        format,
        rect.ToD2D(),
        brush,
        D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT | D2D1_DRAW_TEXT_OPTIONS_CLIP
    );
    target->SetTextAntialiasMode(previousMode);

    // Reset shared format trimming so other callers are not affected.
    DWRITE_TRIMMING none = { DWRITE_TRIMMING_GRANULARITY_NONE, 0, 0 };
    format->SetTrimming(&none, nullptr);
}

void GraphicsContext::DrawTextLayoutOnTarget(
    ID2D1RenderTarget* target,
    IDWriteTextLayout* layout,
    const Rect& originRect,
    D2D1_COLOR_F color,
    D2D1_TEXT_ANTIALIAS_MODE antialiasMode) {

    if (!target || !layout) {
        return;
    }

    ID2D1SolidColorBrush* brush = nullptr;
    if (m_d2dContext.Get() == target) {
        brush = m_resources.GetSolidBrush(color);
    }
    ComPtr<ID2D1SolidColorBrush> ownedBrush;
    if (!brush) {
        if (FAILED(target->CreateSolidColorBrush(color, &ownedBrush)) || !ownedBrush) {
            return;
        }
        brush = ownedBrush.Get();
    }

    const D2D1_TEXT_ANTIALIAS_MODE previousMode = target->GetTextAntialiasMode();
    target->SetTextAntialiasMode(antialiasMode);
    target->DrawTextLayout(
        D2D1::Point2F(originRect.x, originRect.y),
        layout,
        brush,
        D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
    );
    target->SetTextAntialiasMode(previousMode);
}

void GraphicsContext::DrawTextRotated(
    const std::string& text,
    Point center,
    float degrees,
    D2D1_COLOR_F color,
    const std::string& fontName,
    float fontSize,
    DWRITE_FONT_WEIGHT weight) {
    if (text.empty() || !m_d2dContext) {
        return;
    }

    TextLayoutOptions opt;
    opt.maxWidth = 4096.0f;
    opt.maxHeight = 256.0f;
    opt.wrapping = DWRITE_WORD_WRAPPING_NO_WRAP;
    opt.paragraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
    ComPtr<IDWriteTextLayout> layout = CreateTextLayout(
        Utf8ToUtf16(text),
        fontName,
        SnapFontSizeToDevicePixels(fontSize, m_dpiScale),
        opt,
        weight);
    if (!layout) {
        return;
    }

    DWRITE_TEXT_METRICS metrics = {};
    layout->GetMetrics(&metrics);
    const float textW = (std::max)(1.0f,
        metrics.widthIncludingTrailingWhitespace > 0.1f
            ? metrics.widthIncludingTrailingWhitespace
            : metrics.width);
    const float textH = (std::max)(1.0f, metrics.height);

    // Blit a grayscale snapshot. ClearType + a world transform is ignored by D2D,
    // which is why side auto-hide labels were staying horizontal.
    const float scale = (m_dpiScale > 0.001f) ? m_dpiScale : 1.0f;
    const UINT pixelW = static_cast<UINT>((std::max)(1.0f, std::ceil(textW * scale)));
    const UINT pixelH = static_cast<UINT>((std::max)(1.0f, std::ceil(textH * scale)));
    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        96.0f * scale,
        96.0f * scale);
    ComPtr<ID2D1Bitmap1> bmp;
    if (FAILED(m_d2dContext->CreateBitmap(D2D1::SizeU(pixelW, pixelH), nullptr, 0, &props, &bmp)) || !bmp) {
        return;
    }

    D2D1_MATRIX_3X2_F savedTf = D2D1::IdentityMatrix();
    m_d2dContext->GetTransform(&savedTf);
    ComPtr<ID2D1Image> oldTarget;
    m_d2dContext->GetTarget(&oldTarget);
    if (!oldTarget) {
        return;
    }
    m_d2dContext->SetTarget(bmp.Get());
    m_d2dContext->SetTransform(D2D1::IdentityMatrix());
    m_d2dContext->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
    DrawTextLayoutOnTarget(
        m_d2dContext.Get(),
        layout.Get(),
        Rect(0.0f, 0.0f, textW, textH),
        color,
        D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    m_d2dContext->SetTarget(oldTarget.Get());
    m_d2dContext->SetTransform(savedTf);

    const float destX = center.x - textW * 0.5f;
    const float destY = center.y - textH * 0.5f;
    D2D1_RECT_F dest = D2D1::RectF(destX, destY, destX + textW, destY + textH);
    PushTransform(D2D1::Matrix3x2F::Rotation(degrees, D2D1::Point2F(center.x, center.y)));
    m_d2dContext->DrawBitmap(
        bmp.Get(),
        &dest,
        1.0f,
        D2D1_INTERPOLATION_MODE_LINEAR);
    PopTransform();
}

void GraphicsContext::DrawText(const std::string& text, const Rect& rect, D2D1_COLOR_F color,
                               const std::string& fontName, float fontSize,
                               DWRITE_TEXT_ALIGNMENT align, DWRITE_PARAGRAPH_ALIGNMENT vAlign,
                               DWRITE_FONT_WEIGHT weight, bool truncateWithEllipsis) {
    if (text.empty() || !m_d2dContext) {
        return;
    }

    // Mica/composition targets keep per-pixel alpha; ClearType is invalid there.
    // Offscreen layer caches are also cleared to transparent — ClearType on a
    // transparent bitmap produces vertical RGB fringe garbage (ListView rows).
    const bool layerTarget = !m_targetStack.empty();
    const D2D1_TEXT_ANTIALIAS_MODE mode =
        (m_supportsPerPixelAlpha || layerTarget)
            ? D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE
            : D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE;

    DrawTextOnTarget(
        m_d2dContext.Get(),
        Utf8ToUtf16(text),
        SnapTextRect(rect, m_dpiScale),
        color,
        fontName,
        SnapFontSizeToDevicePixels(fontSize, m_dpiScale),
        align,
        vAlign,
        weight,
        mode,
        truncateWithEllipsis
    );
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
        L"zh-CN",
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
    if (!layout || !m_d2dContext) {
        return;
    }

    const bool layerTarget = !m_targetStack.empty();
    const D2D1_TEXT_ANTIALIAS_MODE mode =
        (m_supportsPerPixelAlpha || layerTarget)
            ? D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE
            : D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE;

    DrawTextLayoutOnTarget(
        m_d2dContext.Get(),
        layout,
        Rect(
            SnapDipToDevicePixel(originRect.x, m_dpiScale),
            SnapDipToDevicePixel(originRect.y, m_dpiScale),
            originRect.width,
            originRect.height
        ),
        color,
        mode
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
