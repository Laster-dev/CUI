#include "MainViewModel.h"
#include "../scanner/StartupScanner.h"
#include "../manager/StartupManager.h"
#include <algorithm>
#include <cwctype>

namespace AutoGuard {

namespace {

std::string ToLower(std::string s) {
    for (char& c : s) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return s;
}

} // namespace

MainViewModel::MainViewModel() {
}

void MainViewModel::StartScan(std::function<void()> onCompleted) {
    m_isScanning = true;
    m_scanCallback = onCompleted;
    m_scanFuture = std::make_shared<std::future<ScanSummary>>(
        std::async(std::launch::async, []() {
            return StartupScanner().Scan();
        })
    );
}

bool MainViewModel::CheckScanFinished() {
    if (m_scanFuture && m_scanFuture->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        m_summary = m_scanFuture->get();
        m_scanFuture = nullptr;
        m_isScanning = false;
        if (m_scanCallback) {
            auto cb = m_scanCallback;
            m_scanCallback = nullptr;
            cb();
        }
        return true;
    }
    return false;
}

void MainViewModel::SetCategory(StartupCategory category) {
    if (m_activeCategory == category) return;
    m_activeCategory = category;
    m_selectedId.clear();
}

void MainViewModel::SetFilterText(const std::string& filter) {
    m_filterText = filter;
    m_selectedId.clear();
}

void MainViewModel::SetSelectedId(const std::string& id) {
    m_selectedId = id;
}

const StartupEntry* MainViewModel::GetSelectedEntry() const {
    if (m_selectedId.empty()) return nullptr;
    for (const auto& e : m_summary.entries) {
        if (e.id == m_selectedId) {
            return &e;
        }
    }
    return nullptr;
}

std::vector<TreeGroupNode> MainViewModel::GetTreeGroups() const {
    std::vector<TreeGroupNode> groups;
    const std::string q = ToLower(m_filterText);

    std::map<std::string, size_t> groupIndexMap;

    for (const auto& entry : m_summary.entries) {
        // 1. Filter category
        if (m_activeCategory != StartupCategory::All && entry.category != m_activeCategory) {
            continue;
        }

        // 2. Filter search query
        if (!q.empty()) {
            if (ToLower(entry.name).find(q) == std::string::npos &&
                ToLower(entry.description).find(q) == std::string::npos &&
                ToLower(entry.publisher).find(q) == std::string::npos &&
                ToLower(entry.command).find(q) == std::string::npos &&
                ToLower(entry.executablePath).find(q) == std::string::npos &&
                ToLower(entry.source).find(q) == std::string::npos) {
                continue;
            }
        }

        // 3. Locate or create group node
        std::string grpName = entry.groupTitle.empty() ? LocationName(entry.location) : entry.groupTitle;
        auto it = groupIndexMap.find(grpName);
        if (it == groupIndexMap.end()) {
            TreeGroupNode groupNode;
            groupNode.title = grpName;
            groupNode.icon = LocationIcon(entry.location);
            groupNode.location = entry.location;
            groupNode.entries.push_back(entry);
            size_t idx = groups.size();
            groups.push_back(std::move(groupNode));
            groupIndexMap[grpName] = idx;
        } else {
            groups[it->second].entries.push_back(entry);
        }
    }

    return groups;
}

bool MainViewModel::ToggleSelectedStatus(std::string& outMsg) {
    return ToggleStatusById(m_selectedId, outMsg);
}

bool MainViewModel::ToggleStatusById(const std::string& id, std::string& outMsg) {
    for (auto& entry : m_summary.entries) {
        if (entry.id == id) {
            bool enable = (entry.status == StartupStatus::Disabled);
            if (StartupManager::ToggleStatus(entry, enable, outMsg)) {
                return true;
            }
            return false;
        }
    }
    outMsg = "未选择有效启动项。";
    return false;
}

bool MainViewModel::DeleteSelected(std::string& outMsg) {
    return DeleteById(m_selectedId, outMsg);
}

bool MainViewModel::DeleteById(const std::string& id, std::string& outMsg) {
    for (auto it = m_summary.entries.begin(); it != m_summary.entries.end(); ++it) {
        if (it->id == id) {
            if (StartupManager::DeleteEntry(*it, outMsg)) {
                m_summary.entries.erase(it);
                m_selectedId.clear();
                return true;
            }
            return false;
        }
    }
    outMsg = "未选择有效启动项。";
    return false;
}

bool MainViewModel::JumpToImage() {
    const auto* entry = GetSelectedEntry();
    if (entry) {
        return StartupManager::JumpToImage(*entry);
    }
    return false;
}

bool MainViewModel::JumpToEntry() {
    const auto* entry = GetSelectedEntry();
    if (entry) {
        return StartupManager::JumpToEntry(*entry);
    }
    return false;
}

bool MainViewModel::SearchOnline() {
    const auto* entry = GetSelectedEntry();
    if (entry) {
        return StartupManager::SearchOnline(*entry);
    }
    return false;
}

bool MainViewModel::CopyCommand(HWND hwnd) {
    const auto* entry = GetSelectedEntry();
    if (entry) {
        return StartupManager::CopyToClipboard(hwnd, entry->command.empty() ? entry->executablePath : entry->command);
    }
    return false;
}

bool MainViewModel::CopyPath(HWND hwnd) {
    const auto* entry = GetSelectedEntry();
    if (entry) {
        return StartupManager::CopyToClipboard(hwnd, entry->executablePath.empty() ? entry->command : entry->executablePath);
    }
    return false;
}

bool MainViewModel::CleanAllMissing(size_t& outCleanedCount) {
    outCleanedCount = 0;
    for (auto it = m_summary.entries.begin(); it != m_summary.entries.end();) {
        if (it->status == StartupStatus::Missing && it->canDelete) {
            std::string msg;
            if (StartupManager::DeleteEntry(*it, msg)) {
                it = m_summary.entries.erase(it);
                ++outCleanedCount;
                continue;
            }
        }
        ++it;
    }
    if (outCleanedCount > 0) {
        m_selectedId.clear();
        return true;
    }
    return false;
}

bool MainViewModel::Export(const std::wstring& path, std::string& outMsg) {
    return StartupManager::ExportReport(m_summary, path, outMsg);
}

std::string MainViewModel::GetStatusLeftText() const {
    if (m_isScanning) return "正在深度扫描系统全启动入口…";
    return "就绪";
}

std::string MainViewModel::GetStatusCountText() const {
    std::string s = "启动项总数: " + std::to_string(m_summary.entries.size());
    s += "   已启用: " + std::to_string(m_summary.enabledCount);
    s += "   已禁用: " + std::to_string(m_summary.disabledCount);
    if (m_summary.missingCount > 0) {
        s += "   失效: " + std::to_string(m_summary.missingCount);
    }
    if (m_summary.suspiciousCount + m_summary.highRiskCount > 0) {
        s += "   潜在风险: " + std::to_string(m_summary.suspiciousCount + m_summary.highRiskCount);
    }
    return s;
}

std::string MainViewModel::GetStatusSelectionText() const {
    const auto* entry = GetSelectedEntry();
    if (entry) {
        return "已选择: " + entry->name + " (" + StatusName(entry->status) + ")";
    }
    return "未选择项目";
}

} // namespace AutoGuard
