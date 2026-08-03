#pragma once
#include "../core/Object.h"
#include "../render/GraphicsContext.h"
#include "../render/RenderNode.h"
#include <vector>
#include <memory>
#include <string>
#include <chrono>

namespace CUI {

class ContextMenu;

struct PropertyMeta {
    std::string name;          // 英文属性名 (如 "width")
    std::string displayName;   // 中文名称 (如 "宽度 (Width)")
    std::string category;      // 分组 (布局/外观/字体/控制)
    std::string type;          // "string", "number", "color", "bool", "enum"
    std::vector<std::string> options; // 枚举可选项
};

class UIElement : public Object {
public:
    UIElement();
    virtual ~UIElement() = default;

    virtual const char* GetClassName() const override { return "UIElement"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const;

    // Resolve a paint color exclusively through ThemeManager.
    // If theme.*Token is set, that token wins; otherwise fallbackToken is used.
    // Stored RGB properties are never the source of truth.
    D2D1_COLOR_F ResolveThemeColor(const char* tokenProp, const char* fallbackToken) const;

    std::string GetId() const { return m_id; }
    void SetId(const std::string& id) { m_id = id; }

    std::string GetToolTip() const { return GetProperty("toolTip").AsString(""); }
    void SetToolTip(const std::string& tip) { SetProperty("toolTip", Value(tip)); }

    // Layout properties
    Thickness GetMargin() const { return GetProperty("margin").AsThickness(); }
    void SetMargin(const Thickness& margin) { SetProperty("margin", Value(margin)); }

    Thickness GetPadding() const { return GetProperty("padding").AsThickness(); }
    void SetPadding(const Thickness& padding) { SetProperty("padding", Value(padding)); }

    bool IsEnabled() const { return GetProperty("isEnabled").AsBool(true); }
    void SetIsEnabled(bool enabled) { SetProperty("isEnabled", Value(enabled)); }

    std::string GetStyleClass() const { return m_styleClass; }
    void SetStyleClass(const std::string& styleClass) { m_styleClass = styleClass; }

    UIElement* GetParent() const { return m_parent; }
    void SetParent(UIElement* parent) { m_parent = parent; }

    const std::vector<std::shared_ptr<UIElement>>& GetChildren() const { return m_children; }
    void AddChild(std::shared_ptr<UIElement> child);
    void RemoveChild(std::shared_ptr<UIElement> child);
    void RemoveChildRaw(UIElement* child);
    void ClearChildren();

    std::shared_ptr<UIElement> FindElementById(const std::string& id);

    Rect GetBounds() const { return m_bounds; }
    void SetBounds(const Rect& bounds);

    Size GetDesiredSize() const { return m_desiredSize; }

    virtual Size Measure(Size availableSize);
    virtual void Arrange(Rect finalRect);
    virtual bool ShouldClipToBounds() const;
    virtual void Render(GraphicsContext& ctx);
    virtual void OnRender(GraphicsContext& ctx);

    virtual void RenderOverlay(GraphicsContext& ctx);
    virtual void OnRenderOverlay(GraphicsContext& ctx) {}

    virtual UIElement* HitTest(float x, float y);
    virtual UIElement* HitTestOverlay(float x, float y);
    virtual UIElement* OnHitTestOverlay(float x, float y) { return nullptr; }

    // Visual states
    bool IsHovered() const { return m_isHovered; }
    bool IsPressed() const { return m_isPressed; }
    bool IsFocused() const { return m_isFocused; }

    virtual HCURSOR GetCursor() const { return nullptr; }

    // Input events
    virtual void OnMouseEnter();
    virtual void OnMouseLeave();
    virtual void OnMouseDown(Point pt);
    virtual void OnMouseDblClick(Point pt) {}
    virtual void OnMouseRightClick(Point pt) {}
    virtual void OnMouseUp(Point pt);
    virtual void OnMouseMove(Point pt);
    virtual void OnMouseWheel(float delta);
    virtual void OnKeyDown(int vkCode);
    virtual void OnAutoScrollTick() {}
    virtual bool NeedsAutoScrollTick() const { return false; }
    // Returns true if any animation still needs frames (smooth scroll, etc.).
    virtual bool OnAnimationTick();
    virtual bool HasSelfAnimation() const { return false; }
    virtual void CollectAnimationBounds(Rect& dirtyRect, bool& hasDirty) const;
    virtual void CollectRenderDirtyRegion(DirtyRegion& dirtyRegion, bool consume = true);
    static void SetAnimationsEnabled(bool enabled);
    static bool AreAnimationsEnabled();
    static void SetAnimationDeltaSeconds(float dtSeconds);
    static float GetAnimationDeltaSeconds();
    RenderNode& GetRenderNode() { return m_renderNode; }
    const RenderNode& GetRenderNode() const { return m_renderNode; }
    virtual void SyncRenderState();
    virtual void MarkRenderContentDirty();

    void SetContextMenu(std::shared_ptr<ContextMenu> menu) { m_contextMenu = menu; }
    std::shared_ptr<ContextMenu> GetContextMenu() const { return m_contextMenu; }

    virtual void OnFocus() { SetProperty("focused", Value(true)); m_isFocused = true; }
    virtual void OnBlur() { SetProperty("focused", Value(false)); m_isFocused = false; }

    // Event delegates
    Event<UIElement*>& OnClick() { return m_onClickEvent; }
    Event<UIElement*, Point>& OnMouseDownEvent() { return m_onMouseDownEvent; }

protected:
    void MarkRenderRectDirty(const Rect& rect);

    std::string m_id;
    std::string m_styleClass;
    UIElement* m_parent = nullptr;
    std::vector<std::shared_ptr<UIElement>> m_children;

    Rect m_bounds;
    Size m_desiredSize;

    bool m_isHovered = false;
    bool m_isPressed = false;
    bool m_isFocused = false;
    Point m_lastMousePos{ 0.0f, 0.0f };
    Point m_tooltipAnchorPos{ 0.0f, 0.0f };
    std::chrono::steady_clock::time_point m_lastMouseMoveTime;
    bool m_tooltipVisible = false;

    Event<UIElement*> m_onClickEvent;
    Event<UIElement*, Point> m_onMouseDownEvent;
    std::shared_ptr<ContextMenu> m_contextMenu;
    static bool s_animationsEnabled;
    static float s_animationDeltaSeconds;
    RenderNode m_renderNode;
};

} // namespace CUI
