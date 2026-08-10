#pragma once

#include "../core/Value.h"
#include <memory>
#include <vector>

namespace CUI {

// Scene-graph node independent of UIElement (Flutter Layer / browser GraphicsLayer analogue).
enum class CompositionLayerKind {
    Container,
    Picture,
    Offset,
    Opacity
};

class CompositionLayer {
public:
    virtual ~CompositionLayer() = default;
    virtual CompositionLayerKind Kind() const = 0;

    void SetOffset(float x, float y) { m_offsetX = x; m_offsetY = y; }
    float OffsetX() const { return m_offsetX; }
    float OffsetY() const { return m_offsetY; }

    void SetOpacity(float o) { m_opacity = o; }
    float Opacity() const { return m_opacity; }

    void SetBounds(const Rect& r) { m_bounds = r; }
    const Rect& Bounds() const { return m_bounds; }

    bool NeedsRaster() const { return m_needsRaster; }
    void SetNeedsRaster(bool v) { m_needsRaster = v; }

    bool NeedsCompose() const { return m_needsCompose; }
    void SetNeedsCompose(bool v) { m_needsCompose = v; }

protected:
    float m_offsetX = 0.0f;
    float m_offsetY = 0.0f;
    float m_opacity = 1.0f;
    Rect m_bounds{};
    bool m_needsRaster = true;
    bool m_needsCompose = true;
};

class ContainerLayer : public CompositionLayer {
public:
    CompositionLayerKind Kind() const override { return CompositionLayerKind::Container; }
    void Add(std::shared_ptr<CompositionLayer> child) {
        if (child) m_children.push_back(std::move(child));
    }
    void Clear() { m_children.clear(); }
    const std::vector<std::shared_ptr<CompositionLayer>>& Children() const { return m_children; }

private:
    std::vector<std::shared_ptr<CompositionLayer>> m_children;
};

class OffsetLayer : public CompositionLayer {
public:
    CompositionLayerKind Kind() const override { return CompositionLayerKind::Offset; }
    void SetChild(std::shared_ptr<CompositionLayer> child) { m_child = std::move(child); }
    std::shared_ptr<CompositionLayer> Child() const { return m_child; }

private:
    std::shared_ptr<CompositionLayer> m_child;
};

class OpacityLayer : public CompositionLayer {
public:
    CompositionLayerKind Kind() const override { return CompositionLayerKind::Opacity; }
    void SetChild(std::shared_ptr<CompositionLayer> child) { m_child = std::move(child); }
    std::shared_ptr<CompositionLayer> Child() const { return m_child; }

private:
    std::shared_ptr<CompositionLayer> m_child;
};

// PictureLayer wraps a retained bitmap (backed by RenderLayer in paint path).
class PictureLayer : public CompositionLayer {
public:
    CompositionLayerKind Kind() const override { return CompositionLayerKind::Picture; }
};

} // namespace CUI
