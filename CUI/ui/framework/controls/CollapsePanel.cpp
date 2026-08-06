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
}

CollapsePanel::CollapsePanel() {
    SetBackgroundToken(ThemeTokenId::CardBackground);
    SetBorderToken(ThemeTokenId::CardBorder);
    SetBorderThickness(1.0f);
    SetCornerRadius(8.0f);
    SetPadding(Thickness(0));

    m_headerButton = std::make_shared<Button>();
    m_headerButton->SetBackgroundToken(ThemeTokenId::CardBackground);
    m_headerButton->SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
    m_headerButton->SetPressedBackgroundToken(ThemeTokenId::PressedBackground);
    m_headerButton->SetColorToken(ThemeTokenId::TextPrimary);
    m_headerButton->SetBorderToken(ThemeTokenId::CardBorder);
    m_headerButton->SetBorderThickness(0.0f);
    m_headerButton->SetCornerRadius(8.0f);
    m_headerButton->SetPadding(Thickness(16, 12, 16, 12));
    m_headerButton->SetAlign(Alignment::Stretch);
    m_headerButton->SetText(""); // Managed custom drawing in CollapsePanel::OnRender
    m_headerButton->OnClick().Connect([this](UIElement*) { SetExpanded(!m_isExpanded); });

    m_contentHost = std::make_shared<StackPanel>();
    m_contentHost->SetOrientation(Orientation::Vertical);
    m_contentHost->SetGap(10.0f);
    m_contentHost->SetPadding(Thickness(16, 12, 16, 16));
    m_contentHost->SetAlign(Alignment::Stretch);
    m_contentHost->SetBackground(D2D1::ColorF(0, 0, 0, 0));

    AddChild(m_headerButton);
    AddChild(m_contentHost);
    m_expandAnim.Reset(1.0f);
    SetHeader(m_headerText);
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

void CollapsePanel::SyncHeaderChrome() {
    // Header text rendering handled directly in OnRender
    MarkRenderContentDirty();
}

void CollapsePanel::SetHeader(const std::string& header) {
    m_headerText = header;
    SyncHeaderChrome();
    InvalidateParentLayout();
}

void CollapsePanel::SetSubtitle(const std::string& subtitle) {
    m_subtitleText = subtitle;
    SyncHeaderChrome();
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
    SyncHeaderChrome();
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
    m_contentHost->SetOpacity(EaseOutCubic(m_expandAnim.Current()));
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
        SyncHeaderChrome();
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
    float contentH = (std::max)(0.0f, availableSize.height - margin.top - margin.bottom - padding.top - padding.bottom);

    float headerH = m_subtitleText.empty() ? 44.0f : 56.0f;
    if (m_headerButton) {
        Size headerSize = m_headerButton->Measure(Size(contentW, headerH));
        headerH = (std::max)(headerH, headerSize.height);
    }

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

    float headerH = m_subtitleText.empty() ? 44.0f : 56.0f;
    if (m_headerButton) {
        m_headerButton->Arrange(Rect(innerX, innerY, innerW, headerH));
    }

    if (m_contentHost) {
        const float eased = EaseOutCubic(m_expandAnim.Current());
        float contentY = innerY + headerH;
        float contentH = (std::max)(0.0f, m_bodyDesiredHeight * eased);
        if (contentH < 0.5f) {
            m_contentHost->Arrange(Rect(innerX, contentY, innerW, 0.0f));
        } else {
            m_contentHost->Arrange(Rect(innerX, contentY, innerW, contentH));
        }
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

    D2D1_COLOR_F chevronColor = ThemeManager::Instance().GetFlatColor(ThemeTokenId::TextSecondary);
    if (m_headerButton && m_headerButton->IsHovered()) {
        chevronColor = ThemeManager::Instance().GetFlatColor(ThemeTokenId::TextPrimary);
    }

    ctx.DrawLine(pLeft, pTip, chevronColor, 1.6f);
    ctx.DrawLine(pTip, pRight, chevronColor, 1.6f);
}

void CollapsePanel::OnRender(GraphicsContext& ctx) {
    D2D1_COLOR_F bg = ResolveThemeColor(GetBackgroundToken(), ThemeTokenId::CardBackground);
    D2D1_COLOR_F border = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);
    float radius = GetCornerRadius();

    // 1. Fill base card background
    ctx.FillRoundedRect(m_bounds, radius, bg);

    // 2. Render children (m_headerButton hover background + m_contentHost)
    UIElement::OnRender(ctx);

    // 3. Render card border
    ctx.DrawRoundedRect(m_bounds, radius, border, 1.0f);

    // 4. Render Header typography & animated vector Chevron over header button
    if (m_headerButton) {
        const Rect hb = m_headerButton->GetBounds();

        D2D1_COLOR_F textPrimary = ThemeManager::Instance().GetFlatColor(ThemeTokenId::TextPrimary);
        D2D1_COLOR_F textSecondary = ThemeManager::Instance().GetFlatColor(ThemeTokenId::TextSecondary);

        float textX = hb.x + 16.0f;
        float chevronW = 36.0f;
        float textW = (std::max)(0.0f, hb.width - 32.0f - chevronW);

        if (m_subtitleText.empty()) {
            Rect titleRect(textX, hb.y, textW, hb.height);
            ctx.DrawText(m_headerText, titleRect, textPrimary, "微软雅黑", 13.5f,
                         DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_SEMI_BOLD);
        } else {
            Rect titleRect(textX, hb.y + 7.0f, textW, 20.0f);
            ctx.DrawText(m_headerText, titleRect, textPrimary, "微软雅黑", 13.5f,
                         DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_SEMI_BOLD);

            Rect subtitleRect(textX, hb.y + 27.0f, textW, 18.0f);
            ctx.DrawText(m_subtitleText, subtitleRect, textSecondary, "微软雅黑", 12.0f,
                         DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_NORMAL);
        }

        // Animated Chevron Icon on far right
        Rect chevronBounds(hb.x + hb.width - 32.0f, hb.y, 24.0f, hb.height);
        DrawAnimatedChevron(ctx, chevronBounds, m_expandAnim.Current());

        // Hairline line between header & body (fades in/out)
        const float animProgress = m_expandAnim.Current();
        if (animProgress > 0.01f) {
            D2D1_COLOR_F line = border;
            line.a *= std::clamp(animProgress * 0.7f, 0.0f, 1.0f);
            float lineY = hb.y + hb.height;
            ctx.DrawLine(Point(m_bounds.x + 1.0f, lineY),
                         Point(m_bounds.x + m_bounds.width - 1.0f, lineY),
                         line, 1.0f);
        }
    }
}

void CollapsePanel::OnMouseDown(Point pt) {
    if (m_headerButton && m_headerButton->GetBounds().Contains(pt.x, pt.y)) {
        m_headerButton->OnMouseDown(pt);
        return;
    }
    if (m_expandAnim.Current() > 0.01f && m_contentHost && m_contentHost->GetBounds().Contains(pt.x, pt.y)) {
        m_contentHost->OnMouseDown(pt);
    }
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

} // namespace CUI
