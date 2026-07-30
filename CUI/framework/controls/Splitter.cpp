#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Splitter.h"

namespace CUI {

Splitter::Splitter() {
    SetProperty("orientation", Value("Vertical"));
    SetProperty("background", Value(D2D1::ColorF(0x2D / 255.0f, 0x2D / 255.0f, 0x2D / 255.0f, 1.0f)));
    SetProperty("hoverBackground", Value(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f)));
    SetProperty("width", Value(6.0f));
    SetProperty("height", Value(200.0f));
}

std::vector<PropertyMeta> Splitter::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "orientation", "拆分方向 (Orientation)", "拆分配置", "enum", { "Vertical", "Horizontal" } });
    return metas;
}

HCURSOR Splitter::GetCursor() const {
    if (!IsEnabled()) return nullptr;
    std::string orient = GetOrientation();
    return LoadCursor(nullptr, (orient == "Vertical") ? IDC_SIZEWE : IDC_SIZENS);
}

Size Splitter::Measure(Size availableSize) {
    std::string orient = GetOrientation();
    float expW = GetProperty("width").AsFloat((orient == "Vertical") ? 6.0f : availableSize.width);
    float expH = GetProperty("height").AsFloat((orient == "Vertical") ? availableSize.height : 6.0f);
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

void Splitter::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);
    m_isDragging = true;
    m_dragStartPt = pt;
}

void Splitter::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);
    if (m_isDragging) {
        std::string orient = GetOrientation();
        float delta = (orient == "Vertical") ? (pt.x - m_dragStartPt.x) : (pt.y - m_dragStartPt.y);
        m_onSplitterMovedEvent.Invoke(this, delta);
        m_dragStartPt = pt;
    }
}

void Splitter::OnMouseUp(Point pt) {
    Control::OnMouseUp(pt);
    m_isDragging = false;
}

void Splitter::OnRender(GraphicsContext& ctx) {
    D2D1_COLOR_F bg = GetProperty("background").AsColor(D2D1::ColorF(0x2D / 255.0f, 0x2D / 255.0f, 0x2D / 255.0f, 1.0f));
    D2D1_COLOR_F hoverBg = GetProperty("hoverBackground").AsColor(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f));

    ctx.FillRect(m_bounds, (m_isHovered || m_isDragging) ? hoverBg : bg);
}

} // namespace CUI
