#include "CollapsePanel.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <windows.h>

namespace CUI {

namespace {
float EaseOutCubic(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    const float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

D2D1_COLOR_F Blend(D2D1_COLOR_F a, D2D1_COLOR_F b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return D2D1::ColorF(
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t
    );
}
} // namespace

CollapsePanel::CollapsePanel() {
    SetBackgroundToken(ThemeTokenId::CardBackground);
    SetBorderToken(ThemeTokenId::CardBorder);
    SetBorderThickness(1.0f);
    SetCornerRadius(8.0f);
    SetPadding(Thickness(0));
    SetHoverBackgroundToken(ThemeTokenId::HoverBackground);

    m_contentHost = std::make_shared<StackPanel>();
    m_contentHost->SetOrientation(Orientation::Vertical);
    m_contentHost->SetGap(10.0f);
    m_contentHost->SetPadding(Thickness(16, 12, 16, 16));
    m_contentHost->SetAlign(Alignment::Stretch);
    m_contentHost->SetBackground(D2D1::ColorF(0, 0, 0, 0));

    AddChild(m_contentHost);
    m_expandAnim.Reset(1.0f);
    UpdateContentVisibility();
}

CollapsePanel::CollapsePanel(const std::string& headerText) : CollapsePanel() {
    SetHeader(headerText);
}

std::vector<PropertyMeta> CollapsePanel::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "header", "标题 (Header)", "基本信息", "string" });
    metas.push_back({ "subtitle", "副标题 (Subtitle)", "基本信息", "string" });
    metas.push_back({ "isExpanded", "是否展开 (IsExpanded)", "交互状态", "bool" });
    return metas;
}

float CollapsePanel::GetHeaderHeight() const {
    return m_subtitleText.empty() ? 44.0f : 56.0f;
}

Rect CollapsePanel::GetHeaderRect() const {
    Thickness padding = GetPadding();
    return Rect(
        m_bounds.x + padding.left,
        m_bounds.y + padding.top,
        (std::max)(0.0f, m_bounds.width - padding.left - padding.right),
        GetHeaderHeight()
    );
}

bool CollapsePanel::IsPointInHeader(Point pt) const {
    return GetHeaderRect().Contains(pt.x, pt.y);
}

void CollapsePanel::SetHeader(const std::string& header) {
    if (m_headerText == header) {
        return;
    }
    m_headerText = header;
    MarkRenderRectDirty(GetHeaderRect().Inflate(2.0f));
    InvalidateParentLayout();
}

void CollapsePanel::SetSubtitle(const std::string& subtitle) {
    if (m_subtitleText == subtitle) {
        return;
    }
    m_subtitleText = subtitle;
    MarkRenderContentDirty();
    InvalidateParentLayout();
}

void CollapsePanel::SetExpanded(bool expanded) {
    if (m_isExpanded == expanded) {
        return;
    }
    m_isExpanded = expanded;
    m_expandAnim.SetTarget(expanded ? 1.0f : 0.0f);
    if (!UIElement::AreAnimationsEnabled()) {
        m_expandAnim.Reset(expanded ? 1.0f : 0.0f);
    } else {
        RequestAnimationTicks();
    }
    UpdateContentVisibility();
    m_onExpandedChangedEvent.Invoke(this, m_isExpanded);
    InvalidateParentLayout();
}

void CollapsePanel::InvalidateParentLayout() {
    MarkRenderContentDirty();

    std::vector<UIElement*> chain;
    for (UIElement* p = this; p; p = p->GetParent()) {
        chain.push_back(p);
    }
    if (chain.empty()) {
        return;
    }

    UIElement* root = chain.back();
    const Rect rootBounds = root->GetBounds();
    if (rootBounds.IsEmpty()) {
        for (UIElement* p : chain) {
            p->MarkRenderContentDirty();
        }
        return;
    }

    root->Measure(Size(rootBounds.width, rootBounds.height));
    root->Arrange(rootBounds);
}

void CollapsePanel::SetContent(std::shared_ptr<UIElement> content) {
    if (m_content && m_contentHost) {
        m_contentHost->RemoveChild(m_content);
    }
    m_content = content;
    if (m_content && m_contentHost) {
        m_contentHost->AddChild(m_content);
    }
    UpdateContentVisibility();
    InvalidateParentLayout();
}

void CollapsePanel::UpdateContentVisibility() {
    if (!m_contentHost) {
        return;
    }
    const bool show = m_isExpanded || m_expandAnim.Current() > 0.01f;
    m_contentHost->SetVisibility(show ? Visibility::Visible : Visibility::Collapsed);
    m_contentHost->SetOpacity(1.0f);
}

void CollapsePanel::SetHeaderHovered(bool hovered) {
    if (m_headerHovered == hovered) {
        return;
    }
    m_headerHovered = hovered;
    MarkRenderRectDirty(GetHeaderRect().Inflate(2.0f));
}

