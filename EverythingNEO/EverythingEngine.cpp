#include "EverythingEngine.h"
#include "VolumeIndexer.h"
#include "DbSnapshot.h"
#include "FastMatch.h"
#include <windows.h>
#include <shlwapi.h>
#include <chrono>
#include <algorithm>
#include <numeric>

#pragma comment(lib, "shlwapi.lib")

namespace EverythingNEO {

namespace {

std::wstring Utf8SvToWide(std::string_view utf8) {
    if (utf8.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    if (n <= 0) return {};
    std::wstring w(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), w.data(), n);
    return w;
}

} // namespace

EverythingEngine::EverythingEngine() = default;

EverythingEngine::~EverythingEngine() {
    Stop();
}

void EverythingEngine::ClearPathCache() {
    std::lock_guard<std::mutex> pathLock(m_pathCacheMutex);
    m_folderPathCache.clear();
}

void EverythingEngine::Stop() {
    m_cancel.store(true);
    m_usnWatcher.Stop();
    for (int i = 0; i < 50 && m_isIndexing.load(); ++i) {
        Sleep(100);
    }
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        DbSnapshot::Save(m_index, DbSnapshot::DefaultPath());
    }
}

void EverythingEngine::StartAsync(StatusCallback onStatus, ReadyCallback onReady) {
    if (m_isIndexing.exchange(true)) return;
    m_cancel.store(false);
    m_ready.store(false);
    m_journalResetRequested.store(false);

    std::thread([this, onStatus, onReady]() {
        auto start = std::chrono::high_resolution_clock::now();

        bool loaded = false;
        std::wstring loadedFrom;
        {
            if (onStatus) onStatus("正在加载 Everything.db ...");
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            ClearPathCache();
            loaded = DbSnapshot::LoadPreferred(m_index, &loadedFrom);
            m_loadedFromDb = loaded;
        }

        if (loaded) {
            const size_t files = [&]() {
                std::shared_lock<std::shared_mutex> lock(m_mutex);
                return m_index.GetFiles().size();
            }();
            if (onStatus) {
                onStatus("已从数据库载入 " + std::to_string(files)
                         + " 个文件，正在 USN 增量补齐 ...");
            }
            CatchUpUsn();
            if (m_journalResetRequested.exchange(false)) {
                if (onStatus) onStatus("USN 日志已重置，正在重建索引 ...");
                IndexAllDrives(onStatus);
                {
                    std::unique_lock<std::shared_mutex> lock(m_mutex);
                    m_index.ShrinkToFit();
                }
            }
            // Persist advanced nextUsn so the next launch stays incremental.
            {
                std::unique_lock<std::shared_mutex> lock(m_mutex);
                DbSnapshot::Save(m_index, DbSnapshot::DefaultPath());
            }
        } else {
            if (onStatus) onStatus("未找到数据库，开始全盘索引 ...");
            IndexAllDrives(onStatus);
            {
                std::unique_lock<std::shared_mutex> lock(m_mutex);
                m_index.ShrinkToFit();
                DbSnapshot::Save(m_index, DbSnapshot::DefaultPath());
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        m_lastScanDurationMs = std::chrono::duration<double, std::milli>(end - start).count();

        m_usnWatcher.Start(m_index, m_mutex, [this]() {
            {
                std::unique_lock<std::shared_mutex> lock(m_mutex);
                ClearPathCache();
            }
            if (m_changeNotify) m_changeNotify();
        }, [this](char driveLetter) { OnVolumeJournalReset(driveLetter); });

        m_ready.store(true);
        m_isIndexing.store(false);
        if (onStatus) onStatus(loaded ? "索引就绪（增量）" : "索引就绪");
        if (onReady) onReady();
        if (m_changeNotify) m_changeNotify();
    }).detach();
}

void EverythingEngine::RebuildIndexAsync(StatusCallback onStatus, ReadyCallback onReady) {
    if (m_isIndexing.exchange(true)) return;
    m_cancel.store(false);
    m_ready.store(false);
    m_usnWatcher.Stop();

    std::thread([this, onStatus, onReady]() {
        auto start = std::chrono::high_resolution_clock::now();
        IndexAllDrives(onStatus);
        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            m_index.ShrinkToFit();
            m_loadedFromDb = false;
            DbSnapshot::Save(m_index, DbSnapshot::DefaultPath());
        }
        auto end = std::chrono::high_resolution_clock::now();
        m_lastScanDurationMs = std::chrono::duration<double, std::milli>(end - start).count();

        m_usnWatcher.Start(m_index, m_mutex, [this]() {
            {
                std::unique_lock<std::shared_mutex> lock(m_mutex);
                ClearPathCache();
            }
            if (m_changeNotify) m_changeNotify();
        }, [this](char driveLetter) { OnVolumeJournalReset(driveLetter); });

        m_ready.store(true);
        m_isIndexing.store(false);
        if (onStatus) onStatus("重建完成");
        if (onReady) onReady();
        if (m_changeNotify) m_changeNotify();
    }).detach();
}

void EverythingEngine::IndexAllDrives(StatusCallback onStatus) {
    auto drives = VolumeIndexer::EnumerateFixedDrives();
    if (drives.empty()) drives.push_back('C');

    FileIndexTable built;
    for (char d : drives) {
        if (m_cancel.load()) break;
        if (onStatus) onStatus(std::string("正在索引 ") + d + ": ...");
        VolumeIndexer::IndexVolume(built, d,
            [&](const IndexProgress& p) {
                if (onStatus) {
                    onStatus(std::string("索引 ") + p.driveLetter + ":  文件 "
                             + std::to_string(p.filesSeen) + " / 文件夹 "
                             + std::to_string(p.foldersSeen)
                             + (p.usingUsnEnum ? " [USN/MFT]" : " [FindFirstFile]"));
                }
            },
            &m_cancel);
    }

    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_index.Swap(built);
        ClearPathCache();
    }
}

