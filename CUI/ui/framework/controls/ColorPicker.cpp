#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ColorPicker.h"
#include "../style/ThemeManager.h"
#include <algorithm>

namespace CUI {

ColorPicker::ColorPicker() {
    SetProperty("selectedColor", Value(ThemeManager::Instance().GetTokens().accentColor));
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

static D2D1_COLOR_F HSVToRGB(float h, float s, float v) {
    float c = v * s;
    float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;

    float r1 = 0, g1 = 0, b1 = 0;
    if (h >= 0 && h < 60) { r1 = c; g1 = x; b1 = 0; }
    else if (h >= 60 && h < 120) { r1 = x; g1 = c; b1 = 0; }
    else if (h >= 120 && h < 180) { r1 = 0; g1 = c; b1 = x; }
    else if (h >= 180 && h < 240) { r1 = 0; g1 = x; b1 = c; }
    else if (h >= 240 && h < 300) { r1 = x; g1 = 0; b1 = c; }
    else { r1 = c; g1 = 0; b1 = x; }

    return D2D1::ColorF(r1 + m, g1 + m, b1 + m, 1.0f);
}

UIElement* ColorPicker::OnHitTestOverlay(float x, float y) {
    if (!m_isPopupOpen) return nullptr;
    float popW = 240.0f;
    float popH = 220.0f;
    Rect popRect(m_bounds.x, m_bounds.y + m_bounds.height + 4.0f, popW, popH);
    if (popRect.Contains(x, y)) {
        return this;
    }
    return nullptr;
}

bool ColorPicker::OnAnimationTick() {
    float dt = UIElement::GetAnimationDeltaSeconds();
    m_popupAnim.SetTarget(m_isPopupOpen ? 1.0f : 0.0f);
    return m_popupAnim.Tick(dt, AnimationSpec{ 0.55f, 0.01f });
}

bool ColorPicker::HasSelfAnimation() const {
    return std::abs(m_popupAnim.Target() - m_popupAnim.Current()) > 0.001f;
}

void ColorPicker::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);

    if (m_isPopupOpen) {
        float popW = 240.0f;
        float popH = 220.0f;
        Rect popRect(m_bounds.x, m_bounds.y + m_bounds.height + 4.0f, popW, popH);

        if (popRect.Contains(pt.x, pt.y)) {
            // Swatch selection
            float boxW = 22.0f;
            float startX = popRect.x + 12.0f;
            float startY = popRect.y + 140.0f;

            for (size_t i = 0; i < m_swatches.size(); ++i) {
                Rect rect(startX + i * (boxW + 6.0f), startY, boxW, boxW);
                if (rect.Contains(pt.x, pt.y)) {
                    SetSelectedColor(m_swatches[i]);
                    return;
                }
            }

            // High Precision Saturation / Value Canvas Click
            Rect canvasRect(popRect.x + 12.0f, popRect.y + 28.0f, 160.0f, 100.0f);
            if (canvasRect.Contains(pt.x, pt.y)) {
                float sat = std::clamp((pt.x - canvasRect.x) / canvasRect.width, 0.0f, 1.0f);
                float val = std::clamp(1.0f - (pt.y - canvasRect.y) / canvasRect.height, 0.0f, 1.0f);
                SetSelectedColor(HSVToRGB(m_hue, sat, val));
                return;
            }

            // Hue Slider Click
            Rect hueRect(popRect.x + 184.0f, popRect.y + 28.0f, 20.0f, 100.0f);
            if (hueRect.Contains(pt.x, pt.y)) {
                m_hue = std::clamp((pt.y - hueRect.y) / hueRect.height, 0.0f, 1.0f) * 360.0f;
                SetSelectedColor(HSVToRGB(m_hue, 1.0f, 1.0f));
                return;
            }
            return;
        }

        m_isPopupOpen = false;
    } else {
        if (m_bounds.Contains(pt.x, pt.y)) {
            m_isPopupOpen = true;
        }
    }
}

