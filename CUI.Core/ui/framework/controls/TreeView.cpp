#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "TreeView.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cmath>

namespace CUI {

namespace {
float GetChromiumWheelStep(float viewportHeight) {
    UINT lines = 3;
    SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &lines, 0);
    if (lines == WHEEL_PAGESCROLL) {
        return (std::max)(40.0f, viewportHeight);
    }
    return (std::max)(1u, lines) * 40.0f;
}

float FrameBlend(float factorAt60Hz) {
    factorAt60Hz = std::clamp(factorAt60Hz, 0.0f, 0.999f);
    float frames = UIElement::GetAnimationDeltaSeconds() * 60.0f;
    return 1.0f - std::pow(1.0f - factorAt60Hz, (std::max)(0.1f, frames));
}

constexpr float kTreeSelectPillRadius = 4.0f;
} // namespace

TreeView::TreeView() {
    SetBackgroundToken(ThemeTokenId::CardBackground);
    SetBorderToken(ThemeTokenId::CardBorder);
    SetColorToken(ThemeTokenId::TextPrimary);
    SetSelectedBackgroundToken(ThemeTokenId::SelectedBackground);
    SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
    SetBackground(ThemeManager::Instance().GetColor("cardBackground"));
    SetBorderBrush(ThemeManager::Instance().GetColor("cardBorder"));
    SetBorderThickness(1.0f);
    SetColor(ThemeManager::Instance().GetColor("textPrimary"));
    SetHoverBackground(ThemeManager::Instance().GetColor("hoverBackground"));
    SetFontSize(16.0f);
    SetFontFamily("微软雅黑");
    SetKeyboardNavigationMode(KeyboardNavigationMode::Contained);
    SetFontWeight("Normal");
    SetItemHeight(28.0f);
    SetCornerRadius(4.0f);
    // Let parent panes stretch the tree; fixed 260px left a blank strip beside it.
    SetWidth(-1.0f);
    SetHeight(-1.0f);
    m_scrollAnimator.Reset(0.0f);
}

std::vector<PropertyMeta> TreeView::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "fontFamily", "字体名称 (FontFamily)", "字体文本", "enum", { "微软雅黑", "Segoe UI", "Consolas", "Times New Roman" } });
    metas.push_back({ "fontSize", "字体大小 (FontSize)", "字体文本", "number" });
    metas.push_back({ "itemHeight", "节点行高 (ItemHeight)", "树形配置", "number" });
    metas.push_back({ "indentWidth", "层级缩进 (IndentWidth)", "树形配置", "number" });
    return metas;
}

HCURSOR TreeView::GetCursor() const {
    return IsEnabled() ? LoadCursor(nullptr, IDC_ARROW) : nullptr;
}

void TreeView::SetParentRecursive(const std::shared_ptr<TreeViewItem>& item, TreeViewItem* parent) {
    if (!item) return;
    item->parent = parent;
    for (auto& child : item->children) {
        SetParentRecursive(child, item.get());
    }
}

void TreeView::ClearItems() {
    m_items.clear();
    m_selectedItem = nullptr;
    m_visibleItems.clear();
    m_visibleDirty = true;
    m_scrollY = 0.0f;
    m_scrollAnimator.Reset(0.0f);
    m_hoveredVisibleIndex = -1;
    m_pressedVisibleIndex = -1;
}

void TreeView::AddItem(std::shared_ptr<TreeViewItem> item) {
    if (item) {
        SetParentRecursive(item, nullptr);
        m_items.push_back(item);
        m_visibleDirty = true;
    }
}

std::shared_ptr<TreeViewItem> TreeView::AddItem(const std::string& header, bool expanded) {
    auto item = std::make_shared<TreeViewItem>();
    item->header = header;
    item->isExpanded = expanded;
    item->expandAnim.Reset(expanded ? 1.0f : 0.0f);
    AddItem(item);
    return item;
}

void TreeView::SetItems(const std::vector<std::shared_ptr<TreeViewItem>>& items) {
    m_items = items;
    for (auto& item : m_items) {
        SetParentRecursive(item, nullptr);
    }
    m_selectedItem = nullptr;
    m_visibleDirty = true;
    m_scrollY = 0.0f;
    m_scrollAnimator.Reset(0.0f);
    m_hoveredVisibleIndex = -1;
    m_pressedVisibleIndex = -1;
}

