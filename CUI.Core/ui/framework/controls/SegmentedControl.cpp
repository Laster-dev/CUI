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
    SetHeight(32.0f);
    SetWidth(280.0f);
    SetCornerRadius(6.0f);
    SetBorderThickness(1.5f);
    SetFontSize(13.0f);
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

std::vector<PropertyMeta> SegmentedControl::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "items", "选项 (Items)", "分段", "string" });
    metas.push_back({ "selectedIndex", "选中项 (SelectedIndex)", "分段", "number" });
    metas.push_back({ "fontFamily", "字体名称 (FontFamily)", "字体文本", "enum", { "微软雅黑", "Segoe UI", "Consolas", "Times New Roman" } });
    metas.push_back({ "fontSize", "字体大小 (FontSize)", "字体文本", "number" });
    return metas;
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
    UIElement::Arrange(finalRect);
    SyncPill(m_pillW.Current() < 1.0f);
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
    if (m_selectedIndex < 0) {
        return;
    }
    const Rect r = SegmentRect(m_selectedIndex);
    if (r.IsEmpty()) {
        return;
    }
    if (snap) {
        m_pillX.Reset(r.x);
        m_pillW.Reset(r.width);
    } else {
        m_pillX.SetTarget(r.x);
        m_pillW.SetTarget(r.width);
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

    if (!IsEnabled()) {
        accent.a *= 0.45f;
        selectedFill.a *= 0.45f;
        idleText.a *= 0.45f;
        accentFg.a *= 0.55f;
    }

    if (radius > 0.0f) {
        ctx.FillRoundedRect(m_bounds, radius, bg);
    } else {
        ctx.FillRect(m_bounds, bg);
    }

    const int n = static_cast<int>(m_items.size());
    if (m_hoverIndex >= 0 && m_hoverIndex != m_selectedIndex) {
        const Rect hover = SegmentRect(m_hoverIndex);
        ctx.PushRoundedClip(m_bounds, radius);
        ctx.FillRect(hover, WithAlpha(hoverBg, 0.65f));
        ctx.PopClip();
    }

    if (m_selectedIndex >= 0 && n > 0) {
        const float pillX = UIElement::AreAnimationsEnabled() ? m_pillX.Current() : SegmentRect(m_selectedIndex).x;
        const float pillW = UIElement::AreAnimationsEnabled() ? m_pillW.Current() : SegmentRect(m_selectedIndex).width;
        if (pillW > 0.5f) {
            ctx.PushRoundedClip(m_bounds, radius);
            ctx.FillRect(Rect(pillX, m_bounds.y, pillW, m_bounds.height), selectedFill);
            ctx.PopClip();
        }
    }

    const float pillLeft = UIElement::AreAnimationsEnabled() ? m_pillX.Current() : (m_selectedIndex >= 0 ? SegmentRect(m_selectedIndex).x : 0.0f);
    const float pillRight = pillLeft + (UIElement::AreAnimationsEnabled() ? m_pillW.Current() : (m_selectedIndex >= 0 ? SegmentRect(m_selectedIndex).width : 0.0f));
    for (int i = 1; i < n; ++i) {
        const Rect left = SegmentRect(i - 1);
        const float x = left.x + left.width;
        if (x > pillLeft + 1.0f && x < pillRight - 1.0f) {
            continue;
        }
        ctx.DrawLine(
            Point(x, m_bounds.y + 6.0f),
            Point(x, m_bounds.y + m_bounds.height - 6.0f),
            WithAlpha(accent, 0.55f),
            1.0f);
    }

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