void EverythingEngine::CatchUpUsn() {
    UsnWatcher::CatchUpVolumes(m_index, m_mutex, [this](char driveLetter) {
        OnVolumeJournalReset(driveLetter);
    });
    ClearPathCache();
}

void EverythingEngine::OnVolumeJournalReset(char driveLetter) {
    (void)driveLetter;
    // During StartAsync catch-up, queue a rebuild after CatchUp returns instead
    // of ignoring the reset (old code returned early while m_isIndexing).
    if (m_isIndexing.load()) {
        m_journalResetRequested.store(true);
        return;
    }
    RebuildIndexAsync({}, [this]() {
        if (m_changeNotify) m_changeNotify();
    });
}

bool EverythingEngine::SaveDatabase(const std::wstring& path) {
    std::wstring p = path.empty() ? DbSnapshot::DefaultPath() : path;
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    return DbSnapshot::Save(m_index, p);
}

bool EverythingEngine::LoadDatabase(const std::wstring& path) {
    std::wstring p = path.empty() ? DbSnapshot::DefaultPath() : path;
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    ClearPathCache();
    return DbSnapshot::Load(m_index, p);
}

size_t EverythingEngine::Search(const std::string& query, const SearchOptions& opts,
                                std::vector<SearchResultRef>& outResults, size_t maxResults,
                                const std::atomic<uint64_t>* cancelGen, uint64_t myGen) {
    outResults.clear();
    if (query.empty()) return 0;

    QueryMatcher matcher;
    matcher.use_regex = opts.use_regex;
    matcher.match_whole_word = opts.match_whole_word;
    matcher.match_case = opts.match_case;
    matcher.Prepare(query);

    std::shared_lock<std::shared_mutex> lock(m_mutex);
    const auto& files = m_index.GetFiles();
    const auto& folders = m_index.GetFolders();

    const bool wantFiles = opts.result_kind != SearchResultKind::FoldersOnly;
    const bool wantFolders = opts.result_kind != SearchResultKind::FilesOnly;

    // Early-cancel helper: check every 4096 items if a newer search superseded us
    auto cancelled = [&]() -> bool {
        return cancelGen && cancelGen->load(std::memory_order_relaxed) != myGen;
    };

    outResults.reserve(4096);

    // Folders first in index/traversal order (no sort needed)
    if (wantFolders) {
        const uint32_t folderCount = static_cast<uint32_t>(folders.size());
        for (uint32_t i = 1; i < folderCount; ++i) {
            if ((i & 0xFFF) == 0 && cancelled()) return 0;
            if ((folders[i].attributes & kAttrDeleted) != 0) continue;
            std::string_view name = m_index.GetFolderName(i);
            if (name.empty()) continue;

            bool hit;
            if (opts.match_path) {
                std::string path = m_index.GetFolderPath(folders[i].parent_folder_id);
                if (!path.empty()) path += '\\';
                path.append(name.data(), name.length());
                hit = matcher.Matches(path);
            } else {
                hit = matcher.Matches(name);
            }
            if (hit) {
                outResults.push_back({ i, true });
                if (maxResults > 0 && outResults.size() >= maxResults) return outResults.size();
            }
        }
    }

    if (wantFiles) {
        const uint32_t fileCount = static_cast<uint32_t>(files.size());
        for (uint32_t i = 0; i < fileCount; ++i) {
            if ((i & 0xFFF) == 0 && cancelled()) return 0;
            if ((files[i].attributes & kAttrDeleted) != 0) continue;

            std::string_view name = m_index.GetFileName(i);
            bool hit;
            if (opts.match_path) {
                std::string path = m_index.GetFolderPath(files[i].parent_folder_id);
                if (!path.empty()) path += '\\';
                path.append(name.data(), name.length());
                hit = matcher.Matches(path);
            } else {
                hit = matcher.Matches(name);
            }
            if (hit) {
                outResults.push_back({ i, false });
                if (maxResults > 0 && outResults.size() >= maxResults) return outResults.size();
            }
        }
    }
    return outResults.size();
}

