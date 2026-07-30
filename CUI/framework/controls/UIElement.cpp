#include "UIElement.h"
#include "../layout/Layout.h"
#include <algorithm>

namespace CUI {

UIElement::UIElement() {
    // Default properties
    SetProperty("width", Value(-1.0f)); // -1 means auto
    SetProperty("height", Value(-1.0f));
    SetProperty("minWidth", Value(0.0f));
    SetProperty("minHeight", Value(0.0f));
    SetProperty("margin", Value(Thickness(0)));
    SetProperty("padding", Value(Thickness(0)));
    SetProperty("visibility", Value("Visible"));
    SetProperty("background", Value(D2D1::ColorF(0, 0, 0, 0)));
    SetProperty("borderBrush", Value(D2D1::ColorF(0, 0, 0, 0)));
    SetProperty("borderThickness", Value(0.0f));
    SetProperty("flexGrow", Value(0.0f));
    SetProperty("align", Value("Stretch"));
}

std::vector<PropertyMeta> UIElement::GetPropertyMetas() const {
    return {
        { "text", "文本内容 (Text)", "基本信息", "string" },
        { "width", "宽度 (Width) [-1自适应]", "尺寸布局", "number" },
        { "height", "高度 (Height) [-1自适应]", "尺寸布局", "number" },
        { "minWidth", "最小宽度 (MinWidth)", "尺寸布局", "number" },
        { "minHeight", "最小高度 (MinHeight)", "尺寸布局", "number" },
        { "margin", "外边距 (Margin)", "尺寸布局", "string" },
        { "padding", "内边距 (Padding)", "尺寸布局", "string" },
        { "alignHorizontal", "水平对齐 (AlignH)", "尺寸布局", "enum", { "Stretch", "Start", "Center", "End" } },
        { "alignVertical", "垂直对齐 (AlignV)", "尺寸布局", "enum", { "Stretch", "Start", "Center", "End" } },
        { "background", "背景颜色 (Background)", "色彩外观", "color" },
        { "hoverBackground", "悬停背景色 (HoverBg)", "色彩外观", "color" },
        { "pressedBackground", "按下背景色 (PressedBg)", "色彩外观", "color" },
        { "disabledBackground", "禁用背景色 (DisabledBg)", "色彩外观", "color" },
        { "borderBrush", "边框颜色 (BorderBrush)", "色彩外观", "color" },
        { "borderThickness", "边框粗细 (BorderThickness)", "色彩外观", "number" },
        { "cornerRadius", "圆角半径 (CornerRadius)", "色彩外观", "number" },
        { "opacity", "不透明度 (Opacity) [0-1]", "色彩外观", "number" },
        { "isEnabled", "是否启用 (IsEnabled)", "交互状态", "bool" },
        { "visibility", "显示状态 (Visibility)", "交互状态", "enum", { "Visible", "Hidden", "Collapsed" } }
    };
}

void UIElement::AddChild(std::shared_ptr<UIElement> child) {
    if (!child) return;
    child->SetParent(this);
    m_children.push_back(child);
}

void UIElement::RemoveChild(std::shared_ptr<UIElement> child) {
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        (*it)->SetParent(nullptr);
        m_children.erase(it);
    }
}

void UIElement::ClearChildren() {
    for (auto& child : m_children) {
        child->SetParent(nullptr);
    }
    m_children.clear();
}

std::shared_ptr<UIElement> UIElement::FindElementById(const std::string& id) {
    if (m_id == id) {
        return std::static_pointer_cast<UIElement>(shared_from_this());
    }
    for (auto& child : m_children) {
        auto found = child->FindElementById(id);
        if (found) return found;
    }
    return nullptr;
}

Size UIElement::Measure(Size availableSize) {
    std::string visStr = GetProperty("visibility").AsString("Visible");
    if (visStr == "Collapsed") {
        m_desiredSize = Size(0, 0);
        return m_desiredSize;
    }

    m_desiredSize = LayoutEngine::MeasureElement(this, availableSize);
    return m_desiredSize;
}

