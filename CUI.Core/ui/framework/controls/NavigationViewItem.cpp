#include "NavigationViewItem.h"
#include "NavigationView.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cmath>

namespace CUI {

namespace {
constexpr float kItemHeight = 40.0f;
constexpr float kHeaderHeight = 28.0f;
constexpr float kSeparatorHeight = 9.0f;
constexpr float kIconSlot = 40.0f;
constexpr float kChevronSize = 12.0f;
constexpr float kChevronRightInset = 12.0f;
constexpr float kHighlightInsetX = 2.0f;

float FrameBlend(float factorAt60Hz) {
    factorAt60Hz = std::clamp(factorAt60Hz, 0.0f, 0.999f);
    float frames = UIElement::GetAnimationDeltaSeconds() * 60.0f;
    return 1.0f - std::pow(1.0f - factorAt60Hz, (std::max)(0.1f, frames));
}
}

void NavigationViewItemBase::SetIsSelected(bool selected) {
    if (m_isSelected == selected) {
        return;
    }
    m_isSelected = selected;
    // Local only — content-dirty bubbles through NavigationView and stalls ripples.
    MarkRenderRectDirty(m_bounds);
}

NavigationViewItemHeader::NavigationViewItemHeader(const std::string& text) {
    m_text = text;
    // Defaults via theme tokens only — paint path resolves through ThemeManager.
    SetColorToken(ThemeTokenId::TextSecondary);
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
    const D2D1_COLOR_F color = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextSecondary);
    const float indent = 12.0f + m_depth * 12.0f;
    Rect textRect(m_bounds.x + indent, m_bounds.y, (std::max)(0.0f, m_bounds.width - indent - 8.0f), m_bounds.height);
    ctx.PushClip(textRect);
    ctx.DrawText(m_text, textRect, color, "微软雅黑", 11.0f,
                 DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
                 DWRITE_FONT_WEIGHT_SEMI_BOLD);
    ctx.PopClip();
}

Size NavigationViewItemSeparator::Measure(Size availableSize) {
    m_desiredSize = Size((std::max)(0.0f, availableSize.width), kSeparatorHeight);
    return m_desiredSize;
}

void NavigationViewItemSeparator::OnRender(GraphicsContext& ctx) {
    // No Control chrome fill — a 9px filled strip reads as a black bar on the pane.
    if (GetBorderToken() == ThemeTokenId::Unset) {
        SetBorderToken(ThemeTokenId::CardBorder);
    }
    const D2D1_COLOR_F border = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);
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
    // Bind theme tokens only. Do not bake ColorF snapshots — ThemeManager is
    // the single color source unless user code overrides a property.
    // Hover/selected fills are drawn inset in OnRender — keep Control hover
    // tokens unset so GetAnimatedBackground cannot paint a second full-bounds layer.
    SetHoverBackgroundToken(ThemeTokenId::Unset);
    SetPressedBackgroundToken(ThemeTokenId::Unset);
    SetSelectedBackgroundToken(ThemeTokenId::SelectedBackground);
    SetColorToken(ThemeTokenId::TextPrimary);
    SetSecondaryColorToken(ThemeTokenId::TextSecondary);
    SetIndicatorColorToken(ThemeTokenId::AccentColor);
    SetBackground(D2D1::ColorF(0, 0, 0, 0));
    SetHoverBackground(D2D1::ColorF(0, 0, 0, 0));
    SetPressedBackground(D2D1::ColorF(0, 0, 0, 0));
    SetCornerRadius(6.0f);
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

void NavigationViewItem::SetIsExpandedSilent(bool expanded) {
    if (m_isExpanded == expanded) {
        return;
    }
    m_isExpanded = expanded;
    MarkRenderRectDirty(m_bounds);
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
    // Fixed inset from the item's right edge (overlay scrollbar keeps width stable).
    return Rect(m_bounds.x + m_bounds.width - kChevronSize - kChevronRightInset,
                m_bounds.y + (m_bounds.height - kChevronSize) * 0.5f,
                kChevronSize, kChevronSize);
}

bool NavigationViewItem::HitChevron(Point pt) const {
    const Rect r = GetChevronRect();
    return !r.IsEmpty() && r.Contains(pt.x, pt.y);
}

