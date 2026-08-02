#include "UIElement.h"
#include "../layout/Layout.h"
#include "../render/CompositionContext.h"
#include <algorithm>

namespace CUI {

bool UIElement::s_animationsEnabled = true;
float UIElement::s_animationDeltaSeconds = 1.0f / 60.0f;

namespace {
bool CanCullElementForCurrentPass(const UIElement* element, const GraphicsContext& ctx) {
    if (!element) {
        return true;
    }

    auto* composition = ctx.GetCompositionContext();
    if (!composition || composition->IsFullRepaint()) {
        return false;
    }

    if (!element->ShouldClipToBounds()) {
        return false;
    }

    const Rect bounds = element->GetBounds();
    return !bounds.IsEmpty() && !ctx.IntersectsPaintBounds(bounds);
}
}

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
    m_renderNode.SetOwner(this);
    m_renderNode.SetBounds(m_bounds);
    OnPropertyChanged().Connect([this](const std::string&, const Value&) {
        MarkRenderContentDirty();
    });
}

std::vector<PropertyMeta> UIElement::GetPropertyMetas() const {
    return {
        { "text", "文本内容 (Text)", "基本信息", "string" },
        { "toolTip", "提示信息 (ToolTip)", "基本信息", "string" },
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
    MarkRenderContentDirty();
}

void UIElement::RemoveChild(std::shared_ptr<UIElement> child) {
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        (*it)->SetParent(nullptr);
        m_children.erase(it);
        MarkRenderContentDirty();
    }
}

void UIElement::RemoveChildRaw(UIElement* child) {
    auto it = std::find_if(m_children.begin(), m_children.end(), [child](const std::shared_ptr<UIElement>& ptr) {
        return ptr.get() == child;
    });
    if (it != m_children.end()) {
        (*it)->SetParent(nullptr);
        m_children.erase(it);
        MarkRenderContentDirty();
    }
}

