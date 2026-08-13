#pragma once
#include "../render/GraphicsContext.h"
#include "UIElement.h"
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace CUI {

class BreadcrumbBar;
class TreeView;
struct TreeViewItem;

enum class FileBrowserMode {
    OpenFile,
    OpenFolder
};

// Shared layout + chrome for FilePicker / FolderPicker popup.
class FileBrowserSession {
public:
    void Configure(
        FileBrowserMode mode,
        const std::string& startPath,
        const std::vector<std::pair<std::string, std::string>>& filters,
        int filterIndex = 0);

    void SetFilterIndex(int index);
    void SetCurrentPath(const std::string& path);

    FileBrowserMode GetMode() const { return m_mode; }
    const std::string& GetCurrentPath() const { return m_currentPath; }
    const std::vector<std::pair<std::string, std::string>>& GetFilters() const { return m_filters; }
    int GetFilterIndex() const { return m_filterIndex; }
    const std::string& GetActiveFilterLabel() const;
    const std::string& GetTitle() const { return m_title; }
    bool MatchesFilter(const std::string& fileName) const;

    static constexpr float kPopupW = 520.0f;
    static constexpr float kPopupH = 420.0f;
    static constexpr float kHeaderH = 36.0f;
    static constexpr float kFooterH = 44.0f;
    static constexpr float kFilterItemH = 26.0f;

    Rect HeaderRect(const Rect& pop) const;
    Rect UpButtonRect(const Rect& pop) const;
    Rect BreadcrumbRect(const Rect& pop) const;
    Rect FilterButtonRect(const Rect& pop) const;
    Rect FilterDropdownRect(const Rect& pop) const;
    Rect FilterItemRect(const Rect& pop, int index) const;
    Rect ListRect(const Rect& pop) const;
    Rect FooterRect(const Rect& pop) const;
    Rect CancelButtonRect(const Rect& pop) const;
    Rect ConfirmButtonRect(const Rect& pop) const;

    int HitTestFilterItem(const Rect& pop, Point pt) const;
    float FilterDropdownHeight() const;

    void RenderChrome(
        GraphicsContext& ctx,
        const Rect& popRect,
        float animProgress,
        bool hoverUp,
        bool hoverFilter,
        bool hoverCancel,
        bool hoverConfirm,
        bool filterDropDownOpen = false) const;

    // Drawn after tree/breadcrumb so the menu is not covered by list content.
    void RenderFilterDropdown(
        GraphicsContext& ctx,
        const Rect& popRect,
        float animProgress,
        int hoverFilterItem = -1) const;

private:
    std::string ResolveStartDirectory(const std::string& startPath) const;

    FileBrowserMode m_mode = FileBrowserMode::OpenFile;
    std::string m_title;
    std::string m_currentPath;
    std::vector<std::pair<std::string, std::string>> m_filters;
    int m_filterIndex = 0;
};

std::string GetDefaultUserDirectory();
bool PathExistsUtf8(const std::string& pathUtf8);
bool IsDirectoryUtf8(const std::string& pathUtf8);
std::string ParentPathUtf8(const std::string& pathUtf8);
std::string JoinPathUtf8(const std::string& dir, const std::string& name);

std::vector<std::string> BuildFileBrowserBreadcrumb(const std::string& currentPath);
std::string ResolveFileBrowserBreadcrumbPath(int index, const std::vector<std::string>& nodes);

class FileBrowserBreadcrumbHost {
public:
    using NavigateCallback = std::function<void(const std::string& path)>;

    FileBrowserBreadcrumbHost();
    void AttachTo(UIElement* owner);
    void SetNavigateHandler(NavigateCallback handler);
    void Sync(const std::string& currentPath);
    void Layout(const FileBrowserSession& session, const Rect& pop);
    void Render(GraphicsContext& ctx);
    UIElement* HitTest(float x, float y);
    void DismissOverflowMenu();
    bool IsOverflowMenuOpen() const;
    Rect GetOverflowMenuClientBounds() const;
    const Rect& GetBounds() const { return m_bounds; }
    BreadcrumbBar* GetBar() const { return m_bar.get(); }

private:
    std::shared_ptr<BreadcrumbBar> m_bar;
    NavigateCallback m_onNavigate;
    Rect m_bounds;
};

// TreeView-backed file/folder browser body.
class FileBrowserTreeHost {
public:
    using PathChangedCallback = std::function<void(const std::string& path)>;
    using ConfirmCallback = std::function<void(const std::string& path)>;

    FileBrowserTreeHost();
    void AttachTo(UIElement* owner);

    void SetPathChangedHandler(PathChangedCallback handler);
    void SetConfirmHandler(ConfirmCallback handler);

    void Configure(const FileBrowserSession& session);
    void ApplyFilter(const FileBrowserSession& session);
    void NavigateTo(const std::string& path, const FileBrowserSession& session);
    void GoUp(const FileBrowserSession& session);

    void Layout(const Rect& listRect);
    void Render(GraphicsContext& ctx);
    UIElement* HitTest(float x, float y);

    bool Tick();
    bool NeedsAnimationTicks() const;

    std::string GetSelectedPath() const;
    bool TryConfirm(const FileBrowserSession& session, std::string& outPath) const;
    TreeView* GetTree() const { return m_tree.get(); }

private:
    std::shared_ptr<TreeViewItem> MakePlaceholderChild() const;
    std::shared_ptr<TreeViewItem> MakeDirNode(const std::string& name, const std::string& fullPath) const;
    std::shared_ptr<TreeViewItem> MakeFileNode(const std::string& name, const std::string& fullPath) const;
    void LoadChildren(const std::shared_ptr<TreeViewItem>& node, const FileBrowserSession& session);
    bool EnsureChildrenLoaded(const std::shared_ptr<TreeViewItem>& node, const FileBrowserSession& session);
    std::shared_ptr<TreeViewItem> FindNodeByPath(const std::string& path) const;
    std::shared_ptr<TreeViewItem> FindNodeByPath(
        const std::vector<std::shared_ptr<TreeViewItem>>& items,
        const std::string& path) const;
    void ExpandToPath(const std::string& path, const FileBrowserSession& session);
    void RebuildRoots(const FileBrowserSession& session);

    std::shared_ptr<TreeView> m_tree;
    PathChangedCallback m_onPathChanged;
    ConfirmCallback m_onConfirm;
    FileBrowserMode m_mode = FileBrowserMode::OpenFile;
    std::vector<std::pair<std::string, std::string>> m_filters;
    int m_filterIndex = 0;
    bool m_loadingGuard = false;

    FileBrowserSession MakeSession(const std::string& pathHint = {}) const;
};

} // namespace CUI