void TreeView::RebuildVisibleItems() const {
    if (!m_visibleDirty) return;
    m_visibleItems.clear();

    const float baseH = GetItemHeight();
    const float viewportH = (std::max)(0.0f, m_bounds.height - 4.0f);
    // Overscan keeps keyboard/hit-testing comfortable while scrolling mid-animation.
    const float overscan = (std::max)(viewportH, baseH * 40.0f);
    const float viewTop = m_scrollY - overscan;
    const float viewBottom = m_scrollY + viewportH + overscan;

    // Content height is computed independently so we never materialize tens of
    // thousands of VisibleItems just to measure the scrollbar.
    m_cachedContentHeight = ComputeContentHeight(m_items, 1.0f);

    auto collect = [&](auto self, const std::vector<std::shared_ptr<TreeViewItem>>& list,
                       int depth, float parentClip, float& y) -> void {
        for (const auto& item : list) {
            if (!item) continue;
            const float rowH = baseH * parentClip;
            const float rowTop = y;
            const float rowBottom = y + rowH;
            y += rowH;

            if (rowH > 0.05f && rowBottom >= viewTop && rowTop <= viewBottom) {
                m_visibleItems.push_back({ item, depth, parentClip, rowTop });
            }

            const float selfAnim = std::clamp(item->expandAnim.Current(), 0.0f, 1.0f);
            const float effectiveClip = parentClip * selfAnim;
            const bool showChildren = item->isExpanded || selfAnim > 0.001f;
            if (showChildren && !item->children.empty() && effectiveClip > 0.001f) {
                // Fast path: flat collapsed children can advance y without recursion
                // visiting each node when they're all far below/above the viewport.
                bool anyOpenChild = false;
                for (const auto& child : item->children) {
                    if (!child) continue;
                    if (child->isExpanded || child->expandAnim.Current() > 0.001f) {
                        anyOpenChild = true;
                        break;
                    }
                }
                if (!anyOpenChild) {
                    const float childH = baseH * effectiveClip;
                    const float childBlock = static_cast<float>(item->children.size()) * childH;
                    const float blockTop = y;
                    const float blockBottom = y + childBlock;
                    if (childH > 0.05f && blockBottom >= viewTop && blockTop <= viewBottom) {
                        const int n = static_cast<int>(item->children.size());
                        int startIdx = static_cast<int>(std::floor((viewTop - y) / childH));
                        int endIdx = static_cast<int>(std::ceil((viewBottom - y) / childH)) + 1;
                        startIdx = std::clamp(startIdx, 0, n);
                        endIdx = std::clamp(endIdx, 0, n);
                        for (int i = startIdx; i < endIdx; ++i) {
                            const auto& child = item->children[static_cast<size_t>(i)];
                            if (!child) continue;
                            const float cy = y + static_cast<float>(i) * childH;
                            m_visibleItems.push_back({ child, depth + 1, effectiveClip, cy });
                        }
                    }
                    y += childBlock;
                } else {
                    self(self, item->children, depth + 1, effectiveClip, y);
                }
            }
        }
    };

    float y = 0.0f;
    collect(collect, m_items, 0, 1.0f, y);
    m_visibleDirty = false;
}

float TreeView::ComputeContentHeight(
    const std::vector<std::shared_ptr<TreeViewItem>>& list,
    float parentClip) const {
    if (parentClip <= 0.001f || list.empty()) return 0.0f;
    const float baseH = GetItemHeight();
    float total = 0.0f;
    for (const auto& item : list) {
        if (!item) continue;
        total += baseH * parentClip;
        const float selfAnim = std::clamp(item->expandAnim.Current(), 0.0f, 1.0f);
        const float effectiveClip = parentClip * selfAnim;
        if ((item->isExpanded || selfAnim > 0.001f) && !item->children.empty() && effectiveClip > 0.001f) {
            bool anyOpenChild = false;
            for (const auto& child : item->children) {
                if (!child) continue;
                if (child->isExpanded || child->expandAnim.Current() > 0.001f) {
                    anyOpenChild = true;
                    break;
                }
            }
            if (!anyOpenChild) {
                total += static_cast<float>(item->children.size()) * baseH * effectiveClip;
            } else {
                total += ComputeContentHeight(item->children, effectiveClip);
            }
        }
    }
    return total;
}

float TreeView::GetTotalContentHeight() const {
    if (m_visibleDirty) {
        RebuildVisibleItems();
    }
    return m_cachedContentHeight;
}

float TreeView::GetViewportHeight() const {
    return (std::max)(0.0f, m_bounds.height - 4.0f);
}

float TreeView::GetMaxScroll() const {
    return (std::max)(0.0f, GetTotalContentHeight() - GetViewportHeight());
}

void TreeView::ClampScroll() {
    float maxScroll = GetMaxScroll();
    m_scrollY = std::clamp(m_scrollY, 0.0f, maxScroll);
    m_scrollAnimator.ClampTo(0.0f, maxScroll);
    if (maxScroll > 0.0f) {
        m_scrollbarAutoHide.NotifyActivity(this);
    }
}

void TreeView::StopSmoothScroll() {
    m_scrollAnimator.JumpTo(m_scrollY);
}

bool TreeView::AdvanceSmoothScroll() {
    if (m_isDraggingThumb) {
        StopSmoothScroll();
        return false;
    }

    const float maxScroll = GetMaxScroll();
    if (!UIElement::AreAnimationsEnabled()) {
        if (m_scrollAnimator.IsActive()) {
            m_scrollY = std::clamp(m_scrollAnimator.Target(), 0.0f, maxScroll);
            m_scrollAnimator.JumpTo(m_scrollY);
            m_visibleDirty = true;
            return true;
        }
        return false;
    }

    const float dt = UIElement::GetAnimationDeltaSeconds();
    const float previous = m_scrollY;
    if (!m_scrollAnimator.Tick(static_cast<double>(dt), 0.0f, maxScroll)) {
        return false;
    }

    m_scrollY = m_scrollAnimator.Current();
    if (std::abs(previous - m_scrollY) > 0.01f) {
        m_visibleDirty = true;
        m_scrollbarAutoHide.NotifyActivity(this);
        MarkRenderRectDirty(m_bounds);
    }
    return true;
}