void UIElement::Arrange(Rect finalRect) {
    std::string visStr = GetProperty("visibility").AsString("Visible");
    if (visStr == "Collapsed") {
        m_bounds = Rect(0, 0, 0, 0);
        return;
    }

    m_bounds = finalRect;
    LayoutEngine::ArrangeElement(this, finalRect);
}

void UIElement::Render(GraphicsContext& ctx) {
    std::string visStr = GetProperty("visibility").AsString("Visible");
    if (visStr != "Visible") return;

    bool clip = ShouldClipToBounds();
    if (clip) {
        ctx.PushClip(m_bounds);
    }

    OnRender(ctx);

    for (auto& child : m_children) {
        child->Render(ctx);
    }

    if (clip) {
        ctx.PopClip();
    }
}

void UIElement::RenderOverlay(GraphicsContext& ctx) {
    std::string visStr = GetProperty("visibility").AsString("Visible");
    if (visStr != "Visible") return;

    OnRenderOverlay(ctx);

    for (auto& child : m_children) {
        child->RenderOverlay(ctx);
    }
}

void UIElement::OnRender(GraphicsContext& ctx) {
    float radius = GetProperty("cornerRadius").AsFloat(0.0f);

    // Draw background
    D2D1_COLOR_F bg = GetProperty("background").AsColor(D2D1::ColorF(0, 0, 0, 0));
    if (bg.a > 0.0f) {
        if (radius > 0.0f) {
            ctx.FillRoundedRect(m_bounds, radius, bg);
        } else {
            ctx.FillRect(m_bounds, bg);
        }
    }

    // Draw border
    D2D1_COLOR_F borderBrush = GetProperty("borderBrush").AsColor(D2D1::ColorF(0, 0, 0, 0));
    float borderThickness = GetProperty("borderThickness").AsFloat(0.0f);
    if (borderBrush.a > 0.0f && borderThickness > 0.0f) {
        if (radius > 0.0f) {
            ctx.DrawRoundedRect(m_bounds, radius, borderBrush, borderThickness);
        } else {
            ctx.DrawRect(m_bounds, borderBrush, borderThickness);
        }
    }
}

UIElement* UIElement::HitTestOverlay(float x, float y) {
    std::string visStr = GetProperty("visibility").AsString("Visible");
    if (visStr != "Visible") return nullptr;

    UIElement* selfOverlay = OnHitTestOverlay(x, y);
    if (selfOverlay) return selfOverlay;

    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        UIElement* hit = (*it)->HitTestOverlay(x, y);
        if (hit) return hit;
    }

    return nullptr;
}

UIElement* UIElement::HitTest(float x, float y) {
    std::string visStr = GetProperty("visibility").AsString("Visible");
    if (visStr != "Visible") return nullptr;

    // 1. Prioritize active overlays (open dropdowns, context menus, popups, content dialogs)
    UIElement* overlayHit = HitTestOverlay(x, y);
    if (overlayHit) return overlayHit;

    // 2. Check children first (topmost first)
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        UIElement* hit = (*it)->HitTest(x, y);
        if (hit) return hit;
    }

    if (m_bounds.Contains(x, y)) {
        return this;
    }

    return nullptr;
}

void UIElement::OnMouseEnter() {
    m_isHovered = true;
}

void UIElement::OnMouseLeave() {
    m_isHovered = false;
    m_isPressed = false;
}

void UIElement::OnMouseDown(Point pt) {
    m_isPressed = true;
    m_onMouseDownEvent.Invoke(this, pt);
}

void UIElement::OnMouseUp(Point pt) {
    if (m_isPressed) {
        m_isPressed = false;
        if (m_bounds.Contains(pt.x, pt.y)) {
            m_onClickEvent.Invoke(this);
        }
    }
}

void UIElement::OnMouseMove(Point pt) {
}

void UIElement::OnMouseWheel(float delta) {
    if (m_parent) {
        m_parent->OnMouseWheel(delta);
    }
}

void UIElement::OnKeyDown(int vkCode) {
}

} // namespace CUI
