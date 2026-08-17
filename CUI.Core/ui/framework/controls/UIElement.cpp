#include "UIElement.h"
#include "../window/Window.h"
#include "../animation/AnimationService.h"
#include "../animation/AnimationManager.h"
#include "../animation/FrameScheduler.h"
#include "../layout/Layout.h"
#include "../render/CompositionContext.h"
#include "../render/RenderLayer.h"
#include "../style/ThemeManager.h"
#include "../window/BubbleChrome.h"
#include "../window/PopupPlacement.h"
#include <algorithm>
#include <cassert>
#include <cmath>

namespace CUI {

bool UIElement::s_animationsEnabled = true;

void UIElement::NotifyVisualTreeChanged() {
    ++s_visualTreeGeneration;
    m_subtreeRenderDirty = true;
    for (UIElement* node = this; node; node = node->m_parent) {
        node->m_subtreeRenderDirty = true;
        node->m_renderNode.GetLayer().Invalidate(
            RenderLayer::ContentDirty
            | RenderLayer::StructureDirty
            | RenderLayer::SizeDirty);
    }
}
float UIElement::s_animationDeltaSeconds = 1.0f / 60.0f;
uint64_t UIElement::s_renderDirtySerial = 0;
uint64_t UIElement::s_visualTreeGeneration = 0;
uint64_t UIElement::s_layoutGeneration = 0;
int UIElement::s_toolTipShowDelayMs = 450;
int UIElement::s_toolTipHideDelayMs = 180;
int UIElement::s_toolTipAutoHideMs = 0;
float UIElement::s_toolTipMaxWidth = 280.0f;

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
    Text.Initialize(*this);
    FontFamily.Initialize(*this);
    FontSize.Initialize(*this);
    FontWeight.Initialize(*this);
    FontStyle.Initialize(*this);
    FontStretch.Initialize(*this);
    Underline.Initialize(*this);
    Strikethrough.Initialize(*this);
    TextColor.Initialize(*this);
    Background.Initialize(*this);
    HoverBackground.Initialize(*this);
    PressedBackground.Initialize(*this);
    BorderBrush.Initialize(*this);
    Foreground.Initialize(*this);
    BackgroundToken.Initialize(*this);
    HoverBackgroundToken.Initialize(*this);
    PressedBackgroundToken.Initialize(*this);
    DisabledBackgroundToken.Initialize(*this);
    BorderToken.Initialize(*this);
    FocusedBorderToken.Initialize(*this);
    ColorToken.Initialize(*this);
    SecondaryColorToken.Initialize(*this);
    PlaceholderColorToken.Initialize(*this);
    SelectedBackgroundToken.Initialize(*this);
    HeaderBackgroundToken.Initialize(*this);
    PaneBackgroundToken.Initialize(*this);
    IndicatorColorToken.Initialize(*this);
    DropdownBackgroundToken.Initialize(*this);
    SelectedItemBackgroundToken.Initialize(*this);
    FillColorToken.Initialize(*this);
    TrackColorToken.Initialize(*this);
    ActiveTrackColorToken.Initialize(*this);
    ThumbColorToken.Initialize(*this);
    OnColorToken.Initialize(*this);
    OffColorToken.Initialize(*this);
    KnobColorToken.Initialize(*this);
    CheckedBackgroundToken.Initialize(*this);
    AccentColorToken.Initialize(*this);
    ActiveColorToken.Initialize(*this);
    UnderlineColorToken.Initialize(*this);
    ActiveUnderlineColorToken.Initialize(*this);
    ActiveTabBackgroundToken.Initialize(*this);
    InactiveTabBackgroundToken.Initialize(*this);
    GridLineBrushToken.Initialize(*this);
    TitleColorToken.Initialize(*this);
    MessageColorToken.Initialize(*this);
    CaretColorToken.Initialize(*this);
    Placeholder.Initialize(*this);
    ToolTip.Initialize(*this);
    Icon.Initialize(*this);
    Width.Initialize(*this);
    Height.Initialize(*this);
    Margin.Initialize(*this);
    Padding.Initialize(*this);
    VisibilityProperty.Initialize(*this);
    Align.Initialize(*this);
    AlignHorizontal.Initialize(*this);
    AlignVertical.Initialize(*this);
    IsEnabledProperty.Initialize(*this);
    Opacity.Initialize(*this);
    MinWidth.Initialize(*this);
    MinHeight.Initialize(*this);
    MaxWidth.Initialize(*this);
    MaxHeight.Initialize(*this);
    CornerRadius.Initialize(*this);
    BorderThickness.Initialize(*this);
    FlexGrow.Initialize(*this);
    Orientation.Initialize(*this);
    Gap.Initialize(*this);
    ItemWidth.Initialize(*this);
    ItemHeight.Initialize(*this);
    LastChildFill.Initialize(*this);
    JustifyLines.Initialize(*this);
    FillLastLine.Initialize(*this);
    Rows.Initialize(*this);
    Columns.Initialize(*this);
    ClipToBounds.Initialize(*this);
    CanvasLeft.Initialize(*this);
    CanvasTop.Initialize(*this);
    CanvasRight.Initialize(*this);
    CanvasBottom.Initialize(*this);
    ZIndex.Initialize(*this);
    GridColumn.Initialize(*this);
    GridRow.Initialize(*this);
    GridColumnSpan.Initialize(*this);
    GridRowSpan.Initialize(*this);
    Id.Initialize(*this);
    Tag.Initialize(*this);
    NavigateUri.Initialize(*this);
    AcceptsReturn.Initialize(*this);
    TextWrapping.Initialize(*this);
    IsReadOnly.Initialize(*this);
    IsPasswordRevealed.Initialize(*this);
    ShowRevealButton.Initialize(*this);
    IsOn.Initialize(*this);
    IsExpanded.Initialize(*this);
    IsOpen.Initialize(*this);
    IsCloseVisible.Initialize(*this);
    IsPaneOpen.Initialize(*this);
    IsSettingsVisible.Initialize(*this);
    Title.Initialize(*this);
    Message.Initialize(*this);
    Subtitle.Initialize(*this);
    Header.Initialize(*this);
    PaneTitle.Initialize(*this);
    GroupName.Initialize(*this);
    ActionText.Initialize(*this);

