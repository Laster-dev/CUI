#include "Control.h"

namespace CUI {

Control::Control() {
    SetProperty("hoverBackground", Value(D2D1::ColorF(0, 0, 0, 0)));
    SetProperty("pressedBackground", Value(D2D1::ColorF(0, 0, 0, 0)));
}

void Control::OnRender(GraphicsContext& ctx) {
    float radius = GetProperty("cornerRadius").AsFloat(0.0f);
    D2D1_COLOR_F bg = GetProperty("background").AsColor(D2D1::ColorF(0, 0, 0, 0));

    bool enabled = IsEnabled();

    if (enabled) {
        if (m_isPressed) {
            D2D1_COLOR_F pbg = GetProperty("pressedBackground").AsColor(D2D1::ColorF(0, 0, 0, 0));
            if (pbg.a > 0.0f) bg = pbg;
        } else if (m_isHovered) {
            D2D1_COLOR_F hbg = GetProperty("hoverBackground").AsColor(D2D1::ColorF(0, 0, 0, 0));
            if (hbg.a > 0.0f) bg = hbg;
        }
    } else {
        bg = D2D1::ColorF(0x28 / 255.0f, 0x28 / 255.0f, 0x28 / 255.0f, 0.6f);
    }

    if (bg.a > 0.0f) {
        if (radius > 0.0f) {
            ctx.FillRoundedRect(m_bounds, radius, bg);
        } else {
            ctx.FillRect(m_bounds, bg);
        }
    }

    D2D1_COLOR_F borderBrush = enabled
        ? GetProperty("borderBrush").AsColor(D2D1::ColorF(0, 0, 0, 0))
        : D2D1::ColorF(0x3A / 255.0f, 0x3A / 255.0f, 0x3A / 255.0f, 0.5f);
    float borderThickness = GetProperty("borderThickness").AsFloat(0.0f);
    if (borderBrush.a > 0.0f && borderThickness > 0.0f) {
        if (radius > 0.0f) {
            ctx.DrawRoundedRect(m_bounds, radius, borderBrush, borderThickness);
        } else {
            ctx.DrawRect(m_bounds, borderBrush, borderThickness);
        }
    }
}

void Control::OnMouseEnter() {
    if (!IsEnabled()) return;
    UIElement::OnMouseEnter();
}

void Control::OnMouseLeave() {
    UIElement::OnMouseLeave();
}

void Control::OnMouseDown(Point pt) {
    if (!IsEnabled()) return;
    UIElement::OnMouseDown(pt);
}

void Control::OnMouseUp(Point pt) {
    if (!IsEnabled()) return;
    UIElement::OnMouseUp(pt);
}

void Control::OnMouseMove(Point pt) {
    if (!IsEnabled()) return;
    UIElement::OnMouseMove(pt);
}

} // namespace CUI
