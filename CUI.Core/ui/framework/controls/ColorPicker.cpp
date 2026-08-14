#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ColorPicker.h"
#include "../style/ThemeManager.h"
#include "../window/PopupPlacement.h"
#include <algorithm>

namespace CUI {

ColorPicker::ColorPicker() {
    SelectedColor.Initialize(*this);
    const ThemeTokens& tokens = ThemeManager::Instance().GetTokens();
    m_selectedColor = tokens.accentColor;
    SetWidth(220.0f);
    SetHeight(32.0f);

    m_swatches = {
        tokens.accentColor,
        D2D1::ColorF(0x4E / 255.0f, 0xC9 / 255.0f, 0xB0 / 255.0f, 1.0f), // Teal
        D2D1::ColorF(0xCE / 255.0f, 0x91 / 255.0f, 0x78 / 255.0f, 1.0f), // Orange
        tokens.dangerColor,
        D2D1::ColorF(0xC5 / 255.0f, 0x86 / 255.0f, 0xC0 / 255.0f, 1.0f), // Purple
        D2D1::ColorF(0x6A / 255.0f, 0x99 / 255.0f, 0x55 / 255.0f, 1.0f)  // Green
    };
}

Value ColorPicker::GetProperty(PropertyId id) const {
    if (id == PropertyId::SelectedColor) return Value(m_selectedColor);
    return UIElement::GetProperty(id);
}

bool ColorPicker::HasProperty(PropertyId id) const {
    return id == PropertyId::SelectedColor || UIElement::HasProperty(id);
}

void ColorPicker::SetProperty(PropertyId id, const Value& val) {
    if (id == PropertyId::SelectedColor) {
        SetSelectedColor(val.AsColor());
        return;
    }
    UIElement::SetProperty(id, val);
}

Size ColorPicker::Measure(Size availableSize) {
    float expW = GetWidth(); if (expW < 0) expW = 220.0f;
    float expH = GetHeight(); if (expH < 0) expH = 32.0f;
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

void ColorPicker::SetSelectedColor(D2D1_COLOR_F color) {
    m_selectedColor = color;
    NotifyFieldChanged(PropertyId::SelectedColor, Value(color));
    m_onColorChangedEvent.Invoke(this, color);
    MarkPopupDirty();
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
    Rect popRect = GetPopupBounds();
    if (popRect.Contains(x, y)) {
        return this;
    }
    return nullptr;
}

void ColorPicker::MarkPopupDirty() {
    MarkRenderRectDirty(m_bounds.Inflate(2.0f));
    if (m_isPopupOpen || m_popupAnim.Current() > 0.001f) {
        MarkRenderRectDirty(GetPopupBounds().Inflate(6.0f));
    }
}

Rect ColorPicker::CanvasRect(const Rect& popRect) const {
    return Rect(popRect.x + 12.0f, popRect.y + 28.0f, 160.0f, 100.0f);
}

Rect ColorPicker::HueRect(const Rect& popRect) const {
    return Rect(popRect.x + 184.0f, popRect.y + 28.0f, 20.0f, 100.0f);
}

Rect ColorPicker::SwatchRect(const Rect& popRect, size_t index) const {
    constexpr float boxW = 22.0f;
    return Rect(popRect.x + 12.0f + static_cast<float>(index) * (boxW + 6.0f), popRect.y + 140.0f, boxW, boxW);
}

ColorPicker::PopupPart ColorPicker::HitTestPopupPart(Point pt, int* swatchIndex) const {
    if (swatchIndex) {
        *swatchIndex = -1;
    }
    const Rect popRect = GetPopupBounds();
    if (!popRect.Contains(pt.x, pt.y)) {
        return PopupPart::None;
    }
    if (CanvasRect(popRect).Contains(pt.x, pt.y)) {
        return PopupPart::Canvas;
    }
    if (HueRect(popRect).Contains(pt.x, pt.y)) {
        return PopupPart::Hue;
    }
    for (size_t i = 0; i < m_swatches.size(); ++i) {
        if (SwatchRect(popRect, i).Contains(pt.x, pt.y)) {
            if (swatchIndex) {
                *swatchIndex = static_cast<int>(i);
            }
            return PopupPart::Swatch;
        }
    }
    return PopupPart::None;
}

bool ColorPicker::ApplyPopupPoint(Point pt, bool allowSwatch) {
    const Rect popRect = GetPopupBounds();
    const Rect canvasRect = CanvasRect(popRect);
    if (canvasRect.Contains(pt.x, pt.y)) {
        m_sat = std::clamp((pt.x - canvasRect.x) / canvasRect.width, 0.0f, 1.0f);
        m_val = std::clamp(1.0f - (pt.y - canvasRect.y) / canvasRect.height, 0.0f, 1.0f);
        SetSelectedColor(HSVToRGB(m_hue, m_sat, m_val));
        return true;
    }

    const Rect hueRect = HueRect(popRect);
    if (hueRect.Contains(pt.x, pt.y)) {
        m_hue = std::clamp((pt.y - hueRect.y) / hueRect.height, 0.0f, 1.0f) * 360.0f;
        SetSelectedColor(HSVToRGB(m_hue, m_sat, m_val));
        return true;
    }

    if (!allowSwatch) {
        return popRect.Contains(pt.x, pt.y);
    }
    for (size_t i = 0; i < m_swatches.size(); ++i) {
        if (SwatchRect(popRect, i).Contains(pt.x, pt.y)) {
            SetSelectedColor(m_swatches[i]);
            return true;
        }
    }
    return popRect.Contains(pt.x, pt.y);
}

bool ColorPicker::OnAnimationTick() {
    float dt = UIElement::GetAnimationDeltaSeconds();
    m_popupAnim.SetTarget(m_isPopupOpen ? 1.0f : 0.0f);
    bool animating = m_popupAnim.Tick(dt, PopupReveal::kSpec);
    if (animating) {
        MarkPopupDirty();
        RequestAnimationTicks();
    }
    return animating;
}

bool ColorPicker::HasSelfAnimation() const {
    return std::abs(m_popupAnim.Target() - m_popupAnim.Current()) > 0.001f;
}

void ColorPicker::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);
    m_dragPart = PopupPart::None;

