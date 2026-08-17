#pragma once
#include <windows.h>
#include <unknwn.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <wrl/client.h>
#include "../core/Value.h"

namespace CUI {

using Microsoft::WRL::ComPtr;

/**
 * @brief 渲染缓存版本戳。
 *
 * 缓存是否有效不仅看 m_valid / dirty 位，还必须与当前帧的 VisualTree / 布局 /
 * 主题 / 材质代次、表面尺寸、DPI 与透明模式一致。任意一项不一致都必须重新光栅化，
 * 防止旧页面像素（或旧材质帧）继续参与当前帧合成。
 */
struct RenderCacheStamp {
    uint64_t visualTreeGeneration = 0; // VisualTree 结构代次（切页 / 增删子节点）
    uint64_t layoutGeneration = 0;     // 布局失效代次
    uint64_t themeGeneration = 0;      // 主题代次
    uint64_t materialGeneration = 0;   // 材质代次
    Size surfaceSize;                  // 光栅化时的表面尺寸
    float dpiScale = 1.0f;             // DPI 缩放
    bool transparent = false;          // 光栅化时根表面是否透明

    bool operator==(const RenderCacheStamp& o) const {
        return visualTreeGeneration == o.visualTreeGeneration
            && layoutGeneration == o.layoutGeneration
            && themeGeneration == o.themeGeneration
            && materialGeneration == o.materialGeneration
            && surfaceSize.width == o.surfaceSize.width
            && surfaceSize.height == o.surfaceSize.height
            && dpiScale == o.dpiScale
            && transparent == o.transparent;
    }
    bool operator!=(const RenderCacheStamp& o) const { return !(*this == o); }
};

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
    void ClearDirtyFlags(unsigned flags);
    void Validate();
    void ResetCache();

    // 缓存版本戳：光栅化完成后记录当前帧上下文，供 Window 逐帧校验。
    const RenderCacheStamp& GetStamp() const { return m_stamp; }
    void SetStamp(const RenderCacheStamp& stamp) { m_stamp = stamp; }
    bool StampMatches(const RenderCacheStamp& stamp) const { return m_stamp == stamp; }

    bool IsValid() const { return m_valid; }
    bool HasDirtyFlags() const { return m_dirtyFlags != None; }
    unsigned GetDirtyFlags() const { return m_dirtyFlags; }
    bool NeedsContentRaster() const;
    bool NeedsComposeOnly() const;

private:
    friend class GraphicsContext;

    bool m_cacheable = false;
    bool m_valid = false;
    RenderCacheStamp m_stamp; // 最近一次成功光栅化时的帧上下文版本戳
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
