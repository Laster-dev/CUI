#pragma once
#include "../render/GraphicsContext.h"
#include "ChromiumScrollAnimator.h"
#include "ScrollbarAutoHide.h"
#include "UIElement.h"
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace CUI {

class BreadcrumbBar;

enum class FileBrowserMode {
    OpenFile,
    OpenFolder
};

struct FileBrowserEntry {
    std::string name;
    std::string fullPath;
    bool isDirectory = false;
};

// ScrollViewer-style viewport scroll for the file browser list (Chromium animation + chrome).
class FileBrowserScrollController {
public:
    void Reset();
    void SetViewport(const Rect& viewport);
    void SetContentHeight(float height);
    float GetOffset() const { return m_animator.Current(); }
    float GetMaxScroll() const;
    void JumpTo(float offset);
    void ScrollWheel(float delta, UIElement* owner = nullptr);
    void EnsureVisible(float itemTop, float itemBottom, UIElement* owner = nullptr);

    bool HandleMouseDown(Point pt, UIElement* owner);
    void HandleMouseMove(Point pt, UIElement* owner);
    void HandleMouseUp(UIElement* owner);
    void HandleMouseLeave(UIElement* owner);
    bool IsDraggingThumb() const { return m_draggingThumb; }
    bool IsPointerOverScrollbar(Point pt) const;

    bool Tick(UIElement* owner, float dtSeconds);
    bool NeedsAnimationTicks() const;
    void RenderChrome(GraphicsContext& ctx) const;

    Rect GetViewport() const { return m_viewport; }
    Rect GetTrackRect() const;
    Rect GetThumbRect() const;

private:
    void ClampOffset();
    bool AdvanceSmoothScroll(UIElement* owner);
    double SecondsSinceLastTick();
    void MarkScrollDirty(UIElement* owner) const;

    Rect m_viewport;
    float m_contentHeight = 0.0f;
    float m_offsetY = 0.0f;
    ChromiumScrollAnimator m_animator;
    ScrollbarAutoHide m_autoHide;
    bool m_draggingThumb = false;
    bool m_scrollbarHovered = false;
    float m_dragStartY = 0.0f;
    float m_dragStartOffset = 0.0f;
    LARGE_INTEGER m_qpcFreq{};
    LONGLONG m_lastAnimQpc = 0;

    static constexpr float kScrollbarInset = 3.0f;
    static constexpr float kScrollbarWidth = 8.0f;
};

// Shared self-drawn file/folder browser popup used by FilePicker / FolderPicker.
class FileBrowserSession {
public:
    void Configure(
        FileBrowserMode mode,
        const std::string& startPath,
        const std::vector<std::pair<std::string, std::string>>& filters,
        int filterIndex = 0);

    void Refresh();
    void GoUp();
    void NavigateTo(const std::string& path);
    void SetFilterIndex(int index);

    FileBrowserMode GetMode() const { return m_mode; }
    const std::string& GetCurrentPath() const { return m_currentPath; }
    const std::vector<FileBrowserEntry>& GetEntries() const { return m_entries; }
    const std::vector<std::pair<std::string, std::string>>& GetFilters() const { return m_filters; }
    int GetSelectedIndex() const { return m_selectedIndex; }
    void SetSelectedIndex(int index);
    FileBrowserScrollController& Scroll() { return m_scroll; }
    const FileBrowserScrollController& Scroll() const { return m_scroll; }
    void UpdateScrollMetrics(const Rect& pop);
    void ResetScroll();
    void EnsureRowVisible(int index, UIElement* owner = nullptr);
    int GetFilterIndex() const { return m_filterIndex; }
    const std::string& GetActiveFilterLabel() const;
    const std::string& GetTitle() const { return m_title; }

    // Returns true if a path was confirmed into outPath.
    bool TryConfirm(std::string& outPath) const;
    // Activate selected entry: navigate into folder, or confirm file.
    // Returns true if path confirmed.
    bool ActivateSelected(std::string& outPath);

    static constexpr float kPopupW = 460.0f;
    static constexpr float kPopupH = 360.0f;
    static constexpr float kHeaderH = 36.0f;
    static constexpr float kFooterH = 44.0f;
    static constexpr float kRowH = 28.0f;
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
    Rect RowRect(const Rect& pop, int index) const;

    float ContentHeight() const;
    int HitTestRow(const Rect& pop, Point pt) const;
    int HitTestFilterItem(const Rect& pop, Point pt) const;
    float FilterDropdownHeight() const;

    void Render(
        GraphicsContext& ctx,
        const Rect& popRect,
        float animProgress,
        int hoverRow,
        bool hoverUp,
        bool hoverFilter,
        bool hoverCancel,
        bool hoverConfirm,
        bool filterDropDownOpen = false,
        int hoverFilterItem = -1) const;

private:
    bool MatchesFilter(const std::string& fileName) const;
    std::string ResolveStartDirectory(const std::string& startPath) const;

    FileBrowserMode m_mode = FileBrowserMode::OpenFile;
    std::string m_title;
    std::string m_currentPath;
    std::vector<FileBrowserEntry> m_entries;
    std::vector<std::pair<std::string, std::string>> m_filters;
    int m_filterIndex = 0;
    int m_selectedIndex = -1;
    FileBrowserScrollController m_scroll;
};

std::string GetDefaultUserDirectory();
bool PathExistsUtf8(const std::string& pathUtf8);
bool IsDirectoryUtf8(const std::string& pathUtf8);
std::string ParentPathUtf8(const std::string& pathUtf8);
std::string JoinPathUtf8(const std::string& dir, const std::string& name);

std::vector<std::string> BuildFileBrowserBreadcrumb(const std::string& currentPath);
std::string ResolveFileBrowserBreadcrumbPath(int index, const std::vector<std::string>& nodes);

// Embeds BreadcrumbBar for the file browser popup header.
class FileBrowserBreadcrumbHost {
public:
    using NavigateCallback = std::function<void(const std::string& path)>;

    FileBrowserBreadcrumbHost();
    void SetNavigateHandler(NavigateCallback handler);
    void Sync(const FileBrowserSession& session);
    void Layout(const FileBrowserSession& session, const Rect& pop);
    void Render(GraphicsContext& ctx);
    bool HandleMouseDown(Point pt);
    const Rect& GetBounds() const { return m_bounds; }

private:
    std::shared_ptr<BreadcrumbBar> m_bar;
    NavigateCallback m_onNavigate;
    Rect m_bounds;
};

} // namespace CUI
