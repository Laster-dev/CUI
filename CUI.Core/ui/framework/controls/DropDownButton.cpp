#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "DropDownButton.h"
#include "../style/ThemeManager.h"
#include "../window/PopupPlacement.h"
#include <algorithm>
#include <cmath>

namespace CUI {

DropDownButton::DropDownButton() {
    SetText("DropDown");
    SetPadding(Thickness(10.0f, 4.0f, 4.0f, 4.0f));
}

DropDownButton::DropDownButton(const std::string& text) : DropDownButton() {
    SetText(text);
}

DropDownButton::~DropDownButton() {
    if (PopupHost* host = PopupHost::Current()) {
        host->Close(this);
    }
}

Size DropDownButton::Measure(Size availableSize) {
    Size base = Button::Measure(availableSize);
    if (GetWidth() < 0.0f) {
        base.width += kChevronSlot;
        m_desiredSize.width = base.width;
    }
    return m_desiredSize;
}

Rect DropDownButton::ChevronRect() const {
    return Rect(
        m_bounds.x + m_bounds.width - kChevronSlot,
        m_bounds.y,
        kChevronSlot,
        m_bounds.height);
}

Rect DropDownButton::LabelRect() const {
    Thickness padding = GetPadding();
    return Rect(
        m_bounds.x + padding.left,
        m_bounds.y + padding.top,
        (std::max)(0.0f, m_bounds.width - padding.left - padding.right - kChevronSlot),
        (std::max)(0.0f, m_bounds.height - padding.top - padding.bottom));
}

float DropDownButton::MenuContentHeight() const {
    float h = kMenuPad * 2.0f;
    for (const auto& item : m_items) {
        h += item.separator ? kSepH : kItemH;
    }
    if (m_items.empty()) {
        h += kItemH;
    }
    return h;
}

Rect DropDownButton::MenuRect() const {
    const float width = (std::max)(m_bounds.width, 160.0f);
    const Rect viewport = GetPopupViewportOrDefault();
    return PlacePopupNearAnchor(m_bounds, width, MenuContentHeight(), viewport, 2.0f);
}

Rect DropDownButton::GetPopupBounds() const {
    return MenuRect();
}

bool DropDownButton::HitDismissExempt(float x, float y) const {
    return m_bounds.Contains(x, y) || MenuRect().Contains(x, y);
}

int DropDownButton::HitTestMenuItem(Point pt) const {
    const Rect menu = MenuRect();
    if (!menu.Contains(pt.x, pt.y)) {
        return -1;
    }
    float y = menu.y + kMenuPad;
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        const float h = m_items[i].separator ? kSepH : kItemH;
        if (pt.y >= y && pt.y < y + h) {
            return m_items[i].separator || !m_items[i].enabled ? -1 : i;
        }
        y += h;
    }
    return -1;
}

UIElement* DropDownButton::HitTestOverlay(float x, float y) {
    const float progress = UIElement::AreAnimationsEnabled() ? m_popupAnim.Current() : (m_isDropDownOpen ? 1.0f : 0.0f);
    if (progress <= 0.5f) {
        return nullptr;
    }
    return MenuRect().Contains(x, y) ? this : nullptr;
}

int DropDownButton::AddItem(const std::string& text, std::function<void()> onClick) {
    ButtonFlyoutItem item;
    item.text = text;
    item.onClick = std::move(onClick);
    m_items.push_back(std::move(item));
    return static_cast<int>(m_items.size()) - 1;
}

void DropDownButton::AddSeparator() {
    ButtonFlyoutItem item;
    item.separator = true;
    m_items.push_back(std::move(item));
}

void DropDownButton::ClearItems() {
    m_items.clear();
    m_hoverIndex = -1;
    if (m_isDropDownOpen) {
        MarkRenderRectDirty(GetPopupBounds().Inflate(6.0f));
    }
}

void DropDownButton::SetDropDownOpen(bool open) {
    if (m_isDropDownOpen == open) {
        return;
    }
    m_isDropDownOpen = open;
    if (open) {
        m_hoverIndex = -1;
        for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
            if (!m_items[i].separator && m_items[i].enabled) {
                m_hoverIndex = i;
                break;
            }
        }
    }
    if (PopupHost* host = PopupHost::Current()) {
        if (open) {
            host->Open(this);
        } else {
            host->Close(this);
        }
    }
    RequestAnimationTicks();
    MarkRenderRectDirty(m_bounds.Inflate(2.0f));
    MarkRenderRectDirty(GetPopupBounds().Inflate(6.0f));
}

