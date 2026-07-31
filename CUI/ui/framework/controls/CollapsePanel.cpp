#include "CollapsePanel.h"
#include <algorithm>

namespace CUI {

CollapsePanel::CollapsePanel() {
    SetProperty("background", Value("#252526"));
    SetProperty("borderBrush", Value("#3E3E42"));
    SetProperty("borderThickness", Value(1.0f));
    SetProperty("cornerRadius", Value(6.0f));
    SetProperty("padding", Value(Thickness(0)));

    m_headerButton = std::make_shared<Button>();
    m_headerButton->SetProperty("background", Value("#2D2D30"));
    m_headerButton->SetProperty("hoverBackground", Value("#3A3D41"));
    m_headerButton->SetProperty("pressedBackground", Value("#007ACC"));
    m_headerButton->SetProperty("color", Value("#E0E0E0"));
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
    m_isExpanded = expanded;
    SetHeader(m_headerText);
    UpdateContentVisibility();
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
        bodySize = m_contentHost->Measure(Size(contentW, (std::max)(0.0f, contentH - headerSize.height)));
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
            m_contentHost->Arrange(Rect(innerX, contentY, innerW, contentH));
        } else {
            m_contentHost->Arrange(Rect(innerX, innerY + headerSize.height, innerW, 0.0f));
        }
    }
}

void CollapsePanel::OnRender(GraphicsContext& ctx) {
    // Draw outer container background & border
    D2D1_COLOR_F bg = GetProperty("background").AsColor(D2D1::ColorF(0x25 / 255.0f, 0x25 / 255.0f, 0x26 / 255.0f, 1.0f));
    D2D1_COLOR_F border = GetProperty("borderBrush").AsColor(D2D1::ColorF(0x3E / 255.0f, 0x3E / 255.0f, 0x42 / 255.0f, 1.0f));
    float radius = GetProperty("cornerRadius").AsFloat(6.0f);

    ctx.FillRoundedRect(m_bounds, radius, bg);
    ctx.DrawRoundedRect(m_bounds, radius, border, 1.0f);

    UIElement::OnRender(ctx);
}

void CollapsePanel::OnMouseDown(Point pt) {
    if (m_headerButton && m_headerButton->GetBounds().Contains(pt.x, pt.y)) {
        SetExpanded(!m_isExpanded);
        return;
    }
    if (m_isExpanded && m_contentHost && m_contentHost->GetBounds().Contains(pt.x, pt.y)) {
        m_contentHost->OnMouseDown(pt);
    }
}

} // namespace CUI
