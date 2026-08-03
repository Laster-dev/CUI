#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Splitter.h"
#include "../style/ThemeManager.h"
#include <algorithm>

namespace CUI {

Splitter::Splitter() {
    SetProperty("orientation", Value("Vertical"));
    SetProperty("theme.backgroundToken", Value("cardBorder"));
    SetProperty("theme.hoverBackgroundToken", Value("accentColor"));
    SetProperty("background", Value(ThemeManager::Instance().GetColor("cardBorder")));
    SetProperty("hoverBackground", Value(ThemeManager::Instance().GetColor("accentColor")));
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
                    if (siblings[prev] && siblings[prev]->GetProperty("visibility").AsString("Visible") != "Collapsed") {
                        prevElem = siblings[prev];
                        break;
                    }
                }

                for (size_t next = i + 1; next < siblings.size(); ++next) {
                    if (siblings[next] && siblings[next]->GetProperty("visibility").AsString("Visible") != "Collapsed") {
                        nextElem = siblings[next];
                        break;
                    }
                }

                if (prevElem && nextElem) {
                    if (orient == "Vertical") {
                        float prevMin = std::max(20.0f, prevElem->GetProperty("minWidth").AsFloat(0.0f));
                        float nextMin = std::max(20.0f, nextElem->GetProperty("minWidth").AsFloat(0.0f));
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
                            prevElem->SetProperty("width", Value(prevWidth + appliedDelta));
                            nextElem->SetProperty("width", Value(nextWidth - appliedDelta));
                        }
                    } else {
                        float prevMin = std::max(20.0f, prevElem->GetProperty("minHeight").AsFloat(0.0f));
                        float nextMin = std::max(20.0f, nextElem->GetProperty("minHeight").AsFloat(0.0f));
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
                            prevElem->SetProperty("height", Value(prevHeight + appliedDelta));
                            nextElem->SetProperty("height", Value(nextHeight - appliedDelta));
                        }
                    }
                } else if (prevElem) {
                    if (orient == "Vertical") {
                        float newW = std::max(20.0f, prevElem->GetBounds().width + delta);
                        prevElem->SetProperty("width", Value(newW));
                    } else {
                        float newH = std::max(20.0f, prevElem->GetBounds().height + delta);
                        prevElem->SetProperty("height", Value(newH));
                    }
                }
                break;
            }

            UIElement* root = m_parent;
            while (root && root->GetParent()) {
                root = root->GetParent();
            }
            if (root) {
                Rect rootBounds = root->GetBounds();
                root->Measure(Size(rootBounds.width, rootBounds.height));
                root->Arrange(rootBounds);
            }
        }

        m_onSplitterMovedEvent.Invoke(this, delta);
        m_dragStartPt = pt;
    }
}

void Splitter::OnMouseUp(Point pt) {
    Control::OnMouseUp(pt);
    m_isDragging = false;
}

void Splitter::OnRender(GraphicsContext& ctx) {
    D2D1_COLOR_F bg = ResolveThemeColor("theme.backgroundToken", "cardBorder");
    D2D1_COLOR_F hoverBg = ResolveThemeColor("theme.hoverBackgroundToken", "accentColor");

    ctx.FillRect(m_bounds, (m_isHovered || m_isDragging) ? hoverBg : bg);
}

} // namespace CUI
