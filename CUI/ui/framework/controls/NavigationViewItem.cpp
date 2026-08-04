#include "NavigationViewItem.h"
#include "NavigationView.h"
#include "../style/ThemeManager.h"
#include <algorithm>

namespace CUI {

namespace {
constexpr float kItemHeight = 40.0f;
constexpr float kHeaderHeight = 28.0f;
constexpr float kSeparatorHeight = 9.0f;
constexpr float kIconSlot = 40.0f;
constexpr float kChevronSize = 16.0f;
}

void NavigationViewItemBase::SetIsSelected(bool selected) {
    if (m_isSelected == selected) {
        return;
    }
    m_isSelected = selected;
    MarkRenderContentDirty();
}

NavigationViewItemHeader::NavigationViewItemHeader(const std::string& text) {
    m_text = text;
    SetProperty("theme.colorToken", Value("textPrimary"));
}

void NavigationViewItemHeader::SetText(const std::string& text) {
    m_text = text;
    MarkRenderContentDirty();
}

Size NavigationViewItemHeader::Measure(Size availableSize) {
    m_desiredSize = Size((std::max)(0.0f, availableSize.width), kHeaderHeight);
    return m_desiredSize;
}

void NavigationViewItemHeader::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);
    // Headers should remain readable in both themes.
    const D2D1_COLOR_F color = ResolveThemeColor("theme.colorToken", "textPrimary");
    const float indent = 12.0f + m_depth * 12.0f;
    Rect textRect(m_bounds.x + indent, m_bounds.y, (std::max)(0.0f, m_bounds.width - indent - 8.0f), m_bounds.height);
    ctx.PushClip(textRect);
    ctx.DrawText(m_text, textRect, color, "Segoe UI", 11.0f,
                 DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
                 DWRITE_FONT_WEIGHT_SEMI_BOLD);
    ctx.PopClip();
}

Size NavigationViewItemSeparator::Measure(Size availableSize) {
    m_desiredSize = Size((std::max)(0.0f, availableSize.width), kSeparatorHeight);
    return m_desiredSize;
}

void NavigationViewItemSeparator::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);
    const D2D1_COLOR_F border = ThemeManager::Instance().GetTokens().cardBorder;
    const float y = m_bounds.y + m_bounds.height * 0.5f;
    const float inset = 12.0f + m_depth * 12.0f;
    ctx.DrawLine(Point(m_bounds.x + inset, y),
                 Point(m_bounds.x + m_bounds.width - 12.0f, y),
                 border, 1.0f);
}

NavigationViewItem::NavigationViewItem() {
    StyleDefaults();
}

NavigationViewItem::NavigationViewItem(const std::string& content, const std::string& icon)
    : m_content(content), m_icon(icon) {
    StyleDefaults();
}

void NavigationViewItem::StyleDefaults() {
    auto& theme = ThemeManager::Instance();
    SetProperty("theme.hoverBackgroundToken", Value("hoverBackground"));
    SetProperty("theme.selectedBackgroundToken", Value("selectedBackground"));
    SetProperty("theme.colorToken", Value("textPrimary"));
    SetProperty("theme.secondaryColorToken", Value("textSecondary"));
    SetProperty("theme.indicatorColorToken", Value("accentColor"));
    SetProperty("background", Value(D2D1::ColorF(0, 0, 0, 0)));
    SetProperty("hoverBackground", Value(theme.GetColor("hoverBackground")));
    SetProperty("selectedBackground", Value(theme.GetColor("selectedBackground")));
    SetProperty("color", Value(theme.GetColor("textPrimary")));
    SetProperty("cornerRadius", Value(4.0f));
}

void NavigationViewItem::SetContent(const std::string& content) {
    m_content = content;
    MarkRenderContentDirty();
}

void NavigationViewItem::SetIcon(const std::string& icon) {
    m_icon = icon;
    MarkRenderContentDirty();
}

void NavigationViewItem::SetIsExpanded(bool expanded) {
    if (m_isExpanded == expanded) {
        return;
    }
    m_isExpanded = expanded;
    m_expandChanged.Invoke(this);
    MarkRenderContentDirty();
}

