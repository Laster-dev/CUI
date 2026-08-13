#include "UIElement.h"
#include "../animation/AnimationManager.h"
#include "../animation/FrameScheduler.h"
#include "../layout/Layout.h"
#include "../render/CompositionContext.h"
#include "../render/RenderLayer.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cassert>
#include <cmath>

namespace CUI {

bool UIElement::s_animationsEnabled = true;
float UIElement::s_animationDeltaSeconds = 1.0f / 60.0f;
uint64_t UIElement::s_renderDirtySerial = 0;

namespace {
// Inflate cull bounds so ripples/shadows that draw slightly outside still get painted.
constexpr float kCullBoundsSlop = 8.0f;

bool CanCullElementForCurrentPass(const UIElement* element, const GraphicsContext& ctx) {
    if (!element) {
        return true;
    }

    auto* composition = ctx.GetCompositionContext();
    if (!composition || composition->IsFullRepaint()) {
        return false;
    }

    const Rect bounds = element->GetBounds();
    if (bounds.IsEmpty()) {
        return true;
    }
    return !ctx.IntersectsPaintBounds(bounds.Inflate(kCullBoundsSlop));
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
    if (AnimationManager* mgr = AnimationManager::Current()) {
        mgr->CancelWake(this);
    }
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

void UIElement::SetAnimationHost(UIElement* host) {
    if (host == this) {
        return;
    }
    m_animationHost = host;
    // Attaching the host makes IsInLiveTree true (via host → live root). Re-arm
    // any pending self animation that RequestAnimationTicks dropped earlier
    // while this control looked detached.
    if (m_animationHost && HasSelfAnimation()) {
        RequestAnimationTicks();
    }
}

void UIElement::RequestAnimationTicks() {
    // May no-op if AnimationManager live-tree gate rejects this element (detached
    // after page swap, or popup-hosted without SetAnimationHost). Check
    // IsAnimationTicksRegistered() / IsInLiveTree if ticks never arrive.
    if (AnimationManager* mgr = AnimationManager::Current()) {
        if (m_animationTicksRegistered && !mgr->IsRegistered(this)) {
            // Flag/list desync — recover so pending work (e.g. Nav content swap) can run.
            m_animationTicksRegistered = false;
        }
        if (!m_animationTicksRegistered) {
            mgr->RegisterAnimating(this);
            m_animationTicksRegistered = mgr->IsRegistered(this);
        }
        // Always wake a frame. Early-return when already registered used to skip
        // ScheduleFrame, so SetContentFactory after a settled Nav never applied.
        if (FrameScheduler* sched = FrameScheduler::Current()) {
            sched->ScheduleFrame();
        }
    }
}

void UIElement::InvalidateMeasure() {
    // Controls that override Measure without clearing m_measureDirty can stay
    // dirty forever. The old early-out then never walked to a clean parent, so
    // nested hosts (e.g. Expander inside a Column) kept a stale DesiredSize.
    if (m_measureDirty) {
        if (m_parent && !m_parent->m_measureDirty) {
            m_parent->InvalidateMeasure();
        }
        return;
    }
    m_measureDirty = true;
    m_arrangeDirty = true;
    if (m_parent) {
        m_parent->InvalidateMeasure();
    }
}

void UIElement::InvalidateArrange() {
    if (m_arrangeDirty) {
        return;
    }
    m_arrangeDirty = true;
    if (m_parent) {
        m_parent->InvalidateArrange();
    }
}

void UIElement::FlushLayout(Size availableSize, const Rect& arrangeRect) {
    if (m_visibility == Visibility::Collapsed) {
        SetBounds(Rect(0, 0, 0, 0));
        m_measureDirty = false;
        m_arrangeDirty = false;
        return;
    }

    if (m_measureDirty) {
        Measure(availableSize);
        m_measureDirty = false;
        m_arrangeDirty = true;
    }

    if (m_arrangeDirty) {
        Arrange(arrangeRect);
        m_arrangeDirty = false;
    } else {
        // Children may still be dirty even if this node is not.
        for (auto& child : m_children) {
            if (child && (child->m_measureDirty || child->m_arrangeDirty)) {
                child->FlushLayout(
                    Size(child->m_bounds.width, child->m_bounds.height),
                    child->m_bounds);
            }
        }
    }
}

void UIElement::PromoteLayer(bool promote) {
    m_layerPromoted = promote;
    m_renderNode.GetLayer().SetCacheable(promote);
    if (promote) {
        m_composeOpacity = m_opacity;
        m_renderNode.GetLayer().Invalidate(RenderLayer::ContentDirty | RenderLayer::OpacityDirty);
    }
}

void UIElement::SetComposeOpacity(float opacity) {
    opacity = std::clamp(opacity, 0.0f, 1.0f);
    if (std::abs(opacity - m_composeOpacity) < 0.0005f) {
        return;
    }
    m_composeOpacity = opacity;
    if (m_layerPromoted) {
        m_opacity = opacity;
        m_composeDirty = true;
        m_renderNode.GetLayer().Invalidate(RenderLayer::OpacityDirty);
        // Footprint for commit — old and new are same bounds for opacity-only.
        MarkRenderRectDirty(m_bounds.Inflate(2.0f));
    } else {
        SetOpacity(opacity);
    }
}

void UIElement::SetComposeOffset(float x, float y) {
    if (std::abs(x - m_composeOffsetX) < 0.01f && std::abs(y - m_composeOffsetY) < 0.01f) {
        return;
    }
    Rect oldFootprint = Rect(
        m_bounds.x + m_composeOffsetX,
        m_bounds.y + m_composeOffsetY,
        m_bounds.width,
        m_bounds.height).Inflate(2.0f);
    m_composeOffsetX = x;
    m_composeOffsetY = y;
    m_composeDirty = true;
    if (m_layerPromoted) {
        m_renderNode.GetLayer().SetTranslation(x, y);
        m_renderNode.GetLayer().Invalidate(RenderLayer::TransformDirty);
    }
    Rect newFootprint = Rect(
        m_bounds.x + m_composeOffsetX,
        m_bounds.y + m_composeOffsetY,
        m_bounds.width,
        m_bounds.height).Inflate(2.0f);
    MarkRenderRectDirty(oldFootprint.Union(newFootprint));
}

void UIElement::OnNavigatedTo() {
    ResumeAnimationSubtree();
}

void UIElement::OnNavigatedFrom() {
    PauseAnimationSubtree();
}

void UIElement::PauseAnimationSubtree() {
    CancelAnimationTicks();
    if (AnimationManager* mgr = AnimationManager::Current()) {
        mgr->CancelWake(this);
    }
    for (auto& child : m_children) {
        if (child) {
            child->PauseAnimationSubtree();
        }
    }
}

void UIElement::ResumeAnimationSubtree() {
    if (HasSelfAnimation()) {
        RequestAnimationTicks();
    }
    for (auto& child : m_children) {
        if (child) {
            child->ResumeAnimationSubtree();
        }
    }
}

void UIElement::OnRoutedEvent(RoutedEventArgs& args) {
    if (args.handled) {
        return;
    }
    if (args.phase == RoutedEventPhase::Tunnel) {
        return;
    }
    switch (args.type) {
    case RoutedEventType::PointerPressed:
        OnMouseDown(args.position);
        break;
    case RoutedEventType::PointerReleased:
        OnMouseUp(args.position);
        break;
    case RoutedEventType::PointerMoved:
        OnMouseMove(args.position);
        break;
    case RoutedEventType::KeyDown:
        OnKeyDown(args.keyCode);
        break;
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
    InvalidateMeasure();
}

void UIElement::AddChildQuiet(std::shared_ptr<UIElement> child) {
    if (!child) return;
    child->SetParent(this);
    m_children.push_back(child);
}

void UIElement::RemoveChild(std::shared_ptr<UIElement> child) {
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        (*it)->SetParent(nullptr);
        m_children.erase(it);
        MarkRenderContentDirty();
        InvalidateMeasure();
    }
}

void UIElement::RemoveChildQuiet(std::shared_ptr<UIElement> child) {
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        (*it)->SetParent(nullptr);
        m_children.erase(it);
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
        m_measureDirty = false;
        m_lastMeasureAvailable = availableSize;
        return m_desiredSize;
    }

    // Skip re-measure when layout is clean and constraints unchanged (Relayout thrash).
    if (!m_measureDirty
        && std::abs(m_lastMeasureAvailable.width - availableSize.width) < 0.5f
        && std::abs(m_lastMeasureAvailable.height - availableSize.height) < 0.5f) {
        return m_desiredSize;
    }

    m_lastMeasureAvailable = availableSize;
    m_desiredSize = LayoutEngine::MeasureElement(this, availableSize);
    m_measureDirty = false;
    return m_desiredSize;
}

bool UIElement::ShouldClipToBounds() const {
    return m_clipToBounds;
}

void UIElement::Arrange(Rect finalRect) {
    if (m_visibility == Visibility::Collapsed) {
        SetBounds(Rect(0, 0, 0, 0));
        m_arrangeDirty = false;
        return;
    }

    // Layout slot is finalRect; visual/hit-test bounds exclude Margin (WPF-style).
    // Without this, Grid/UniformGrid fill the whole cell and SetMargin appears ignored.
    const Thickness margin = GetMargin();
    Rect arranged(
        finalRect.x + margin.left,
        finalRect.y + margin.top,
        (std::max)(0.0f, finalRect.width - margin.left - margin.right),
        (std::max)(0.0f, finalRect.height - margin.top - margin.bottom));
    SetBounds(arranged);
    LayoutEngine::ArrangeElement(this, arranged);
    m_arrangeDirty = false;
}

void UIElement::Render(GraphicsContext& ctx) {
    if (m_visibility != Visibility::Visible) return;
    const float drawOpacity = m_layerPromoted ? m_composeOpacity : m_opacity;
    if (drawOpacity <= 0.001f) return;
    if (CanCullElementForCurrentPass(this, ctx)) {
        return;
    }

    auto& layer = m_renderNode.GetLayer();
    if (m_layerPromoted && layer.IsCacheable()) {
        layer.SetBounds(m_bounds);
        const bool needRaster = layer.NeedsContentRaster();
        if (needRaster) {
            const float w = (std::max)(1.0f, std::ceil(m_bounds.width));
            const float h = (std::max)(1.0f, std::ceil(m_bounds.height));
            if (ctx.PushLayerTarget(
                    layer,
                    Size(w, h),
                    Rect(0.0f, 0.0f, w, h),
                    D2D1::ColorF(0, 0, 0, 0),
                    true)) {
                // Record in local space so the bitmap is (0,0)-(w,h).
                // Paint-bounds cull uses world coords — disable for this pass.
                const Rect savedPaintBounds = ctx.GetPaintBounds();
                ctx.SetPaintBounds(Rect());
                ctx.PushTransform(D2D1::Matrix3x2F::Translation(-m_bounds.x, -m_bounds.y));
                if (auto* composition = ctx.GetCompositionContext()) {
                    composition->CountRasterizedNode();
                    composition->CountLayerCacheMiss();
                }
                OnRender(ctx);
                for (auto& child : m_children) {
                    if (child && child->PresentsOnOwnerWindow()) {
                        child->Render(ctx);
                    }
                }
                ctx.PopTransform();
                ctx.SetPaintBounds(savedPaintBounds);
                ctx.PopLayerTarget(layer);
                layer.Validate();
            }
        } else if (auto* composition = ctx.GetCompositionContext()) {
            composition->CountLayerCacheHit();
            composition->CountLayerCacheReuse();
        }

        if (layer.GetCacheBitmap()) {
            const Rect dest(
                m_bounds.x + m_composeOffsetX,
                m_bounds.y + m_composeOffsetY,
                m_bounds.width,
                m_bounds.height);
            ctx.DrawLayer(layer, dest, nullptr, drawOpacity);
            layer.ClearDirtyFlags(RenderLayer::OpacityDirty | RenderLayer::TransformDirty);
            m_composeDirty = false;
            return;
        }
        // Fall through to immediate path if layer alloc failed.
    }

    const bool useOffset = std::abs(m_composeOffsetX) > 0.01f || std::abs(m_composeOffsetY) > 0.01f;
    if (useOffset) {
        ctx.PushTransform(D2D1::Matrix3x2F::Translation(m_composeOffsetX, m_composeOffsetY));
    }

    const bool useOpacity = !m_layerPromoted && drawOpacity < 0.999f;
    if (useOpacity) {
        ctx.PushOpacity(drawOpacity);
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
        if (child && child->PresentsOnOwnerWindow()) {
            child->Render(ctx);
        }
    }

    if (clip) {
        ctx.PopClip();
    }

    if (useOpacity) {
        ctx.PopOpacity();
    }

    if (useOffset) {
        ctx.PopTransform();
    }
}

void UIElement::RenderOverlay(GraphicsContext& ctx) {
    if (m_visibility != Visibility::Visible) return;

    OnRenderOverlay(ctx);

    if (m_isHovered && m_tooltipVisible) {
        const std::string& tip = GetToolTip();
        if (!tip.empty()) {
            std::string font = "微软雅黑";
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
        if (child && child->PresentsOnOwnerWindow()) {
            child->RenderOverlay(ctx);
        }
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
        if (!(*it) || !(*it)->PresentsOnOwnerWindow()) {
            continue;
        }
        UIElement* hit = (*it)->HitTestOverlay(x, y);
        if (hit) return hit;
    }

    return nullptr;
}

UIElement* UIElement::HitTest(float x, float y) {
    if (m_visibility != Visibility::Visible) return nullptr;

    // Overlays are hit via HitTestOverlay explicitly (Window already probes overlay
    // before the scene tree). Recursing overlay here doubled the walk on every move.

    if (ShouldClipToBounds() && !m_bounds.Contains(x, y)) {
        return nullptr;
    }

    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if (!(*it) || !(*it)->PresentsOnOwnerWindow()) {
            continue;
        }
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
    // Do NOT MarkRenderContentDirty here — large panels would dirty the whole page on
    // every hover transit. Controls that paint hover chrome mark locally themselves.
    if (!GetToolTip().empty()) {
        RequestAnimationTicks();
    }
}

void UIElement::OnMouseLeave() {
    m_isHovered = false;
    m_isPressed = false;
    if (m_tooltipVisible) {
        m_tooltipVisible = false;
        MarkRenderRectDirty(m_bounds);
    }
}

void UIElement::OnMouseDown(Point pt) {
    if (!IsEnabled()) {
        return;
    }
    m_isPressed = true;
    m_onMouseDownEvent.Invoke(this, pt);
    // Local rect only — MarkRenderContentDirty bubbles through ScrollViewer and
    // used to StructureDirty the whole menu/PropertyGrid content bitmap on click.
    MarkRenderRectDirty(m_bounds);
}

void UIElement::OnMouseUp(Point pt) {
    if (m_isPressed) {
        m_isPressed = false;
        if (IsEnabled() && m_bounds.Contains(pt.x, pt.y)) {
            m_onClickEvent.Invoke(this);
        }
        MarkRenderRectDirty(m_bounds);
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

    // Do NOT recurse into children here. AnimationManager ticks only registered
    // elements; walking the tree from ScrollViewer/NavigationView on every mouse
    // move was starving NavigationViewItem ripples (Buttons were fine — no parent
    // walker registered).
    return any;
}

void UIElement::CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const {
    if (m_visibility != Visibility::Visible || !m_presentsOnOwnerWindow) {
        return;
    }
    if (HasSelfAnimation() && !m_bounds.IsEmpty()) {
        dirtyRect = hasDirty ? dirtyRect.Union(m_bounds) : m_bounds;
        hasDirty = true;
    }
}

void UIElement::CollectAnimationBounds(Rect& dirtyRect, bool& hasDirty) const {
    CollectSelfAnimationBounds(dirtyRect, hasDirty);
    for (const auto& child : m_children) {
        if (child) {
            child->CollectAnimationBounds(dirtyRect, hasDirty);
        }
    }
}

void UIElement::CollectRenderDirtyRegion(DirtyRegion& dirtyRegion, bool consume) {
    if (!m_presentsOnOwnerWindow) {
        return;
    }
    // Prune subtrees that were never marked — PropertyGrid has dozens of inputs;
    // a full walk on every hover transit dominated CPU while sliding.
    if (!m_subtreeRenderDirty && m_renderNode.GetWorldDirtyRegion().IsEmpty()) {
        return;
    }

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

    if (consume) {
        m_subtreeRenderDirty = false;
    }
}

void UIElement::MarkRenderContentDirty() {
    ++s_renderDirtySerial;
    m_renderNode.MarkContentDirty();
    if (m_layerPromoted) {
        m_renderNode.GetLayer().Invalidate(RenderLayer::ContentDirty);
    }
    for (UIElement* walk = m_parent; walk; walk = walk->m_parent) {
        if (walk->m_layerPromoted) {
            walk->m_renderNode.GetLayer().Invalidate(RenderLayer::ContentDirty);
        }
    }
    m_subtreeRenderDirty = true;
    // Bubble only this element's rect — ancestors must not mark their full bounds dirty
    // or a tiny control animation expands into a full-window repaint.
    if (m_parent) {
        m_parent->MarkRenderRectDirty(m_bounds);
    } else if (m_animationHost) {
        // No layout parent (popup-hosted): invalidate through AnimationHost so
        // Window still collects a dirty rect and PopupHost::Render runs again.
        m_animationHost->MarkRenderRectDirty(m_bounds);
    }
}

void UIElement::MarkRenderRectDirty(const Rect& rect) {
    ++s_renderDirtySerial;
    m_renderNode.MarkDirtyRect(rect);
    m_subtreeRenderDirty = true;
    // Flag ancestors only — do NOT stamp the same rect onto every ancestor's
    // world dirty region. Collect walks the subtree and would otherwise emit
    // N copies of one ripple rect; Window then hits rects.size()>8 and
    // RequestFullRepaint() every animation frame (整帧 + ~13 FPS).
    for (UIElement* walk = m_parent; walk; walk = walk->m_parent) {
        walk->m_subtreeRenderDirty = true;
    }
    if (!m_parent && m_animationHost) {
        // Same as MarkRenderContentDirty: popup-hosted controls must dirty the
        // live-tree host or the owner window never invalidates.
        m_animationHost->m_subtreeRenderDirty = true;
        m_animationHost->m_renderNode.MarkDirtyRect(rect);
    }
}

void UIElement::OnThemeChanged() {
    MarkRenderContentDirty();
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

} // namespace CUI
