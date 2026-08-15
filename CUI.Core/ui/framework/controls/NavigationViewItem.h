#pragma once
#include "Control.h"
#include <memory>
#include <string>
#include <vector>

namespace CUI {

class NavigationView;

// WinUI: NavigationViewItemBase — common base for pane entries.
class NavigationViewItemBase : public Control {
public:
    NavigationViewItemBase() = default;
    ~NavigationViewItemBase() override = default;

    bool IsSelected() const { return m_isSelected; }
    void SetIsSelected(bool selected);

    // Depth in the menu tree (0 = top-level). Used for indent.
    int Depth() const { return m_depth; }
    void SetDepth(int depth) { m_depth = depth; }

    NavigationView* Owner() const { return m_owner; }
    void SetOwner(NavigationView* owner) { m_owner = owner; }

protected:
    bool m_isSelected = false;
    int m_depth = 0;
    NavigationView* m_owner = nullptr;
};

// WinUI: NavigationViewItemHeader — non-selectable group label.
class NavigationViewItemHeader : public NavigationViewItemBase {
public:
    NavigationViewItemHeader() = default;
    explicit NavigationViewItemHeader(const std::string& text);

    const char* GetClassName() const override { return "NavigationViewItemHeader"; }

    void SetText(const std::string& text);
    const std::string& GetText() const { return m_text; }

    Size Measure(Size availableSize) override;
    void OnRender(GraphicsContext& ctx) override;

private:
    std::string m_text;
};

// WinUI: NavigationViewItemSeparator — visual divider (or spacer when opacity 0).
class NavigationViewItemSeparator : public NavigationViewItemBase {
public:
    NavigationViewItemSeparator() = default;
    const char* GetClassName() const override { return "NavigationViewItemSeparator"; }

    Size Measure(Size availableSize) override;
    void OnRender(GraphicsContext& ctx) override;
};

// WinUI: NavigationViewItem — selectable destination with optional hierarchy.
class NavigationViewItem : public NavigationViewItemBase {
public:
    NavigationViewItem();
    NavigationViewItem(const std::string& content, const std::string& icon = std::string());

    const char* GetClassName() const override { return "NavigationViewItem"; }

    /**
     * @brief 导航项标题文本内容属性代理。
     */
    struct NavItemContentProperty {
        NavigationViewItem* owner;
        NavItemContentProperty& operator=(const std::string& c) { owner->SetContent(c); return *this; }
        operator const std::string&() const { return owner->GetContent(); }
        const std::string& Get() const { return owner->GetContent(); }
    } Content{this};

    void SetContent(const std::string& content);
    const std::string& GetContent() const { return m_content; }

    void SetIcon(const std::string& icon);
    const std::string& GetIcon() const { return m_icon; }

    void SetTag(const std::string& tag) { m_tag = tag; }
    const std::string& GetTag() const { return m_tag; }

    // If false, invoke expands/collapses children instead of selecting (WinUI).
    /**
     * @brief 导航项被点击激活时是否自动选中该项属性代理。
     */
    struct NavItemSelectsOnInvokedProperty {
        NavigationViewItem* owner = nullptr;
        NavItemSelectsOnInvokedProperty() = default;
        explicit NavItemSelectsOnInvokedProperty(NavigationViewItem* o) : owner(o) {}
        NavItemSelectsOnInvokedProperty& operator=(bool v) { if (owner) owner->SetSelectsOnInvoked(v); return *this; }
        operator bool() const { return owner ? owner->GetSelectsOnInvoked() : true; }
        bool Get() const { return owner ? owner->GetSelectsOnInvoked() : true; }
    } SelectsOnInvoked;

    void SetSelectsOnInvoked(bool value) { m_selectsOnInvoked = value; }
    bool GetSelectsOnInvoked() const { return m_selectsOnInvoked; }

    void SetIsExpanded(bool expanded);
    // Expand/collapse without firing OnExpandChanged (avoids sync Relayout on select).
    void SetIsExpandedSilent(bool expanded);
    bool IsExpanded() const { return m_isExpanded; }

    bool IsChildSelected() const { return m_isChildSelected; }
    void SetIsChildSelected(bool value) {
        if (m_isChildSelected == value) {
            return;
        }
        m_isChildSelected = value;
        MarkRenderRectDirty(m_bounds);
    }

    void AddMenuItem(const std::shared_ptr<NavigationViewItemBase>& item);
    const std::vector<std::shared_ptr<NavigationViewItemBase>>& MenuItems() const { return m_menuItems; }
    std::vector<std::shared_ptr<NavigationViewItemBase>>& MenuItems() { return m_menuItems; }
    bool HasChildren() const { return !m_menuItems.empty(); }

    // Compact / closed-compact: icon rail (hide label). Full: show label.
    void SetCompact(bool compact);
    bool IsCompact() const { return m_compact; }

    // Top nav: horizontal layout cue for indicator placement.
    void SetTopMode(bool top) { m_topMode = top; }
    bool IsTopMode() const { return m_topMode; }

    Size Measure(Size availableSize) override;
    void Arrange(Rect finalRect) override;
    void OnRender(GraphicsContext& ctx) override;
    void OnMouseDown(Point pt) override;
    void OnMouseEnter() override;
    void OnMouseLeave() override;
    bool OnAnimationTick() override;
    bool HasSelfAnimation() const override;

    // Raised by the item; NavigationView wires this to ItemInvoked / selection.
    Event<NavigationViewItem*>& OnInvoked() { return m_invoked; }
    Event<NavigationViewItem*>& OnExpandChanged() { return m_expandChanged; }

private:
    void StyleDefaults();
    void StartRipple(Point pt);
    Rect GetChevronRect() const;
    bool HitChevron(Point pt) const;

    std::string m_content;
    std::string m_icon;
    std::string m_tag;
    bool m_selectsOnInvoked = true;
    bool m_isExpanded = false;
    bool m_isChildSelected = false;
    bool m_compact = false;
    bool m_topMode = false;
    bool m_hovered = false;
    Point m_rippleCenter{};
    float m_rippleRadius = 0.0f;
    float m_rippleOpacity = 0.0f;
    bool m_rippleActive = false;
    std::vector<std::shared_ptr<NavigationViewItemBase>> m_menuItems;
    Event<NavigationViewItem*> m_invoked;
    Event<NavigationViewItem*> m_expandChanged;
};

} // namespace CUI
