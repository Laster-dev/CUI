#pragma once

#include "../core/Value.h"

namespace CUI {

class RenderLayer {
public:
    enum DirtyFlags : unsigned {
        None = 0,
        ContentDirty = 1 << 0,
        TransformDirty = 1 << 1,
        ClipDirty = 1 << 2,
        OpacityDirty = 1 << 3,
        SizeDirty = 1 << 4,
        StructureDirty = 1 << 5
    };

    void SetCacheable(bool cacheable) { m_cacheable = cacheable; }
    bool IsCacheable() const { return m_cacheable; }

    void SetBounds(const Rect& bounds) { m_bounds = bounds; }
    const Rect& GetBounds() const { return m_bounds; }

    void SetLastRenderedBounds(const Rect& bounds) { m_lastRenderedBounds = bounds; }
    const Rect& GetLastRenderedBounds() const { return m_lastRenderedBounds; }

    void Invalidate(unsigned flags);
    void Validate();

    bool IsValid() const { return m_valid; }
    bool HasDirtyFlags() const { return m_dirtyFlags != None; }
    unsigned GetDirtyFlags() const { return m_dirtyFlags; }

private:
    bool m_cacheable = false;
    bool m_valid = false;
    unsigned m_dirtyFlags = ContentDirty | TransformDirty | ClipDirty | SizeDirty | StructureDirty;
    Rect m_bounds;
    Rect m_lastRenderedBounds;
};

} // namespace CUI
