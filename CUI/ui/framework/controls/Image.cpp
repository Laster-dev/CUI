#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Image.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <iostream>

namespace CUI {

Image::Image() {
    m_badgeColor = ThemeManager::Instance().GetColor(ThemeTokenId::AccentColor);
    SetWidth(24.0f);
    SetHeight(24.0f);
}

Image::Image(ImageType type, const std::string& text)
    : m_imageType(type), m_badgeText(text), m_badgeColor(ThemeManager::Instance().GetColor(ThemeTokenId::AccentColor)) {
    SetWidth(24.0f);
    SetHeight(24.0f);
}

Image::Image(ImageType type, const std::string& text, D2D1_COLOR_F color)
    : m_imageType(type), m_badgeText(text), m_badgeColor(color) {
    SetWidth(24.0f);
    SetHeight(24.0f);
}

bool Image::InitDynamicBitmap(ID2D1DeviceContext* d2dCtx, UINT width, UINT height) {
    if (!d2dCtx || width == 0 || height == 0) return false;

    m_bmpWidth = width;
    m_bmpHeight = height;
    m_imageType = ImageType::DynamicBitmap;

    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );

    HRESULT hr = d2dCtx->CreateBitmap(
        D2D1::SizeU(width, height),
        nullptr,
        0,
        &props,
        m_d2dBitmap.ReleaseAndGetAddressOf()
    );

    return SUCCEEDED(hr);
}

void Image::UpdatePixelBuffer(const uint32_t* bgraPixelData, UINT width, UINT height, UINT pitch) {
    if (!bgraPixelData) return;

    // Copy to CPU double-buffer thread-safely (avoids cross-thread Direct2D Device Context call crash)
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    size_t totalPixels = static_cast<size_t>(width) * height;
    if (m_pendingPixelBuffer.size() != totalPixels) {
        m_pendingPixelBuffer.resize(totalPixels);
    }
    std::memcpy(m_pendingPixelBuffer.data(), bgraPixelData, totalPixels * sizeof(uint32_t));
    m_hasPendingUpdate = true;
}

Size Image::Measure(Size availableSize) {
    (void)availableSize;
    float w = GetWidth(); if (w < 0) w = 24.0f;
    float h = GetHeight(); if (h < 0) h = 24.0f;
    m_desiredSize = Size(w, h);
    return m_desiredSize;
}

void Image::OnRender(GraphicsContext& ctx) {
    if (m_imageType == ImageType::DynamicBitmap && m_d2dBitmap) {
        // Safely upload pending CPU buffer to GPU Texture on UI Rendering Thread!
        {
            std::lock_guard<std::mutex> lock(m_bufferMutex);
            if (m_hasPendingUpdate && !m_pendingPixelBuffer.empty()) {
                D2D1_RECT_U dstRect = D2D1::RectU(0, 0, m_bmpWidth, m_bmpHeight);
                m_d2dBitmap->CopyFromMemory(&dstRect, m_pendingPixelBuffer.data(), m_bmpWidth * sizeof(uint32_t));
                m_hasPendingUpdate = false;
            }
        }

        // Ultra High Performance Dynamic Hardware Bitmap Rendering
        D2D1_RECT_F destRect = D2D1::RectF(m_bounds.x, m_bounds.y, m_bounds.x + m_bounds.width, m_bounds.y + m_bounds.height);
        
        ctx.GetD2DContext()->DrawBitmap(
            m_d2dBitmap.Get(),
            &destRect,
            1.0f,
            D2D1_INTERPOLATION_MODE_LINEAR
        );
        return;
    }

    if (m_imageType == ImageType::Avatar) {
        // Draw Circular Avatar Badge
        float minDim = (std::min)(m_bounds.width, m_bounds.height);
        float radius = minDim / 2.0f;
        Rect circleRect(m_bounds.x + (m_bounds.width - minDim) / 2.0f, m_bounds.y + (m_bounds.height - minDim) / 2.0f, minDim, minDim);

        ctx.FillRoundedRect(circleRect, radius, m_badgeColor);
        ctx.DrawText(m_badgeText, circleRect, ThemeManager::Instance().GetColor(ThemeTokenId::AccentForeground), "微软雅黑", minDim * 0.45f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
    } else if (m_imageType == ImageType::FileIcon) {
        // Draw Document / File Badge Image
        ctx.FillRoundedRect(m_bounds, 3.0f, m_badgeColor);
        ctx.DrawText(m_badgeText, m_bounds, ThemeManager::Instance().GetColor(ThemeTokenId::AccentForeground), "微软雅黑", m_bounds.height * 0.4f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
    } else if (m_imageType == ImageType::StatusBadge) {
        // Draw Glowing Status Dot Badge Image
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
