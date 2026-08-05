#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Splitter.h"
#include "../style/ThemeManager.h"
#include <algorithm>

namespace CUI {

namespace {
constexpr float kDefaultThickness = 10.0f;
constexpr float kHitPad = 4.0f;
}

Splitter::Splitter() {
    SetOrientation(Orientation::Vertical);
    SetBackgroundToken(ThemeTokenId::CardBorder);
    SetHoverBackgroundToken(ThemeTokenId::AccentColor);
    SetBackground(ThemeManager::Instance().GetColor("cardBorder"));
    SetHoverBackground(ThemeManager::Instance().GetColor("accentColor"));
    // Vertical bar: fixed width, stretch height (-1). Never set both to thickness
    // or Measure/Arrange produce a 10x10 square.
    SetWidth(kDefaultThickness);
    SetHeight(-1.0f);
    SetAlign(Alignment::Stretch);
}

std::vector<PropertyMeta> Splitter::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "orientation", "拆分方向 (Orientation)", "拆分配置", "enum", { "Vertical", "Horizontal" } });
    return metas;
}

HCURSOR Splitter::GetCursor() const {
    if (!IsEnabled()) return nullptr;
    return LoadCursor(nullptr, IsVerticalSplitter() ? IDC_SIZEWE : IDC_SIZENS);
}

void Splitter::SetOrientation(const std::string& orient) {
    const bool vertical = (orient == "Vertical");
    UIElement::SetOrientation(vertical ? Orientation::Vertical : Orientation::Horizontal);
    if (vertical) {
        SetWidth(kDefaultThickness);
        SetHeight(-1.0f);
    } else {
        SetWidth(-1.0f);
        SetHeight(kDefaultThickness);
    }
    SetAlign(Alignment::Stretch);
}

Size Splitter::Measure(Size availableSize) {
    (void)availableSize;
    float width = GetWidth();
    float height = GetHeight();

    if (IsVerticalSplitter()) {
        // Thick in X; cross-axis (-1) lets StackPanel Stretch fill row height.
        if (width < 0.0f) width = kDefaultThickness;
        if (height < 0.0f) height = 0.0f;
    } else {
        // Thick in Y; cross-axis (-1) lets Column Stretch fill width.
        if (height < 0.0f) height = kDefaultThickness;
        if (width < 0.0f) width = 0.0f;
    }

    m_desiredSize = Size(width, height);
    return m_desiredSize;
}

UIElement* Splitter::HitTest(float x, float y) {
    if (!IsEnabled()) return nullptr;
    if (GetVisibility() != Visibility::Visible) return nullptr;

    Rect hit = m_bounds;
    if (IsVerticalSplitter()) {
        hit.x -= kHitPad;
        hit.width += kHitPad * 2.0f;
    } else {
        hit.y -= kHitPad;
        hit.height += kHitPad * 2.0f;
    }
    return hit.Contains(x, y) ? this : nullptr;
}

void Splitter::OnMouseDown(Point pt) {
    if (!IsEnabled()) {
        m_isDragging = false;
        return;
    }
    Control::OnMouseDown(pt);
    m_isDragging = true;
    m_dragStartPt = pt;
}

