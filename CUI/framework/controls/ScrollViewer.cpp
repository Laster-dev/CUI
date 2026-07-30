#include "ScrollViewer.h"
#include <algorithm>

namespace CUI {

ScrollViewer::ScrollViewer() {
    SetProperty("background", Value(D2D1::ColorF(0, 0, 0, 0)));
}

Size ScrollViewer::Measure(Size availableSize) {
    Thickness margin = GetProperty("margin").AsThickness(Thickness(0));
    Thickness padding = GetProperty("padding").AsThickness(Thickness(0));

    Size contentAvail(availableSize.width - margin.left - margin.right - padding.left - padding.right, 100000.0f);
    if (contentAvail.width < 0) contentAvail.width = 0;

    m_contentHeight = 0.0f;
    for (auto& child : GetChildren()) {
        std::string vis = child->GetProperty("visibility").AsString("Visible");
        if (vis == "Collapsed") continue;

        Size dSize = child->Measure(contentAvail);
        m_contentHeight = (std::max)(m_contentHeight, dSize.height);
    }

    float expW = GetProperty("width").AsFloat(-1.0f);
    float expH = GetProperty("height").AsFloat(-1.0f);

    float finalW = (expW >= 0.0f) ? expW : availableSize.width;
    float finalH = (expH >= 0.0f) ? expH : availableSize.height;

    m_desiredSize = Size(finalW, finalH);
    return m_desiredSize;
}

void ScrollViewer::Arrange(Rect finalRect) {
    m_bounds = finalRect;
    Thickness padding = GetProperty("padding").AsThickness(Thickness(0));

    float maxScroll = (std::max)(0.0f, m_contentHeight - m_bounds.height);
    if (m_offsetY > maxScroll) m_offsetY = maxScroll;
    if (m_offsetY < 0.0f) m_offsetY = 0.0f;

    Rect childRect(
        finalRect.x + padding.left,
        finalRect.y + padding.top - m_offsetY,
        finalRect.width - padding.left - padding.right,
        (std::max)(finalRect.height, m_contentHeight)
    );

    for (auto& child : GetChildren()) {
        std::string vis = child->GetProperty("visibility").AsString("Visible");
        if (vis == "Collapsed") continue;

        child->Arrange(childRect);
    }
}

void ScrollViewer::Render(GraphicsContext& ctx) {
    std::string visStr = GetProperty("visibility").AsString("Visible");
    if (visStr != "Visible") return;

    ctx.PushClip(m_bounds);
    OnRender(ctx);

    for (auto& child : GetChildren()) {
        child->Render(ctx);
    }

    ctx.PopClip();
}

void ScrollViewer::OnRender(GraphicsContext& ctx) {
    UIElement::OnRender(ctx);

    // Render scrollbar indicator if content exceeds view
    if (m_contentHeight > m_bounds.height && m_bounds.height > 0.0f) {
        float trackHeight = m_bounds.height;
        float thumbHeight = (m_bounds.height / m_contentHeight) * trackHeight;
        if (thumbHeight < 20.0f) thumbHeight = 20.0f;

        float maxScroll = m_contentHeight - m_bounds.height;
        float scrollRatio = (maxScroll > 0.0f) ? (m_offsetY / maxScroll) : 0.0f;
        float thumbY = m_bounds.y + scrollRatio * (trackHeight - thumbHeight);

        Rect thumbRect(m_bounds.x + m_bounds.width - 6.0f, thumbY, 4.0f, thumbHeight);
        ctx.FillRoundedRect(thumbRect, 2.0f, D2D1::ColorF(0x79 / 255.0f, 0x79 / 255.0f, 0x79 / 255.0f, 0.4f));
    }
}

void ScrollViewer::OnMouseWheel(float delta) {
    float maxScroll = (std::max)(0.0f, m_contentHeight - m_bounds.height);
    m_offsetY -= delta * 40.0f;
    if (m_offsetY < 0.0f) m_offsetY = 0.0f;
    if (m_offsetY > maxScroll) m_offsetY = maxScroll;

    Arrange(m_bounds);
}

} // namespace CUI