    Bounds.Initialize([this]() { return GetBounds(); });
    HWND.Initialize([this]() { return GetHWND(); });
    DesiredSize.Initialize([this]() { return GetDesiredSize(); });
    ActualWidth.Initialize([this]() { return GetBounds().width; });
    ActualHeight.Initialize([this]() { return GetBounds().height; });
    Children.Initialize(
        [this]() -> const std::vector<Element>& { return GetChildren(); },
        [this](const std::vector<Element>& children) {
            ClearChildren();
            for (const auto& child : children) {
                if (child) AddChild(child);
            }
        });
    OnPropertyIdChanged().Connect([this](PropertyId, const Value&) {
        MarkRenderContentDirty();
    });
}

UIElement::~UIElement() {
    UnbindText();
    AnimationService::Instance().UnregisterElement(this);
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

::HWND UIElement::GetHWND() const {
    if (m_parent) return m_parent->GetHWND();
    return nullptr;
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
    if (NeedsOverlayHitTest()) {
        m_subtreeNeedsOverlayHit = true;
    }
    if (m_subtreeNeedsOverlayHit && parent) {
        parent->MarkSubtreeNeedsOverlayHitTest();
    }
}

void UIElement::MarkSubtreeNeedsOverlayHitTest() {
    for (UIElement* walk = this; walk; walk = walk->m_parent) {
        if (walk->m_subtreeNeedsOverlayHit) {
            break;
        }
        walk->m_subtreeNeedsOverlayHit = true;
    }
}

void UIElement::SetAnimationHost(UIElement* host) {
    if (host == this) {
        return;
    }
    m_animationHost = host;
    if (m_animationHost && HasSelfAnimation()) {
        RequestAnimationTicks();
    }
}

void UIElement::RequestAnimationTicks() {
    AnimationService::Instance().RequestAnimationTicks(this);
    m_animationTicksRegistered = true;
    if (AnimationManager* mgr = AnimationManager::Current()) {
        if (!mgr->IsRegistered(this)) {
            mgr->RegisterAnimating(this);
        }
    }
    if (FrameScheduler* sched = FrameScheduler::Current()) {
        sched->ScheduleFrame();
    }
}

void UIElement::InvalidateMeasure() {
    if (FrameScheduler* scheduler = FrameScheduler::Current()) {
        scheduler->ScheduleFrame();
    }
    // 布局失效代次：仅在新失效发生时递增（已脏时走下方 early-out，避免逐帧抖动）。
    if (!m_measureDirty) {
        ++s_layoutGeneration;
    }

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
    if (FrameScheduler* scheduler = FrameScheduler::Current()) {
        scheduler->ScheduleFrame();
    }

    if (m_arrangeDirty) {
        return;
    }
    m_arrangeDirty = true;
    if (m_parent) {
        m_parent->InvalidateArrange();
    }
}

bool UIElement::HasLayoutDirtyInSubtree() const {
    if (m_measureDirty || m_arrangeDirty) {
        return true;
    }
    for (const auto& child : m_children) {
        if (child && child->HasLayoutDirtyInSubtree()) {
            return true;
        }
    }
    return false;
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
    HideToolTipNow();
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
        args.handled = OnKeyDown(args.keyCode);
        break;
    }
}

