#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "SegmentedControl.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <windows.h>

namespace CUI {

namespace {
AnimationSpec PillSlideSpec() {
    AnimationSpec s;
    s.responseAt60Hz = 0.26f;
    s.epsilon = 0.02f;
    s.maxDurationSeconds = 0.22f;
    return s;
}

D2D1_COLOR_F WithAlpha(D2D1_COLOR_F c, float a) {
    c.a = a;
    return c;
}
} // namespace

SegmentedControl::SegmentedControl() {
    SelectedIndex.Initialize(*this);
    SetHeight(32.0f);
    SetWidth(280.0f);
    SetCornerRadius(6.0f);
    SetBorderThickness(1.5f);
    SetFontSize(12.0f);
    SetFontFamily("微软雅黑");
    SetPadding(Thickness(0, 0, 0, 0));
    SetBackgroundToken(ThemeTokenId::InputBackground);
    SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
    SetPressedBackgroundToken(ThemeTokenId::PressedBackground);
    SetBorderToken(ThemeTokenId::AccentColor);
    SetFocusedBorderToken(ThemeTokenId::FocusedBorder);
    SetColorToken(ThemeTokenId::AccentColor);
    SetSelectedItemBackgroundToken(ThemeTokenId::AccentColor);
    SetKeyboardNavigationMode(KeyboardNavigationMode::Contained);
}

namespace {
std::string JoinCsv(const std::vector<std::string>& items) {
    std::string out;
    for (size_t i = 0; i < items.size(); ++i) {
        if (i) out.push_back(',');
        out += items[i];
    }
    return out;
}
}

Value SegmentedControl::GetProperty(PropertyId id) const {
    switch (id) {
    case PropertyId::Items: return Value(JoinCsv(m_items));
    case PropertyId::SelectedIndex: return Value(static_cast<float>(m_selectedIndex));
    default: return Control::GetProperty(id);
    }
}

bool SegmentedControl::HasProperty(PropertyId id) const {
    return id == PropertyId::Items || id == PropertyId::SelectedIndex || Control::HasProperty(id);
}

void SegmentedControl::SetProperty(PropertyId id, const Value& val) {
    if (id == PropertyId::Items) {
        SetItems(val.AsString(""));
        return;
    }
    if (id == PropertyId::SelectedIndex) {
        SetSelectedIndex(static_cast<int>(val.AsFloat(0.0f)));
        return;
    }
    Control::SetProperty(id, val);
}

void SegmentedControl::AddItem(const std::string& item) {
    m_items.push_back(item);
    if (m_selectedIndex < 0) {
        SetSelectedIndex(0);
    }
    InvalidateMeasure();
    MarkRenderContentDirty();
}

void SegmentedControl::ClearItems() {
    m_items.clear();
    m_selectedIndex = -1;
    m_hoverIndex = -1;
    m_pressedIndex = -1;
    InvalidateMeasure();
    MarkRenderContentDirty();
}

void SegmentedControl::SetItems(const std::string& itemsCsv) {
    ClearItems();
    std::stringstream ss(itemsCsv);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (!item.empty()) {
            AddItem(item);
        }
    }
}

void SegmentedControl::SetSelectedIndex(int index) {
    if (m_items.empty()) {
        m_selectedIndex = -1;
        return;
    }
    index = std::clamp(index, 0, static_cast<int>(m_items.size()) - 1);
    const bool changed = (m_selectedIndex != index);
    m_selectedIndex = index;
    SyncPill(!changed || !UIElement::AreAnimationsEnabled());
    if (changed) {
        m_onSelectionChangedEvent.Invoke(this, m_selectedIndex, m_items[static_cast<size_t>(m_selectedIndex)]);
        NotifyFieldChanged(PropertyId::SelectedIndex, Value(static_cast<float>(m_selectedIndex)));
        RequestAnimationTicks();
        MarkRenderContentDirty();
    }
}

std::string SegmentedControl::GetSelectedItem() const {
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_items.size())) {
        return m_items[static_cast<size_t>(m_selectedIndex)];
    }
    return "";
}