void NavigationViewItem::OnRender(GraphicsContext& ctx) {
    // Intentionally skip Control::OnRender. Control would fill m_bounds with the
    // hover/pressed animated chrome; we paint a single inset highlight below.
    const float radius = GetCornerRadius();
    const bool showSelected = m_isSelected || m_isChildSelected;
    // Inset so rounded corners aren't clipped by the pane/scroll edge.
    const Rect highlight(
        m_bounds.x + kHighlightInsetX,
        m_bounds.y,
        (std::max)(0.0f, m_bounds.width - kHighlightInsetX * 2.0f),
        m_bounds.height
    );
    D2D1_COLOR_F fill = D2D1::ColorF(0, 0, 0, 0);
    if (showSelected) {
        fill = ResolveThemeColor(GetSelectedBackgroundToken(), ThemeTokenId::SelectedBackground);
    } else if (m_hovered) {
        fill = ResolveThemeColor(GetHoverBackgroundToken(), ThemeTokenId::HoverBackground);
    }
    if (fill.a > 0.001f && !highlight.IsEmpty()) {
        ctx.FillRoundedRect(highlight, radius, fill);
    }

    // Always resolve through ThemeManager (token → GetColor). User overrides of
    // theme.colorToken are honored; never use a DIY palette here.
    const D2D1_COLOR_F textColor = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextPrimary);

    const float indent = m_topMode ? 0.0f : (m_depth * 12.0f);
    float x = m_bounds.x + indent;

    Rect iconRect(x, m_bounds.y, kIconSlot, m_bounds.height);
    if (!m_icon.empty()) {
        const float iconSize = 16.0f;
        const Rect glyph(
            iconRect.x + (iconRect.width - iconSize) * 0.5f,
            iconRect.y + (iconRect.height - iconSize) * 0.5f,
            iconSize,
            iconSize);
        ctx.DrawIcon(m_icon, glyph, textColor, 1.0f, "微软雅黑", 14.0f);
    }
    x += kIconSlot;

    if (!m_compact) {
        const Rect chevron = GetChevronRect();
        const float rightPad = chevron.IsEmpty()
            ? (8.0f + kHighlightInsetX)
            : (kChevronSize + kChevronRightInset + 4.0f);
        Rect textRect(x, m_bounds.y, (std::max)(0.0f, m_bounds.x + m_bounds.width - x - rightPad), m_bounds.height);
        ctx.PushClip(textRect);
        ctx.DrawText(m_content, textRect, textColor, "微软雅黑", 14.0f,
                     DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
                     showSelected ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL);
        ctx.PopClip();

        if (!chevron.IsEmpty()) {
            ctx.DrawChevron(
                chevron,
                textColor,
                m_isExpanded ? GraphicsContext::ChevronDirection::Down
                             : GraphicsContext::ChevronDirection::Right,
                1.5f
            );
        }
    }
}

void NavigationViewItem::OnMouseDown(Point pt) {
    // Skip Control::OnMouseDown — avoids full-bounds visual-state animation chrome.
    UIElement::OnMouseDown(pt);
    if (!IsEnabled()) {
        return;
    }

    if (HitChevron(pt)) {
        SetIsExpanded(!m_isExpanded);
        return;
    }

    if (HasChildren() && !m_selectsOnInvoked) {
        SetIsExpanded(!m_isExpanded);
    }
    // Same as Button: invoke immediately. Deferred invoke left the item mid-ripple
    // while NavigationView/ScrollViewer also registered and walked the tree.
    m_invoked.Invoke(this);
}

bool NavigationViewItem::OnAnimationTick() {
    // Do not run Control visual-state hover fade — chrome is discrete via m_hovered.
    return UIElement::OnAnimationTick();
}

bool NavigationViewItem::HasSelfAnimation() const {
    return false;
}

void NavigationViewItem::OnMouseEnter() {
    UIElement::OnMouseEnter();
    if (!m_hovered) {
        m_hovered = true;
        MarkRenderRectDirty(m_bounds.Inflate(2.0f));
    }
}

void NavigationViewItem::OnMouseLeave() {
    UIElement::OnMouseLeave();
    if (m_hovered) {
        m_hovered = false;
        MarkRenderRectDirty(m_bounds.Inflate(2.0f));
    }
}

} // namespace CUI