void NavigationViewItem::AddMenuItem(const std::shared_ptr<NavigationViewItemBase>& item) {
    if (!item) {
        return;
    }
    item->SetDepth(m_depth + 1);
    item->SetOwner(m_owner);
    m_menuItems.push_back(item);
    MarkRenderContentDirty();
}

void NavigationViewItem::SetCompact(bool compact) {
    if (m_compact == compact) {
        return;
    }
    m_compact = compact;
    MarkRenderContentDirty();
}

Size NavigationViewItem::Measure(Size availableSize) {
    const float w = m_compact ? kIconSlot : (std::max)(kIconSlot, availableSize.width);
    m_desiredSize = Size(w, kItemHeight);
    return m_desiredSize;
}

void NavigationViewItem::Arrange(Rect finalRect) {
    SetBounds(finalRect);
}

Rect NavigationViewItem::GetChevronRect() const {
    if (!HasChildren() || m_compact || m_topMode) {
        return Rect();
    }
    return Rect(m_bounds.x + m_bounds.width - kChevronSize - 8.0f,
                m_bounds.y + (m_bounds.height - kChevronSize) * 0.5f,
                kChevronSize, kChevronSize);
}

bool NavigationViewItem::HitChevron(Point pt) const {
    const Rect r = GetChevronRect();
    return !r.IsEmpty() && r.Contains(pt.x, pt.y);
}

void NavigationViewItem::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    const float radius = GetProperty("cornerRadius").AsFloat(4.0f);
    const bool showSelected = m_isSelected || m_isChildSelected;
    D2D1_COLOR_F fill = D2D1::ColorF(0, 0, 0, 0);
    if (showSelected) {
        fill = ResolveThemeColor("theme.selectedBackgroundToken", "selectedBackground");
    } else if (m_hovered) {
        fill = ResolveThemeColor("theme.hoverBackgroundToken", "hoverBackground");
    }
    if (fill.a > 0.001f) {
        ctx.FillRoundedRect(m_bounds, radius, fill);
    }

    // WinUI NavigationViewItem uses the primary text brush for labels in both
    // selected and unselected states (weight changes). Never use a washed-out
    // secondary brush here — light mode made unselected items nearly invisible.
    const D2D1_COLOR_F textColor = ResolveThemeColor("theme.colorToken", "textPrimary");

    const float indent = m_topMode ? 0.0f : (m_depth * 12.0f);
    float x = m_bounds.x + indent;

    // Note: the animated blue selection indicator (the "moving bar") is drawn by
    // NavigationView (overlay pass) so it can slide smoothly between items.

    // Icon slot (WinUI: 40 DIP).
    Rect iconRect(x, m_bounds.y, kIconSlot, m_bounds.height);
    if (!m_icon.empty()) {
        ctx.DrawText(m_icon, iconRect, textColor, "Segoe UI", 14.0f,
                     DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
    x += kIconSlot;

    if (!m_compact) {
        const Rect chevron = GetChevronRect();
        const float rightPad = chevron.IsEmpty() ? 8.0f : (kChevronSize + 16.0f);
        Rect textRect(x, m_bounds.y, (std::max)(0.0f, m_bounds.x + m_bounds.width - x - rightPad), m_bounds.height);
        ctx.PushClip(textRect);
        ctx.DrawText(m_content, textRect, textColor, "Segoe UI", 14.0f,
                     DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
                     showSelected ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL);
        ctx.PopClip();

        if (!chevron.IsEmpty()) {
            ctx.DrawText(m_isExpanded ? "▾" : "▸", chevron, textColor, "Segoe UI", 11.0f,
                         DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }
}

void NavigationViewItem::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);
    if (!IsEnabled()) {
        return;
    }

    if (HitChevron(pt)) {
        SetIsExpanded(!m_isExpanded);
        return;
    }

    // Body click: invoke (+ optionally expand when it has children).
    if (HasChildren() && !m_selectsOnInvoked) {
        SetIsExpanded(!m_isExpanded);
    }
    m_invoked.Invoke(this);
}

void NavigationViewItem::OnMouseEnter() {
    Control::OnMouseEnter();
    m_hovered = true;
    MarkRenderContentDirty();
}

void NavigationViewItem::OnMouseLeave() {
    Control::OnMouseLeave();
    m_hovered = false;
    MarkRenderContentDirty();
}

} // namespace CUI