bool CollapsePanel::OnAnimationTick() {
    bool base = UIElement::OnAnimationTick();
    if (!UIElement::AreAnimationsEnabled()) {
        m_expandAnim.Reset(m_isExpanded ? 1.0f : 0.0f);
        UpdateContentVisibility();
        return base;
    }

    const bool moving = m_expandAnim.Tick(UIElement::GetAnimationDeltaSeconds(), AnimationSpec{ 0.32f, 0.01f });
    if (moving) {
        UpdateContentVisibility();
        InvalidateParentLayout();
        RequestAnimationTicks();
    } else if (!m_isExpanded) {
        UpdateContentVisibility();
        InvalidateParentLayout();
    }
    return base || moving;
}

bool CollapsePanel::HasSelfAnimation() const {
    return m_expandAnim.IsAnimating(0.01f);
}

Size CollapsePanel::Measure(Size availableSize) {
    Thickness margin = GetMargin();
    Thickness padding = GetPadding();

    float contentW = (std::max)(0.0f, availableSize.width - margin.left - margin.right - padding.left - padding.right);
    const float headerH = GetHeaderHeight();

    Size bodySize(0, 0);
    if (m_contentHost && (m_isExpanded || m_expandAnim.Current() > 0.01f)) {
        bodySize = m_contentHost->Measure(Size(contentW, 100000.0f));
        m_bodyDesiredHeight = bodySize.height;
    } else {
        m_bodyDesiredHeight = 0.0f;
    }

    const float eased = EaseOutCubic(m_expandAnim.Current());
    const float bodyH = m_bodyDesiredHeight * eased;

    float width = (std::max)(contentW, bodySize.width) + margin.left + margin.right + padding.left + padding.right;
    float height = headerH + bodyH + margin.top + margin.bottom + padding.top + padding.bottom;

    float expW = GetWidth();
    float expH = GetHeight();
    if (expW >= 0.0f) width = expW;
    if (expH >= 0.0f) height = expH;

    m_desiredSize = Size(width, height);
    return m_desiredSize;
}

void CollapsePanel::Arrange(Rect finalRect) {
    m_bounds = finalRect;

    Thickness padding = GetPadding();
    float innerX = finalRect.x + padding.left;
    float innerY = finalRect.y + padding.top;
    float innerW = (std::max)(0.0f, finalRect.width - padding.left - padding.right);
    const float headerH = GetHeaderHeight();

    if (m_contentHost) {
        // Full content height; panel bounds + clip reveal during expand/collapse.
        float contentY = innerY + headerH;
        if (m_bodyDesiredHeight > 0.5f && m_expandAnim.Current() > 0.01f) {
            m_contentHost->Arrange(Rect(innerX, contentY, innerW, m_bodyDesiredHeight));
        } else {
            m_contentHost->Arrange(Rect(innerX, contentY, innerW, 0.0f));
        }
    }
}

UIElement* CollapsePanel::HitTest(float x, float y) {
    if (GetVisibility() != Visibility::Visible || !IsEnabled()) {
        return nullptr;
    }
    if (!m_bounds.Contains(x, y)) {
        return nullptr;
    }

    // Body content first when expanded.
    if (m_expandAnim.Current() > 0.01f && m_contentHost
        && m_contentHost->GetVisibility() == Visibility::Visible) {
        if (UIElement* hit = m_contentHost->HitTest(x, y)) {
            return hit;
        }
    }

    // Header / chrome belongs to this control.
    return this;
}

void CollapsePanel::OnMouseEnter() {
    UIElement::OnMouseEnter();
}

void CollapsePanel::OnMouseLeave() {
    UIElement::OnMouseLeave();
    m_headerPressed = false;
    SetHeaderHovered(false);
}

void CollapsePanel::OnMouseMove(Point pt) {
    UIElement::OnMouseMove(pt);
    SetHeaderHovered(IsPointInHeader(pt));
}

void CollapsePanel::OnMouseDown(Point pt) {
    if (!IsEnabled()) {
        return;
    }
    UIElement::OnMouseDown(pt);
    if (IsPointInHeader(pt)) {
        m_headerPressed = true;
        SetHeaderHovered(true);
        MarkRenderRectDirty(GetHeaderRect().Inflate(2.0f));
    }
}

void CollapsePanel::OnMouseUp(Point pt) {
    const bool wasPressed = m_headerPressed;
    m_headerPressed = false;
    UIElement::OnMouseUp(pt);
    if (wasPressed && IsPointInHeader(pt) && IsEnabled()) {
        SetExpanded(!m_isExpanded);
    }
    MarkRenderRectDirty(GetHeaderRect().Inflate(2.0f));
}

void CollapsePanel::OnKeyDown(int vkCode) {
    switch (vkCode) {
    case VK_SPACE:
    case VK_RETURN:
        SetExpanded(!m_isExpanded);
        break;
    case VK_LEFT:
    case VK_UP:
        if (m_isExpanded) SetExpanded(false);
        break;
    case VK_RIGHT:
    case VK_DOWN:
        if (!m_isExpanded) SetExpanded(true);
        break;
    }
}