void EverythingEngine::SortSearchResults(std::vector<SearchResultRef>& results, int column, bool ascending) const {
    if (results.size() <= 1) return;

    std::shared_lock<std::shared_mutex> lock(m_mutex);

    const size_t n = results.size();

    auto foldersFirst = [&](size_t i, size_t j) -> bool {
        return results[i].is_folder && !results[j].is_folder;
    };

    // Use indirect sort via index array, then permute once
    std::vector<uint32_t> order(n);
    std::iota(order.begin(), order.end(), 0u);

    if (column <= 0) {
        // Pre-extract all names to wstring once (avoid repeated UTF8→UTF16 in comparator)
        std::vector<std::wstring> names(n);
        for (size_t i = 0; i < n; ++i) {
            const auto& r = results[i];
            std::string_view sv = r.is_folder ? m_index.GetFolderName(r.index) : m_index.GetFileName(r.index);
            names[i] = Utf8SvToWide(sv);
        }
        std::sort(order.begin(), order.end(), [&](uint32_t i, uint32_t j) {
            if (results[i].is_folder != results[j].is_folder) return foldersFirst(i, j);
            const int c = StrCmpLogicalW(names[i].c_str(), names[j].c_str());
            return ascending ? c < 0 : c > 0;
        });
    } else if (column == 1) {
        std::vector<std::wstring> paths(n);
        for (size_t i = 0; i < n; ++i) {
            const auto& r = results[i];
            if (r.is_folder) {
                paths[i] = Utf8SvToWide(m_index.GetFolderPath(r.index));
            } else {
                paths[i] = Utf8SvToWide(m_index.GetFolderPath(m_index.GetFiles()[r.index].parent_folder_id));
            }
        }
        std::sort(order.begin(), order.end(), [&](uint32_t i, uint32_t j) {
            if (results[i].is_folder != results[j].is_folder) return foldersFirst(i, j);
            const int c = StrCmpLogicalW(paths[i].c_str(), paths[j].c_str());
            return ascending ? c < 0 : c > 0;
        });
    } else if (column == 2) {
        std::sort(order.begin(), order.end(), [&](uint32_t i, uint32_t j) {
            if (results[i].is_folder != results[j].is_folder) return foldersFirst(i, j);
            const uint64_t sa = results[i].is_folder ? 0ULL : (
                results[i].index < m_index.GetFiles().size() ? m_index.GetFiles()[results[i].index].file_size : 0ULL);
            const uint64_t sb = results[j].is_folder ? 0ULL : (
                results[j].index < m_index.GetFiles().size() ? m_index.GetFiles()[results[j].index].file_size : 0ULL);
            return ascending ? sa < sb : sa > sb;
        });
    } else {
        std::sort(order.begin(), order.end(), [&](uint32_t i, uint32_t j) {
            if (results[i].is_folder != results[j].is_folder) return foldersFirst(i, j);
            const uint64_t da = results[i].is_folder ? 0ULL : (
                results[i].index < m_index.GetFiles().size() ? m_index.GetFiles()[results[i].index].date_modified : 0ULL);
            const uint64_t db = results[j].is_folder ? 0ULL : (
                results[j].index < m_index.GetFiles().size() ? m_index.GetFiles()[results[j].index].date_modified : 0ULL);
            return ascending ? da < db : da > db;
        });
    }

    std::vector<SearchResultRef> sorted;
    sorted.reserve(n);
    for (uint32_t i : order) sorted.push_back(results[i]);
    results = std::move(sorted);
}

