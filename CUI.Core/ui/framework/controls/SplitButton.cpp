#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "SplitButton.h"
#include "../style/ThemeManager.h"
#include <algorithm>

namespace CUI {

SplitButton::SplitButton() {
    SetText("Split");
}

SplitButton::SplitButton(const std::string& text) : SplitButton() {
    SetText(text);
}

Rect SplitButton::PrimaryRect() const {
    return Rect(m_bounds.x, m_bounds.y, (std::max)(0.0f, m_bounds.width - kChevronSlot), m_bounds.height);
}

void SplitButton::OnRender(GraphicsContext& ctx) {
    DropDownButton::OnRender(ctx);

    const Rect chevron = ChevronRect();
    D2D1_COLOR_F line = ResolveThemeColor(GetColorToken(), ThemeTokenId::AccentForeground);
    line.a *= 0.35f;
    const float x = chevron.x;
    const float inset = (std::max)(4.0f, m_bounds.height * 0.22f);
    ctx.DrawLine(
        Point(x, m_bounds.y + inset),
        Point(x, m_bounds.y + m_bounds.height - inset),
        line,
        1.0f);
}

void SplitButton::OnMouseDown(Point pt) {
    if (!IsEnabled()) {
        return;
    }
    if (HandleMenuMouseDown(pt)) {
        m_pressInChevron = false;
        EndPressWithoutClick();
        return;
    }
    m_pressInChevron = ChevronRect().Contains(pt.x, pt.y);
    Button::OnMouseDown(pt);
    if (m_pressInChevron) {
        SetDropDownOpen(!IsDropDownOpen());
    } else if (IsDropDownOpen()) {
        SetDropDownOpen(false);
    }
}

void SplitButton::OnMouseUp(Point pt) {
    const bool primaryClick = IsPressed() && !m_pressInChevron
        && IsEnabled() && PrimaryRect().Contains(pt.x, pt.y);
    if (primaryClick) {
        Control::OnMouseUp(pt);
        return;
    }
    EndPressWithoutClick();
}

void SplitButton::OnKeyDown(int vkCode) {
    if (!IsEnabled()) {
        return;
    }
    const bool altDown = (GetKeyState(VK_MENU) & 0x8000) != 0;
    if (vkCode == VK_ESCAPE) {
        SetDropDownOpen(false);
        return;
    }
    if ((vkCode == VK_DOWN && altDown) || vkCode == VK_F4) {
        SetDropDownOpen(true);
        return;
    }
    if (IsDropDownOpen()) {
        DropDownButton::OnKeyDown(vkCode);
        return;
    }
    if (vkCode == VK_SPACE || vkCode == VK_RETURN) {
        OnClick().Invoke(this);
        return;
    }
    Button::OnKeyDown(vkCode);
}

} // namespace CUI