    if (m_isPopupOpen) {
        const PopupPart part = HitTestPopupPart(pt);
        if (part != PopupPart::None) {
            m_dragPart = (part == PopupPart::Canvas || part == PopupPart::Hue) ? part : PopupPart::None;
            ApplyPopupPoint(pt, true);
            return;
        }
        if (GetPopupBounds().Contains(pt.x, pt.y)) {
            return;
        }
        SetPopupOpen(false);
        return;
    }

    if (m_bounds.Contains(pt.x, pt.y)) {
        SetPopupOpen(true);
    }
}

void ColorPicker::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);
    if (!m_isPopupOpen || m_dragPart == PopupPart::None) {
        return;
    }
    const Rect popRect = GetPopupBounds();
    if (m_dragPart == PopupPart::Canvas) {
        const Rect canvasRect = CanvasRect(popRect);
        m_sat = std::clamp((pt.x - canvasRect.x) / canvasRect.width, 0.0f, 1.0f);
        m_val = std::clamp(1.0f - (pt.y - canvasRect.y) / canvasRect.height, 0.0f, 1.0f);
        SetSelectedColor(HSVToRGB(m_hue, m_sat, m_val));
        return;
    }
    if (m_dragPart == PopupPart::Hue) {
        const Rect hueRect = HueRect(popRect);
        m_hue = std::clamp((pt.y - hueRect.y) / hueRect.height, 0.0f, 1.0f) * 360.0f;
        SetSelectedColor(HSVToRGB(m_hue, m_sat, m_val));
    }
}

void ColorPicker::OnMouseUp(Point pt) {
    Control::OnMouseUp(pt);
    m_dragPart = PopupPart::None;
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
    ctx.DrawText(hexBuf, textRect, ThemeManager::Instance().GetTokens().textPrimary, "微软雅黑", 12.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

void ColorPicker::SetPopupOpen(bool open) {
    if (m_isPopupOpen == open) return;
    m_isPopupOpen = open;
    if (!open) {
        m_dragPart = PopupPart::None;
    }
    if (PopupHost* host = PopupHost::Current()) {
        if (open) {
            host->Open(this);
        } else {
            host->Close(this);
        }
    }
    RequestAnimationTicks();
    MarkPopupDirty();
}

Rect ColorPicker::GetPopupBounds() const {
    constexpr float popW = 240.0f;
    constexpr float popH = 220.0f;
    return PlacePopupNearAnchor(m_bounds, popW, popH, GetPopupViewportOrDefault(), 4.0f);
}

bool ColorPicker::HitDismissExempt(float x, float y) const {
    if (m_bounds.Contains(x, y)) return true;
    return GetPopupBounds().Contains(x, y);
}

void ColorPicker::OnRenderOverlay(GraphicsContext& ctx) {
    if (PopupHost::Current() && m_isPopupOpen) return;
    RenderPopup(ctx);
}

void ColorPicker::RenderPopup(GraphicsContext& ctx) {
    float progress = UIElement::AreAnimationsEnabled() ? m_popupAnim.Current() : (m_isPopupOpen ? 1.0f : 0.0f);
    if (progress <= 0.001f) return;

    Rect popRect = GetPopupBounds();
    float popW = popRect.width;
    ctx.PushPopupReveal(popRect, progress, Point(popRect.x + popW * 0.5f, popRect.y));

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
    ctx.DrawText("高精度 HSV 调色盘面板", headerRect, accentCol, "微软雅黑", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);

    // Render High Precision Color Canvas (Grid Sampling Saturation / Value)
    Rect canvasRect = CanvasRect(popRect);
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
    Rect hueRect = HueRect(popRect);
    float hueStepH = hueRect.height / 12.0f;
    for (int i = 0; i < 12; ++i) {
        float hVal = (static_cast<float>(i) / 12.0f) * 360.0f;
        D2D1_COLOR_F hColor = HSVToRGB(hVal, 1.0f, 1.0f);
        Rect hBar(hueRect.x, hueRect.y + i * hueStepH, hueRect.width, hueStepH + 0.5f);
        ctx.FillRect(hBar, hColor);
    }
    ctx.DrawRect(hueRect, border);

    // Render Presets Bar on Bottom
    for (size_t i = 0; i < m_swatches.size(); ++i) {
        Rect rect = SwatchRect(popRect, i);
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
    ctx.DrawText(hexDetail, bottomText, textMutedCol, "微软雅黑", 11.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    ctx.PopPopupReveal();
}

} // namespace CUI