std::string EverythingEngine::GetResultName(const SearchResultRef& r) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    if (r.is_folder) {
        auto sv = m_index.GetFolderName(r.index);
        return std::string(sv);
    }
    auto sv = m_index.GetFileName(r.index);
    return std::string(sv);
}

std::string EverythingEngine::GetResultPath(const SearchResultRef& r) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    if (r.is_folder) return m_index.GetFolderPath(r.index);
    return m_index.GetFilePath(r.index);
}

std::string EverythingEngine::GetResultFolderPath(const SearchResultRef& r) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    if (r.is_folder) {
        if (r.index >= m_index.GetFolders().size()) return {};
        return m_index.GetFolderPath(m_index.GetFolders()[r.index].parent_folder_id);
    }
    if (r.index >= m_index.GetFiles().size()) return {};
    return m_index.GetFolderPath(m_index.GetFiles()[r.index].parent_folder_id);
}

uint64_t EverythingEngine::GetResultSize(const SearchResultRef& r) const {
    if (r.is_folder) return 0;
    return GetFileSize(r.index);
}

uint64_t EverythingEngine::GetResultDateModified(const SearchResultRef& r) const {
    if (r.is_folder) return 0;
    return GetFileDateModified(r.index);
}

uint16_t EverythingEngine::GetResultAttrs(const SearchResultRef& r) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    if (r.is_folder) {
        if (r.index >= m_index.GetFolders().size()) return FILE_ATTRIBUTE_DIRECTORY;
        return static_cast<uint16_t>(m_index.GetFolders()[r.index].attributes & ~(kAttrDeleted | kAttrMetaLoaded));
    }
    if (r.index >= m_index.GetFiles().size()) return 0;
    return static_cast<uint16_t>(m_index.GetFiles()[r.index].attributes & ~(kAttrDeleted | kAttrMetaLoaded));
}

std::string EverythingEngine::GetFileName(uint32_t fileIndex) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto sv = m_index.GetFileName(fileIndex);
    return std::string(sv);
}

