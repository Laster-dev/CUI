#include "TabView.h"
#include "../render/CompositionContext.h"
#include <algorithm>
#include <cmath>
#include <windows.h>

namespace CUI {
namespace {}

TabView::TabView() {
    SetProperty("background", Value("#1E1E1E"));
    SetProperty("headerBackground", Value("#252526"));
    SetProperty("activeTabBackground", Value("#1E1E1E"));
    SetProperty("inactiveTabBackground", Value("#2D2D2D"));
    SetProperty("underlineColor", Value(D2D1::ColorF(0x3B / 255.0f, 0x4A / 255.0f, 0x57 / 255.0f, 1.0f)));
    SetProperty("activeUnderlineColor", Value(D2D1::ColorF(0x51 / 255.0f, 0xA8 / 255.0f, 0xF7 / 255.0f, 1.0f)));
    SetProperty("minTabWidth", Value(80.0f));
    SetProperty("maxTabWidth", Value(260.0f));
    m_headerLayer.SetCacheable(true);
    m_contentLayer.SetCacheable(true);
}

std::vector<PropertyMeta> TabView::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "minTabWidth", "最小标签宽度 (MinTabWidth)", "标签栏配置", "number" });
    metas.push_back({ "maxTabWidth", "最大标签宽度 (MaxTabWidth)", "标签栏配置", "number" });
    return metas;
}

void TabView::AddTab(const std::string& title, std::shared_ptr<UIElement> content, const std::string& icon, bool isClosable) {
    TabViewItem item;
    item.title = title;
    item.icon = icon;
    item.content = content;
    item.isClosable = isClosable;

    if (content) {
        AddChild(content);
        int addedIndex = static_cast<int>(m_tabs.size());
        if (addedIndex != m_selectedIndex) {
            content->SetProperty("visibility", Value("Collapsed"));
        }
    }

    m_tabs.push_back(item);
    MarkHeaderDirty();
    MarkContentDirty();

    if (m_tabs.size() == 1) {
        SetSelectedIndex(0);
    } else {
        EnsureSelectedTabVisible();
    }
}

void TabView::RemoveTab(int index) {
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) return;

    if (m_tabs[index].content) {
        RemoveChild(m_tabs[index].content);
    }

    m_tabs.erase(m_tabs.begin() + index);
    MarkHeaderDirty();
    MarkContentDirty();

    if (m_selectedIndex >= static_cast<int>(m_tabs.size())) {
        m_selectedIndex = static_cast<int>(m_tabs.size()) - 1;
    }

    if (m_selectedIndex >= 0) {
        SetSelectedIndex(m_selectedIndex);
    }

    m_tabClosedEvent.Invoke(this, index);
}

void TabView::SetSelectedIndex(int index) {
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) return;
    if (m_selectedIndex == index && m_tabs.size() > 1) return;

    m_selectedIndex = index;

    // Show active tab content, hide other contents
    float headerH = GetHeaderHeight();
    Rect contentRect(m_bounds.x, m_bounds.y + headerH, m_bounds.width, m_bounds.height - headerH);
    Size contentAvail(m_bounds.width, m_bounds.height - headerH);

    for (size_t i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs[i].content) {
            if (static_cast<int>(i) == m_selectedIndex) {
                m_tabs[i].content->SetProperty("visibility", Value("Visible"));
                m_tabs[i].content->Measure(contentAvail);
                m_tabs[i].content->Arrange(contentRect);
            } else {
                m_tabs[i].content->SetProperty("visibility", Value("Collapsed"));
            }
        }
    }

    EnsureSelectedTabVisible();
    MarkHeaderDirty();
    MarkContentDirty();
    m_selectionChangedEvent.Invoke(this, m_selectedIndex);
}

Size TabView::Measure(Size availableSize) {
    float headerH = GetHeaderHeight();
    Size contentAvail(availableSize.width, availableSize.height - headerH);

    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_tabs.size())) {
        if (m_tabs[m_selectedIndex].content) {
            m_tabs[m_selectedIndex].content->Measure(contentAvail);
        }
    }

    m_desiredSize = availableSize;
    return m_desiredSize;
}