void UIElement::CancelAnimationTicks() {
    AnimationService::Instance().CancelAnimationTicks(this);
    if (!m_animationTicksRegistered) {
        return;
    }
    if (AnimationManager* mgr = AnimationManager::Current()) {
        mgr->UnregisterAnimating(this);
    }
    m_animationTicksRegistered = false;
}

std::vector<std::shared_ptr<UIElement>> UIElement::GetVisualChildren() const {
    std::vector<std::shared_ptr<UIElement>> children = m_children;
    if (!UsesZIndexOrdering()) {
        return children;
    }

    std::stable_sort(children.begin(), children.end(), [](const auto& lhs, const auto& rhs) {
        if (!lhs) return false;
        if (!rhs) return true;
        return lhs->GetZIndex() < rhs->GetZIndex();
    });
    return children;
}

void UIElement::AddChild(std::shared_ptr<UIElement> child) {
    if (!child) return;
    NotifyVisualTreeChanged();
    child->SetParent(this);
    m_children.push_back(child);
    MarkRenderContentDirty();
    InvalidateMeasure();
}

void UIElement::AddChildQuiet(std::shared_ptr<UIElement> child) {
    if (!child) return;
    NotifyVisualTreeChanged();
    child->SetParent(this);
    m_children.push_back(child);
}

void UIElement::RemoveChild(std::shared_ptr<UIElement> child) {
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        NotifyVisualTreeChanged();
        (*it)->SetParent(nullptr);
        m_children.erase(it);
        MarkRenderContentDirty();
        InvalidateMeasure();
    }
}

void UIElement::RemoveChildQuiet(std::shared_ptr<UIElement> child) {
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        NotifyVisualTreeChanged();
        (*it)->SetParent(nullptr);
        m_children.erase(it);
    }
}

void UIElement::RemoveChildRaw(UIElement* child) {
    auto it = std::find_if(m_children.begin(), m_children.end(), [child](const std::shared_ptr<UIElement>& ptr) {
        return ptr.get() == child;
    });
    if (it != m_children.end()) {
        NotifyVisualTreeChanged();
        (*it)->SetParent(nullptr);
        m_children.erase(it);
        MarkRenderContentDirty();
    }
}

void UIElement::ClearChildren() {
    if (m_children.empty()) {
        return;
    }
    NotifyVisualTreeChanged();
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
    if (m_maxWidth >= 0.0f) arranged.width = (std::min)(arranged.width, m_maxWidth);
    if (m_maxHeight >= 0.0f) arranged.height = (std::min)(arranged.height, m_maxHeight);
    SetBounds(arranged);
    LayoutEngine::ArrangeElement(this, arranged);
    m_arrangeDirty = false;
}