std::string EverythingEngine::GetFilePath(uint32_t fileIndex) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_index.GetFilePath(fileIndex);
}

std::string EverythingEngine::GetFileFolderPath(uint32_t fileIndex) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    if (fileIndex >= m_index.GetFiles().size()) return {};
    uint32_t parent = m_index.GetFiles()[fileIndex].parent_folder_id;

    {
        std::lock_guard<std::mutex> pathLock(m_pathCacheMutex);
        auto it = m_folderPathCache.find(parent);
        if (it != m_folderPathCache.end()) return it->second;
    }

    std::string path = m_index.GetFolderPath(parent);
    std::lock_guard<std::mutex> pathLock(m_pathCacheMutex);
    auto [it, inserted] = m_folderPathCache.emplace(parent, path);
    return inserted ? path : it->second;
}

uint64_t EverythingEngine::GetFileSize(uint32_t fileIndex) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    if (fileIndex >= m_index.GetFiles().size()) return 0;
    return m_index.GetFiles()[fileIndex].file_size;
}

uint64_t EverythingEngine::GetFileDateModified(uint32_t fileIndex) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    if (fileIndex >= m_index.GetFiles().size()) return 0;
    return m_index.GetFiles()[fileIndex].date_modified;
}

uint16_t EverythingEngine::GetFileAttrs(uint32_t fileIndex) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    if (fileIndex >= m_index.GetFiles().size()) return 0;
    return static_cast<uint16_t>(m_index.GetFiles()[fileIndex].attributes & ~(kAttrDeleted | kAttrMetaLoaded));
}

void EverythingEngine::EnsureFileMeta(uint32_t fileIndex) {
    std::string path;
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        if (fileIndex >= m_index.GetFiles().size()) return;
        const auto& node = m_index.GetFiles()[fileIndex];
        if ((node.attributes & kAttrDeleted) != 0) return;
        // Only skip when we already queried the filesystem (empty files stay size 0).
        if ((node.attributes & kAttrMetaLoaded) != 0) return;
        path = m_index.GetFilePath(fileIndex);
    }
    if (path.empty()) return;

    int wlen = MultiByteToWideChar(CP_UTF8, 0, path.data(), static_cast<int>(path.size()), nullptr, 0);
    if (wlen <= 0) return;
    std::wstring wpath(static_cast<size_t>(wlen), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, path.data(), static_cast<int>(path.size()), wpath.data(), wlen);

    WIN32_FILE_ATTRIBUTE_DATA fad{};
    if (!GetFileAttributesExW(wpath.c_str(), GetFileExInfoStandard, &fad)) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_index.UpdateFileMeta(fileIndex, 0, 0);
        return;
    }

    ULARGE_INTEGER size{};
    size.LowPart = fad.nFileSizeLow;
    size.HighPart = fad.nFileSizeHigh;
    ULARGE_INTEGER date{};
    date.LowPart = fad.ftLastWriteTime.dwLowDateTime;
    date.HighPart = fad.ftLastWriteTime.dwHighDateTime;

    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_index.UpdateFileMeta(fileIndex, size.QuadPart, date.QuadPart);
}

EngineStats EverythingEngine::GetStats() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    EngineStats s{};
    s.file_count = m_index.GetFileCount();
    s.folder_count = m_index.GetFolderCount();
    s.live_file_count = m_index.GetLiveFileCount();
    s.file_nodes_bytes = m_index.GetFileNodesMemory();
    s.folder_nodes_bytes = m_index.GetFolderNodesMemory();
    s.string_arena_bytes = m_index.GetStringArenaMemory();
    s.total_memory_bytes = m_index.GetTotalMemoryUsage();
    s.scan_duration_ms = m_lastScanDurationMs;
    s.loaded_from_db = m_loadedFromDb;
    s.usn_active = m_usnWatcher.IsRunning();
    s.elevated = VolumeIndexer::IsElevated();
    return s;
}

} // namespace EverythingNEO
