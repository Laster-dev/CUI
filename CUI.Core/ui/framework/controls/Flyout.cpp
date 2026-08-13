#include "Flyout.h"
#include "../style/ThemeManager.h"
#include "../window/PopupPlacement.h"
#include <algorithm>
#include <cmath>

namespace CUI {

// ---------------- FlyoutPresenter ----------------

FlyoutPresenter::FlyoutPresenter() {
    SetBackgroundToken(ThemeTokenId::CardBackground);
    SetBorderToken(ThemeTokenId::CardBorder);
    SetBorderThickness(1.0f);
    SetCornerRadius(8.0f);
    SetPadding(Thickness(14.0f));
}

void FlyoutPresenter::SetContent(std::shared_ptr<UIElement> content) {
    if (m_content) {
        RemoveChild(m_content);
    }
    m_content = content;
    if (m_content) {
        AddChild(m_content);
    }
}

Size FlyoutPresenter::Measure(Size availableSize) {
    Thickness pad = GetPadding();
    float innerW = (std::max)(0.0f, availableSize.width - pad.left - pad.right);
    float innerH = (std::max)(0.0f, availableSize.height - pad.top - pad.bottom);

    Size contentSize(0.0f, 0.0f);
    if (m_content) {
        contentSize = m_content->Measure(Size(innerW, innerH));
    }

    // Minimum readable flyout surface
    float w = (std::max)(contentSize.width + pad.left + pad.right, 160.0f);
    float h = (std::max)(contentSize.height + pad.top + pad.bottom, 48.0f);
    m_desiredSize = Size(w, h);
    return m_desiredSize;
}

void FlyoutPresenter::Arrange(Rect finalRect) {
    SetBounds(finalRect);
    Thickness pad = GetPadding();
    Rect contentRect(
        finalRect.x + pad.left,
        finalRect.y + pad.top,
        (std::max)(0.0f, finalRect.width - pad.left - pad.right),
        (std::max)(0.0f, finalRect.height - pad.top - pad.bottom)
    );

    if (m_content) {
        m_content->Arrange(contentRect);
    }
}

void FlyoutPresenter::OnRender(GraphicsContext& ctx) {
    // Popups stay fully opaque — never borrow Surface/Mica alpha.
    float radius = GetCornerRadius();
    D2D1_COLOR_F bg = ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBackground);
    D2D1_COLOR_F border = ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBorder);

    if (radius > 0.0f) {
        ctx.FillRoundedRect(m_bounds, radius, bg);
        ctx.DrawRoundedRect(m_bounds, radius, border, GetBorderThickness() > 0.0f ? GetBorderThickness() : 1.0f);
    } else {
        ctx.FillRect(m_bounds, bg);
        ctx.DrawRect(m_bounds, border, 1.0f);
    }
}

// ---------------- Flyout ----------------

Flyout::Flyout() {
    // Presenter is overlay-only — not a layout child (avoids eating Column space
    // and having Arrange overwrite ShowAt coordinates on every Relayout).
    m_presenter = std::make_shared<FlyoutPresenter>();
    SetVisibility(Visibility::Visible);
    SetClipToBounds(false);
}

Flyout::Flyout(std::shared_ptr<UIElement> content) : Flyout() {
    SetContent(content);
}

void Flyout::SetContent(std::shared_ptr<UIElement> content) {
    if (m_presenter) {
        m_presenter->SetContent(content);
    }
}

Size Flyout::Measure(Size availableSize) {
    (void)availableSize;
    // Zero-size host in document flow; popup is overlay-only.
    m_desiredSize = Size(0.0f, 0.0f);
    return m_desiredSize;
}

void Flyout::Arrange(Rect finalRect) {
    // Keep a 0x0 bounds at the flow position so HitTest does not claim space,
    // but never re-layout the presenter here (ShowAt owns that).
    SetBounds(Rect(finalRect.x, finalRect.y, 0.0f, 0.0f));
}

void Flyout::ShowAt(UIElement* target) {
    if (!target || !m_presenter) return;
    m_anchor = target;
    Rect targetBounds = target->GetBounds();

    Size available(480.0f, 640.0f);
    m_popupSize = m_presenter->Measure(available);
    const Rect viewport = GetPopupViewportOrDefault();

    PopupVerticalPlacement vertical = PopupVerticalPlacement::AutoFlip;
    switch (m_placement) {
    case FlyoutPlacement::Top:
        vertical = PopupVerticalPlacement::Above;
        break;
    case FlyoutPlacement::Bottom:
        vertical = PopupVerticalPlacement::Below;
        break;
    case FlyoutPlacement::Left:
    case FlyoutPlacement::Right:
    default:
        break;
    }

    if (m_placement == FlyoutPlacement::Left || m_placement == FlyoutPlacement::Right) {
        float popupX = (m_placement == FlyoutPlacement::Left)
            ? targetBounds.x - m_popupSize.width - 6.0f
            : targetBounds.x + targetBounds.width + 6.0f;
        float popupY = targetBounds.y;
        constexpr float margin = 4.0f;

        // Clamp flyout size to visible viewport so it never draws/clips outside.
        const float maxW = (std::max)(0.0f, viewport.width - margin * 2.0f);
        const float maxH = (std::max)(0.0f, viewport.height - margin * 2.0f);
        m_popupSize.width = (std::min)(m_popupSize.width, maxW);
        m_popupSize.height = (std::min)(m_popupSize.height, maxH);

        const float maxX = viewport.x + viewport.width - m_popupSize.width - margin;
        const float maxY = viewport.y + viewport.height - m_popupSize.height - margin;
        popupX = std::clamp(popupX, viewport.x + margin, (std::max)(viewport.x + margin, maxX));
        popupY = std::clamp(popupY, viewport.y + margin, (std::max)(viewport.y + margin, maxY));
        ShowAt(Point(popupX, popupY));
        return;
    }

    Rect placed = PlacePopupNearAnchor(
        targetBounds,
        m_popupSize.width,
        m_popupSize.height,
        viewport,
        6.0f,
        4.0f,
        vertical);
    // Keep presenter bounds synced with the clamped placement size.
    m_popupSize.width = placed.width;
    m_popupSize.height = placed.height;
    ShowAt(Point(placed.x, placed.y));
}

