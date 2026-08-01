#pragma once

#include "../core/Value.h"
#include <d2d1_1.h>
#include <wrl/client.h>

namespace CUI {

using Microsoft::WRL::ComPtr;

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
    void SetTranslation(float x, float y) { m_translationX = x; m_translationY = y; }
    float GetTranslationX() const { return m_translationX; }
    float GetTranslationY() const { return m_translationY; }

    void SetLastRenderedBounds(const Rect& bounds) { m_lastRenderedBounds = bounds; }
    const Rect& GetLastRenderedBounds() const { return m_lastRenderedBounds; }
    void SetCacheSurfaceSize(const Size& size) { m_cacheSurfaceSize = size; }
    const Size& GetCacheSurfaceSize() const { return m_cacheSurfaceSize; }
    ID2D1DeviceContext* GetCacheContext() const { return m_cacheContext.Get(); }
    ID2D1Bitmap1* GetCacheBitmap() const { return m_cacheBitmap.Get(); }
    ID2D1Bitmap1* GetScratchBitmap() const { return m_scratchBitmap.Get(); }

    void Invalidate(unsigned flags);
    void Validate();
    void ResetCache();

    bool IsValid() const { return m_valid; }
    bool HasDirtyFlags() const { return m_dirtyFlags != None; }
    unsigned GetDirtyFlags() const { return m_dirtyFlags; }

private:
    friend class GraphicsContext;

    bool m_cacheable = false;
    bool m_valid = false;
    unsigned m_dirtyFlags = ContentDirty | TransformDirty | ClipDirty | SizeDirty | StructureDirty;
    Rect m_bounds;
    Rect m_lastRenderedBounds;
    float m_translationX = 0.0f;
    float m_translationY = 0.0f;
    Size m_cacheSurfaceSize;
    ComPtr<ID2D1DeviceContext> m_cacheContext;
    ComPtr<ID2D1Bitmap1> m_cacheBitmap;
    ComPtr<ID2D1Bitmap1> m_scratchBitmap;
};

} // namespace CUI