void DropDownButton::EndPressWithoutClick() {
    if (m_isPressed) {
        m_isPressed = false;
        UpdateVisualStateTarget();
        m_visualStateAnim.SetTarget(m_visualStateTarget);
        MarkRenderRectDirty(m_bounds);
    }
}

bool DropDownButton::HandleMenuMouseDown(Point pt) {
    if (!m_isDropDownOpen) {
        return false;
    }
    const int idx = HitTestMenuItem(pt);
    if (idx < 0) {
        if (MenuRect().Contains(pt.x, pt.y)) {
            return true;
        }
        return false;
    }
    if (m_items[idx].onClick) {
        m_items[idx].onClick();
    }
    m_onItemChosenEvent.Invoke(this, idx, m_items[idx].text);
    SetDropDownOpen(false);
    return true;
}

void DropDownButton::MoveHighlight(int delta) {
    if (m_items.empty()) {
        return;
    }
    int idx = m_hoverIndex;
    for (int n = 0; n < static_cast<int>(m_items.size()); ++n) {
        idx += delta;
        if (idx < 0) {
            idx = static_cast<int>(m_items.size()) - 1;
        } else if (idx >= static_cast<int>(m_items.size())) {
            idx = 0;
        }
        if (!m_items[idx].separator && m_items[idx].enabled) {
            if (m_hoverIndex != idx) {
                m_hoverIndex = idx;
                MarkRenderRectDirty(GetPopupBounds().Inflate(4.0f));
            }
            return;
        }
    }
}

void DropDownButton::ActivateHighlighted() {
    if (m_hoverIndex < 0 || m_hoverIndex >= static_cast<int>(m_items.size())) {
        return;
    }
    const auto& item = m_items[m_hoverIndex];
    if (item.separator || !item.enabled) {
        return;
    }
    if (item.onClick) {
        item.onClick();
    }
    m_onItemChosenEvent.Invoke(this, m_hoverIndex, item.text);
    SetDropDownOpen(false);
}

void DropDownButton::OnMouseDown(Point pt) {
    if (!IsEnabled()) {
        return;
    }
    if (HandleMenuMouseDown(pt)) {
        EndPressWithoutClick();
        return;
    }
    Button::OnMouseDown(pt);
    if (OpensOnPrimaryPress()) {
        SetDropDownOpen(!m_isDropDownOpen);
    }
}

void DropDownButton::OnMouseMove(Point pt) {
    Button::OnMouseMove(pt);
    if (!m_isDropDownOpen) {
        return;
    }
    const int idx = HitTestMenuItem(pt);
    if (idx != m_hoverIndex && idx >= 0) {
        m_hoverIndex = idx;
        MarkRenderRectDirty(GetPopupBounds().Inflate(4.0f));
    }
}

void DropDownButton::OnMouseUp(Point pt) {
    (void)pt;
    EndPressWithoutClick();
}

void DropDownButton::OnMouseWheel(float delta) {
    if (m_isDropDownOpen) {
        MoveHighlight(delta > 0.0f ? -1 : 1);
    }
}

void DropDownButton::OnKeyDown(int vkCode) {
    Button::OnKeyDown(vkCode);
    if (!IsEnabled()) {
        return;
    }
    const bool altDown = (GetKeyState(VK_MENU) & 0x8000) != 0;
    if (vkCode == VK_ESCAPE) {
        SetDropDownOpen(false);
        return;
    }
    if (vkCode == VK_DOWN && (altDown || !m_isDropDownOpen)) {
        SetDropDownOpen(true);
        return;
    }
    if (m_isDropDownOpen) {
        if (vkCode == VK_DOWN) {
            MoveHighlight(1);
        } else if (vkCode == VK_UP) {
            MoveHighlight(-1);
        } else if (vkCode == VK_RETURN || vkCode == VK_SPACE) {
            ActivateHighlighted();
        }
        return;
    }
    if (vkCode == VK_SPACE || vkCode == VK_RETURN) {
        SetDropDownOpen(true);
    }
}

void DropDownButton::OnBlur() {
    Button::OnBlur();
    SetDropDownOpen(false);
}

void DropDownButton::OnNavigatedFrom() {
    SetDropDownOpen(false);
    Button::OnNavigatedFrom();
}

