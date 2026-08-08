#include "RenderNode.h"

namespace CUI {

RenderNode::RenderNode(UIElement* owner) : m_owner(owner) {}

void RenderNode::SetBounds(const Rect& bounds) {
    if (bounds.x == m_bounds.x && bounds.y == m_bounds.y
        && bounds.width == m_bounds.width && bounds.height == m_bounds.height) {
        return;
    }

    const bool sizeChanged =
        std::abs(bounds.width - m_bounds.width) > 0.5f
        || std::abs(bounds.height - m_bounds.height) > 0.5f;

    MarkTransformDirty(m_bounds, bounds);
    m_previousBounds = m_bounds;
    m_bounds = bounds;
    m_layer.SetBounds(bounds);
    // Translation-only moves (ScrollViewer offset) must NOT SizeDirty — that
    // invalidates cached bitmaps and forces full PropertyGrid re-raster on scroll.
    if (sizeChanged) {
        m_layer.Invalidate(RenderLayer::TransformDirty | RenderLayer::SizeDirty);
    } else {
        m_layer.Invalidate(RenderLayer::TransformDirty);
    }
}

void RenderNode::MarkContentDirty() {
    m_localDirty.AddRect(m_bounds);
    m_worldDirty.AddRect(m_bounds);
    m_layer.Invalidate(RenderLayer::ContentDirty);
}

void RenderNode::MarkStructureDirty() {
    m_localDirty.AddRect(m_bounds);
    m_worldDirty.AddRect(m_bounds);
    m_layer.Invalidate(RenderLayer::StructureDirty);
}

void RenderNode::MarkTransformDirty(const Rect& oldBounds, const Rect& newBounds) {
    if (!oldBounds.IsEmpty()) {
        m_localDirty.AddRect(oldBounds);
        m_worldDirty.AddRect(oldBounds);
    }
    if (!newBounds.IsEmpty()) {
        m_localDirty.AddRect(newBounds);
        m_worldDirty.AddRect(newBounds);
    }
    m_layer.Invalidate(RenderLayer::TransformDirty);
}

void RenderNode::MarkDirtyRect(const Rect& rect) {
    m_localDirty.AddRect(rect);
    m_worldDirty.AddRect(rect);
    m_layer.Invalidate(RenderLayer::ContentDirty);
}

DirtyRegion RenderNode::ConsumeWorldDirtyRegion() {
    DirtyRegion copy = m_worldDirty;
    m_localDirty.Clear();
    m_worldDirty.Clear();
    return copy;
}

void RenderNode::SyncLayerState() {
    m_layer.SetBounds(m_bounds);
}

} // namespace CUI