void TabView::Arrange(Rect finalRect) {
    SetBounds(finalRect);
    float headerH = GetHeaderHeight();
    Rect contentRect(finalRect.x, finalRect.y + headerH, finalRect.width, (std::max)(0.0f, finalRect.height - headerH));
    Size contentAvail(contentRect.width, contentRect.height);

    for (size_t i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs[i].content) {
            m_tabs[i].content->Measure(contentAvail);
            m_tabs[i].content->Arrange(contentRect);
        }
    }

    GraphicsContext ctx;
    float maxScroll = (std::max)(0.0f, GetTotalTabsWidth(ctx) - (std::max)(0.0f, m_bounds.width - 8.0f));
    m_scrollTargetX = std::clamp(m_scrollTargetX, 0.0f, maxScroll);
    m_scrollOffsetXAnim.Reset(std::clamp(m_scrollOffsetXAnim.Current(), 0.0f, maxScroll));
    m_headerLayer.SetBounds(GetHeaderRect());
    m_contentLayer.SetBounds(GetContentRect());
    MarkHeaderDirty();
    MarkContentDirty();
}

float TabView::GetHeaderHeight() const {
    return 36.0f;
}

float TabView::MeasureTabWidth(GraphicsContext& ctx, const TabViewItem& tab) const {
    float minW = GetProperty("minTabWidth").AsFloat(80.0f);
    float maxW = GetProperty("maxTabWidth").AsFloat(260.0f);

    float width = 24.0f;
    if (!tab.icon.empty()) {
        width += 16.0f + 8.0f;
    }
    width += ctx.MeasureText(tab.title, "Segoe UI", 12.0f, DWRITE_FONT_WEIGHT_NORMAL).width;
    width += tab.isClosable ? 24.0f : 10.0f;
    return std::clamp(width, minW, maxW);
}

float TabView::GetTotalTabsWidth(GraphicsContext& ctx) const {
    float total = 4.0f;
    for (const auto& tab : m_tabs) {
        total += MeasureTabWidth(ctx, tab) + 4.0f;
    }
    return total;
}

void TabView::EnsureSelectedTabVisible() {
    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_tabs.size()) || m_bounds.width <= 0.0f) {
        return;
    }

    GraphicsContext ctx;
    float tabX = m_bounds.x + 4.0f;
    for (int i = 0; i < m_selectedIndex; ++i) {
        tabX += MeasureTabWidth(ctx, m_tabs[i]) + 4.0f;
    }

    float selectedW = MeasureTabWidth(ctx, m_tabs[m_selectedIndex]);
    float relativeStart = tabX - m_bounds.x;
    float relativeEnd = relativeStart + selectedW;
    float viewportStart = m_scrollTargetX;
    float viewportEnd = m_scrollTargetX + (std::max)(0.0f, m_bounds.width - 8.0f);

    if (relativeStart < viewportStart) {
        m_scrollTargetX = relativeStart;
    } else if (relativeEnd > viewportEnd) {
        m_scrollTargetX = relativeEnd - (std::max)(0.0f, m_bounds.width - 8.0f);
    }

    float maxScroll = (std::max)(0.0f, GetTotalTabsWidth(ctx) - (std::max)(0.0f, m_bounds.width - 8.0f));
    m_scrollTargetX = std::clamp(m_scrollTargetX, 0.0f, maxScroll);
}

Rect TabView::GetHeaderRect() const {
    return Rect(m_bounds.x, m_bounds.y, m_bounds.width, GetHeaderHeight());
}

Rect TabView::GetContentRect() const {
    float headerH = GetHeaderHeight();
    return Rect(m_bounds.x, m_bounds.y + headerH, m_bounds.width, (std::max)(0.0f, m_bounds.height - headerH));
}

void TabView::MarkHeaderDirty() {
    Rect header = GetHeaderRect();
    if (!header.IsEmpty()) {
        m_headerDirty.AddRect(header.Inflate(2.0f));
        m_headerLayer.Invalidate(RenderLayer::ContentDirty | RenderLayer::StructureDirty | RenderLayer::TransformDirty);
        MarkRenderRectDirty(header.Inflate(2.0f));
    }
}

void TabView::MarkContentDirty() {
    Rect content = GetContentRect();
    if (!content.IsEmpty()) {
        m_contentDirty.AddRect(content.Inflate(2.0f));
        m_contentLayer.Invalidate(RenderLayer::ContentDirty | RenderLayer::StructureDirty | RenderLayer::TransformDirty);
        MarkRenderRectDirty(content.Inflate(2.0f));
    }
}

