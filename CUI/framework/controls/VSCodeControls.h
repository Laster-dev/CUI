#pragma once
#include "UIElement.h"
#include "Control.h"
#include <string>
#include <vector>

#include "MenuBar.h"

namespace CUI {

// 1. TitleBar
class TitleBar : public Control {
public:
    TitleBar();
    virtual ~TitleBar() = default;

    virtual const char* GetClassName() const override { return "TitleBar"; }
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseLeave() override;

    virtual UIElement* HitTest(float x, float y) override;

    MenuBar& GetMenuBar() { return m_menuBar; }

private:
    MenuBar m_menuBar;
};

// 2. ActivityBar
class ActivityBar : public Control {
public:
    struct Item {
        std::string icon;
        std::string name;
    };

    ActivityBar();
    virtual ~ActivityBar() = default;

    virtual const char* GetClassName() const override { return "ActivityBar"; }
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;

    int GetSelectedIndex() const { return m_selectedIndex; }
    void SetSelectedIndex(int index) { m_selectedIndex = index; }

private:
    std::vector<Item> m_items;
    int m_selectedIndex = 0;
};

// 3. SideBar
class SideBar : public Control {
public:
    struct TreeItem {
        std::string icon;
        std::string name;
        int level = 0;
        bool isFolder = false;
        bool isExpanded = true;
    };

    SideBar();
    virtual ~SideBar() = default;

    virtual const char* GetClassName() const override { return "SideBar"; }
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;

    void SetTitle(const std::string& title) { SetProperty("title", Value(title)); }

private:
    std::vector<TreeItem> m_fileTree;
    int m_selectedIndex = 2;
};

// 4. TabBar
class TabBar : public Control {
public:
    struct Tab {
        std::string icon;
        std::string title;
        bool isModified = false;
    };

    TabBar();
    virtual ~TabBar() = default;

    virtual const char* GetClassName() const override { return "TabBar"; }
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;

    int GetActiveIndex() const { return m_activeIndex; }
    void SetActiveIndex(int index) { m_activeIndex = index; }
    void AddTab(const std::string& icon, const std::string& title, bool modified = false);

private:
    std::vector<Tab> m_tabs;
    int m_activeIndex = 0;
};

// 5. EditorView
class EditorView : public Control {
public:
    EditorView();
    virtual ~EditorView() = default;

    virtual const char* GetClassName() const override { return "EditorView"; }
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;

    void SetCodeContent(const std::vector<std::string>& lines) { m_lines = lines; }
    int GetCursorLine() const { return m_cursorLine; }
    int GetCursorCol() const { return m_cursorCol; }

private:
    std::vector<std::string> m_lines;
    int m_cursorLine = 14;
    int m_cursorCol = 28;
};

// 6. StatusBar
class StatusBar : public Control {
public:
    StatusBar();
    virtual ~StatusBar() = default;

    virtual const char* GetClassName() const override { return "StatusBar"; }
    virtual void OnRender(GraphicsContext& ctx) override;
};

} // namespace CUI