void ColorPicker::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    D2D1_COLOR_F selColor = GetSelectedColor();

    Rect previewRect(m_bounds.x + 6.0f, m_bounds.y + 5.0f, 22.0f, 22.0f);
    ctx.FillRoundedRect(previewRect, 3.0f, selColor);
    ctx.DrawRoundedRect(previewRect, 3.0f, ThemeManager::Instance().GetTokens().inputBorder, 1.0f);

    char hexBuf[32];
    sprintf_s(hexBuf, "🎨 #%02X%02X%02X", static_cast<int>(selColor.r * 255), static_cast<int>(selColor.g * 255), static_cast<int>(selColor.b * 255));
    Rect textRect(m_bounds.x + 36.0f, m_bounds.y, m_bounds.width - 44.0f, m_bounds.height);
    ctx.DrawText(hexBuf, textRect, ThemeManager::Instance().GetTokens().textPrimary, "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

void ColorPicker::OnRenderOverlay(GraphicsContext& ctx) {
    float progress = UIElement::AreAnimationsEnabled() ? m_popupAnim.Current() : (m_isPopupOpen ? 1.0f : 0.0f);
    if (progress <= 0.001f) return;

    float popW = 240.0f;
    float popH = 220.0f;
    float currentH = (m_isPopupOpen && progress >= 0.98f) ? popH : (popH * progress);
    Rect popRect(m_bounds.x, m_bounds.y + m_bounds.height + 4.0f, popW, popH);
    Rect clipRect(m_bounds.x, m_bounds.y + m_bounds.height + 4.0f, popW, currentH);

    ctx.PushClip(clipRect);

    D2D1_COLOR_F bg = ThemeManager::Instance().GetTokens().cardBackground;
    D2D1_COLOR_F border = ThemeManager::Instance().GetTokens().cardBorder;
    D2D1_COLOR_F textCol = ThemeManager::Instance().GetTokens().textPrimary;
    D2D1_COLOR_F accentCol = ThemeManager::Instance().GetTokens().accentColor;
    D2D1_COLOR_F textMutedCol = ThemeManager::Instance().GetTokens().textMuted;
    D2D1_COLOR_F selColor = GetSelectedColor();

    ctx.FillRoundedRect(popRect, 6.0f, bg);
    ctx.DrawRoundedRect(popRect, 6.0f, border, 1.5f);

    // Title
    Rect headerRect(popRect.x, popRect.y + 4.0f, popW, 20.0f);
    ctx.DrawText("高精度 HSV 调色盘面板", headerRect, accentCol, "Segoe UI", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);

    // Render High Precision Color Canvas (Grid Sampling Saturation / Value)
    Rect canvasRect(popRect.x + 12.0f, popRect.y + 28.0f, 160.0f, 100.0f);
    ctx.DrawRoundedRect(canvasRect, 2.0f, border, 1.0f);

    float stepX = canvasRect.width / 16.0f;
    float stepY = canvasRect.height / 10.0f;
    for (int y = 0; y < 10; ++y) {
        float valRatio = 1.0f - static_cast<float>(y) / 10.0f;
        for (int x = 0; x < 16; ++x) {
            float satRatio = static_cast<float>(x) / 16.0f;
            D2D1_COLOR_F cellColor = HSVToRGB(m_hue, satRatio, valRatio);
            Rect cCell(canvasRect.x + x * stepX, canvasRect.y + y * stepY, stepX + 0.5f, stepY + 0.5f);
            ctx.FillRect(cCell, cellColor);
        }
    }

    // Render Hue Rainbow Bar (20x100)
    Rect hueRect(popRect.x + 184.0f, popRect.y + 28.0f, 20.0f, 100.0f);
    float hueStepH = hueRect.height / 12.0f;
    for (int i = 0; i < 12; ++i) {
        float hVal = (static_cast<float>(i) / 12.0f) * 360.0f;
        D2D1_COLOR_F hColor = HSVToRGB(hVal, 1.0f, 1.0f);
        Rect hBar(hueRect.x, hueRect.y + i * hueStepH, hueRect.width, hueStepH + 0.5f);
        ctx.FillRect(hBar, hColor);
    }
    ctx.DrawRect(hueRect, border);

    // Render Presets Bar on Bottom
    float boxW = 22.0f;
    float startX = popRect.x + 12.0f;
    float startY = popRect.y + 140.0f;

    for (size_t i = 0; i < m_swatches.size(); ++i) {
        Rect rect(startX + i * (boxW + 6.0f), startY, boxW, boxW);
        ctx.FillRoundedRect(rect, 4.0f, m_swatches[i]);
        if (m_swatches[i].r == selColor.r && m_swatches[i].g == selColor.g && m_swatches[i].b == selColor.b) {
            ctx.DrawRoundedRect(rect, 4.0f, textCol, 2.0f);
        } else {
            ctx.DrawRoundedRect(rect, 4.0f, border, 1.0f);
        }
    }

    // HEX Output Line
    char hexDetail[64];
    sprintf_s(hexDetail, "RGB(%d, %d, %d)", static_cast<int>(selColor.r * 255), static_cast<int>(selColor.g * 255), static_cast<int>(selColor.b * 255));
    Rect bottomText(popRect.x + 12.0f, popRect.y + 175.0f, popW - 24.0f, 20.0f);
    ctx.DrawText(hexDetail, bottomText, textMutedCol, "Segoe UI", 11.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    ctx.PopClip();
}

} // namespace CUI
