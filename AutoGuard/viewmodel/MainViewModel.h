#pragma once

#include "../model/StartupEntry.h"
#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <future>
#include <map>

namespace AutoGuard {

struct TreeGroupNode {
    std::string title;
    std::string icon;
    StartupLocation location = StartupLocation::Unknown;
    std::vector<StartupEntry> entries;
};

class MainViewModel {
public:
    MainViewModel();
    ~MainViewModel() = default;

    void StartScan(std::function<void()> onCompleted = nullptr);
    bool CheckScanFinished();

    void SetCategory(StartupCategory category);
    StartupCategory GetCategory() const { return m_activeCategory; }

    void SetFilterText(const std::string& filter);
    const std::string& GetFilterText() const { return m_filterText; }

    void SetSelectedId(const std::string& id);
    const std::string& GetSelectedId() const { return m_selectedId; }
    const StartupEntry* GetSelectedEntry() const;

    std::vector<TreeGroupNode> GetTreeGroups() const;
    const ScanSummary& GetSummary() const { return m_summary; }
    bool IsScanning() const { return m_isScanning; }

    // Actions
    bool ToggleSelectedStatus(std::string& outMsg);
    bool ToggleStatusById(const std::string& id, std::string& outMsg);
    bool DeleteSelected(std::string& outMsg);
    bool DeleteById(const std::string& id, std::string& outMsg);
    bool JumpToImage();
    bool JumpToEntry();
    bool SearchOnline();
    bool CopyCommand(HWND hwnd);
    bool CopyPath(HWND hwnd);
    bool CleanAllMissing(size_t& outCleanedCount);
    bool Export(const std::wstring& path, std::string& outMsg);

    // Status bar metrics
    std::string GetStatusLeftText() const;
    std::string GetStatusCountText() const;
    std::string GetStatusSelectionText() const;

private:
    ScanSummary m_summary;
    StartupCategory m_activeCategory = StartupCategory::All;
    std::string m_filterText;
    std::string m_selectedId;
    bool m_isScanning = false;
    std::shared_ptr<std::future<ScanSummary>> m_scanFuture;
    std::function<void()> m_scanCallback;
};

} // namespace AutoGuard
