#pragma once

#include "../model/StartupEntry.h"
#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <future>
#include <map>
#include <unordered_set>

namespace AutoGuard {

struct TreeListRow {
    bool isGroupHeader = false;
    std::string groupTitle;
    std::string groupIcon;
    bool isExpanded = true;
    size_t groupCount = 0;
    StartupEntry entry;
};

class MainViewModel {
public:
    MainViewModel();
    ~MainViewModel() = default;

    void ScanSync();
    void StartScan(std::function<void()> onCompleted = nullptr);

    void ApplyCategory(StartupCategory category);
    StartupCategory GetCategory() const { return m_activeCategory; }

    void ApplyFilterText(const std::string& filter);
    const std::string& GetFilterText() const { return m_filterText; }

    void ApplySelectedId(const std::string& id);
    const std::string& GetSelectedId() const { return m_selectedId; }
    const StartupEntry* GetSelectedEntry() const;

    // Tree-ListView rows
    std::vector<TreeListRow> GetTreeListRows() const;
    void ToggleGroupExpanded(const std::string& groupTitle);
    bool IsGroupExpanded(const std::string& groupTitle) const;

    const ScanSummary& GetSummary() const { return m_summary; }
    bool IsScanning() const { return m_isScanning; }

    // Actions
    bool ToggleSelectedStatus(std::string& outMsg);
    bool ToggleStatusById(const std::string& id, std::string& outMsg);
    bool DeleteSelected(std::string& outMsg);
    bool DeleteById(const std::string& id, std::string& outMsg);
    bool JumpToImage();
    bool JumpToEntry();
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
    std::unordered_set<std::string> m_collapsedGroups;
    bool m_isScanning = false;
};

} // namespace AutoGuard