Rect TreeView::GetScrollbarTrackRect() const {
    float trackX = m_bounds.x + m_bounds.width - kScrollbarInset - kScrollbarWidth;
    return Rect(trackX, m_bounds.y + 2.0f, kScrollbarWidth, (std::max)(0.0f, m_bounds.height - 4.0f));
}

Rect TreeView::GetScrollbarThumbRect() const {
    Rect track = GetScrollbarTrackRect();
    float contentH = GetTotalContentHeight();
    float viewportH = GetViewportHeight();
    float maxScroll = GetMaxScroll();
    if (maxScroll <= 0.0f || viewportH <= 0.0f || contentH <= 0.0f || track.height <= 0.0f) {
        return Rect(track.x, track.y, track.width, 0.0f);
    }

    float thumbH = (viewportH / contentH) * track.height;
    if (thumbH < 24.0f) thumbH = 24.0f;
    if (thumbH > track.height) thumbH = track.height;

    float scrollRatio = (maxScroll > 0.0f) ? (m_scrollY / maxScroll) : 0.0f;
    float thumbY = track.y + scrollRatio * (track.height - thumbH);
    return Rect(track.x, thumbY, track.width, thumbH);
}

int TreeView::GetVisibleIndexFromY(float y) const {
    RebuildVisibleItems();
    if (m_visibleItems.empty() || y < m_bounds.y + 2.0f || y > m_bounds.y + m_bounds.height - 2.0f) {
        return -1;
    }

    const float baseH = GetItemHeight();
    for (size_t i = 0; i < m_visibleItems.size(); ++i) {
        const float rowH = baseH * m_visibleItems[i].clipFactor;
        if (rowH <= 0.5f) continue;
        const float currY = m_bounds.y + 2.0f + m_visibleItems[i].contentY - m_scrollY;
        if (y >= currY && y < currY + rowH) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int TreeView::GetVisibleIndexOfItem(TreeViewItem* item) const {
    if (!item) return -1;
    RebuildVisibleItems();
    for (size_t i = 0; i < m_visibleItems.size(); ++i) {
        if (m_visibleItems[i].item.get() == item) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

Rect TreeView::GetItemRect(int visibleIndex) const {
    if (visibleIndex < 0) return Rect(0, 0, 0, 0);
    RebuildVisibleItems();
    if (visibleIndex >= static_cast<int>(m_visibleItems.size())) return Rect(0, 0, 0, 0);

    const auto& vis = m_visibleItems[visibleIndex];
    const float baseH = GetItemHeight();
    const float rowH = baseH * vis.clipFactor;
    const float currY = m_bounds.y + 2.0f + vis.contentY - m_scrollY;
    return Rect(m_bounds.x + 2.0f, currY, (std::max)(0.0f, m_bounds.width - 4.0f), rowH);
}

Rect TreeView::GetToggleRect(const VisibleItem& visibleItem, const Rect& rowRect) const {
    float indentW = GetIndentWidth();
    float toggleX = rowRect.x + visibleItem.depth * indentW + 4.0f;
    float toggleY = rowRect.y + (GetItemHeight() - 14.0f) * 0.5f;
    return Rect(toggleX, toggleY, 14.0f, 14.0f);
}

// Full-height chevron column — easier to hit than the 14×14 glyph.
Rect TreeView::GetToggleHitRect(const VisibleItem& visibleItem, const Rect& rowRect) const {
    float indentW = GetIndentWidth();
    float toggleX = rowRect.x + visibleItem.depth * indentW;
    return Rect(toggleX, rowRect.y, (std::max)(22.0f, indentW + 6.0f), rowRect.height);
}

void TreeView::ToggleItem(std::shared_ptr<TreeViewItem> item) {
    if (!item || item->children.empty()) return;
    item->isExpanded = !item->isExpanded;
    item->expandAnim.SetTarget(item->isExpanded ? 1.0f : 0.0f);
    if (!UIElement::AreAnimationsEnabled()) {
        item->expandAnim.Reset(item->isExpanded ? 1.0f : 0.0f);
        m_expandAnimActive = false;
    } else {
        m_expandAnimActive = true;
        RequestAnimationTicks();
        // Popup-hosted trees can miss the animation pump; snap so children aren't
        // stuck at expandAnim==0 (zero height) after a successful toggle.
        if (!IsAnimationTicksRegistered()) {
            item->expandAnim.Reset(item->isExpanded ? 1.0f : 0.0f);
            m_expandAnimActive = false;
        }
    }
    m_visibleDirty = true;
    ClampScroll();
    MarkRenderContentDirty();
    m_onItemToggledEvent.Invoke(this, item);
}

void TreeView::SetItemExpanded(std::shared_ptr<TreeViewItem> item, bool expanded) {
    if (!item || item->children.empty()) {
        return;
    }
    if (item->isExpanded == expanded) {
        return;
    }
    ToggleItem(item);
}

void TreeView::ToggleExpanded(std::shared_ptr<TreeViewItem> item) {
    ToggleItem(item);
}

void TreeView::SetSelectedItem(std::shared_ptr<TreeViewItem> item) {
    if (m_selectedItem != item) {
        if (m_selectedItem) m_selectedItem->isSelected = false;
        m_selectedItem = item;
        if (m_selectedItem) m_selectedItem->isSelected = true;

        // Auto expand ancestors so selected item remains visible
        if (m_selectedItem) {
            TreeViewItem* p = m_selectedItem->parent;
            while (p) {
                p->isExpanded = true;
                p->expandAnim.Reset(1.0f);
                p = p->parent;
            }
            m_visibleDirty = true;
        }

        ClampScroll();
        m_onSelectionChangedEvent.Invoke(this, m_selectedItem);
    }
}

Size TreeView::Measure(Size availableSize) {
    float expW = GetWidth();
    float expH = GetHeight();
    if (expW < 0.0f) {
        expW = (availableSize.width > 0.0f && availableSize.width < 1.0e6f)
                   ? availableSize.width
                   : 260.0f;
    }
    if (expH < 0.0f) {
        expH = (availableSize.height > 0.0f && availableSize.height < 1.0e6f)
                   ? availableSize.height
                   : 340.0f;
    }
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

UIElement* TreeView::HitTest(float x, float y) {
    if (GetVisibility() != Visibility::Visible) return nullptr;
    if (m_bounds.Contains(x, y)) {
        return this;
    }
    return nullptr;
}

void TreeView::Render(GraphicsContext& ctx) {
    if (GetVisibility() != Visibility::Visible) return;

    ctx.PushClip(m_bounds);
    OnRender(ctx);
    ctx.PopClip();
}

void TreeView::OnRender(GraphicsContext& ctx) {
    // Draw TreeView Container Background & Border
    float radius = GetCornerRadius();
    D2D1_COLOR_F bg = ResolveThemeColor(GetBackgroundToken(), ThemeTokenId::CardBackground);
    D2D1_COLOR_F border = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);
    float borderThick = GetBorderThickness();

    ctx.FillRoundedRect(m_bounds, radius, bg);
    ctx.DrawRoundedRect(m_bounds, radius, border, borderThick);

    RebuildVisibleItems();

    float baseH = GetItemHeight();
    float indentW = GetIndentWidth();
    std::string fontFamily = GetFontFamily();
    float fontSize = GetFontSize();
    D2D1_COLOR_F textColor = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextPrimary);
    D2D1_COLOR_F selBg = ResolveThemeColor(GetSelectedBackgroundToken(), ThemeTokenId::SelectedBackground);
    D2D1_COLOR_F hoverBg = ResolveThemeColor(GetHoverBackgroundToken(), ThemeTokenId::HoverBackground);
    D2D1_COLOR_F accentColor = ThemeManager::Instance().GetColor(ThemeTokenId::AccentColor);

    for (size_t i = 0; i < m_visibleItems.size(); ++i) {
        const auto& visItem = m_visibleItems[i];
        const auto& item = visItem.item;
        float rowH = baseH * visItem.clipFactor;

        const float rowScreenY = m_bounds.y + 2.0f + visItem.contentY - m_scrollY;
        Rect rowRect(m_bounds.x + 2.0f, rowScreenY, (std::max)(0.0f, m_bounds.width - 4.0f), rowH);

        // Skip rows below 0.5px height or outside viewport clip
        if (rowH <= 0.5f || rowRect.y + rowRect.height < m_bounds.y || rowRect.y > m_bounds.y + m_bounds.height) {
            continue;
        }

        // Push clip for expanding/collapsing height clip
        ctx.PushClip(rowRect);

        bool isSelected = (m_selectedItem == item);
        bool isHovered = (m_hoveredVisibleIndex == static_cast<int>(i));

        // Full item standard cell rect
        Rect drawCellRect(rowRect.x, rowRect.y, rowRect.width, baseH);

        // WinUI Selection Pill Indicator & Row Highlight
        auto rippleItem = m_selectRippleItem.lock();
        const bool rippleHere = m_selectRippleActive && rippleItem && rippleItem.get() == item.get();
        const bool rippleCovering = rippleHere && !m_selectRippleCovered;

        if (isSelected) {
            if (rippleCovering) {
                ctx.PushRoundedClip(drawCellRect, kTreeSelectPillRadius);
                const float cx = drawCellRect.x + m_selectRippleLocalX;
                const float cy = drawCellRect.y + m_selectRippleLocalY;
                Rect rippleRect(
                    cx - m_selectRippleRadius,
                    cy - m_selectRippleRadius,
                    m_selectRippleRadius * 2.0f,
                    m_selectRippleRadius * 2.0f);
                ctx.FillRoundedRect(rippleRect, m_selectRippleRadius, selBg);
                ctx.PopClip();
            } else {
                ctx.FillRoundedRect(drawCellRect, kTreeSelectPillRadius, selBg);
            }
            // Draw WinUI 3 left vertical pill indicator
            Rect pillRect(drawCellRect.x + 2.0f, drawCellRect.y + (baseH - 16.0f) * 0.5f, 3.0f, 16.0f);
            ctx.FillRoundedRect(pillRect, 1.5f, accentColor);
        } else if (isHovered) {
            ctx.FillRoundedRect(drawCellRect, kTreeSelectPillRadius, hoverBg);
        }

        float contentX = drawCellRect.x + visItem.depth * indentW + 4.0f;
        float rowAlpha = std::clamp(visItem.clipFactor, 0.0f, 1.0f);

        // WinUI Chevron Icon & Continuous Angle Interpolation
        if (!item->children.empty()) {
            Rect toggleRect = GetToggleRect(visItem, drawCellRect);
            const float animProgress = std::clamp(item->expandAnim.Current(), 0.0f, 1.0f);

            D2D1_COLOR_F arrowColor = ThemeManager::Instance().GetColor(ThemeTokenId::TextMuted);
            arrowColor.a *= rowAlpha;

            // Interpolate Chevron matrix transform or smooth step transition
            if (animProgress > 0.001f && animProgress < 0.999f) {
                // Perform rotation transform around center of toggle rect for smooth WinUI chevron motion
                D2D1_POINT_2F center = D2D1::Point2F(toggleRect.x + toggleRect.width * 0.5f, toggleRect.y + toggleRect.height * 0.5f);
                float angleDegrees = animProgress * 90.0f; // Rotate from Right (0deg) to Down (90deg)
                D2D1_MATRIX_3X2_F rotMatrix = D2D1::Matrix3x2F::Rotation(angleDegrees, center);

                ctx.PushTransform(rotMatrix);
                ctx.DrawChevron(
                    toggleRect,
                    arrowColor,
                    GraphicsContext::ChevronDirection::Right,
                    1.6f
                );
                ctx.PopTransform();
            } else {
                ctx.DrawChevron(
                    toggleRect,
                    arrowColor,
                    animProgress >= 0.5f ? GraphicsContext::ChevronDirection::Down
                                        : GraphicsContext::ChevronDirection::Right,
                    1.6f
                );
            }
        }
        contentX += 16.0f;

        // Render Icon (native HICON preferred, else text/emoji glyph)
        Rect iconRect(contentX, drawCellRect.y + (baseH - 16.0f) * 0.5f, 16.0f, 16.0f);
        if (item->nativeIcon) {
            ctx.DrawHIcon(item->nativeIcon, iconRect, rowAlpha);
        } else {
            std::string iconText = item->icon;
            if (iconText.empty()) {
                iconText = !item->children.empty() ? (item->expandAnim.Current() > 0.5f ? "📂" : "📁") : "📄";
            }
            Rect glyphRect(contentX, drawCellRect.y + (baseH - 16.0f) * 0.5f, 16.0f, 16.0f);
            ctx.DrawIcon(iconText, glyphRect, accentColor, rowAlpha, "Segoe UI Emoji", 11.0f);
        }
        contentX += 20.0f;

        // Render Header Text with opacity fade during expansion
        float textW = (std::max)(0.0f, drawCellRect.x + drawCellRect.width - contentX - 4.0f);
        Rect textRect(contentX, drawCellRect.y, textW, baseH);
        D2D1_COLOR_F rowText = isSelected ? ThemeManager::Instance().GetColor(ThemeTokenId::TextPrimary) : textColor;
        rowText.a *= rowAlpha;
        ctx.DrawText(item->header, textRect, rowText, fontFamily, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
                     isSelected ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL, true);

        ctx.PopClip();
    }

    // Overlay scrollbar (ScrollViewer-style track + thumb)
    float maxScroll = GetMaxScroll();
    if (maxScroll > 0.0f && m_scrollbarAutoHide.IsDrawn()) {
        Rect track = GetScrollbarTrackRect();
        Rect thumb = GetScrollbarThumbRect();
        const float vis = m_scrollbarAutoHide.Opacity();
        D2D1_COLOR_F muted = ThemeManager::Instance().GetColor(ThemeTokenId::TextMuted);
        float trackAlpha = (m_scrollbarHovered || m_isDraggingThumb ? 0.18f : 0.08f) * vis;
        ctx.FillRoundedRect(track, 4.0f, D2D1::ColorF(border.r, border.g, border.b, trackAlpha));
        float thumbAlpha = (m_isDraggingThumb ? 0.75f : (m_scrollbarHovered ? 0.55f : 0.40f)) * vis;
        ctx.FillRoundedRect(thumb, 4.0f, D2D1::ColorF(muted.r, muted.g, muted.b, thumbAlpha));
    }
}

void TreeView::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);
    m_isMouseDown = true;

    float maxScroll = GetMaxScroll();
    if (maxScroll > 0.0f) {
        Rect track = GetScrollbarTrackRect();
        Rect thumb = GetScrollbarThumbRect();
        if (thumb.Contains(pt.x, pt.y) || track.Contains(pt.x, pt.y)) {
            m_scrollbarAutoHide.NotifyActivity(this);
            RequestAnimationTicks();
        }
        if (thumb.Contains(pt.x, pt.y)) {
            StopSmoothScroll();
            m_isDraggingThumb = true;
            m_scrollbarAutoHide.SetDragging(true, this);
            m_dragStartY = pt.y;
            m_dragStartScrollY = m_scrollY;
            return;
        }
        if (track.Contains(pt.x, pt.y)) {
            StopSmoothScroll();
            float trackH = track.height;
            float thumbH = thumb.height;
            float clickRatio = (pt.y - track.y - thumbH * 0.5f) / (std::max)(1.0f, trackH - thumbH);
            clickRatio = std::clamp(clickRatio, 0.0f, 1.0f);
            m_scrollY = clickRatio * maxScroll;
            ClampScroll();
            m_scrollAnimator.JumpTo(m_scrollY);
            m_visibleDirty = true;
            MarkRenderContentDirty();
            return;
        }
    }

    int idx = GetVisibleIndexFromY(pt.y);
    if (idx >= 0 && idx < static_cast<int>(m_visibleItems.size())) {
        // Copy before SetSelectedItem/ClampScroll rebuilds m_visibleItems
        // (a reference into the vector would dangle and crash on item->).
        const VisibleItem visItem = m_visibleItems[idx];
        std::shared_ptr<TreeViewItem> item = visItem.item;
        if (!item) {
            return;
        }

        Rect rowRect = GetItemRect(idx);
        const bool canExpand = !item->children.empty();

        // Chevron column — toggle expand/collapse.
        if (canExpand) {
            Rect toggleHit = GetToggleHitRect(visItem, rowRect);
            if (toggleHit.Contains(pt.x, pt.y)) {
                ToggleItem(item);
                return;
            }
        }

        // Row click: select; collapsed branches also expand (Explorer / WinUI-like).
        m_pressedVisibleIndex = idx;
        SetSelectedItem(item);
        StartSelectRipple(item, pt);
        if (canExpand && !item->isExpanded) {
            ToggleItem(item);
        }
        MarkRenderContentDirty();
    }
}

void TreeView::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);
    int oldHover = m_hoveredVisibleIndex;
    m_hoveredVisibleIndex = GetVisibleIndexFromY(pt.y);

    bool wasBarHover = m_scrollbarHovered;
    m_scrollbarHovered = GetMaxScroll() > 0.0f && GetScrollbarTrackRect().Contains(pt.x, pt.y);
    m_scrollbarAutoHide.SetPointerOver(m_scrollbarHovered, this);
    if (wasBarHover != m_scrollbarHovered) {
        RequestAnimationTicks();
        MarkRenderRectDirty(GetScrollbarTrackRect().Inflate(2.0f));
    }

    if (m_isDraggingThumb && m_isMouseDown) {
        float maxScroll = GetMaxScroll();
        Rect track = GetScrollbarTrackRect();
        Rect thumb = GetScrollbarThumbRect();
        float scrollableTrack = (std::max)(1.0f, track.height - thumb.height);
        float deltaY = pt.y - m_dragStartY;
        m_scrollY = m_dragStartScrollY + (deltaY / scrollableTrack) * maxScroll;
        ClampScroll();
        m_scrollAnimator.JumpTo(m_scrollY);
        m_visibleDirty = true;
        m_scrollbarAutoHide.NotifyActivity(this);
        MarkRenderContentDirty();
        return;
    }

    if (oldHover != m_hoveredVisibleIndex) {
        MarkRenderContentDirty();
    }
}

