#include "CollapsePanel.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <vector>
#include <windows.h>

namespace CUI {

CollapsePanel::CollapsePanel() {
    // 外壳用 pane 玻璃，避免大块实心 card 盖死材质。
    SetProperty("theme.backgroundToken", Value("paneBackground"));
    SetProperty("theme.borderToken", Value("cardBorder"));
    SetProperty("borderThickness", Value(1.0f));
    SetProperty("cornerRadius", Value(6.0f));
    SetProperty("padding", Value(Thickness(0)));

    m_headerButton = std::make_shared<Button>();
    m_headerButton->SetProperty("theme.backgroundToken", Value("paneBackground"));
    m_headerButton->SetProperty("theme.hoverBackgroundToken", Value("hoverBackground"));
    m_headerButton->SetProperty("theme.pressedBackgroundToken", Value("pressedBackground"));
    m_headerButton->SetProperty("theme.colorToken", Value("textPrimary"));
    m_headerButton->SetProperty("theme.borderToken", Value("cardBorder"));
    m_headerButton->SetProperty("borderThickness", Value(0.0f));
    m_headerButton->SetProperty("cornerRadius", Value(6.0f));
    m_headerButton->SetProperty("padding", Value(Thickness(12, 10, 12, 10)));
    m_headerButton->SetProperty("align", Value("Stretch"));
    m_headerButton->OnClick().Connect([this](UIElement*) { SetExpanded(!m_isExpanded); });

    m_contentHost = std::make_shared<StackPanel>();
    m_contentHost->SetProperty("orientation", Value("Vertical"));
    m_contentHost->SetProperty("gap", Value(8.0f));
    m_contentHost->SetProperty("padding", Value(Thickness(12, 12, 12, 12)));
    m_contentHost->SetProperty("align", Value("Stretch"));
    // 内容宿主透明，露出外层 pane / 底下 SystemBackdrop。
    m_contentHost->SetProperty("background", Value(D2D1::ColorF(0, 0, 0, 0)));

    AddChild(m_headerButton);
    AddChild(m_contentHost);
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

void CollapsePanel::SetHeader(const std::string& header) {
    m_headerText = header;
    if (m_headerButton) {
        m_headerButton->SetProperty("text", Value(m_isExpanded ? "📂 " + header + "  [▲ 点击折叠]" : "📁 " + header + "  [▼ 点击展开]"));
    }
}

void CollapsePanel::SetExpanded(bool expanded) {
    if (m_isExpanded != expanded) {
        m_isExpanded = expanded;
        SetHeader(m_headerText);
        UpdateContentVisibility();
        m_onExpandedChangedEvent.Invoke(this, m_isExpanded);
        InvalidateParentLayout();
    }
}

void CollapsePanel::InvalidateParentLayout() {
    MarkRenderContentDirty();

    // Re-measure/arrange the ancestor chain so collapsed height actually releases space,
    // and expanded height is picked up without requiring a page switch.
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
    if (m_contentHost) {
        m_contentHost->SetProperty("visibility", Value(m_isExpanded ? "Visible" : "Collapsed"));
    }
}

Size CollapsePanel::Measure(Size availableSize) {
    Thickness margin = GetProperty("margin").AsThickness(Thickness(0));
    Thickness padding = GetProperty("padding").AsThickness(Thickness(0));

    float contentW = (std::max)(0.0f, availableSize.width - margin.left - margin.right - padding.left - padding.right);
    float contentH = (std::max)(0.0f, availableSize.height - margin.top - margin.bottom - padding.top - padding.bottom);

    Size headerSize = m_headerButton ? m_headerButton->Measure(Size(contentW, contentH)) : Size(0, 0);
    Size bodySize(0, 0);
    if (m_isExpanded && m_contentHost) {
        // Give body unbounded height so it can report true desired size for ScrollViewer parents.
        bodySize = m_contentHost->Measure(Size(contentW, 100000.0f));
    }

    float width = (std::max)(headerSize.width, bodySize.width) + margin.left + margin.right + padding.left + padding.right;
    float height = headerSize.height + (m_isExpanded ? bodySize.height : 0.0f) + margin.top + margin.bottom + padding.top + padding.bottom;

    float expW = GetProperty("width").AsFloat(-1.0f);
    float expH = GetProperty("height").AsFloat(-1.0f);
    if (expW >= 0.0f) width = expW;
    if (expH >= 0.0f) height = expH;

    m_desiredSize = Size(width, height);
    return m_desiredSize;
}

void CollapsePanel::Arrange(Rect finalRect) {
    m_bounds = finalRect;

    Thickness padding = GetProperty("padding").AsThickness(Thickness(0));
    float innerX = finalRect.x + padding.left;
    float innerY = finalRect.y + padding.top;
    float innerW = (std::max)(0.0f, finalRect.width - padding.left - padding.right);
    float innerH = (std::max)(0.0f, finalRect.height - padding.top - padding.bottom);

    Size headerSize = m_headerButton ? m_headerButton->Measure(Size(innerW, innerH)) : Size(0, 0);
    if (m_headerButton) {
        m_headerButton->Arrange(Rect(innerX, innerY, innerW, headerSize.height));
    }

    if (m_contentHost) {
        if (m_isExpanded) {
            float contentY = innerY + headerSize.height;
            float contentH = (std::max)(0.0f, innerH - headerSize.height);
            // Prefer content's desired height when parent gave us exact measured height.
            Size bodyDesired = m_contentHost->GetDesiredSize();
            if (bodyDesired.height > 0.0f && bodyDesired.height < contentH) {
                contentH = bodyDesired.height;
            }
            m_contentHost->Arrange(Rect(innerX, contentY, innerW, contentH));
        } else {
            m_contentHost->Arrange(Rect(innerX, innerY + headerSize.height, innerW, 0.0f));
        }
    }
}

void CollapsePanel::OnRender(GraphicsContext& ctx) {
    D2D1_COLOR_F bg = ResolveThemeColor("theme.backgroundToken", "paneBackground");
    D2D1_COLOR_F border = ResolveThemeColor("theme.borderToken", "cardBorder");
    float radius = GetProperty("cornerRadius").AsFloat(6.0f);

    ctx.FillRoundedRect(m_bounds, radius, bg);
    ctx.DrawRoundedRect(m_bounds, radius, border, 1.0f);

    UIElement::OnRender(ctx);
}

void CollapsePanel::OnMouseDown(Point pt) {
    // Header Button already toggles via OnClick — do not double-toggle here.
    if (m_headerButton && m_headerButton->GetBounds().Contains(pt.x, pt.y)) {
        m_headerButton->OnMouseDown(pt);
        return;
    }
    if (m_isExpanded && m_contentHost && m_contentHost->GetBounds().Contains(pt.x, pt.y)) {
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
