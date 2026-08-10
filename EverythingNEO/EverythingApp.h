#pragma once

#include "EverythingEngine.h"
#include "SearchTypes.h"
#include "framework/window/Window.h"
#include "framework/controls/ListView.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <mutex>
#include <thread>
#include <deque>
#include <condition_variable>
#include <windows.h>

namespace CUI {
class WindowTitleBar;
class TextBox;
class TextBlock;
class ComboBox;
class UIElement;
class MenuItem;
class ContextMenu;
}

namespace EverythingNEO {

class FileIconCache {
public:
    FileIconCache();
    ~FileIconCache();

    void SetNotifyHwnd(HWND hwnd, UINT msg);
    void ResetNotifyFlag() { m_notifyPosted.store(false); }
    HICON GetIcon(const std::string& fullPath, const std::string& fileName, bool isFolder);
    void Clear();

private:
    void WorkerLoop();
    HICON GetExtIconUnlocked(const std::string& fileName);
    HICON GetFolderIconUnlocked();
    static std::string ExtKey(const std::string& fileName);

    std::mutex m_mutex;
    std::unordered_map<std::string, HICON> m_byExt;
    std::unordered_map<std::string, HICON> m_byPath;
    std::unordered_set<std::string> m_pending;
    std::deque<std::string> m_queue;
    std::condition_variable m_cv;
    std::thread m_worker;
    std::atomic<bool> m_stop{ false };
    std::atomic<bool> m_notifyPosted{ false };
    HICON m_defaultIcon = nullptr;
    HICON m_folderIcon = nullptr;
    HWND m_notifyHwnd = nullptr;
    UINT m_notifyMsg = 0;
};

enum class ListDisplayMode {
    FrequentFiles,
    SearchResults
};

struct FrequentFileEntry {
    std::string path;
    bool is_folder = false;
    uint32_t use_count = 0;
};

class ResultsDataSource : public CUI::ListViewDataSource {
public:
    EverythingEngine* engine = nullptr;
    std::vector<SearchResultRef>* results = nullptr;
    std::vector<FrequentFileEntry>* frequentFiles = nullptr;
    ListDisplayMode* displayMode = nullptr;
    FileIconCache* icons = nullptr;

    void ClearCaches();
    std::string GetCellText(int row, int col) override;
    HICON GetRowIcon(int row) override;

    static std::string BaseNameFromPath(const std::string& path);
    static std::string FolderFromPath(const std::string& path);
    static bool QueryPathMeta(const std::string& path, uint64_t& size, uint64_t& date, bool& isFolder);

private:
    std::string GetName(const SearchResultRef& r);
    std::string GetFolder(const SearchResultRef& r);
    std::string GetFullPath(const SearchResultRef& r);

    std::unordered_map<uint64_t, std::string> m_nameCache;
    std::unordered_map<uint64_t, std::string> m_folderCache;
    std::unordered_map<uint64_t, std::string> m_pathCache;

    static uint64_t CacheKey(const SearchResultRef& r) {
        return (static_cast<uint64_t>(r.index) << 1) | (r.is_folder ? 1ULL : 0ULL);
    }
};

class EverythingApp {
public:
    EverythingApp() = default;
    ~EverythingApp();

    int Run();

private:
    std::shared_ptr<CUI::UIElement> BuildRoot();
    void BuildMenus();
    void QueueSearch(const std::string& query);
    void ApplySearchResults(std::vector<SearchResultRef>&& results, double searchMs, double totalMs, double sortMs, bool isSortMessage, uint64_t generation);
    void ShowFrequentFiles();
    void RecordFileAccess(const SearchResultRef& ref);
    void RecordFileAccessByPath(const std::string& path, bool isFolder);
    void LoadFrequentFiles();
    void SaveFrequentFiles();
    void RefreshStatusBar();
    void OpenSelected();
    void OpenSelectedPath();
    void CopyFullPath();
    void RenameSelected();
    void ToggleTheme();
    void ToggleSearchOption(bool SearchOptions::* flag, const char* menuLabel);
    void RefreshSearchMenuChecks();
    void SortResults(int column, bool ascending);
    void ApplyChromeColors();
    void OnEngineReady();
    void OnEngineStatus(const std::string& status);
    std::vector<int> SelectedRows() const;

    static LRESULT CALLBACK WndSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                            UINT_PTR id, DWORD_PTR refData);

    CUI::Window m_window;
    std::shared_ptr<CUI::UIElement> m_root;
    std::shared_ptr<CUI::WindowTitleBar> m_titleBar;
    std::shared_ptr<CUI::TextBox> m_searchBox;
    std::shared_ptr<CUI::ComboBox> m_typeFilter;
    std::shared_ptr<CUI::ListView> m_resultsList;
    std::shared_ptr<CUI::TextBlock> m_statusLeft;
    std::shared_ptr<CUI::TextBlock> m_statusRight;
    std::shared_ptr<CUI::UIElement> m_statusBar;

    EverythingEngine m_engine;
    SearchOptions m_searchOptions;
    ResultsDataSource m_dataSource;
    FileIconCache m_iconCache;
    ListDisplayMode m_displayMode = ListDisplayMode::FrequentFiles;
    std::vector<SearchResultRef> m_results;
    std::vector<FrequentFileEntry> m_frequentFiles;
    std::string m_lastQuery;
    std::atomic<uint64_t> m_searchGeneration{ 0 };
    int m_sortColumn = 0;
    bool m_sortAscending = true;
    bool m_statusBarVisible = true;

    std::shared_ptr<CUI::MenuItem> m_menuRegex;
    std::shared_ptr<CUI::MenuItem> m_menuMatchPath;
    std::shared_ptr<CUI::MenuItem> m_menuWholeWord;
    std::shared_ptr<CUI::MenuItem> m_menuMatchCase;
    std::shared_ptr<CUI::MenuItem> m_menuStatusBar;
    std::shared_ptr<CUI::ContextMenu> m_fileContextMenu;
};

} // namespace EverythingNEO