UIElement* TabView::GetSelectedContent() const {
    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_tabs.size())) {
        return nullptr;
    }
    return m_tabs[m_selectedIndex].content.get();
}

void TabView::Render(GraphicsContext& ctx) {
    std::string visStr = GetProperty("visibility").AsString("Visible");
    if (visStr != "Visible") return;

    ctx.PushClip(m_bounds);
    OnRender(ctx);
    RenderContentLayer(ctx);
    ctx.PopClip();
}

void TabView::OnRender(GraphicsContext& ctx) {
    RenderHeaderLayer(ctx);
}

void TabView::RenderHeaderLayer(GraphicsContext& ctx) {
    float headerH = GetHeaderHeight();
    Rect headerBarRect(m_bounds.x, m_bounds.y, m_bounds.width, headerH);
    if (!headerBarRect.IsEmpty()) {
        Size cacheSize(headerBarRect.width, headerBarRect.height);
        const bool needsRerender = !m_headerLayer.IsValid()
            || m_headerLayer.HasDirtyFlags()
            || m_headerDirty.GetRectCount() > 0;
        if (auto* composition = ctx.GetCompositionContext()) {
            if (needsRerender) {
                composition->CountLayerCacheRerender();
                if (!m_headerLayer.GetCacheBitmap()) {
                    composition->CountLayerCacheMiss();
                }
            } else {
                composition->CountLayerCacheHit();
                composition->CountLayerCacheReuse();
            }
        }

        if (needsRerender && ctx.PushLayerTarget(m_headerLayer, cacheSize, Rect(0, 0, cacheSize.width, cacheSize.height), D2D1::ColorF(0, 0, 0, 0))) {
            auto* d2d = ctx.GetD2DContext();
            D2D1_MATRIX_3X2_F oldTransform{};
            d2d->GetTransform(&oldTransform);
            d2d->SetTransform(D2D1::Matrix3x2F::Translation(-headerBarRect.x, -headerBarRect.y));
            RenderHeaderContents(ctx);
            d2d->SetTransform(oldTransform);
            ctx.PopLayerTarget(m_headerLayer);
            m_headerLayer.Validate();
            m_headerDirty.Clear();
        }

        Rect sourceRect(0.0f, 0.0f, headerBarRect.width, headerBarRect.height);
        ctx.DrawLayer(m_headerLayer, headerBarRect, &sourceRect);
    }
}

void TabView::RenderContentLayer(GraphicsContext& ctx) {
    Rect contentRect = GetContentRect();
    if (contentRect.IsEmpty()) {
        return;
    }

    UIElement* selectedContent = GetSelectedContent();
    Size cacheSize(contentRect.width, contentRect.height);
    const bool needsRerender = !m_contentLayer.IsValid()
        || m_contentLayer.HasDirtyFlags()
        || m_contentDirty.GetRectCount() > 0;

    if (auto* composition = ctx.GetCompositionContext()) {
        if (needsRerender) {
            composition->CountLayerCacheRerender();
            if (!m_contentLayer.GetCacheBitmap()) {
                composition->CountLayerCacheMiss();
            }
        } else {
            composition->CountLayerCacheHit();
            composition->CountLayerCacheReuse();
        }
    }

    if (needsRerender && ctx.PushLayerTarget(m_contentLayer, cacheSize, contentRect, D2D1::ColorF(0, 0, 0, 0))) {
        auto* d2d = ctx.GetD2DContext();
        D2D1_MATRIX_3X2_F oldTransform{};
        d2d->GetTransform(&oldTransform);
        d2d->SetTransform(D2D1::Matrix3x2F::Translation(-contentRect.x, -contentRect.y));

        D2D1_COLOR_F bg = GetProperty("background").AsColor(D2D1::ColorF(0x1E / 255.0f, 0x1E / 255.0f, 0x1E / 255.0f, 1.0f));
        ctx.FillRect(contentRect, bg);
        if (selectedContent) {
            selectedContent->Render(ctx);
        }

        d2d->SetTransform(oldTransform);
        ctx.PopLayerTarget(m_contentLayer);
        m_contentLayer.Validate();
        m_contentDirty.Clear();
    }

    Rect sourceRect(0.0f, 0.0f, contentRect.width, contentRect.height);
    ctx.PushClip(contentRect);
    ctx.DrawLayer(m_contentLayer, contentRect, &sourceRect);
    ctx.PopClip();
}

