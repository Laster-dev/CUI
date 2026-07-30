#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ColorPicker.h"
#include <algorithm>

namespace CUI {

ColorPicker::ColorPicker() {
    SetProperty("selectedColor", Value(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f)));
    SetProperty("width", Value(220.0f));
    SetProperty("height", Value(32.0f));

    m_swatches = {
        D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f), // VS Blue
        D2D1::ColorF(0x4E / 255.0f, 0xC9 / 255.0f, 0xB0 / 255.0f, 1.0f), // Teal
        D2D1::ColorF(0xCE / 255.0f, 0x91 / 255.0f, 0x78 / 255.0f, 1.0f), // Orange
        D2D1::ColorF(0xD1 / 255.0f, 0x69 / 255.0f, 0x69 / 255.0f, 1.0f), // Red
        D2D1::ColorF(0xC5 / 255.0f, 0x86 / 255.0f, 0xC0 / 255.0f, 1.0f), // Purple
        D2D1::ColorF(0x6A / 255.0f, 0x99 / 255.0f, 0x55 / 255.0f, 1.0f)  // Green
    };
}

std::vector<PropertyMeta> ColorPicker::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "selectedColor", "已选色彩 (SelectedColor)", "色彩拾取", "color" });
    return metas;
}

Size ColorPicker::Measure(Size availableSize) {
    float expW = GetProperty("width").AsFloat(220.0f);
    float expH = GetProperty("height").AsFloat(32.0f);
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

void ColorPicker::SetSelectedColor(D2D1_COLOR_F color) {
    SetProperty("selectedColor", Value(color));
    m_onColorChangedEvent.Invoke(this, color);
}

void ColorPicker::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);

    float boxW = 24.0f;
    float boxH = 24.0f;
    float startX = m_bounds.x + 48.0f;
    float startY = m_bounds.y + (m_bounds.height - boxH) * 0.5f;

    for (size_t i = 0; i < m_swatches.size(); ++i) {
        Rect rect(startX + i * (boxW + 6.0f), startY, boxW, boxH);
        if (rect.Contains(pt.x, pt.y)) {
            SetSelectedColor(m_swatches[i]);
            break;
        }
    }
}

void ColorPicker::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    // Draw main selected preview box
    float boxW = 24.0f;
    float boxH = 24.0f;
    float startY = m_bounds.y + (m_bounds.height - boxH) * 0.5f;

    Rect previewRect(m_bounds.x + 4.0f, startY, 32.0f, boxH);
    D2D1_COLOR_F selColor = GetSelectedColor();
    ctx.FillRoundedRect(previewRect, 4.0f, selColor);
    ctx.DrawRoundedRect(previewRect, 4.0f, D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.6f), 1.5f);

    // Draw palette swatches
    float startX = m_bounds.x + 48.0f;
    for (size_t i = 0; i < m_swatches.size(); ++i) {
        Rect rect(startX + i * (boxW + 6.0f), startY, boxW, boxH);
        ctx.FillRoundedRect(rect, 4.0f, m_swatches[i]);
        if (m_swatches[i].r == selColor.r && m_swatches[i].g == selColor.g && m_swatches[i].b == selColor.b) {
            ctx.DrawRoundedRect(rect, 4.0f, D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), 2.0f);
        } else {
            ctx.DrawRoundedRect(rect, 4.0f, D2D1::ColorF(0.3f, 0.3f, 0.3f, 1.0f), 1.0f);
        }
    }
}

} // namespace CUI
