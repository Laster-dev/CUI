#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Image.h"
#include "../core/Value.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cstring>

#pragma comment(lib, "windowscodecs.lib")

namespace CUI {
namespace {

bool EndsWithIgnoreCase(const std::string& s, const char* ext) {
    const size_t n = std::strlen(ext);
    if (s.size() < n) {
        return false;
    }
    for (size_t i = 0; i < n; ++i) {
        char a = s[s.size() - n + i];
        char b = ext[i];
        if (a >= 'A' && a <= 'Z') {
            a = static_cast<char>(a + 32);
        }
        if (b >= 'A' && b <= 'Z') {
            b = static_cast<char>(b + 32);
        }
        if (a != b) {
            return false;
        }
    }
    return true;
}

const char* FileNameOf(const std::string& path) {
    size_t slash = path.find_last_of("\\/");
    return slash == std::string::npos ? path.c_str() : path.c_str() + slash + 1;
}

} // namespace

Image::Image() {
    m_badgeColor = ThemeManager::Instance().GetColor(ThemeTokenId::AccentColor);
    SetWidth(24.0f);
    SetHeight(24.0f);
    SetClipToBounds(true);
}

Image::Image(ImageType type, const std::string& text)
    : m_imageType(type), m_badgeText(text), m_badgeColor(ThemeManager::Instance().GetColor(ThemeTokenId::AccentColor)) {
    SetWidth(24.0f);
    SetHeight(24.0f);
    SetClipToBounds(true);
}

Image::Image(ImageType type, const std::string& text, D2D1_COLOR_F color)
    : m_imageType(type), m_badgeText(text), m_badgeColor(color) {
    SetWidth(24.0f);
    SetHeight(24.0f);
    SetClipToBounds(true);
}

std::vector<PropertyMeta> Image::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "text", "源路径 (Source)", "图像", "string" });
    return metas;
}

void Image::SetImageType(ImageType type) {
    if (m_imageType == type) {
        return;
    }
    if (type != ImageType::FileSource && type != ImageType::DynamicBitmap) {
        ReleaseDecoded();
        m_sourcePath.clear();
        m_loadError.clear();
    }
    m_imageType = type;
    MarkRenderRectDirty(m_bounds);
}

void Image::SetStretch(Stretch stretch) {
    if (m_stretch == stretch) {
        return;
    }
    m_stretch = stretch;
    MarkRenderRectDirty(m_bounds);
}

void Image::SetBitmap(ID2D1Bitmap1* bitmap) {
    ReleaseDecoded();
    m_d2dBitmap = bitmap;
    m_imageType = ImageType::DynamicBitmap;
    m_sourcePath.clear();
    m_loadError.clear();
    if (bitmap) {
        const D2D1_SIZE_U px = bitmap->GetPixelSize();
        m_bmpWidth = px.width;
        m_bmpHeight = px.height;
    }
    MarkRenderRectDirty(m_bounds);
}

void Image::ClearSource() {
    ReleaseDecoded();
    m_sourcePath.clear();
    m_loadError.clear();
    if (m_imageType == ImageType::FileSource) {
        m_imageType = ImageType::Custom;
    }
    InvalidateMeasure();
    MarkRenderRectDirty(m_bounds);
}

void Image::ReleaseDecoded() {
    m_wicBitmap.Reset();
    m_d2dBitmap.Reset();
    m_boundDevice.Reset();
    m_bmpWidth = 0;
    m_bmpHeight = 0;
    m_hasPendingUpdate.store(false, std::memory_order_release);
    m_pendingPixelBuffer.clear();
}

IWICImagingFactory2* Image::EnsureWicFactory() {
    if (m_wicFactory) {
        return m_wicFactory.Get();
    }
    const HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory2,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(m_wicFactory.ReleaseAndGetAddressOf()));
    return SUCCEEDED(hr) ? m_wicFactory.Get() : nullptr;
}

