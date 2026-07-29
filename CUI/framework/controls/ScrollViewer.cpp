#include "ScrollViewer.h"

namespace CUI {

ScrollViewer::ScrollViewer() {
    SetProperty("background", Value(D2D1::ColorF(0, 0, 0, 0)));
}

void ScrollViewer::OnRender(GraphicsContext& ctx) {
    UIElement::OnRender(ctx);

    // Render scrollbar indicator if content exceeds view
    if (m_contentHeight > m_bounds.height && m_bounds.height > 0.0f) {
        float trackHeight = m_bounds.height;
        float thumbHeight = (m_bounds.height / m_contentHeight) * trackHeight;
        if (thumbHeight < 20.0f) thumbHeight = 20.0f;

        float maxScroll = m_contentHeight - m_bounds.height;
        float scrollRatio = m_offsetY / maxScroll;
        float thumbY = m_bounds.y + scrollRatio * (trackHeight - thumbHeight);

        Rect thumbRect(m_bounds.x + m_bounds.width - 6.0f, thumbY, 4.0f, thumbHeight);
        ctx.FillRoundedRect(thumbRect, 2.0f, D2D1::ColorF(0x79 / 255.0f, 0x79 / 255.0f, 0x79 / 255.0f, 0.4f));
    }
}

void ScrollViewer::OnMouseMove(Point pt) {
    UIElement::OnMouseMove(pt);
}

} // namespace CUI