void TabView::RenderHeaderContents(GraphicsContext& ctx) {
    float headerH = GetHeaderHeight();
    Rect headerBarRect(m_bounds.x, m_bounds.y, m_bounds.width, headerH);

    // Draw TabBar Header background
    D2D1_COLOR_F headerBg = GetProperty("headerBackground").AsColor(D2D1::ColorF(0x25 / 255.0f, 0x25 / 255.0f, 0x26 / 255.0f));
    ctx.FillRect(headerBarRect, headerBg);

    // Clip tab buttons within header bounds
    ctx.PushClip(headerBarRect);

    float tabX = m_bounds.x + 4.0f - m_scrollOffsetXAnim.Current();

    for (size_t i = 0; i < m_tabs.size(); ++i) {
        const auto& tab = m_tabs[i];
        bool isActive = (static_cast<int>(i) == m_selectedIndex);

        float tabW = MeasureTabWidth(ctx, tab);
        Rect tabRect(tabX, m_bounds.y + 4.0f, tabW, headerH - 4.0f);

        D2D1_COLOR_F tabBg = isActive ? GetProperty("activeTabBackground").AsColor(D2D1::ColorF(0x1E / 255.0f, 0x1E / 255.0f, 0x1E / 255.0f))
                                      : GetProperty("inactiveTabBackground").AsColor(D2D1::ColorF(0x2D / 255.0f, 0x2D / 255.0f, 0x2D / 255.0f));

        // Rounded top corners only; bottom edge stays flush with content.
        ctx.FillRoundedRect(tabRect, 4.0f, tabBg);
        ctx.FillRect(Rect(tabRect.x, tabRect.y + tabRect.height - 4.0f, tabRect.width, 4.0f), tabBg);

        float indicatorInset = 6.0f;
        float indicatorWidth = (std::max)(0.0f, tabW - indicatorInset * 2.0f);
        float indicatorY = m_bounds.y + 4.0f;
        D2D1_COLOR_F underlineColor = GetProperty("underlineColor").AsColor(D2D1::ColorF(0x3B / 255.0f, 0x4A / 255.0f, 0x57 / 255.0f, 1.0f));
        D2D1_COLOR_F activeUnderlineColor = GetProperty("activeUnderlineColor").AsColor(D2D1::ColorF(0x51 / 255.0f, 0xA8 / 255.0f, 0xF7 / 255.0f, 1.0f));

        if (isActive || tab.accentAnim.Current() > 0.01f) {
            ctx.DrawLine(
                Point(tabX + indicatorInset, indicatorY),
                Point(tabX + indicatorInset + indicatorWidth, indicatorY),
                underlineColor,
                1.0f
            );
        }

        float focusFactor = std::clamp(tab.accentAnim.Current(), 0.0f, 1.0f);
        if (focusFactor > 0.01f && indicatorWidth > 0.0f) {
            float eased = 1.0f - std::pow(1.0f - focusFactor, 2.4f);
            float activeWidth = indicatorWidth * eased;
            float activeX = tabX + indicatorInset + (indicatorWidth - activeWidth) * 0.5f;
            ctx.DrawLine(
                Point(activeX, indicatorY),
                Point(activeX + activeWidth, indicatorY),
                activeUnderlineColor,
                1.0f + eased
            );
        }

        // Draw Icon if available
        float titleX = tabX + 12.0f;
        if (!tab.icon.empty()) {
            ctx.DrawText(tab.icon, Rect(titleX, tabRect.y, 16, tabRect.height), D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f), "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            titleX += 20.0f;
        }

        // Draw Title Text
        D2D1_COLOR_F textColor = isActive ? D2D1::ColorF(1.0f, 1.0f, 1.0f) : D2D1::ColorF(0x99 / 255.0f, 0x99 / 255.0f, 0x99 / 255.0f);
        ctx.DrawText(tab.title, Rect(titleX, tabRect.y, tabW - (titleX - tabX) - 24.0f, tabRect.height), textColor, "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, isActive ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL);

        // Draw Close 'X' Button if closable
        if (tab.isClosable) {
            float closeX = tabX + tabW - 22.0f;
            float closeY = tabRect.y + (tabRect.height - 16.0f) * 0.5f;
            Rect closeRect(closeX, closeY, 16.0f, 16.0f);

            bool isCloseHover = (m_hoveredCloseIndex == static_cast<int>(i));
            if (isCloseHover) {
                ctx.FillRoundedRect(closeRect, 3.0f, D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.15f));
            }

            D2D1_COLOR_F closeColor = isCloseHover ? D2D1::ColorF(1.0f, 1.0f, 1.0f) : D2D1::ColorF(0x88 / 255.0f, 0x88 / 255.0f, 0x88 / 255.0f);
            float cx = closeX + 8.0f;
            float cy = closeY + 8.0f;
            ctx.DrawLine(Point(cx - 3.5f, cy - 3.5f), Point(cx + 3.5f, cy + 3.5f), closeColor, 1.2f);
            ctx.DrawLine(Point(cx + 3.5f, cy - 3.5f), Point(cx - 3.5f, cy + 3.5f), closeColor, 1.2f);
        }

        tabX += tabW + 4.0f;
    }

    ctx.PopClip();

    // Draw TabBar Bottom border line
    ctx.DrawLine(Point(m_bounds.x, m_bounds.y + headerH - 1), Point(m_bounds.x + m_bounds.width, m_bounds.y + headerH - 1), D2D1::ColorF(0x33 / 255.0f, 0x33 / 255.0f, 0x33 / 255.0f));
}