bool Image::DecodeFrame(IWICBitmapDecoder* decoder) {
    if (!decoder) {
        return false;
    }
    IWICImagingFactory2* factory = EnsureWicFactory();
    if (!factory) {
        m_loadError = "WIC 工厂创建失败";
        return false;
    }

    ComPtr<IWICBitmapFrameDecode> frame;
    HRESULT hr = decoder->GetFrame(0, frame.ReleaseAndGetAddressOf());
    if (FAILED(hr) || !frame) {
        m_loadError = "无法读取图像帧";
        return false;
    }

    ComPtr<IWICFormatConverter> converter;
    hr = factory->CreateFormatConverter(converter.ReleaseAndGetAddressOf());
    if (FAILED(hr)) {
        m_loadError = "WIC 格式转换失败";
        return false;
    }
    hr = converter->Initialize(
        frame.Get(),
        GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0,
        WICBitmapPaletteTypeMedianCut);
    if (FAILED(hr)) {
        m_loadError = "像素格式转换失败";
        return false;
    }

    ComPtr<IWICBitmap> bitmap;
    hr = factory->CreateBitmapFromSource(converter.Get(), WICBitmapCacheOnLoad, bitmap.ReleaseAndGetAddressOf());
    if (FAILED(hr) || !bitmap) {
        m_loadError = "无法缓存位图";
        return false;
    }

    UINT w = 0;
    UINT h = 0;
    bitmap->GetSize(&w, &h);
    if (w == 0 || h == 0) {
        m_loadError = "图像尺寸为 0";
        return false;
    }

    m_wicBitmap = bitmap;
    m_d2dBitmap.Reset();
    m_boundDevice.Reset();
    m_bmpWidth = w;
    m_bmpHeight = h;
    m_loadError.clear();
    return true;
}

bool Image::SetSource(const std::string& path) {
    m_sourcePath = path;
    m_loadError.clear();
    ReleaseDecoded();

    if (path.empty()) {
        m_imageType = ImageType::Custom;
        InvalidateMeasure();
        MarkRenderRectDirty(m_bounds);
        return false;
    }

    m_imageType = ImageType::FileSource;
    if (GraphicsContext::LooksLikeSvg(path) || EndsWithIgnoreCase(path, ".svg")) {
        InvalidateMeasure();
        MarkRenderRectDirty(m_bounds);
        return true;
    }

    IWICImagingFactory2* factory = EnsureWicFactory();
    if (!factory) {
        m_loadError = "WIC 工厂创建失败";
        InvalidateMeasure();
        MarkRenderRectDirty(m_bounds);
        return false;
    }

    const std::wstring wpath = Utf8ToUtf16(path);
    ComPtr<IWICBitmapDecoder> decoder;
    const HRESULT hr = factory->CreateDecoderFromFilename(
        wpath.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnDemand,
        decoder.ReleaseAndGetAddressOf());
    if (FAILED(hr) || !decoder) {
        m_loadError = "无法打开文件";
        InvalidateMeasure();
        MarkRenderRectDirty(m_bounds);
        return false;
    }

    const bool ok = DecodeFrame(decoder.Get());
    InvalidateMeasure();
    MarkRenderRectDirty(m_bounds);
    return ok;
}

bool Image::LoadFromMemory(const void* bytes, size_t size) {
    m_sourcePath.clear();
    m_loadError.clear();
    ReleaseDecoded();
    m_imageType = ImageType::FileSource;

    if (!bytes || size == 0) {
        m_loadError = "空缓冲区";
        InvalidateMeasure();
        MarkRenderRectDirty(m_bounds);
        return false;
    }

    IWICImagingFactory2* factory = EnsureWicFactory();
    if (!factory) {
        m_loadError = "WIC 工厂创建失败";
        return false;
    }

    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!mem) {
        m_loadError = "内存分配失败";
        return false;
    }
    if (void* dst = GlobalLock(mem)) {
        std::memcpy(dst, bytes, size);
        GlobalUnlock(mem);
    } else {
        GlobalFree(mem);
        m_loadError = "内存锁定失败";
        return false;
    }

    ComPtr<IStream> stream;
    HRESULT hr = CreateStreamOnHGlobal(mem, TRUE, stream.ReleaseAndGetAddressOf());
    if (FAILED(hr) || !stream) {
        GlobalFree(mem);
        m_loadError = "无法创建内存流";
        return false;
    }

    ComPtr<IWICBitmapDecoder> decoder;
    hr = factory->CreateDecoderFromStream(
        stream.Get(),
        nullptr,
        WICDecodeMetadataCacheOnDemand,
        decoder.ReleaseAndGetAddressOf());
    if (FAILED(hr) || !decoder) {
        m_loadError = "无法解码内存图像";
        InvalidateMeasure();
        MarkRenderRectDirty(m_bounds);
        return false;
    }

    const bool ok = DecodeFrame(decoder.Get());
    InvalidateMeasure();
    MarkRenderRectDirty(m_bounds);
    return ok;
}