void CollapsePanel::DrawAnimatedChevron(GraphicsContext& ctx, const Rect& bounds, float progress) {
    float cx = bounds.x + bounds.width * 0.5f;
    float cy = bounds.y + bounds.height * 0.5f;
    float arm = 4.5f;

    const float eased = EaseOutCubic(progress);
    float wingY = cy - 2.0f + 4.0f * eased;
    float tipY = cy + 2.5f - 5.0f * eased;

    Point pLeft(cx - arm, wingY);
    Point pTip(cx, tipY);
    Point pRight(cx + arm, wingY);

    D2D1_COLOR_F chevronColor = ThemeManager::Instance().GetFlatColor(
        (m_headerHovered || m_headerPressed) ? ThemeTokenId::TextPrimary : ThemeTokenId::TextSecondary);

    ctx.DrawLine(pLeft, pTip, chevronColor, 1.6f);
    ctx.DrawLine(pTip, pRight, chevronColor, 1.6f);
}

void CollapsePanel::DrawHeader(GraphicsContext& ctx) {
    const Rect header = GetHeaderRect();
    if (header.IsEmpty()) {
        return;
    }

    const float radius = GetCornerRadius();
    D2D1_COLOR_F cardBg = ResolveThemeColor(GetBackgroundToken(), ThemeTokenId::CardBackground);
    D2D1_COLOR_F hoverBg = ResolveThemeColor(GetHoverBackgroundToken(), ThemeTokenId::HoverBackground);
    D2D1_COLOR_F border = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);

    // Header interaction wash — same surface as the card, not a separate Button.
    if (m_headerPressed || m_headerHovered) {
        D2D1_COLOR_F wash = Blend(cardBg, hoverBg, m_headerPressed ? 0.85f : 0.65f);
        // Round only the top when expanded; full round when collapsed (header == card).
        const bool collapsedVisual = m_expandAnim.Current() < 0.02f;
        if (collapsedVisual) {
            ctx.FillRoundedRect(header, radius, wash);
        } else {
            ctx.FillRoundedRect(header, radius, wash);
            // Square off the bottom of the header so it meets the body cleanly.
            ctx.FillRect(Rect(header.x, header.y + header.height - radius, header.width, radius), wash);
        }
    }

    D2D1_COLOR_F textPrimary = ThemeManager::Instance().GetFlatColor(ThemeTokenId::TextPrimary);
    D2D1_COLOR_F textSecondary = ThemeManager::Instance().GetFlatColor(ThemeTokenId::TextSecondary);

    float textX = header.x + 16.0f;
    float chevronW = 36.0f;
    float textW = (std::max)(0.0f, header.width - 32.0f - chevronW);

    if (m_subtitleText.empty()) {
        Rect titleRect(textX, header.y, textW, header.height);
        ctx.DrawText(m_headerText, titleRect, textPrimary, GetFontFamily(), 13.5f,
                     DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_SEMI_BOLD);
    } else {
        Rect titleRect(textX, header.y + 7.0f, textW, 20.0f);
        ctx.DrawText(m_headerText, titleRect, textPrimary, GetFontFamily(), 13.5f,
                     DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_SEMI_BOLD);

        Rect subtitleRect(textX, header.y + 27.0f, textW, 18.0f);
        ctx.DrawText(m_subtitleText, subtitleRect, textSecondary, GetFontFamily(), 12.0f,
                     DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_NORMAL);
    }

    Rect chevronBounds(header.x + header.width - 32.0f, header.y, 24.0f, header.height);
    DrawAnimatedChevron(ctx, chevronBounds, m_expandAnim.Current());

    const float animProgress = m_expandAnim.Current();
    if (animProgress > 0.01f) {
        D2D1_COLOR_F line = border;
        line.a *= std::clamp(animProgress * 0.7f, 0.0f, 1.0f);
        float lineY = header.y + header.height;
        ctx.DrawLine(Point(m_bounds.x + 1.0f, lineY),
                     Point(m_bounds.x + m_bounds.width - 1.0f, lineY),
                     line, 1.0f);
    }
}

void CollapsePanel::Render(GraphicsContext& ctx) {
    if (GetVisibility() != Visibility::Visible) {
        return;
    }
    if (GetOpacity() <= 0.001f) {
        return;
    }

    const float radius = GetCornerRadius();
    if (radius > 0.01f) {
        ctx.PushRoundedClip(m_bounds, radius);
    } else {
        ctx.PushClip(m_bounds);
    }

    const bool useOpacity = GetOpacity() < 0.999f;
    if (useOpacity) {
        ctx.PushOpacity(GetOpacity());
    }

    OnRender(ctx);
    for (auto& child : GetChildren()) {
        if (child) {
            child->Render(ctx);
        }
    }
    DrawHeader(ctx);

    if (useOpacity) {
        ctx.PopOpacity();
    }
    ctx.PopClip();
}

void CollapsePanel::OnRender(GraphicsContext& ctx) {
    D2D1_COLOR_F bg = ResolveThemeColor(GetBackgroundToken(), ThemeTokenId::CardBackground);
    D2D1_COLOR_F border = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);
    float radius = GetCornerRadius();

    ctx.FillRoundedRect(m_bounds, radius, bg);
    ctx.DrawRoundedRect(m_bounds, radius, border, 1.0f);
}

} // namespace CUI