void UIElement::ClearChildren() {
    for (auto& child : m_children) {
        child->SetParent(nullptr);
    }
    m_children.clear();
    MarkRenderContentDirty();
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

bool UIElement::ShouldClipToBounds() const {
    // Allow scroll content panels to disable clipping so last items aren't cut off
    // when content-height measurement is slightly short.
    if (HasProperty("clipToBounds")) {
        return GetProperty("clipToBounds").AsBool(true);
    }
    return true;
}

void UIElement::Arrange(Rect finalRect) {
    std::string visStr = GetProperty("visibility").AsString("Visible");
    if (visStr == "Collapsed") {
        SetBounds(Rect(0, 0, 0, 0));
        return;
    }

    SetBounds(finalRect);
    LayoutEngine::ArrangeElement(this, finalRect);
}

void UIElement::Render(GraphicsContext& ctx) {
    std::string visStr = GetProperty("visibility").AsString("Visible");
    if (visStr != "Visible") return;
    if (CanCullElementForCurrentPass(this, ctx)) {
        return;
    }

    bool clip = ShouldClipToBounds();
    if (clip) {
        ctx.PushClip(m_bounds);
    }

    if (auto* composition = ctx.GetCompositionContext()) {
        composition->CountRasterizedNode();
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

    if (m_isHovered && m_tooltipVisible) {
        std::string tip = GetToolTip();
        if (!tip.empty()) {
            std::string font = "Segoe UI";
            float fontSize = 12.0f;
            Size textSize = ctx.MeasureText(tip, font, fontSize);

            Thickness padding(8, 5, 8, 5);
            float cardW = textSize.width + padding.left + padding.right;
            float cardH = textSize.height + padding.top + padding.bottom;

            float cardX = m_tooltipAnchorPos.x + 10.0f;
            float cardY = m_tooltipAnchorPos.y + 18.0f;

            if (cardX + cardW > 1200.0f) {
                cardX = m_tooltipAnchorPos.x - cardW - 6.0f;
            }
            if (cardY + cardH > 800.0f) {
                cardY = m_tooltipAnchorPos.y - cardH - 6.0f;
            }
            if (cardX < 4.0f) cardX = 4.0f;
            if (cardY < 4.0f) cardY = 4.0f;

            Rect cardRect(cardX, cardY, cardW, cardH);
            D2D1_COLOR_F bg = D2D1::ColorF(0x2B / 255.0f, 0x2B / 255.0f, 0x2B / 255.0f, 0.95f);
            D2D1_COLOR_F border = D2D1::ColorF(0x45 / 255.0f, 0x45 / 255.0f, 0x45 / 255.0f, 0.90f);
            D2D1_COLOR_F textColor = D2D1::ColorF(0xE0 / 255.0f, 0xE0 / 255.0f, 0xE0 / 255.0f, 1.0f);

            ctx.FillRoundedRect(cardRect, 4.0f, bg);
            ctx.DrawRoundedRect(cardRect, 4.0f, border, 1.0f);

            Rect textRect(cardX + padding.left, cardY + padding.top, textSize.width, textSize.height);
            ctx.DrawText(tip, textRect, textColor, font, fontSize, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }

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
    m_tooltipVisible = false;
    m_lastMouseMoveTime = std::chrono::steady_clock::now();
    MarkRenderContentDirty();
}

void UIElement::OnMouseLeave() {
    m_isHovered = false;
    m_isPressed = false;
    if (m_tooltipVisible) {
        m_tooltipVisible = false;
        MarkRenderContentDirty();
    }
    MarkRenderContentDirty();
}

void UIElement::OnMouseDown(Point pt) {
    m_isPressed = true;
    m_onMouseDownEvent.Invoke(this, pt);
    MarkRenderContentDirty();
}

void UIElement::OnMouseUp(Point pt) {
    if (m_isPressed) {
        m_isPressed = false;
        if (m_bounds.Contains(pt.x, pt.y)) {
            m_onClickEvent.Invoke(this);
        }
        MarkRenderContentDirty();
    }
}

void UIElement::OnMouseMove(Point pt) {
    float dx = pt.x - m_lastMousePos.x;
    float dy = pt.y - m_lastMousePos.y;
    float distSq = dx * dx + dy * dy;
    m_lastMousePos = pt;

    if (distSq > 4.0f) {
        m_lastMouseMoveTime = std::chrono::steady_clock::now();
        if (m_tooltipVisible) {
            m_tooltipVisible = false;
            MarkRenderContentDirty();
        }
    }
}

void UIElement::OnMouseWheel(float delta) {
    if (m_parent) {
        m_parent->OnMouseWheel(delta);
    }
}

void UIElement::OnKeyDown(int vkCode) {
}

bool UIElement::OnAnimationTick() {
    std::string visStr = GetProperty("visibility").AsString("Visible");
    if (visStr != "Visible") {
        return false;
    }

    bool any = false;

    if (m_isHovered && !GetToolTip().empty()) {
        auto now = std::chrono::steady_clock::now();
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastMouseMoveTime).count();
        if (elapsedMs >= 450) {
            if (!m_tooltipVisible) {
                m_tooltipVisible = true;
                m_tooltipAnchorPos = m_lastMousePos;
                MarkRenderContentDirty();
            }
        } else {
            any = true;
        }
    }

    for (auto& child : m_children) {
        if (child && child->OnAnimationTick()) {
            any = true;
        }
    }
    return any;
}

void UIElement::CollectAnimationBounds(Rect& dirtyRect, bool& hasDirty) const {
    std::string visStr = GetProperty("visibility").AsString("Visible");
    if (visStr != "Visible") {
        return;
    }

    if (HasSelfAnimation() && !m_bounds.IsEmpty()) {
        dirtyRect = hasDirty ? dirtyRect.Union(m_bounds) : m_bounds;
        hasDirty = true;
    }

    for (const auto& child : m_children) {
        if (child) {
            child->CollectAnimationBounds(dirtyRect, hasDirty);
        }
    }
}

void UIElement::CollectRenderDirtyRegion(DirtyRegion& dirtyRegion, bool consume) {
    if (consume) {
        dirtyRegion.UnionWith(m_renderNode.ConsumeWorldDirtyRegion());
    } else {
        dirtyRegion.UnionWith(m_renderNode.GetWorldDirtyRegion());
    }

    for (auto& child : m_children) {
        if (child) {
            child->CollectRenderDirtyRegion(dirtyRegion, consume);
        }
    }
}

void UIElement::SetAnimationsEnabled(bool enabled) {
    s_animationsEnabled = enabled;
}

bool UIElement::AreAnimationsEnabled() {
    return s_animationsEnabled;
}

void UIElement::SetAnimationDeltaSeconds(float dtSeconds) {
    s_animationDeltaSeconds = std::clamp(dtSeconds, 1.0f / 240.0f, 0.050f);
}

float UIElement::GetAnimationDeltaSeconds() {
    return s_animationDeltaSeconds;
}

void UIElement::SetBounds(const Rect& bounds) {
    if (bounds.x == m_bounds.x && bounds.y == m_bounds.y
        && bounds.width == m_bounds.width && bounds.height == m_bounds.height) {
        return;
    }

    m_bounds = bounds;
    m_renderNode.SetBounds(m_bounds);
}

void UIElement::SyncRenderState() {
    m_renderNode.SetOwner(this);
    m_renderNode.SetBounds(m_bounds);
    m_renderNode.SyncLayerState();
    for (auto& child : m_children) {
        if (child) {
            child->SyncRenderState();
        }
    }
}

void UIElement::MarkRenderContentDirty() {
    m_renderNode.MarkContentDirty();
    if (m_parent) {
        m_parent->MarkRenderContentDirty();
    }
}

void UIElement::MarkRenderRectDirty(const Rect& rect) {
    m_renderNode.MarkDirtyRect(rect);
    if (m_parent) {
        m_parent->MarkRenderRectDirty(rect);
    }
}

} // namespace CUI