void Image::OnThemeChanged() {
    Control::OnThemeChanged();
    // D2D bitmaps die with the device; WIC copy is device-independent.
    m_d2dBitmap.Reset();
    m_boundDevice.Reset();
}

void Image::EnsureD2dBitmap(GraphicsContext& ctx) {
    if (!m_wicBitmap || !ctx.GetD2DContext()) {
        return;
    }
    if (m_d2dBitmap && m_boundDevice.Get() == ctx.GetD2DDevice()) {
        return;
    }
    m_d2dBitmap.Reset();
    m_boundDevice = ctx.GetD2DDevice();
    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
    ctx.GetD2DContext()->CreateBitmapFromWicBitmap(
        m_wicBitmap.Get(),
        &props,
        m_d2dBitmap.ReleaseAndGetAddressOf());
}

Rect Image::DestRectFor(float srcW, float srcH) const {
    if (srcW < 1.0f) {
        srcW = 1.0f;
    }
    if (srcH < 1.0f) {
        srcH = 1.0f;
    }
    const Rect box = m_bounds;
    if (m_stretch == Stretch::Fill) {
        return box;
    }
    if (m_stretch == Stretch::None) {
        return Rect(box.x, box.y, srcW, srcH);
    }
    const float sx = box.width / srcW;
    const float sy = box.height / srcH;
    const float s = (m_stretch == Stretch::UniformToFill)
        ? (std::max)(sx, sy)
        : (std::min)(sx, sy);
    const float w = srcW * s;
    const float h = srcH * s;
    return Rect(
        box.x + (box.width - w) * 0.5f,
        box.y + (box.height - h) * 0.5f,
        w,
        h);
}

void Image::DrawFilePlaceholder(GraphicsContext& ctx) {
    const ThemeTokens& tokens = ThemeManager::Instance().GetTokens();
    ctx.FillRoundedRect(m_bounds, 4.0f, tokens.cardBackground);
    ctx.DrawRoundedRect(m_bounds, 4.0f, tokens.cardBorder, 1.0f);
    std::string line = m_loadError.empty() ? "无图像" : m_loadError;
    if (!m_sourcePath.empty()) {
        line += "\n";
        line += FileNameOf(m_sourcePath);
    }
    ctx.DrawText(
        line,
        m_bounds,
        tokens.textMuted,
        "微软雅黑",
        11.0f,
        DWRITE_TEXT_ALIGNMENT_CENTER,
        DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

bool Image::InitDynamicBitmap(ID2D1DeviceContext* d2dCtx, UINT width, UINT height) {
    if (!d2dCtx || width == 0 || height == 0) return false;

    m_bmpWidth = width;
    m_bmpHeight = height;
    m_imageType = ImageType::DynamicBitmap;
    m_wicBitmap.Reset();
    m_sourcePath.clear();
    m_loadError.clear();

    // IGNORE: GDI/GetDIBits 32bpp often leaves alpha=0. Premultiplied would draw nothing.
    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)
    );

    HRESULT hr = d2dCtx->CreateBitmap(
        D2D1::SizeU(width, height),
        nullptr,
        0,
        &props,
        m_d2dBitmap.ReleaseAndGetAddressOf()
    );

    m_boundDevice.Reset();
    return SUCCEEDED(hr);
}

