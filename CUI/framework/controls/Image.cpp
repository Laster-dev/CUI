#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Image.h"
#include <algorithm>

namespace CUI {

Image::Image() {
    SetProperty("width", Value(24.0f));
    SetProperty("height", Value(24.0f));
}

Image::Image(ImageType type, const std::string& text, D2D1_COLOR_F color)
    : m_imageType(type), m_badgeText(text), m_badgeColor(color) {
    SetProperty("width", Value(24.0f));
    SetProperty("height", Value(24.0f));
}

Size Image::Measure(Size availableSize) {
    float w = GetProperty("width").AsFloat(24.0f);
    float h = GetProperty("height").AsFloat(24.0f);
    m_desiredSize = Size(w, h);
    return m_desiredSize;
}

void Image::OnRender(GraphicsContext& ctx) {
    if (m_imageType == ImageType::Avatar) {
        // Draw Circular Avatar Badge
        float minDim = std::min(m_bounds.width, m_bounds.height);
        float radius = minDim / 2.0f;
        Rect circleRect(m_bounds.x + (m_bounds.width - minDim) / 2.0f, m_bounds.y + (m_bounds.height - minDim) / 2.0f, minDim, minDim);

        ctx.FillRoundedRect(circleRect, radius, m_badgeColor);
        ctx.DrawText(m_badgeText, circleRect, D2D1::ColorF(1.0f, 1.0f, 1.0f), "Segoe UI", minDim * 0.45f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
    } else if (m_imageType == ImageType::FileIcon) {
        // Draw Document / File Badge Image
        ctx.FillRoundedRect(m_bounds, 3.0f, m_badgeColor);
        ctx.DrawText(m_badgeText, m_bounds, D2D1::ColorF(1.0f, 1.0f, 1.0f), "Segoe UI", m_bounds.height * 0.4f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
    } else if (m_imageType == ImageType::StatusBadge) {
        // Draw Glowing Status Dot Badge Image
        float minDim = std::min(m_bounds.width, m_bounds.height);
        float radius = minDim / 2.0f;
        Rect dotRect(m_bounds.x + (m_bounds.width - minDim) / 2.0f, m_bounds.y + (m_bounds.height - minDim) / 2.0f, minDim, minDim);

        ctx.FillRoundedRect(dotRect, radius, m_badgeColor);
        ctx.DrawRoundedRect(dotRect, radius, D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.4f), 1.5f);
    } else {
        ctx.FillRoundedRect(m_bounds, 4.0f, m_badgeColor);
        ctx.DrawText(m_badgeText, m_bounds, D2D1::ColorF(1.0f, 1.0f, 1.0f), "Segoe UI", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
}

} // namespace CUI