void TreeView::OnMouseUp(Point pt) {
    Control::OnMouseUp(pt);
    m_isMouseDown = false;
    m_pressedVisibleIndex = -1;
    if (m_isDraggingThumb) {
        MarkRenderRectDirty(GetScrollbarTrackRect().Inflate(2.0f));
        RequestAnimationTicks();
    }
    m_isDraggingThumb = false;
    m_scrollbarAutoHide.SetDragging(false, this);
}

void TreeView::OnMouseDblClick(Point pt) {
    Control::OnMouseDblClick(pt);
    RebuildVisibleItems();
    const int idx = GetVisibleIndexFromY(pt.y);
    if (idx < 0 || idx >= static_cast<int>(m_visibleItems.size())) {
        return;
    }
    std::shared_ptr<TreeViewItem> item = m_visibleItems[static_cast<size_t>(idx)].item;
    if (item) {
        m_onItemDoubleClickedEvent.Invoke(this, item);
    }
}

void TreeView::OnMouseLeave() {
    Control::OnMouseLeave();
    if (m_scrollbarHovered) {
        m_scrollbarHovered = false;
        MarkRenderRectDirty(GetScrollbarTrackRect().Inflate(2.0f));
    }
    m_scrollbarAutoHide.SetPointerOver(false, this);
    RequestAnimationTicks();
}