void Image::UpdatePixelBuffer(const uint32_t* bgraPixelData, UINT width, UINT height, UINT pitch) {
    if (!bgraPixelData || width == 0 || height == 0) {
        return;
    }

    // Copy to CPU double-buffer thread-safely (avoids cross-thread Direct2D Device Context call crash)
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        m_imageType = ImageType::DynamicBitmap;
        m_bmpWidth = width;
        m_bmpHeight = height;
        const size_t totalPixels = static_cast<size_t>(width) * height;
        if (m_pendingPixelBuffer.size() != totalPixels) {
            m_pendingPixelBuffer.resize(totalPixels);
        }
        if (pitch == 0 || pitch == width * 4) {
            std::memcpy(m_pendingPixelBuffer.data(), bgraPixelData, totalPixels * sizeof(uint32_t));
        } else {
            const uint8_t* src = reinterpret_cast<const uint8_t*>(bgraPixelData);
            for (UINT y = 0; y < height; ++y) {
                std::memcpy(
                    m_pendingPixelBuffer.data() + static_cast<size_t>(y) * width,
                    src + static_cast<size_t>(y) * pitch,
                    static_cast<size_t>(width) * sizeof(uint32_t));
            }
        }
        for (uint32_t& px : m_pendingPixelBuffer) {
            px |= 0xFF000000u;
        }
        m_hasPendingUpdate.store(true, std::memory_order_release);
    }
}

bool Image::OnAnimationTick() {
    if (!m_hasPendingUpdate.load(std::memory_order_acquire)) {
        return Control::OnAnimationTick();
    }
    if (!m_bounds.IsEmpty()) {
        MarkRenderRectDirty(m_bounds);
    }
    return true;
}

bool Image::HasSelfAnimation() const {
    return m_hasPendingUpdate.load(std::memory_order_acquire) || Control::HasSelfAnimation();
}

Size Image::Measure(Size availableSize) {
    float w = GetWidth();
    float h = GetHeight();
    if (w < 0.0f) {
        if (m_bmpWidth > 0) {
            w = static_cast<float>(m_bmpWidth);
        } else if (availableSize.width > 0.0f && availableSize.width < 1e6f) {
            w = availableSize.width;
        } else {
            w = 24.0f;
        }
    }
    if (h < 0.0f) {
        if (m_bmpWidth > 0 && m_bmpHeight > 0 && w > 0.0f
            && (m_stretch == Stretch::Uniform || m_stretch == Stretch::None)) {
            h = w * (static_cast<float>(m_bmpHeight) / static_cast<float>(m_bmpWidth));
        } else if (m_bmpHeight > 0) {
            h = static_cast<float>(m_bmpHeight);
        } else {
            h = 24.0f;
        }
    }
    m_desiredSize = Size(w, h);
    return m_desiredSize;
}