void UIElement::Render(GraphicsContext& ctx) {
    if (m_visibility != Visibility::Visible) return;
    float drawOpacity = m_layerPromoted ? m_composeOpacity : m_opacity;
    // Fade only this node so a disabled parent does not double-dim children.
    if (!m_isEnabled) {
        drawOpacity *= 0.42f;
    }
    if (drawOpacity <= 0.001f) return;
    if (CanCullElementForCurrentPass(this, ctx)) {
        return;
    }

    ctx.PushInheritedTextStyle({
        ResolveFontWeight(),
        ResolveFontStyle(),
        ResolveFontStretch(),
        IsUnderline(),
        IsStrikethrough()
    });

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
                for (auto& child : GetVisualChildren()) {
                    if (child && child->PresentsOnOwnerWindow() && !child->IsOverlayComposed()) {
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
            ctx.PopInheritedTextStyle();
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

    for (auto& child : GetVisualChildren()) {
        if (child && child->PresentsOnOwnerWindow() && !child->IsOverlayComposed()) {
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

    ctx.PopInheritedTextStyle();
}

void UIElement::RenderOverlay(GraphicsContext& ctx) {
    if (m_visibility != Visibility::Visible) return;

    OnRenderOverlay(ctx);

    if (m_tooltipVisible) {
        const std::string& tip = GetToolTip();
        if (!tip.empty()) {
            const float maxWidth = (m_toolTipMaxWidth > 0.0f) ? m_toolTipMaxWidth : s_toolTipMaxWidth;
            GraphicsContext::TextLayoutOptions options;
            options.maxWidth = (std::max)(32.0f, maxWidth);
            options.maxHeight = 400.0f;
            options.wrapping = DWRITE_WORD_WRAPPING_WRAP;
            ComPtr<IDWriteTextLayout> layout = GraphicsContext::CreateTextLayout(
                Utf8ToUtf16(tip), GetFontFamily(), GetFontSize(), options,
                ResolveFontWeight(), ResolveFontStyle(), ResolveFontStretch());
            Size textSize(maxWidth, 16.0f);
            if (layout) {
                DWRITE_TEXT_METRICS metrics{};
                layout->GetMetrics(&metrics);
                textSize.width = std::ceil(metrics.widthIncludingTrailingWhitespace);
                textSize.height = std::ceil(metrics.height);
            }

            constexpr float padX = 8.0f;
            constexpr float padY = 6.0f;
            const Size cardSize(textSize.width + padX * 2.0f, textSize.height + padY * 2.0f);
            const Rect anchor(m_tooltipAnchorPos.x - 4.0f, m_tooltipAnchorPos.y - 4.0f, 8.0f, 8.0f);
            const BubbleLayout bubble = LayoutBubble(
                anchor,
                cardSize,
                GetPopupViewportOrDefault(),
                BubblePlacement::Auto,
                8.0f,
                6.0f,
                6.0f);

            D2D1_COLOR_F bg = ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBackground);
            D2D1_COLOR_F border = ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBorder);
            D2D1_COLOR_F textColor = ThemeManager::Instance().GetFlatColor(ThemeTokenId::TextPrimary);
            PaintBubble(ctx, bubble, bg, border, 6.0f);

            if (layout) {
                const DWRITE_TEXT_RANGE range = { 0, static_cast<UINT32>(Utf8ToUtf16(tip).length()) };
                layout->SetUnderline(IsUnderline(), range);
                layout->SetStrikethrough(IsStrikethrough(), range);
                ctx.DrawTextLayout(
                    layout.Get(),
                    Rect(bubble.card.x + padX, bubble.card.y + padY, textSize.width, textSize.height),
                    textColor);
            }
            m_tooltipPaintRect = bubble.total.Inflate(3.0f);
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

    D2D1_COLOR_F bg = m_hasBackgroundColor
        ? m_backgroundColor
        : ((m_backgroundToken != ThemeTokenId::Unset)
            ? ResolveThemeColor(m_backgroundToken, ThemeTokenId::CardBackground)
            : D2D1::ColorF(0, 0, 0, 0));
    if (bg.a > 0.0f) {
        if (radius > 0.0f) {
            ctx.FillRoundedRect(m_bounds, radius, bg);
        } else {
            ctx.FillRect(m_bounds, bg);
        }
    }

    D2D1_COLOR_F borderBrush = m_hasBorderBrushColor
        ? m_borderBrushColor
        : ((m_borderToken != ThemeTokenId::Unset)
            ? ResolveThemeColor(m_borderToken, ThemeTokenId::CardBorder)
            : D2D1::ColorF(0, 0, 0, 0));
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
    if (!m_subtreeNeedsOverlayHit && !NeedsOverlayHitTest()) {
        return nullptr;
    }

    UIElement* selfOverlay = OnHitTestOverlay(x, y);
    if (selfOverlay) return selfOverlay;

    const auto visualChildren = GetVisualChildren();
    for (auto it = visualChildren.rbegin(); it != visualChildren.rend(); ++it) {
        if (!(*it) || !(*it)->PresentsOnOwnerWindow() || (*it)->IsOverlayComposed()) {
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

    const auto visualChildren = GetVisualChildren();
    for (auto it = visualChildren.rbegin(); it != visualChildren.rend(); ++it) {
        if (!(*it) || !(*it)->PresentsOnOwnerWindow() || (*it)->IsOverlayComposed()) {
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
    m_tooltipHideArmed = false;
    m_lastMouseMoveTime = std::chrono::steady_clock::now();
    // Do NOT MarkRenderContentDirty here — large panels would dirty the whole page on
    // every hover transit. Controls that paint hover chrome mark locally themselves.
    if (!GetToolTip().empty() && !m_tooltipVisible) {
        RequestAnimationTicks();
    }
}

void UIElement::OnMouseLeave() {
    m_isHovered = false;
    m_isPressed = false;
    if (m_tooltipVisible) {
        ArmToolTipHide();
    }
}

void UIElement::OnMouseDown(Point pt) {
    if (!IsEnabled()) {
        return;
    }
    HideToolTipNow();
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
            ExecuteBoundCommand();
            OnClick.Invoke(this);
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

    if (distSq > 4.0f && !m_tooltipVisible) {
        m_lastMouseMoveTime = std::chrono::steady_clock::now();
        if (!GetToolTip().empty()) {
            RequestAnimationTicks();
        }
    }
}

void UIElement::OnMouseWheel(float delta) {
    if (m_parent) {
        m_parent->OnMouseWheel(delta);
    }
}

bool UIElement::OnKeyDown(int vkCode) {
    (void)vkCode;
    return false;
}

bool UIElement::Focus(FocusState state) {
    if (!IsEnabled() || GetVisibility() != Visibility::Visible) {
        return false;
    }

    if (Window* window = Window::Current()) {
        window->ApplyFocus(this, state);
        return window->GetFocusedElement() == this;
    }

    if (!AcceptsTabFocus()) {
        return false;
    }
    SetFocusState(state);
    if (!IsFocused()) {
        OnFocus();
    }
    return true;
}

void UIElement::Blur() {
    if (Window* window = Window::Current(); window && window->GetFocusedElement() == this) {
        window->ApplyFocus(nullptr, FocusState::Unfocused);
        return;
    }
    if (IsFocused()) {
        OnBlur();
    }
}
void UIElement::OnFocus() {
    m_isFocused = true;
    NotifyFieldChanged(PropertyId::Focused, Value(true));
    MarkRenderRectDirty(m_bounds.Inflate(6.0f));
}

void UIElement::OnBlur() {
    m_isFocused = false;
    m_focusState = FocusState::Unfocused;
    NotifyFieldChanged(PropertyId::Focused, Value(false));
    MarkRenderRectDirty(m_bounds.Inflate(6.0f));
}

void UIElement::SetCommand(std::shared_ptr<Command> command) {
    m_command = std::move(command);
    if (m_command) {
        if (Window* win = Window::Current()) {
            win->GetCommands().Register(m_command);
        }
        if (m_command->GetLabel().empty() && !GetText().empty()) {
            m_command->SetLabel(GetText());
        }
    }
}

bool UIElement::ExecuteBoundCommand() {
    if (!m_command) {
        return false;
    }
    if (m_command->CanExecute()) {
        m_command->Execute();
    }
    return true;
}

bool UIElement::OnAnimationTick() {
    if (m_visibility != Visibility::Visible) {
        return false;
    }

    bool any = ToolTipTick(std::chrono::steady_clock::now());

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
    if (m_tooltipVisible && !m_tooltipPaintRect.IsEmpty()) {
        dirtyRect = hasDirty ? dirtyRect.Union(m_tooltipPaintRect) : m_tooltipPaintRect;
        hasDirty = true;
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
    AnimationService::Instance().SetAnimationsEnabled(enabled);
    s_animationsEnabled = enabled;
}

bool UIElement::AreAnimationsEnabled() {
    return AnimationService::Instance().AreAnimationsEnabled();
}

void UIElement::SetAnimationDeltaSeconds(float dtSeconds) {
    AnimationService::Instance().SetDeltaSeconds(dtSeconds);
    s_animationDeltaSeconds = std::clamp(dtSeconds, 1.0f / 240.0f, 0.050f);
}

float UIElement::GetAnimationDeltaSeconds() {
    return AnimationService::Instance().GetDeltaSeconds();
}

void UIElement::SetBounds(const Rect& bounds) {
    if (bounds.x == m_bounds.x && bounds.y == m_bounds.y
        && bounds.width == m_bounds.width && bounds.height == m_bounds.height) {
        return;
    }

    MarkRenderContentDirty();
    m_bounds = bounds;
    m_renderNode.SetBounds(m_bounds);
    MarkRenderContentDirty();
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

void UIElement::SetToolTipShowDelayMs(int ms) {
    s_toolTipShowDelayMs = (std::max)(0, ms);
}

void UIElement::SetToolTipHideDelayMs(int ms) {
    s_toolTipHideDelayMs = (std::max)(0, ms);
}

void UIElement::SetDefaultToolTipMaxWidth(float width) {
    s_toolTipMaxWidth = (std::max)(48.0f, width);
}

void UIElement::SetDefaultToolTipAutoHideMs(int ms) {
    s_toolTipAutoHideMs = (std::max)(0, ms);
}

int UIElement::GetToolTipShowDelayMs() { return s_toolTipShowDelayMs; }
int UIElement::GetToolTipHideDelayMs() { return s_toolTipHideDelayMs; }
float UIElement::GetDefaultToolTipMaxWidth() { return s_toolTipMaxWidth; }
int UIElement::GetDefaultToolTipAutoHideMs() { return s_toolTipAutoHideMs; }

void UIElement::SetToolTipMaxWidth(float width) {
    if (std::abs(m_toolTipMaxWidth - width) < 0.01f) {
        return;
    }
    m_toolTipMaxWidth = width;
    if (m_tooltipVisible) {
        DirtyToolTipRect();
    }
}

void UIElement::SetToolTipAutoHideMs(int ms) {
    if (m_toolTipAutoHideMs == ms) {
        return;
    }
    m_toolTipAutoHideMs = ms;
    if (m_tooltipVisible) {
        RequestAnimationTicks();
    }
}

void UIElement::DirtyToolTipRect() {
    if (!m_tooltipPaintRect.IsEmpty()) {
        MarkRenderRectDirty(m_tooltipPaintRect);
    }
}

void UIElement::ShowToolTipNow() {
    if (GetToolTip().empty()) {
        return;
    }
    m_tooltipVisible = true;
    m_tooltipHideArmed = false;
    m_tooltipAnchorPos = m_lastMousePos;
    m_tooltipShownAt = std::chrono::steady_clock::now();
    m_tooltipPaintRect = Rect(
        m_tooltipAnchorPos.x - 8.0f,
        m_tooltipAnchorPos.y - 8.0f,
        320.0f,
        120.0f);
    DirtyToolTipRect();
}

void UIElement::HideToolTipNow() {
    m_tooltipHideArmed = false;
    if (!m_tooltipVisible) {
        return;
    }
    m_tooltipVisible = false;
    DirtyToolTipRect();
    m_tooltipPaintRect = Rect();
}

void UIElement::ArmToolTipHide() {
    if (!m_tooltipVisible || m_tooltipHideArmed) {
        return;
    }
    m_tooltipHideArmed = true;
    m_tooltipHideAt = std::chrono::steady_clock::now()
        + std::chrono::milliseconds(s_toolTipHideDelayMs);
    RequestAnimationTicks();
}

bool UIElement::ToolTipTick(std::chrono::steady_clock::time_point now) {
    if (GetToolTip().empty()) {
        if (m_tooltipVisible) {
            HideToolTipNow();
        }
        return false;
    }

    if (m_tooltipHideArmed) {
        if (m_isHovered) {
            m_tooltipHideArmed = false;
        } else if (now >= m_tooltipHideAt) {
            HideToolTipNow();
            return false;
        } else {
            return true;
        }
    }

    if (m_tooltipVisible) {
        const int autoHide = (m_toolTipAutoHideMs >= 0) ? m_toolTipAutoHideMs : s_toolTipAutoHideMs;
        if (autoHide > 0) {
            const auto shownMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - m_tooltipShownAt).count();
            if (shownMs >= autoHide) {
                ArmToolTipHide();
                return m_tooltipHideArmed;
            }
            return true;
        }
        return false;
    }

    if (!m_isHovered) {
        return false;
    }

    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_lastMouseMoveTime).count();
    if (elapsedMs >= s_toolTipShowDelayMs) {
        ShowToolTipNow();
        const int autoHide = (m_toolTipAutoHideMs >= 0) ? m_toolTipAutoHideMs : s_toolTipAutoHideMs;
        return autoHide > 0;
    }
    return true;
}

} // namespace CUI