void TreeView::OnMouseRightClick(Point pt) {
    Control::OnMouseRightClick(pt);
    int idx = GetVisibleIndexFromY(pt.y);
    if (idx >= 0 && idx < static_cast<int>(m_visibleItems.size())) {
        SetSelectedItem(m_visibleItems[idx].item);
        StartSelectRipple(m_visibleItems[idx].item, pt);
        MarkRenderContentDirty();
    }
}

void TreeView::OnMouseWheel(float delta) {
    RebuildVisibleItems();
    float maxScroll = GetMaxScroll();
    if (maxScroll <= 0.0f) {
        UIElement::OnMouseWheel(delta);
        return;
    }

    const float step = GetChromiumWheelStep(GetViewportHeight());
    const float prevTarget = m_scrollAnimator.IsActive() ? m_scrollAnimator.Target() : m_scrollY;

    if (!UIElement::AreAnimationsEnabled()) {
        m_scrollY = std::clamp(m_scrollY - delta * step, 0.0f, maxScroll);
        m_scrollAnimator.JumpTo(m_scrollY);
        m_visibleDirty = true;
    } else {
        m_scrollAnimator.ScrollBy(-delta * step, 0.0f, maxScroll);
        RequestAnimationTicks();
    }

    const float newTarget = m_scrollAnimator.IsActive() ? m_scrollAnimator.Target() : m_scrollY;
    if (std::abs(newTarget - prevTarget) < 0.001f && !m_scrollAnimator.IsActive()) {
        // At scroll edge — bubble so outer page can still scroll.
        UIElement::OnMouseWheel(delta);
        return;
    }

    m_scrollbarAutoHide.NotifyActivity(this);
    MarkRenderContentDirty();
}

