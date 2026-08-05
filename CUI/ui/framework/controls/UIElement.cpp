#include "UIElement.h"
#include "../animation/AnimationManager.h"
#include "../layout/Layout.h"
#include "../render/CompositionContext.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cassert>

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
} // namespace

UIElement::UIElement() {
    // Defaults live in members — no string property bag.
    m_renderNode.SetOwner(this);
    m_renderNode.SetBounds(m_bounds);
    OnPropertyIdChanged().Connect([this](PropertyId, const Value&) {
        MarkRenderContentDirty();
    });
}

UIElement::~UIElement() {
    CancelAnimationTicks();
    // Detach children so surviving shared_ptrs do not keep a dangling m_parent.
    // Avoid ClearChildren() — it also marks render dirty during teardown.
    for (auto& child : m_children) {
        if (child && child->GetParent() == this) {
            child->SetParent(nullptr);
        }
    }
    m_children.clear();
}

void UIElement::SetParent(UIElement* parent) {
#ifdef _DEBUG
    assert(parent != this);
    if (parent) {
        for (UIElement* walk = parent->GetParent(); walk; walk = walk->GetParent()) {
            assert(walk != this && "SetParent would create a cycle");
        }
    }
#endif
    m_parent = parent;
}

void UIElement::RequestAnimationTicks() {
    if (m_animationTicksRegistered) {
        return;
    }
    if (AnimationManager* mgr = AnimationManager::Current()) {
        mgr->RegisterAnimating(this);
        m_animationTicksRegistered = true;
    }
}

void UIElement::CancelAnimationTicks() {
    if (!m_animationTicksRegistered) {
        return;
    }
    if (AnimationManager* mgr = AnimationManager::Current()) {
        mgr->UnregisterAnimating(this);
    }
    m_animationTicksRegistered = false;
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
    if (m_visibility == Visibility::Collapsed) {
        m_desiredSize = Size(0, 0);
        return m_desiredSize;
    }

    m_desiredSize = LayoutEngine::MeasureElement(this, availableSize);
    return m_desiredSize;
}

bool UIElement::ShouldClipToBounds() const {
    return m_clipToBounds;
}

void UIElement::Arrange(Rect finalRect) {
    if (m_visibility == Visibility::Collapsed) {
        SetBounds(Rect(0, 0, 0, 0));
        return;
    }

    SetBounds(finalRect);
    LayoutEngine::ArrangeElement(this, finalRect);
}

void UIElement::Render(GraphicsContext& ctx) {
    if (m_visibility != Visibility::Visible) return;
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
    if (m_visibility != Visibility::Visible) return;

    OnRenderOverlay(ctx);

    if (m_isHovered && m_tooltipVisible) {
        const std::string& tip = GetToolTip();
        if (!tip.empty()) {
            std::string font = "Segoe UI";
            float fontSize = 12.0f;
            Size textSize = ctx.MeasureText(tip, font, fontSize);

            Thickness padding(8, 5, 8, 5);
            float cardW = textSize.width + padding.left + padding.right + 2.0f;
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
            D2D1_COLOR_F bg = ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBackground);
            D2D1_COLOR_F border = ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBorder);
            D2D1_COLOR_F textColor = ThemeManager::Instance().GetFlatColor(ThemeTokenId::TextPrimary);

            ctx.FillRoundedRect(cardRect, 4.0f, bg);
            ctx.DrawRoundedRect(cardRect, 4.0f, border, 1.0f);

            Rect textRect(cardX + padding.left, cardY + padding.top, textSize.width + 2.0f, textSize.height);
            ctx.DrawText(tip, textRect, textColor, font, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }

    for (auto& child : m_children) {
        child->RenderOverlay(ctx);
    }
}

void UIElement::OnRender(GraphicsContext& ctx) {
    const float radius = m_cornerRadius;

    D2D1_COLOR_F bg = (m_backgroundToken != ThemeTokenId::Unset)
        ? ResolveThemeColor(m_backgroundToken, ThemeTokenId::CardBackground)
        : (m_hasBackgroundColor ? m_backgroundColor : D2D1::ColorF(0, 0, 0, 0));
    if (bg.a > 0.0f) {
        if (radius > 0.0f) {
            ctx.FillRoundedRect(m_bounds, radius, bg);
        } else {
            ctx.FillRect(m_bounds, bg);
        }
    }

    D2D1_COLOR_F borderBrush = (m_borderToken != ThemeTokenId::Unset)
        ? ResolveThemeColor(m_borderToken, ThemeTokenId::CardBorder)
        : (m_hasBorderBrushColor ? m_borderBrushColor : D2D1::ColorF(0, 0, 0, 0));
    const float borderThickness = m_borderThickness;
    if (borderBrush.a > 0.0f && borderThickness > 0.0f) {
        if (radius > 0.0f) {
            ctx.DrawRoundedRect(m_bounds, radius, borderBrush, borderThickness);
        } else {
            ctx.DrawRect(m_bounds, borderBrush, borderThickness);
        }
    }
}

UIElement* UIElement::HitTestOverlay(float x, float y) {
    if (m_visibility != Visibility::Visible) return nullptr;

    UIElement* selfOverlay = OnHitTestOverlay(x, y);
    if (selfOverlay) return selfOverlay;

    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        UIElement* hit = (*it)->HitTestOverlay(x, y);
        if (hit) return hit;
    }

    return nullptr;
}

UIElement* UIElement::HitTest(float x, float y) {
    if (m_visibility != Visibility::Visible) return nullptr;

    UIElement* overlayHit = HitTestOverlay(x, y);
    if (overlayHit) return overlayHit;

    if (ShouldClipToBounds() && !m_bounds.Contains(x, y)) {
        return nullptr;
    }

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
    if (!GetToolTip().empty()) {
        RequestAnimationTicks();
    }
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
    if (!IsEnabled()) {
        return;
    }
    m_isPressed = true;
    m_onMouseDownEvent.Invoke(this, pt);
    MarkRenderContentDirty();
}

void UIElement::OnMouseUp(Point pt) {
    if (m_isPressed) {
        m_isPressed = false;
        if (IsEnabled() && m_bounds.Contains(pt.x, pt.y)) {
            m_onClickEvent.Invoke(this);
        }
        MarkRenderContentDirty();
    }
}

void UIElement::OnMouseMove(Point pt) {
    if (!IsEnabled()) {
        return;
    }
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
    (void)vkCode;
    if (!IsEnabled()) {
        return;
    }
}

bool UIElement::OnAnimationTick() {
    if (m_visibility != Visibility::Visible) {
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
    if (m_visibility != Visibility::Visible) {
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
