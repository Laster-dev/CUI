#include "Flyout.h"
#include <algorithm>

namespace CUI {

// ---------------- FlyoutPresenter ----------------

FlyoutPresenter::FlyoutPresenter() {
    SetProperty("background", Value(D2D1::ColorF(0x25 / 255.0f, 0x25 / 255.0f, 0x26 / 255.0f, 1.0f)));
    SetProperty("borderBrush", Value(D2D1::ColorF(0x45 / 255.0f, 0x45 / 255.0f, 0x45 / 255.0f, 1.0f)));
    SetProperty("borderThickness", Value(1.0f));
    SetProperty("cornerRadius", Value(6.0f));
    SetProperty("padding", Value(Thickness(12.0f)));
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
    Thickness pad = GetProperty("padding").AsThickness(Thickness(12.0f));
    float innerW = (std::max)(0.0f, availableSize.width - pad.left - pad.right);
    float innerH = (std::max)(0.0f, availableSize.height - pad.top - pad.bottom);

    Size contentSize(0.0f, 0.0f);
    if (m_content) {
        contentSize = m_content->Measure(Size(innerW, innerH));
    }

    m_desiredSize = Size(contentSize.width + pad.left + pad.right, contentSize.height + pad.top + pad.bottom);
    return m_desiredSize;
}

void FlyoutPresenter::Arrange(Rect finalRect) {
    SetBounds(finalRect);
    Thickness pad = GetProperty("padding").AsThickness(Thickness(12.0f));
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

// ---------------- Flyout ----------------

Flyout::Flyout() {
    m_presenter = std::make_shared<FlyoutPresenter>();
    AddChild(m_presenter);
}

Flyout::Flyout(std::shared_ptr<UIElement> content) : Flyout() {
    SetContent(content);
}

void Flyout::SetContent(std::shared_ptr<UIElement> content) {
    if (m_presenter) {
        m_presenter->SetContent(content);
    }
}

void Flyout::ShowAt(UIElement* target) {
    if (!target) return;
    Rect targetBounds = target->GetBounds();

    Size available(280.0f, 200.0f);
    if (m_presenter) {
        m_popupSize = m_presenter->Measure(available);
    }

    float popupX = targetBounds.x;
    float popupY = targetBounds.y + targetBounds.height + 4.0f;

    switch (m_placement) {
    case FlyoutPlacement::Top:
        popupY = targetBounds.y - m_popupSize.height - 4.0f;
        break;
    case FlyoutPlacement::Left:
        popupX = targetBounds.x - m_popupSize.width - 4.0f;
        popupY = targetBounds.y;
        break;
    case FlyoutPlacement::Right:
        popupX = targetBounds.x + targetBounds.width + 4.0f;
        popupY = targetBounds.y;
        break;
    case FlyoutPlacement::Bottom:
    default:
        popupX = targetBounds.x;
        popupY = targetBounds.y + targetBounds.height + 4.0f;
        break;
    }

    ShowAt(Point(popupX, popupY));
}

void Flyout::ShowAt(Point pt) {
    m_popupPos = pt;
    if (m_presenter) {
        m_presenter->Arrange(Rect(pt.x, pt.y, m_popupSize.width, m_popupSize.height));
    }
    m_isOpen = true;
    MarkRenderContentDirty();
}

void Flyout::Hide() {
    m_isOpen = false;
    MarkRenderContentDirty();
}

void Flyout::OnRenderOverlay(GraphicsContext& ctx) {
    float progress = UIElement::AreAnimationsEnabled() ? m_popupAnim.Current() : (m_isOpen ? 1.0f : 0.0f);
    if (progress <= 0.001f || !m_presenter) return;

    float currentH = (m_isOpen && progress >= 0.98f) ? m_popupSize.height : (m_popupSize.height * progress);
    Rect fullRect(m_popupPos.x, m_popupPos.y, m_popupSize.width, m_popupSize.height);
    Rect clipRect(m_popupPos.x, m_popupPos.y, m_popupSize.width, currentH);

    ctx.PushClip(clipRect);
    m_presenter->Render(ctx);
    ctx.PopClip();
}

UIElement* Flyout::HitTestOverlay(float x, float y) {
    float progress = UIElement::AreAnimationsEnabled() ? m_popupAnim.Current() : (m_isOpen ? 1.0f : 0.0f);
    if (progress <= 0.5f || !m_isOpen || !m_presenter) return nullptr;

    Rect popRect(m_popupPos.x, m_popupPos.y, m_popupSize.width, m_popupSize.height);
    if (popRect.Contains(x, y)) {
        return m_presenter->HitTest(x, y);
    }
    return nullptr;
}

bool Flyout::OnAnimationTick() {
    float dt = UIElement::GetAnimationDeltaSeconds();
    m_popupAnim.SetTarget(m_isOpen ? 1.0f : 0.0f);
    return m_popupAnim.Tick(dt, AnimationSpec{ 0.55f, 0.01f });
}

bool Flyout::HasSelfAnimation() const {
    return std::abs(m_popupAnim.Target() - m_popupAnim.Current()) > 0.001f;
}

} // namespace CUI
