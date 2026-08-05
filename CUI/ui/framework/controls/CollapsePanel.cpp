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
    SetCornerRadius(6.0f);
    SetPadding(Thickness(0));

    m_headerButton = std::make_shared<Button>();
    m_headerButton->SetBackgroundToken(ThemeTokenId::CardBackground);
    m_headerButton->SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
    m_headerButton->SetPressedBackgroundToken(ThemeTokenId::PressedBackground);
    m_headerButton->SetColorToken(ThemeTokenId::TextPrimary);
    m_headerButton->SetBorderToken(ThemeTokenId::CardBorder);
    m_headerButton->SetBorderThickness(0.0f);
    m_headerButton->SetCornerRadius(0.0f);
    m_headerButton->SetPadding(Thickness(12, 10, 12, 10));
    m_headerButton->SetAlign(Alignment::Stretch);
    m_headerButton->SetFontSize(13.0f);
    m_headerButton->OnClick().Connect([this](UIElement*) { SetExpanded(!m_isExpanded); });

    m_contentHost = std::make_shared<StackPanel>();
    m_contentHost->SetOrientation(Orientation::Vertical);
    m_contentHost->SetGap(8.0f);
    m_contentHost->SetPadding(Thickness(12, 4, 12, 12));
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
    metas.push_back({ "isExpanded", "是否展开 (IsExpanded)", "交互状态", "bool" });
    return metas;
}

void CollapsePanel::SyncHeaderChrome() {
    if (!m_headerButton) {
        return;
    }
    // WinUI Expander-like: title + chevron, no emoji / instructional chrome.
    const char* chevron = (m_expandAnim.Current() > 0.5f) ? "▾  " : "▸  ";
    m_headerButton->SetText(std::string(chevron) + m_headerText);
}

void CollapsePanel::SetHeader(const std::string& header) {
    m_headerText = header;
    SyncHeaderChrome();
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
    // Keep host measurable while collapsing so height can animate down.
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

    Size headerSize = m_headerButton ? m_headerButton->Measure(Size(contentW, contentH)) : Size(0, 0);
    Size bodySize(0, 0);
    if (m_contentHost && (m_isExpanded || m_expandAnim.Current() > 0.01f)) {
        bodySize = m_contentHost->Measure(Size(contentW, 100000.0f));
        m_bodyDesiredHeight = bodySize.height;
    } else {
        m_bodyDesiredHeight = 0.0f;
    }

    const float eased = EaseOutCubic(m_expandAnim.Current());
    const float bodyH = m_bodyDesiredHeight * eased;

    float width = (std::max)(headerSize.width, bodySize.width) + margin.left + margin.right + padding.left + padding.right;
    float height = headerSize.height + bodyH + margin.top + margin.bottom + padding.top + padding.bottom;

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
    float innerH = (std::max)(0.0f, finalRect.height - padding.top - padding.bottom);

    Size headerSize = m_headerButton ? m_headerButton->Measure(Size(innerW, innerH)) : Size(0, 0);
    if (m_headerButton) {
        m_headerButton->Arrange(Rect(innerX, innerY, innerW, headerSize.height));
    }

    if (m_contentHost) {
        const float eased = EaseOutCubic(m_expandAnim.Current());
        float contentY = innerY + headerSize.height;
        float contentH = (std::max)(0.0f, m_bodyDesiredHeight * eased);
        if (contentH < 0.5f) {
            m_contentHost->Arrange(Rect(innerX, contentY, innerW, 0.0f));
        } else {
            m_contentHost->Arrange(Rect(innerX, contentY, innerW, contentH));
        }
    }
}

void CollapsePanel::OnRender(GraphicsContext& ctx) {
    D2D1_COLOR_F bg = ResolveThemeColor(GetBackgroundToken(), ThemeTokenId::CardBackground);
    D2D1_COLOR_F border = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);
    float radius = GetCornerRadius();

    ctx.FillRoundedRect(m_bounds, radius, bg);
    ctx.DrawRoundedRect(m_bounds, radius, border, 1.0f);

    // Hairline under header (Expander chrome).
    if (m_headerButton) {
        const Rect hb = m_headerButton->GetBounds();
        D2D1_COLOR_F line = border;
        line.a = 0.55f;
        ctx.DrawLine(Point(m_bounds.x + 1.0f, hb.y + hb.height),
                     Point(m_bounds.x + m_bounds.width - 1.0f, hb.y + hb.height),
                     line, 1.0f);
    }

    UIElement::OnRender(ctx);
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
