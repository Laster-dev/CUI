#include "PixelSnapshotTest.h"
#include "../render/RenderLayer.h"

#include <wincodec.h>
#include <wrl/client.h>
#include <algorithm>
#include <cmath>

namespace CUI {

using Microsoft::WRL::ComPtr;

SnapshotBuffer PixelSnapshotTest::CaptureElement(UIElement* element, Size targetSize, GraphicsContext& ctx) {
    SnapshotBuffer result;
    if (!element || targetSize.width <= 0.0f || targetSize.height <= 0.0f) return result;

    const uint32_t w = static_cast<uint32_t>(std::ceil(targetSize.width));
    const uint32_t h = static_cast<uint32_t>(std::ceil(targetSize.height));
    result.width = w;
    result.height = h;
    result.pixels.resize(w * h, 0);

    ID2D1DeviceContext* d2dCtx = ctx.GetD2DContext();
    if (!d2dCtx) return result;

    // 1. 利用现有 RenderLayer 与 PushLayerTarget 离屏栅格化
    RenderLayer layer;
    layer.ApplyCacheable(true);
    const Rect bounds(0.0f, 0.0f, targetSize.width, targetSize.height);

    element->Measure(targetSize);
    element->Arrange(bounds);

    if (!ctx.PushLayerTarget(layer, targetSize, bounds, Color(0.0f, 0.0f, 0.0f, 0.0f), true)) {
        return result;
    }

    element->Render(ctx);
    ctx.PopLayerTarget(layer);

    // 2. 创建 CPU 可读回读 staging bitmap
    const D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_CPU_READ,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    ComPtr<ID2D1Bitmap1> cpuBitmap;
    HRESULT hr = d2dCtx->CreateBitmap(D2D1::SizeU(w, h), nullptr, 0, &props, &cpuBitmap);
    if (SUCCEEDED(hr) && cpuBitmap && layer.GetCacheContext()) {
        const D2D1_POINT_2U dstPoint = { 0, 0 };
        hr = cpuBitmap->CopyFromRenderTarget(&dstPoint, layer.GetCacheContext(), nullptr);
        if (SUCCEEDED(hr)) {
            D2D1_MAPPED_RECT mapped{};
            hr = cpuBitmap->Map(D2D1_MAP_OPTIONS_READ, &mapped);
            if (SUCCEEDED(hr) && mapped.bits) {
                for (uint32_t y = 0; y < h; ++y) {
                    const uint32_t* rowSrc = reinterpret_cast<const uint32_t*>(mapped.bits + y * mapped.pitch);
                    for (uint32_t x = 0; x < w; ++x) {
                        result.pixels[y * w + x] = rowSrc[x];
                    }
                }
                cpuBitmap->Unmap();
            }
        }
    }

    return result;
}

SnapshotDiffReport PixelSnapshotTest::Compare(
    const SnapshotBuffer& golden,
    const SnapshotBuffer& candidate,
    float tolerancePerChannel,
    float maxAllowedDiffRatio) {

    SnapshotDiffReport report;
    if (golden.width != candidate.width || golden.height != candidate.height ||
        golden.pixels.empty() || candidate.pixels.empty()) {
        report.passed = false;
        report.diffRatio = 1.0f;
        report.totalPixels = static_cast<uint32_t>(golden.pixels.size());
        report.diffPixels = report.totalPixels;
        return report;
    }

    const uint32_t total = golden.width * golden.height;
    report.totalPixels = total;
    report.diffHeatmap.width = golden.width;
    report.diffHeatmap.height = golden.height;
    report.diffHeatmap.pixels.resize(total, 0xFF000000);

    uint32_t diffCount = 0;
    float maxDeltaFound = 0.0f;

    for (uint32_t i = 0; i < total; ++i) {
        const uint32_t p1 = golden.pixels[i];
        const uint32_t p2 = candidate.pixels[i];

        const float b1 = static_cast<float>(p1 & 0xFF) / 255.0f;
        const float g1 = static_cast<float>((p1 >> 8) & 0xFF) / 255.0f;
        const float r1 = static_cast<float>((p1 >> 16) & 0xFF) / 255.0f;
        const float a1 = static_cast<float>((p1 >> 24) & 0xFF) / 255.0f;

        const float b2 = static_cast<float>(p2 & 0xFF) / 255.0f;
        const float g2 = static_cast<float>((p2 >> 8) & 0xFF) / 255.0f;
        const float r2 = static_cast<float>((p2 >> 16) & 0xFF) / 255.0f;
        const float a2 = static_cast<float>((p2 >> 24) & 0xFF) / 255.0f;

        const float db = std::abs(b1 - b2);
        const float dg = std::abs(g1 - g2);
        const float dr = std::abs(r1 - r2);
        const float da = std::abs(a1 - a2);

        const float maxD = (std::max)({ db, dg, dr, da });
        maxDeltaFound = (std::max)(maxDeltaFound, maxD);

        if (maxD > tolerancePerChannel) {
            diffCount++;
            // 差异点以高饱和度警示品红/亮红高亮 (0xFFFF0055)
            report.diffHeatmap.pixels[i] = 0xFFFF0055;
        } else {
            // 背景暗灰显示原图轮廓
            const uint32_t gray = static_cast<uint32_t>((r1 * 0.299f + g1 * 0.587f + b1 * 0.114f) * 80.0f);
            report.diffHeatmap.pixels[i] = (0xFF << 24) | (gray << 16) | (gray << 8) | gray;
        }
    }

    report.diffPixels = diffCount;
    report.diffRatio = static_cast<float>(diffCount) / static_cast<float>(total);
    report.maxDelta = maxDeltaFound;
    report.passed = (report.diffRatio <= maxAllowedDiffRatio);

    return report;
}

bool PixelSnapshotTest::SaveToPng(const SnapshotBuffer& buffer, const std::wstring& filePath, GraphicsContext& ctx) {
    if (buffer.width == 0 || buffer.height == 0 || buffer.pixels.empty()) return false;

    IWICImagingFactory2* wicFactory = ctx.GetWicFactory();
    if (!wicFactory) return false;

    ComPtr<IWICStream> stream;
    HRESULT hr = wicFactory->CreateStream(&stream);
    if (FAILED(hr)) return false;

    hr = stream->InitializeFromFilename(filePath.c_str(), GENERIC_WRITE);
    if (FAILED(hr)) return false;

    ComPtr<IWICBitmapEncoder> encoder;
    hr = wicFactory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder);
    if (FAILED(hr)) return false;

    hr = encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);
    if (FAILED(hr)) return false;

    ComPtr<IWICBitmapFrameEncode> frameEncode;
    hr = encoder->CreateNewFrame(&frameEncode, nullptr);
    if (FAILED(hr)) return false;

    hr = frameEncode->Initialize(nullptr);
    if (FAILED(hr)) return false;

    hr = frameEncode->SetSize(buffer.width, buffer.height);
    if (FAILED(hr)) return false;

    WICPixelFormatGUID format = GUID_WICPixelFormat32bppPBGRA;
    hr = frameEncode->SetPixelFormat(&format);
    if (FAILED(hr)) return false;

    const UINT stride = buffer.width * 4;
    const UINT totalBytes = stride * buffer.height;
    hr = frameEncode->WritePixels(
        buffer.height,
        stride,
        totalBytes,
        const_cast<BYTE*>(reinterpret_cast<const BYTE*>(buffer.pixels.data())));
    if (FAILED(hr)) return false;

    hr = frameEncode->Commit();
    if (FAILED(hr)) return false;

    hr = encoder->Commit();
    return SUCCEEDED(hr);
}

} // namespace CUI
