#pragma once

#include "../core/Value.h"
#include <vector>

namespace CUI {

class DirtyRegion {
public:
    void Clear();
    bool IsEmpty() const;

    void AddRect(const Rect& rect);
    void AddRectInflated(const Rect& rect, float amount);
    void UnionWith(const DirtyRegion& other);

    const std::vector<Rect>& GetRects() const { return m_rects; }
    Rect GetBounds() const;
    size_t GetRectCount() const { return m_rects.size(); }

    void SetMaxRects(size_t maxRects) { m_maxRects = maxRects; }
    // Collapse all rects into their union (used before HWND invalidate).
    void CollapseToBounds();

private:
    void CollapseIfNeeded();

    std::vector<Rect> m_rects;
    size_t m_maxRects = 12;
};

} // namespace CUI