std::shared_ptr<TreeViewItem> TreeView::FindFirstVisibleSelectable(int startIndex, int direction) const {
    RebuildVisibleItems();
    if (m_visibleItems.empty()) return nullptr;

    int idx = startIndex + direction;
    if (idx < 0) idx = 0;
    if (idx >= static_cast<int>(m_visibleItems.size())) idx = static_cast<int>(m_visibleItems.size()) - 1;

    return m_visibleItems[idx].item;
}

bool TreeView::OnKeyDown(int vkCode) {
    RebuildVisibleItems();
    if (m_visibleItems.empty()) return false;

    int currIdx = GetVisibleIndexOfItem(m_selectedItem.get());
    auto startRippleAtSelection = [this]() {
        if (!m_selectedItem) return;
        int idx = GetVisibleIndexOfItem(m_selectedItem.get());
        if (idx < 0) return;
        Rect row = GetItemRect(idx);
        StartSelectRipple(m_selectedItem, Point(row.x + row.width * 0.5f, row.y + row.height * 0.5f));
        MarkRenderContentDirty();
    };

    switch (vkCode) {
    case VK_UP: {
        int nextIdx = (currIdx > 0) ? currIdx - 1 : 0;
        SetSelectedItem(m_visibleItems[nextIdx].item);
        startRippleAtSelection();
        break;
    }
    case VK_DOWN: {
        int nextIdx = (currIdx < static_cast<int>(m_visibleItems.size()) - 1) ? currIdx + 1 : static_cast<int>(m_visibleItems.size()) - 1;
        SetSelectedItem(m_visibleItems[nextIdx].item);
        startRippleAtSelection();
        break;
    }
    case VK_RIGHT: {
        if (m_selectedItem) {
            if (!m_selectedItem->children.empty()) {
                if (!m_selectedItem->isExpanded) {
                    ToggleItem(m_selectedItem);
                } else if (!m_selectedItem->children.empty()) {
                    SetSelectedItem(m_selectedItem->children[0]);
                    startRippleAtSelection();
                }
            }
        }
        break;
    }
    case VK_LEFT: {
        if (m_selectedItem) {
            if (!m_selectedItem->children.empty() && m_selectedItem->isExpanded) {
                ToggleItem(m_selectedItem);
            } else if (m_selectedItem->parent) {
                for (const auto& vis : m_visibleItems) {
                    if (vis.item.get() == m_selectedItem->parent) {
                        SetSelectedItem(vis.item);
                        startRippleAtSelection();
                        break;
                    }
                }
            }
        }
        break;
    }
    default:
        return false;
    }
    return true;
}

