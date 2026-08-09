#pragma once
#include "../../../ui/framework/controls/UIElement.h"
#include "../../../ui/framework/controls/Control.h"
#include "../../../ui/framework/controls/MenuBar.h"
#include "../../../ui/framework/core/Event.h"
#include "../../../ui/framework/window/IWindowChrome.h"
#include <string>
#include <vector>

namespace CUI {

class TitleBar : public Control, public IWindowChrome {
public:
    TitleBar();
    virtual ~TitleBar() = default;

    virtual const char* GetClassName() const override { return "TitleBar"; }
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual void OnBlur() override;
    virtual UIElement* HitTest(float x, float y) override;

    virtual UIElement* GetChromeElement() override { return this; }
    virtual const UIElement* GetChromeElement() const override { return this; }
    virtual bool IsInteractiveHit(float x, float y) const override;
    virtual bool IsCaptionDragHit(float x, float y, UIElement* treeHit) const override;
    virtual bool ConsumeChromeDirty() override;

    MenuBar& GetMenuBar() { return m_menuBar; }
    bool IsMenuBarHit(float x, float y) const;
    Rect GetLowPerformanceToggleRect() const;
    bool IsLowPerformanceToggleHit(float x, float y) const;
    Rect GetThemeToggleRect() const;
    bool IsThemeToggleHit(float x, float y) const;

    void SetTitle(const std::string& title) { m_title = title; }
    const std::string& GetTitle() const { return m_title; }

    Event<TitleBar*>& OnToggleLowPerformance() { return m_onToggleLowPerformance; }
    Event<TitleBar*>& OnToggleTheme() { return m_onToggleTheme; }

private:
    MenuBar m_menuBar;
    std::string m_title;
    bool m_menuChromeDirty = false;
    Event<TitleBar*> m_onToggleLowPerformance;
    Event<TitleBar*> m_onToggleTheme;
};

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

    void SetTitle(const std::string& title) { m_title = title; }
    const std::string& GetTitle() const { return m_title; }

private:
    std::vector<TreeItem> m_fileTree;
    int m_selectedIndex = 2;
    std::string m_title;
};

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

class StatusBar : public Control {
public:
    StatusBar();
    virtual ~StatusBar() = default;

    virtual const char* GetClassName() const override { return "StatusBar"; }
    virtual void OnRender(GraphicsContext& ctx) override;

    const std::string& GetBranch() const { return m_branch; }
    void SetBranch(const std::string& branch) { m_branch = branch; MarkRenderContentDirty(); }

    const std::string& GetStatus() const { return m_status; }
    void SetStatus(const std::string& status) { m_status = status; MarkRenderContentDirty(); }

    int GetLine() const { return m_line; }
    void SetLine(int line) { m_line = line; MarkRenderContentDirty(); }

    int GetCol() const { return m_col; }
    void SetCol(int col) { m_col = col; MarkRenderContentDirty(); }

private:
    std::string m_branch{ "main*" };
    std::string m_status{ "Ready" };
    int m_line = 14;
    int m_col = 28;
};

} // namespace CUI