void Flyout::ShowAt(Point pt) {
    m_popupPos = pt;
    if (m_popupSize.width <= 0.0f || m_popupSize.height <= 0.0f) {
        if (m_presenter) {
            m_popupSize = m_presenter->Measure(Size(480.0f, 640.0f));
        }
    }
    if (m_presenter) {
        m_presenter->Arrange(Rect(pt.x, pt.y, m_popupSize.width, m_popupSize.height));
    }
    m_isOpen = true;
    m_popupAnim.SetTarget(1.0f);
    if (!UIElement::AreAnimationsEnabled()) {
        m_popupAnim.Reset(1.0f);
    }
    if (PopupHost* host = PopupHost::Current()) {
        host->Open(this);
    }
    RequestAnimationTicks();
    MarkRenderRectDirty(GetPopupBounds().Inflate(6.0f));
}

void Flyout::Hide() {
    if (!m_isOpen && m_popupAnim.Current() <= 0.001f) return;
    m_isOpen = false;
    m_popupAnim.SetTarget(0.0f);
    if (!UIElement::AreAnimationsEnabled()) {
        m_popupAnim.Reset(0.0f);
    }
    if (PopupHost* host = PopupHost::Current()) {
        host->Close(this);
    }
    RequestAnimationTicks();
    MarkRenderRectDirty(GetPopupBounds().Inflate(6.0f));
}

Rect Flyout::GetPopupBounds() const {
    if (m_popupSize.width <= 0.0f || m_popupSize.height <= 0.0f) return Rect();
    return Rect(m_popupPos.x, m_popupPos.y, m_popupSize.width, m_popupSize.height);
}

bool Flyout::HitDismissExempt(float x, float y) const {
    if (!m_isOpen && m_popupAnim.Current() <= 0.001f) return false;
    if (GetPopupBounds().Contains(x, y)) return true;
    if (m_anchor && m_anchor->GetBounds().Contains(x, y)) return true;
    return false;
}

void Flyout::OnRenderOverlay(GraphicsContext& ctx) {
    // When open, PopupHost owns paint; keep tree path for close-animation frames.
    if (PopupHost::Current() && m_isOpen) return;
    RenderPopup(ctx);
}

void Flyout::RenderPopup(GraphicsContext& ctx) {
    float progress = UIElement::AreAnimationsEnabled() ? m_popupAnim.Current() : (m_isOpen ? 1.0f : 0.0f);
    if (progress <= 0.001f || !m_presenter) return;

    const Rect pop(m_popupPos.x, m_popupPos.y, m_popupSize.width, m_popupSize.height);
    m_presenter->Arrange(pop);
    ctx.PushPopupReveal(pop, progress, Point(pop.x + pop.width * 0.5f, pop.y));
    m_presenter->Render(ctx);
    ctx.PopPopupReveal();
}

UIElement* Flyout::HitTestOverlay(float x, float y) {
    float progress = UIElement::AreAnimationsEnabled() ? m_popupAnim.Current() : (m_isOpen ? 1.0f : 0.0f);
    if (progress <= 0.5f || !m_isOpen || !m_presenter) return nullptr;

    Rect popRect = GetPopupBounds();
    if (popRect.Contains(x, y)) {
        UIElement* hit = m_presenter->HitTest(x, y);
        return hit ? hit : m_presenter.get();
    }
    return nullptr;
}

bool Flyout::OnAnimationTick() {
    float dt = UIElement::GetAnimationDeltaSeconds();
    m_popupAnim.SetTarget(m_isOpen ? 1.0f : 0.0f);
    bool animating = m_popupAnim.Tick(dt, PopupReveal::kSpec);
    if (m_presenter) {
        animating = m_presenter->OnAnimationTick() || animating;
        // Height reveal is clip-based; dirty popup footprint only (no full content dirty).
        if (animating) {
            MarkRenderRectDirty(GetPopupBounds().Inflate(6.0f));
        }
    }
    if (animating) {
        RequestAnimationTicks();
    }
    return animating;
}

bool Flyout::HasSelfAnimation() const {
    return std::abs(m_popupAnim.Target() - m_popupAnim.Current()) > 0.001f;
}

void Flyout::CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const {
    if (HasSelfAnimation() || m_isOpen || m_popupAnim.Current() > 0.001f) {
        Rect area = GetPopupBounds().Inflate(6.0f);
        if (!area.IsEmpty()) {
            dirtyRect = hasDirty ? dirtyRect.Union(area) : area;
            hasDirty = true;
        }
    }
}

void Flyout::CollectAnimationBounds(Rect& dirtyRect, bool& hasDirty) const {
    CollectSelfAnimationBounds(dirtyRect, hasDirty);
}

} // namespace CUI