void Image::OnRender(GraphicsContext& ctx) {
    if (m_imageType == ImageType::FileSource) {
        if (GraphicsContext::LooksLikeSvg(m_sourcePath) || GraphicsContext::LooksLikeSvg(GetIcon())) {
            const std::string& svg = GraphicsContext::LooksLikeSvg(m_sourcePath) ? m_sourcePath : GetIcon();
            ctx.PushClip(m_bounds);
            ctx.DrawSvg(svg, DestRectFor(
                m_bounds.width > 1.0f ? m_bounds.width : 24.0f,
                m_bounds.height > 1.0f ? m_bounds.height : 24.0f));
            ctx.PopClip();
            return;
        }
        EnsureD2dBitmap(ctx);
        if (m_d2dBitmap) {
            ctx.PushClip(m_bounds);
            const D2D1_SIZE_F dip = m_d2dBitmap->GetSize();
            const Rect dest = DestRectFor(dip.width, dip.height);
            const D2D1_RECT_F destF = dest.ToD2D();
            ctx.GetD2DContext()->DrawBitmap(
                m_d2dBitmap.Get(),
                &destF,
                1.0f,
                D2D1_INTERPOLATION_MODE_LINEAR);
            ctx.PopClip();
            return;
        }
        DrawFilePlaceholder(ctx);
        return;
    }

    if (m_imageType != ImageType::DynamicBitmap
        && m_hasPendingUpdate.load(std::memory_order_acquire)
        && m_bmpWidth > 0 && m_bmpHeight > 0) {
        m_imageType = ImageType::DynamicBitmap;
    }

    if (m_imageType == ImageType::DynamicBitmap) {
        if ((!m_d2dBitmap || m_d2dBitmap->GetPixelSize().width != m_bmpWidth || m_d2dBitmap->GetPixelSize().height != m_bmpHeight)
            && m_bmpWidth > 0 && m_bmpHeight > 0) {
            InitDynamicBitmap(ctx.GetD2DContext(), m_bmpWidth, m_bmpHeight);
        }
    }

    if (m_imageType == ImageType::DynamicBitmap && m_d2dBitmap) {
        {
            std::lock_guard<std::mutex> lock(m_bufferMutex);
            if (m_hasPendingUpdate.load(std::memory_order_acquire) && !m_pendingPixelBuffer.empty()) {
                D2D1_RECT_U dstRect = D2D1::RectU(0, 0, m_bmpWidth, m_bmpHeight);
                m_d2dBitmap->CopyFromMemory(&dstRect, m_pendingPixelBuffer.data(), m_bmpWidth * sizeof(uint32_t));
                m_hasPendingUpdate.store(false, std::memory_order_release);
            }
        }

        ctx.PushClip(m_bounds);
        const Rect dest = DestRectFor(static_cast<float>(m_bmpWidth), static_cast<float>(m_bmpHeight));
        D2D1_RECT_F destRect = dest.ToD2D();
        ctx.GetD2DContext()->DrawBitmap(
            m_d2dBitmap.Get(),
            &destRect,
            1.0f,
            D2D1_INTERPOLATION_MODE_LINEAR
        );
        ctx.PopClip();
        return;
    }

    if (GraphicsContext::LooksLikeSvg(GetIcon())) {
        ctx.DrawSvg(GetIcon(), m_bounds);
        return;
    }
    if (GraphicsContext::LooksLikeSvg(m_badgeText)) {
        ctx.DrawSvg(m_badgeText, m_bounds);
        return;
    }

    if (m_imageType == ImageType::Avatar) {
        float minDim = (std::min)(m_bounds.width, m_bounds.height);
        float radius = minDim / 2.0f;
        Rect circleRect(m_bounds.x + (m_bounds.width - minDim) / 2.0f, m_bounds.y + (m_bounds.height - minDim) / 2.0f, minDim, minDim);

        ctx.FillRoundedRect(circleRect, radius, m_badgeColor);
        ctx.DrawText(m_badgeText, circleRect, ThemeManager::Instance().GetColor(ThemeTokenId::AccentForeground), "微软雅黑", minDim * 0.45f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
    } else if (m_imageType == ImageType::FileIcon) {
        ctx.FillRoundedRect(m_bounds, 3.0f, m_badgeColor);
        ctx.DrawText(m_badgeText, m_bounds, ThemeManager::Instance().GetColor(ThemeTokenId::AccentForeground), "微软雅黑", m_bounds.height * 0.4f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
    } else if (m_imageType == ImageType::StatusBadge) {
        float minDim = (std::min)(m_bounds.width, m_bounds.height);
        float radius = minDim / 2.0f;
        Rect dotRect(m_bounds.x + (m_bounds.width - minDim) / 2.0f, m_bounds.y + (m_bounds.height - minDim) / 2.0f, minDim, minDim);

        D2D1_COLOR_F ring = ThemeManager::Instance().GetColor(ThemeTokenId::AccentForeground);
        ctx.FillRoundedRect(dotRect, radius, m_badgeColor);
        ctx.DrawRoundedRect(dotRect, radius, D2D1::ColorF(ring.r, ring.g, ring.b, 0.4f), 1.5f);
    } else {
        ctx.FillRoundedRect(m_bounds, 4.0f, m_badgeColor);
        ctx.DrawText(m_badgeText, m_bounds, ThemeManager::Instance().GetColor(ThemeTokenId::AccentForeground), "微软雅黑", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
}

} // namespace CUI