bool TreeView::TickExpandAnims(const std::vector<std::shared_ptr<TreeViewItem>>& list, float dt) {
    bool any = false;
    for (const auto& item : list) {
        if (!item) continue;
        item->expandAnim.SetTarget(item->isExpanded ? 1.0f : 0.0f);
        // Fixed 100ms expand/collapse for every node — duration does NOT scale
        // with child count. Virtualization keeps large trees cheap to paint.
        constexpr AnimationSpec kExpandSpec{ 0.35f, 0.001f, 0.10f };
        if (item->expandAnim.Tick(dt, kExpandSpec)) {
            any = true;
        }
        // While this node itself is animating, its children are almost never
        // animating too — skip the O(N) child scan (critical for HKCR-sized keys).
        if (item->expandAnim.IsAnimating(0.001f)) {
            continue;
        }
        if (item->children.empty()) continue;
        if (!(item->isExpanded || item->expandAnim.Current() > 0.001f)) continue;
        for (const auto& child : item->children) {
            if (!child) continue;
            if (child->isExpanded || child->expandAnim.IsAnimating(0.001f) || child->expandAnim.Current() > 0.001f) {
                if (TickExpandAnims({ child }, dt)) any = true;
            }
        }
    }
    return any;
}

bool TreeView::OnAnimationTick() {
    bool base = Control::OnAnimationTick();
    const float dt = UIElement::GetAnimationDeltaSeconds();
    bool scrolling = AdvanceSmoothScroll();
    bool moving = false;
    if (UIElement::AreAnimationsEnabled()) {
        moving = TickExpandAnims(m_items, dt);
        m_expandAnimActive = moving;
        if (moving) {
            m_visibleDirty = true;
            ClampScroll();
            MarkRenderRectDirty(m_bounds);
        }
    } else {
        m_expandAnimActive = false;
        m_selectRippleActive = false;
        m_selectRippleCovered = true;
    }

    bool rippleAnim = false;
    if (m_selectRippleActive) {
        auto item = m_selectRippleItem.lock();
        int idx = item ? GetVisibleIndexOfItem(item.get()) : -1;
        if (!item || idx < 0) {
            m_selectRippleActive = false;
            m_selectRippleCovered = true;
        } else {
            Rect pill = GetItemRect(idx);
            const float cx = pill.x + m_selectRippleLocalX;
            const float cy = pill.y + m_selectRippleLocalY;
            const float corners[4][2] = {
                { pill.x, pill.y },
                { pill.x + pill.width, pill.y },
                { pill.x, pill.y + pill.height },
                { pill.x + pill.width, pill.y + pill.height },
            };
            float maxRadius = 0.0f;
            for (const auto& c : corners) {
                const float dx = cx - c[0];
                const float dy = cy - c[1];
                maxRadius = (std::max)(maxRadius, std::sqrt(dx * dx + dy * dy));
            }
            // ~2x Button speed (same as ListView selection reveal).
            m_selectRippleRadius += (maxRadius - m_selectRippleRadius) * FrameBlend(0.146f)
                + 74.0f * dt;
            if (m_selectRippleRadius >= maxRadius - 0.5f) {
                m_selectRippleRadius = maxRadius;
                m_selectRippleCovered = true;
                m_selectRippleActive = false;
            }
            MarkRenderContentDirty();
            rippleAnim = true;
        }
    }

    const float prevOpacity = m_scrollbarAutoHide.Opacity();
    const bool hideAnimating = m_scrollbarAutoHide.Tick(dt);
    if (std::abs(prevOpacity - m_scrollbarAutoHide.Opacity()) > 0.001f) {
        MarkRenderRectDirty(GetScrollbarTrackRect().Inflate(2.0f));
    }
    if (scrolling || moving || hideAnimating || m_scrollAnimator.IsActive() || rippleAnim) {
        RequestAnimationTicks();
    }
    return base || scrolling || moving || hideAnimating || rippleAnim;
}