bool TabView::IsPointInHeader(float x, float y) const {
    const float headerH = GetHeaderHeight();
    return x >= m_bounds.x && x <= m_bounds.x + m_bounds.width
        && y >= m_bounds.y && y <= m_bounds.y + headerH;
}

void TabView::ScrollHeaderByWheel(float delta) {
    GraphicsContext ctx;
    float maxScroll = (std::max)(0.0f, GetTotalTabsWidth(ctx) - (std::max)(0.0f, m_bounds.width - 8.0f));
    m_scrollTargetX = std::clamp(m_scrollTargetX - delta * 72.0f, 0.0f, maxScroll);
    MarkHeaderDirty();
}

void TabView::OnMouseWheel(float delta) {
    POINT screenPt{};
    if (GetCursorPos(&screenPt)) {
        HWND hwnd = WindowFromPoint(screenPt);
        if (hwnd) {
            POINT clientPt = screenPt;
            ScreenToClient(hwnd, &clientPt);
            if (IsPointInHeader(static_cast<float>(clientPt.x), static_cast<float>(clientPt.y))) {
                ScrollHeaderByWheel(delta);
                return;
            }
        }
    }

    UIElement::OnMouseWheel(delta);
}

UIElement* TabView::HitTest(float x, float y) {
    std::string visStr = GetProperty("visibility").AsString("Visible");
    if (visStr != "Visible" || !m_bounds.Contains(x, y)) {
        return nullptr;
    }

    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_tabs.size()) && m_tabs[m_selectedIndex].content) {
        if (!IsPointInHeader(x, y)) {
            if (UIElement* childHit = m_tabs[m_selectedIndex].content->HitTest(x, y)) {
                return childHit;
            }
        }
    }

    return this;
}

void TabView::OnMouseMove(Point pt) {
    float headerH = GetHeaderHeight();
    int oldHover = m_hoveredCloseIndex;
    m_hoveredCloseIndex = -1;

    if (pt.y >= m_bounds.y && pt.y <= m_bounds.y + headerH) {
        GraphicsContext ctx;
        float tabX = m_bounds.x + 4.0f - m_scrollOffsetXAnim.Current();

        for (size_t i = 0; i < m_tabs.size(); ++i) {
            float tabW = MeasureTabWidth(ctx, m_tabs[i]);

            if (m_tabs[i].isClosable) {
                float closeX = tabX + tabW - 22.0f;
                float closeY = m_bounds.y + 4.0f + (headerH - 4.0f - 16.0f) * 0.5f;
                Rect closeRect(closeX, closeY, 16.0f, 16.0f);
                if (closeRect.Contains(pt.x, pt.y)) {
                    m_hoveredCloseIndex = static_cast<int>(i);
                    break;
                }
            }

            tabX += tabW + 4.0f;
        }
    }

    if (oldHover != m_hoveredCloseIndex) {
        MarkHeaderDirty();
    }
}