bool DropDownButton::OnAnimationTick() {
    bool any = Button::OnAnimationTick();
    const float dt = UIElement::GetAnimationDeltaSeconds();
    const AnimationSpec spec{ 0.55f, 0.01f };
    m_popupAnim.SetTarget(m_isDropDownOpen ? 1.0f : 0.0f);
    if (m_popupAnim.Tick(dt, spec)) {
        any = true;
    }
    m_arrowAnim.SetTarget(m_isDropDownOpen ? 1.0f : 0.0f);
    if (m_arrowAnim.Tick(dt, spec)) {
        any = true;
    }
    if (any) {
        MarkRenderRectDirty(m_bounds.Inflate(2.0f));
        if (m_isDropDownOpen || m_popupAnim.Current() > 0.001f) {
            MarkRenderRectDirty(GetPopupBounds().Inflate(6.0f));
        }
        RequestAnimationTicks();
    }
    return any;
}

bool DropDownButton::HasSelfAnimation() const {
    return Button::HasSelfAnimation()
        || std::abs(m_popupAnim.Target() - m_popupAnim.Current()) > 0.001f
        || std::abs(m_arrowAnim.Target() - m_arrowAnim.Current()) > 0.001f;
}

void DropDownButton::CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const {
    if (!HasSelfAnimation() || m_bounds.IsEmpty()) {
        return;
    }
    Rect area = m_bounds.Union(MenuRect()).Inflate(4.0f);
    dirtyRect = hasDirty ? dirtyRect.Union(area) : area;
    hasDirty = true;
}

void DropDownButton::RenderPopup(GraphicsContext& ctx) {
    const float progress = UIElement::AreAnimationsEnabled()
        ? m_popupAnim.Current()
        : (m_isDropDownOpen ? 1.0f : 0.0f);
    if (progress <= 0.001f) {
        return;
    }

    const Rect menu = MenuRect();
    Rect clip = menu;
    clip.height = (m_isDropDownOpen && progress >= 0.98f) ? menu.height : menu.height * progress;
    ctx.PushClip(clip);

    const float radius = (std::max)(4.0f, GetCornerRadius());
    D2D1_COLOR_F dropBg = ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBackground);
    D2D1_COLOR_F border = ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBorder);
    D2D1_COLOR_F hoverBg = ThemeManager::Instance().GetFlatColor(ThemeTokenId::HoverBackground);
    D2D1_COLOR_F text = ThemeManager::Instance().GetFlatColor(ThemeTokenId::TextPrimary);
    D2D1_COLOR_F muted = ThemeManager::Instance().GetFlatColor(ThemeTokenId::TextMuted);

    ctx.FillRoundedRect(menu, radius, dropBg);
    ctx.DrawRoundedRect(menu, radius, border, 1.0f);

    float y = menu.y + kMenuPad;
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        const auto& item = m_items[i];
        if (item.separator) {
            const float midY = y + kSepH * 0.5f;
            ctx.DrawLine(
                Point(menu.x + 10.0f, midY),
                Point(menu.x + menu.width - 10.0f, midY),
                border, 1.0f);
            y += kSepH;
            continue;
        }
        Rect row(menu.x + 4.0f, y + 2.0f, menu.width - 8.0f, kItemH - 4.0f);
        if (i == m_hoverIndex && item.enabled) {
            ctx.FillRoundedRect(row, 3.0f, hoverBg);
        }
        ctx.DrawText(
            item.text,
            Rect(row.x + 10.0f, row.y, row.width - 20.0f, row.height),
            item.enabled ? text : muted,
            GetFontFamily(),
            GetFontSize(),
            DWRITE_TEXT_ALIGNMENT_LEADING,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        y += kItemH;
    }

    ctx.PopClip();
}

void DropDownButton::OnRenderOverlay(GraphicsContext& ctx) {
    if (PopupHost::Current() && m_isDropDownOpen) {
        return;
    }
    RenderPopup(ctx);
}

void DropDownButton::OnRender(GraphicsContext& ctx) {
    D2D1_COLOR_F bg = GetAnimatedBackground(ThemeManager::Instance().GetFlatColor(ThemeTokenId::AccentColor));
    D2D1_COLOR_F border = ResolveThemeColor(GetBorderToken(), ThemeTokenId::AccentColor);
    DrawButtonFace(ctx, bg, border, GetBorderThickness());
    DrawButtonLabel(ctx, LabelRect(), DWRITE_TEXT_ALIGNMENT_CENTER);

    const bool open = m_isDropDownOpen || m_arrowAnim.Current() > 0.5f;
    D2D1_COLOR_F chevron = IsEnabled()
        ? ResolveThemeColor(GetColorToken(), ThemeTokenId::AccentForeground)
        : ThemeManager::Instance().GetFlatColor(ThemeTokenId::TextMuted);
    ctx.DrawChevron(
        ChevronRect(),
        chevron,
        open ? GraphicsContext::ChevronDirection::Up : GraphicsContext::ChevronDirection::Down,
        1.7f);
}

} // namespace CUI