void Splitter::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);
    if (!IsEnabled() || !m_isDragging) {
        return;
    }
    const bool vertical = IsVerticalSplitter();
    float delta = vertical ? (pt.x - m_dragStartPt.x) : (pt.y - m_dragStartPt.y);
    if (std::abs(delta) < 0.001f) {
        return;
    }

    if (m_parent) {
        const auto& siblings = m_parent->GetChildren();
        for (size_t i = 0; i < siblings.size(); ++i) {
            if (siblings[i].get() != this) {
                continue;
            }

            std::shared_ptr<UIElement> prevElem;
            std::shared_ptr<UIElement> nextElem;

            for (size_t prev = i; prev-- > 0;) {
                if (siblings[prev] && siblings[prev]->GetVisibility() != Visibility::Collapsed) {
                    prevElem = siblings[prev];
                    break;
                }
            }

            for (size_t next = i + 1; next < siblings.size(); ++next) {
                if (siblings[next] && siblings[next]->GetVisibility() != Visibility::Collapsed) {
                    nextElem = siblings[next];
                    break;
                }
            }

            if (prevElem && nextElem) {
                if (vertical) {
                    float prevMin = std::max(40.0f, prevElem->GetMinWidth());
                    float nextMin = std::max(40.0f, nextElem->GetMinWidth());
                    float prevWidth = prevElem->GetBounds().width;
                    float nextWidth = nextElem->GetBounds().width;
                    float minDelta = prevMin - prevWidth;
                    float maxDelta = nextWidth - nextMin;
                    if (minDelta > maxDelta) {
                        minDelta = 0.0f;
                        maxDelta = 0.0f;
                    }
                    float appliedDelta = std::clamp(delta, minDelta, maxDelta);
                    if (std::abs(appliedDelta) > 0.001f) {
                        prevElem->SetWidth(prevWidth + appliedDelta);
                        nextElem->SetWidth(nextWidth - appliedDelta);
                    }
                } else {
                    float prevMin = std::max(40.0f, prevElem->GetMinHeight());
                    float nextMin = std::max(40.0f, nextElem->GetMinHeight());
                    float prevHeight = prevElem->GetBounds().height;
                    float nextHeight = nextElem->GetBounds().height;
                    float minDelta = prevMin - prevHeight;
                    float maxDelta = nextHeight - nextMin;
                    if (minDelta > maxDelta) {
                        minDelta = 0.0f;
                        maxDelta = 0.0f;
                    }
                    float appliedDelta = std::clamp(delta, minDelta, maxDelta);
                    if (std::abs(appliedDelta) > 0.001f) {
                        prevElem->SetHeight(prevHeight + appliedDelta);
                        nextElem->SetHeight(nextHeight - appliedDelta);
                    }
                }
            } else if (prevElem) {
                if (vertical) {
                    float newW = std::max(40.0f, prevElem->GetBounds().width + delta);
                    prevElem->SetWidth(newW);
                } else {
                    float newH = std::max(40.0f, prevElem->GetBounds().height + delta);
                    prevElem->SetHeight(newH);
                }
            }
            break;
        }

        // Only relayout the immediate parent row/column — full-tree Measure was the lag source.
        Rect parentBounds = m_parent->GetBounds();
        m_parent->Measure(Size(parentBounds.width, parentBounds.height));
        m_parent->Arrange(parentBounds);
        m_parent->MarkRenderContentDirty();
    }

    m_onSplitterMovedEvent.Invoke(this, delta);
    m_dragStartPt = pt;
}

void Splitter::OnMouseUp(Point pt) {
    Control::OnMouseUp(pt);
    m_isDragging = false;
}

void Splitter::OnRender(GraphicsContext& ctx) {
    D2D1_COLOR_F bg = ResolveThemeColor(GetBackgroundToken(), ThemeTokenId::CardBorder);
    D2D1_COLOR_F hoverBg = ResolveThemeColor(GetHoverBackgroundToken(), ThemeTokenId::AccentColor);
    const bool active = m_isHovered || m_isDragging;
    ctx.FillRect(m_bounds, active ? hoverBg : bg);

    // Center grip mark so a thick hit strip still reads as a splitter.
    D2D1_COLOR_F grip = ThemeManager::Instance().GetTokens().textMuted;
    grip.a = active ? 0.9f : 0.55f;
    if (IsVerticalSplitter()) {
        float cx = m_bounds.x + m_bounds.width * 0.5f;
        float cy = m_bounds.y + m_bounds.height * 0.5f;
        ctx.FillRoundedRect(Rect(cx - 1.0f, cy - 14.0f, 2.0f, 28.0f), 1.0f, grip);
    } else {
        float cx = m_bounds.x + m_bounds.width * 0.5f;
        float cy = m_bounds.y + m_bounds.height * 0.5f;
        ctx.FillRoundedRect(Rect(cx - 18.0f, cy - 1.0f, 36.0f, 2.0f), 1.0f, grip);
    }
}

} // namespace CUI