bool TreeView::HasSelfAnimation() const {
    return Control::HasSelfAnimation()
        || m_expandAnimActive
        || m_scrollAnimator.IsActive()
        || m_scrollbarAutoHide.NeedsTicks()
        || m_selectRippleActive;
}

void TreeView::StartSelectRipple(const std::shared_ptr<TreeViewItem>& item, Point pt) {
    if (!item) return;
    if (!UIElement::AreAnimationsEnabled()) {
        m_selectRippleActive = false;
        m_selectRippleCovered = true;
        m_selectRippleItem = item;
        return;
    }

    RebuildVisibleItems();
    int idx = GetVisibleIndexOfItem(item.get());
    Rect pill = (idx >= 0) ? GetItemRect(idx) : Rect(pt.x - 40.0f, pt.y - 12.0f, 80.0f, 24.0f);

    m_selectRippleItem = item;
    m_selectRippleLocalX = std::clamp(pt.x - pill.x, 0.0f, (std::max)(0.0f, pill.width));
    m_selectRippleLocalY = std::clamp(pt.y - pill.y, 0.0f, (std::max)(0.0f, pill.height));
    if (pill.width > 0.0f && (pt.x < pill.x || pt.x > pill.x + pill.width)) {
        m_selectRippleLocalX = pill.width * 0.5f;
    }
    if (pill.height > 0.0f && (pt.y < pill.y || pt.y > pill.y + pill.height)) {
        m_selectRippleLocalY = pill.height * 0.5f;
    }
    m_selectRippleRadius = 4.0f;
    m_selectRippleActive = true;
    m_selectRippleCovered = false;
    RequestAnimationTicks();
}

} // namespace CUI
