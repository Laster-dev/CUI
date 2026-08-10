#pragma once

#include "EverythingEngine.h"
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
class UIElement;
}

namespace EverythingNEO {

struct ResultRow {
    uint32_t fileIndex = 0;
};

// Fast extension placeholders + async real icons extracted from full path.
class FileIconCache {
public:
    FileIconCache();
    ~FileIconCache();

    void SetNotifyHwnd(HWND hwnd, UINT msg);
    void ResetNotifyFlag() { m_notifyPosted.store(false); }
    HICON GetIcon(const std::string& fullPath, const std::string& fileName);
    void Clear();

private:
    void WorkerLoop();
    HICON GetExtIconUnlocked(const std::string& fileName);
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
    HWND m_notifyHwnd = nullptr;
    UINT m_notifyMsg = 0;
};

class ResultsDataSource : public CUI::ListViewDataSource {
public:
    EverythingEngine* engine = nullptr;
    std::vector<uint32_t>* results = nullptr;
    FileIconCache* icons = nullptr;

    void ClearCaches();
    std::string GetCellText(int row, int col) override;
    HICON GetRowIcon(int row) override;

private:
    std::string GetName(uint32_t fileIndex);
    std::string GetFolder(uint32_t fileIndex);
    std::string GetFullPath(uint32_t fileIndex);

    std::unordered_map<uint32_t, std::string> m_nameCache;
    std::unordered_map<uint32_t, std::string> m_folderCache;
    std::unordered_map<uint32_t, std::string> m_pathCache;
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
    void ApplySearchResults(std::vector<uint32_t>&& indices, double seconds, uint64_t generation);
    void RefreshStatusBar();
    void OpenSelected();
    void OpenSelectedPath();
    void CopyFullPath();
    void ToggleTheme();
    void ApplyChromeColors();
    void OnEngineReady();
    void OnEngineStatus(const std::string& status);
    int SelectedRow() const;

    static LRESULT CALLBACK WndSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                            UINT_PTR id, DWORD_PTR refData);

    CUI::Window m_window;
    std::shared_ptr<CUI::UIElement> m_root;
    std::shared_ptr<CUI::WindowTitleBar> m_titleBar;
    std::shared_ptr<CUI::TextBox> m_searchBox;
    std::shared_ptr<CUI::ListView> m_resultsList;
    std::shared_ptr<CUI::TextBlock> m_statusLeft;
    std::shared_ptr<CUI::TextBlock> m_statusRight;
    std::shared_ptr<CUI::UIElement> m_statusBar;

    EverythingEngine m_engine;
    ResultsDataSource m_dataSource;
    FileIconCache m_iconCache;
    std::vector<uint32_t> m_results;
    std::string m_lastQuery;
    std::atomic<uint64_t> m_searchGeneration{ 0 };
    bool m_statusBarVisible = true;
};

} // namespace EverythingNEO