float SegmentedControl::MeasureContentWidth() const {
    GraphicsContext ctx;
    float total = 8.0f;
    const std::string& font = GetFontFamily();
    const float fontSize = GetFontSize();
    for (const auto& item : m_items) {
        const Size ts = ctx.MeasureText(item, font, fontSize, DWRITE_FONT_WEIGHT_SEMI_BOLD);
        total += (std::max)(48.0f, ts.width + 28.0f);
    }
    if (m_items.empty()) {
        total = 120.0f;
    }
    return total;
}

Size SegmentedControl::Measure(Size availableSize) {
    (void)availableSize;
    const float expW = (GetWidth() >= 0.0f) ? GetWidth() : MeasureContentWidth();
    const float expH = (GetHeight() >= 0.0f) ? GetHeight() : 32.0f;
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

void SegmentedControl::Arrange(Rect finalRect) {
    bool boundsChanged = (std::abs(m_bounds.width - finalRect.width) > 0.5f
        || std::abs(m_bounds.height - finalRect.height) > 0.5f
        || std::abs(m_bounds.x - finalRect.x) > 0.5f
        || std::abs(m_bounds.y - finalRect.y) > 0.5f);
    UIElement::Arrange(finalRect);
    SyncPill(boundsChanged || m_pillW.Current() < 1.0f);
}

Rect SegmentedControl::SegmentRect(int index) const {
    const int n = static_cast<int>(m_items.size());
    if (index < 0 || index >= n || m_bounds.width <= 0.0f) {
        return Rect();
    }
    const float w = m_bounds.width / static_cast<float>(n);
    return Rect(m_bounds.x + w * static_cast<float>(index), m_bounds.y, w, m_bounds.height);
}

int SegmentedControl::HitTestIndex(Point pt) const {
    const int n = static_cast<int>(m_items.size());
    for (int i = 0; i < n; ++i) {
        if (SegmentRect(i).Contains(pt.x, pt.y)) {
            return i;
        }
    }
    return -1;
}

void SegmentedControl::SyncPill(bool snap) {
    if (m_selectedIndex < 0 || m_items.empty() || m_bounds.width <= 0.0f) {
        return;
    }
    const int n = static_cast<int>(m_items.size());
    const float w = m_bounds.width / static_cast<float>(n);
    const float targetRelX = w * static_cast<float>(m_selectedIndex);
    const float targetW = w;

    if (snap) {
        m_pillX.Reset(targetRelX);
        m_pillW.Reset(targetW);
    } else {
        m_pillX.SetTarget(targetRelX);
        m_pillW.SetTarget(targetW);
    }
}

void SegmentedControl::OnRender(GraphicsContext& ctx) {
    const float radius = (std::max)(0.0f, GetCornerRadius());
    D2D1_COLOR_F bg = GetAnimatedBackground(ThemeManager::Instance().GetFlatColor(ThemeTokenId::InputBackground));
    D2D1_COLOR_F accent = ResolveThemeColor(GetBorderToken(), ThemeTokenId::AccentColor);
    D2D1_COLOR_F accentFg = ThemeManager::Instance().GetFlatColor(ThemeTokenId::AccentForeground);
    D2D1_COLOR_F selectedFill = ResolveThemeColor(GetSelectedItemBackgroundToken(), ThemeTokenId::AccentColor);
    D2D1_COLOR_F idleText = ResolveThemeColor(GetColorToken(), ThemeTokenId::AccentColor);
    D2D1_COLOR_F hoverBg = ResolveThemeColor(GetHoverBackgroundToken(), ThemeTokenId::HoverBackground);

    // 1. Control Background
    if (radius > 0.0f) {
        ctx.FillRoundedRect(m_bounds, radius, bg);
    } else {
        ctx.FillRect(m_bounds, bg);
    }

    const int n = static_cast<int>(m_items.size());

    // 2. Hover Highlight
    if (m_hoverIndex >= 0 && m_hoverIndex != m_selectedIndex) {
        const Rect hover = SegmentRect(m_hoverIndex);
        ctx.PushRoundedClip(m_bounds, radius);
        ctx.FillRect(hover, WithAlpha(hoverBg, 0.65f));
        ctx.PopClip();
    }

    // 3. Dividers (drawn before selection pill so the selection pill cleanly covers them)
    for (int i = 1; i < n; ++i) {
        const Rect left = SegmentRect(i - 1);
        const float x = left.x + left.width;
        ctx.DrawLine(
            Point(x, m_bounds.y + 6.0f),
            Point(x, m_bounds.y + m_bounds.height - 6.0f),
            WithAlpha(accent, 0.55f),
            1.0f);
    }

    // 4. Selection Pill (drawn on top of background and dividers)
    if (m_selectedIndex >= 0 && n > 0) {
        const float segW = m_bounds.width / static_cast<float>(n);
        const float pillRelX = UIElement::AreAnimationsEnabled() ? m_pillX.Current() : (segW * static_cast<float>(m_selectedIndex));
        const float pillW = UIElement::AreAnimationsEnabled() ? m_pillW.Current() : segW;
        if (pillW > 0.5f) {
            ctx.PushRoundedClip(m_bounds, radius);
            ctx.FillRect(Rect(m_bounds.x + pillRelX, m_bounds.y, pillW + 1.0f, m_bounds.height), selectedFill);
            ctx.PopClip();
        }
    }

    // 5. Text Labels
    const std::string& font = GetFontFamily();
    const float fontSize = GetFontSize();
    for (int i = 0; i < n; ++i) {
        const Rect cell = SegmentRect(i);
        const bool selected = (i == m_selectedIndex);
        ctx.DrawText(
            m_items[static_cast<size_t>(i)],
            cell,
            selected ? accentFg : idleText,
            font,
            fontSize,
            DWRITE_TEXT_ALIGNMENT_CENTER,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
            selected ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL);
    }

    // 6. Outer Border / Focus Indicator
    const D2D1_COLOR_F outline = IsFocused()
        ? ResolveThemeColor(GetFocusedBorderToken(), ThemeTokenId::FocusedBorder)
        : accent;
    const float bt = (std::max)(1.0f, GetBorderThickness());
    if (radius > 0.0f) {
        ctx.DrawRoundedRect(m_bounds, radius, outline, IsFocused() ? 1.8f : bt);
    } else {
        ctx.DrawRect(m_bounds, outline, IsFocused() ? 1.8f : bt);
    }
}

void SegmentedControl::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);
    if (!IsEnabled()) {
        return;
    }
    m_pressedIndex = HitTestIndex(pt);
}

