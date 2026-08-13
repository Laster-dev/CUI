#pragma once
#include "Control.h"
#include <memory>
#include <string>
#include <vector>

namespace CUI {

class ContextMenu;

class BreadcrumbBar : public Control {
public:
    BreadcrumbBar();
    virtual ~BreadcrumbBar() = default;

    virtual const char* GetClassName() const override { return "BreadcrumbBar"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;

    void SetPath(const std::vector<std::string>& pathNodes);
    void PushNode(const std::string& node);
    void PopNode();
    const std::vector<std::string>& GetPath() const { return m_pathNodes; }
    void DismissOverflowMenu();
    bool IsOverflowMenuOpen() const;
    // Owner-client bounds of the overflow menu when it is an in-window overlay.
    // Empty when closed or hosted on an external HWND.
    Rect GetOverflowMenuClientBounds() const;

    // Fired for visible crumbs and for items chosen from the overflow ("...") menu.
    Event<BreadcrumbBar*, int, const std::string&>& OnItemClicked() { return m_onItemClickedEvent; }

private:
    enum class SlotKind { Item, Ellipsis, Separator };

    struct VisualSlot {
        SlotKind kind = SlotKind::Item;
        int pathIndex = -1;
        Rect bounds;
        std::vector<int> collapsedIndices;
    };

    void RebuildVisualSlots(GraphicsContext& ctx);
    void ShowOverflowMenu(const VisualSlot& ellipsisSlot);
    float MeasureNodeWidth(GraphicsContext& ctx, size_t index) const;
    float MeasureSepWidth(GraphicsContext& ctx) const;
    float MeasureEllipsisWidth(GraphicsContext& ctx) const;

    std::vector<std::string> m_pathNodes;
    std::vector<VisualSlot> m_slots;
    Event<BreadcrumbBar*, int, const std::string&> m_onItemClickedEvent;
    std::shared_ptr<ContextMenu> m_overflowMenu;
};

} // namespace CUI