void TabView::OnMouseDown(Point pt) {
    UIElement::OnMouseDown(pt);

    float headerH = GetHeaderHeight();
    if (pt.y >= m_bounds.y && pt.y <= m_bounds.y + headerH) {
        GraphicsContext ctx;
        float tabX = m_bounds.x + 4.0f - m_scrollOffsetXAnim.Current();

        for (size_t i = 0; i < m_tabs.size(); ++i) {
            float tabW = MeasureTabWidth(ctx, m_tabs[i]);
            Rect tabRect(tabX, m_bounds.y + 4.0f, tabW, headerH - 4.0f);

            if (tabRect.Contains(pt.x, pt.y)) {
                // Check if close button was clicked
                if (m_tabs[i].isClosable) {
                    float closeX = tabX + tabW - 22.0f;
                    float closeY = m_bounds.y + 4.0f + (headerH - 4.0f - 16.0f) * 0.5f;
                    Rect closeRect(closeX, closeY, 16.0f, 16.0f);
                    if (closeRect.Contains(pt.x, pt.y)) {
                        RemoveTab(static_cast<int>(i));
                        return;
                    }
                }

                SetSelectedIndex(static_cast<int>(i));
                MarkHeaderDirty();
                break;
            }

            tabX += tabW + 4.0f;
        }
    }
}

bool TabView::OnAnimationTick() {
    bool childAnimating = UIElement::OnAnimationTick();
    bool animating = childAnimating;
    if (childAnimating) {
        m_contentLayer.Invalidate(RenderLayer::ContentDirty);
        Rect content = GetContentRect();
        if (!content.IsEmpty()) {
            m_contentDirty.AddRect(content.Inflate(2.0f));
        }
    }

    m_scrollOffsetXAnim.SetTarget(m_scrollTargetX);
    if (m_scrollOffsetXAnim.Tick(UIElement::GetAnimationDeltaSeconds(), AnimationSpec{ 0.22f, 0.10f })) {
        MarkHeaderDirty();
        animating = true;
    }

    for (size_t i = 0; i < m_tabs.size(); ++i) {
        float target = (static_cast<int>(i) == m_selectedIndex) ? 1.0f : 0.0f;
        m_tabs[i].accentAnim.SetTarget(target);
        if (m_tabs[i].accentAnim.Tick(UIElement::GetAnimationDeltaSeconds(), AnimationSpec{ 0.18f, 0.01f })) {
            MarkHeaderDirty();
            animating = true;
        }
    }

    return animating;
}

bool TabView::HasSelfAnimation() const {
    if (std::abs(m_scrollTargetX - m_scrollOffsetXAnim.Current()) > 0.1f) {
        return true;
    }

    for (size_t i = 0; i < m_tabs.size(); ++i) {
        float target = (static_cast<int>(i) == m_selectedIndex) ? 1.0f : 0.0f;
        if (std::abs(target - m_tabs[i].accentAnim.Current()) > 0.01f) {
            return true;
        }
    }
    return false;
}

void TabView::SyncRenderState() {
    UIElement::SyncRenderState();
    m_headerLayer.SetBounds(GetHeaderRect());
    m_contentLayer.SetBounds(GetContentRect());
    m_headerLayer.Validate();
    m_contentLayer.Validate();
    m_headerDirty.Clear();
    m_contentDirty.Clear();
}

void TabView::CollectRenderDirtyRegion(DirtyRegion& dirtyRegion, bool consume) {
    if (consume) {
        dirtyRegion.UnionWith(m_renderNode.ConsumeWorldDirtyRegion());
    } else {
        dirtyRegion.UnionWith(m_renderNode.GetWorldDirtyRegion());
    }

    if (UIElement* selectedContent = GetSelectedContent()) {
        DirtyRegion selectedDirty;
        selectedContent->CollectRenderDirtyRegion(selectedDirty, false);
        if (!selectedDirty.IsEmpty()) {
            m_contentDirty.UnionWith(selectedDirty);
            m_contentLayer.Invalidate(RenderLayer::ContentDirty);
        }
        selectedContent->CollectRenderDirtyRegion(dirtyRegion, consume);
    }

    dirtyRegion.UnionWith(m_headerDirty);
    dirtyRegion.UnionWith(m_contentDirty);
}

} // namespace CUI
