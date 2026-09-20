#pragma once

#include "DirtyRegion.h"
#include "RenderLayer.h"

namespace CUI {

class UIElement;

class RenderNode {
public:
    explicit RenderNode(UIElement* owner = nullptr);

    void ApplyOwner(UIElement* owner) { m_owner = owner; }
    UIElement* GetOwner() const { return m_owner; }

    RenderLayer& GetLayer() { return m_layer; }
    const RenderLayer& GetLayer() const { return m_layer; }

    void ApplyBounds(const Rect& bounds);
    const Rect& GetBounds() const { return m_bounds; }
    const Rect& GetPreviousBounds() const { return m_previousBounds; }

    void MarkContentDirty();
    void MarkStructureDirty();
    void MarkTransformDirty(const Rect& oldBounds, const Rect& newBounds);
    void MarkDirtyRect(const Rect& rect);

    const DirtyRegion& GetLocalDirtyRegion() const { return m_localDirty; }
    const DirtyRegion& GetWorldDirtyRegion() const { return m_worldDirty; }
    DirtyRegion ConsumeWorldDirtyRegion();

    void SyncLayerState();

private:
    UIElement* m_owner = nullptr;
    RenderLayer m_layer;
    Rect m_bounds;
    Rect m_previousBounds;
    DirtyRegion m_localDirty;
    DirtyRegion m_worldDirty;
};

} // namespace CUI