void SegmentedControl::OnMouseUp(Point pt) {
    Control::OnMouseUp(pt);
    if (!IsEnabled()) {
        m_pressedIndex = -1;
        return;
    }
    const int hit = HitTestIndex(pt);
    if (hit >= 0 && hit == m_pressedIndex) {
        SetSelectedIndex(hit);
    }
    m_pressedIndex = -1;
}

void SegmentedControl::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);
    const int next = HitTestIndex(pt);
    if (next != m_hoverIndex) {
        m_hoverIndex = next;
        MarkRenderContentDirty();
    }
}

void SegmentedControl::OnMouseLeave() {
    Control::OnMouseLeave();
    m_hoverIndex = -1;
    m_pressedIndex = -1;
    MarkRenderContentDirty();
}

bool SegmentedControl::OnKeyDown(int vkCode) {
    if (!IsEnabled() || m_items.empty()) {
        return Control::OnKeyDown(vkCode);
    }
    const int last = static_cast<int>(m_items.size()) - 1;
    switch (vkCode) {
    case VK_LEFT:
        SetSelectedIndex(m_selectedIndex <= 0 ? last : m_selectedIndex - 1);
        return true;
    case VK_RIGHT:
        SetSelectedIndex(m_selectedIndex >= last ? 0 : m_selectedIndex + 1);
        return true;
    case VK_HOME:
        SetSelectedIndex(0);
        return true;
    case VK_END:
        SetSelectedIndex(last);
        return true;
    default:
        return Control::OnKeyDown(vkCode);
    }
}

bool SegmentedControl::OnAnimationTick() {
    bool any = Control::OnAnimationTick();
    const float dt = UIElement::GetAnimationDeltaSeconds();
    const bool pill = m_pillX.Tick(dt, PillSlideSpec()) | m_pillW.Tick(dt, PillSlideSpec());
    if (pill) {
        MarkRenderContentDirty();
        RequestAnimationTicks();
        any = true;
    }
    return any;
}

bool SegmentedControl::HasSelfAnimation() const {
    return Control::HasSelfAnimation()
        || m_pillX.IsAnimating(0.02f)
        || m_pillW.IsAnimating(0.02f);
}

} // namespace CUI
