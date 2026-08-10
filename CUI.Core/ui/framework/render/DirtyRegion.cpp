#include "DirtyRegion.h"

namespace CUI {

void DirtyRegion::Clear() {
    m_rects.clear();
}

bool DirtyRegion::IsEmpty() const {
    return m_rects.empty();
}

void DirtyRegion::AddRect(const Rect& rect) {
    if (rect.IsEmpty()) {
        return;
    }
    m_rects.push_back(rect);
    CollapseIfNeeded();
}

void DirtyRegion::AddRectInflated(const Rect& rect, float amount) {
    AddRect(rect.Inflate(amount));
}

void DirtyRegion::UnionWith(const DirtyRegion& other) {
    for (const auto& rect : other.m_rects) {
        AddRect(rect);
    }
}

Rect DirtyRegion::GetBounds() const {
    Rect bounds;
    bool hasBounds = false;
    for (const auto& rect : m_rects) {
        if (rect.IsEmpty()) {
            continue;
        }
        bounds = hasBounds ? bounds.Union(rect) : rect;
        hasBounds = true;
    }
    return hasBounds ? bounds : Rect();
}

void DirtyRegion::CollapseIfNeeded() {
    if (m_rects.size() <= m_maxRects) {
        return;
    }

    CollapseToBounds();
}

void DirtyRegion::CollapseToBounds() {
    Rect bounds = GetBounds();
    m_rects.clear();
    if (!bounds.IsEmpty()) {
        m_rects.push_back(bounds);
    }
}

} // namespace CUI
