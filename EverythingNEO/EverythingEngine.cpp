#include "EverythingEngine.h"
#include "VolumeIndexer.h"
#include "DbSnapshot.h"
#include "FastMatch.h"
#include <windows.h>
#include <algorithm>
#include <thread>
#include <chrono>
#include <future>

namespace EverythingNEO {

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

    std::thread([this, onStatus, onReady]() {
        auto start = std::chrono::high_resolution_clock::now();

        bool loaded = false;
        {
            if (onStatus) onStatus("正在加载 Everything.db ...");
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            ClearPathCache();
            loaded = DbSnapshot::Load(m_index, DbSnapshot::DefaultPath());
            m_loadedFromDb = loaded;
        }

        if (loaded) {
            if (onStatus) onStatus("数据库已载入，正在通过 USN 增量补齐 ...");
            CatchUpUsn();
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
        });

        m_ready.store(true);
        m_isIndexing.store(false);
        if (onStatus) onStatus("索引就绪");
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
        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            m_index.Clear();
            ClearPathCache();
            m_loadedFromDb = false;
        }
        IndexAllDrives(onStatus);
        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            m_index.ShrinkToFit();
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
        });

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

    for (char d : drives) {
        if (m_cancel.load()) break;
        if (onStatus) onStatus(std::string("正在索引 ") + d + ": ...");
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        VolumeIndexer::IndexVolume(m_index, d,
            [&](const IndexProgress& p) {
                if (onStatus) {
                    onStatus(std::string("索引 ") + p.driveLetter + ":  文件 "
                             + std::to_string(p.filesSeen) + " / 文件夹 "
                             + std::to_string(p.foldersSeen)
                             + (p.usingUsnEnum ? " [USN/MFT]" : " [FindFirstFile]"));
                }
            },
            &m_cancel);
        ClearPathCache();
    }
}

void EverythingEngine::CatchUpUsn() {}

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

size_t EverythingEngine::Search(const std::string& query, std::vector<uint32_t>& outIndices,
                                size_t maxResults) {
    outIndices.clear();
    if (query.empty()) return 0;

    std::string lowerQuery = query;
    ToLowerAsciiInPlace(lowerQuery);

    std::shared_lock<std::shared_mutex> lock(m_mutex);
    const auto& files = m_index.GetFiles();
    const size_t total = files.size();
    if (total == 0) return 0;

    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;
    if (total < 100000) numThreads = 1;
    if (numThreads > 8) numThreads = 8;

    if (numThreads == 1) {
        outIndices.reserve((std::min)(total, size_t{4096}));
        for (uint32_t i = 0; i < static_cast<uint32_t>(total); ++i) {
            if ((files[i].attributes & kAttrDeleted) != 0) continue;
            if (MatchLowerQuery(m_index.GetFileName(i), lowerQuery)) {
                outIndices.push_back(i);
                if (maxResults > 0 && outIndices.size() >= maxResults) break;
            }
        }
        return outIndices.size();
    }

    // Parallel filename scan — Everything-style: no path reconstruction.
    std::vector<std::vector<uint32_t>> locals(numThreads);
    std::vector<std::future<void>> futures;
    const size_t chunk = (total + numThreads - 1) / numThreads;

    for (unsigned t = 0; t < numThreads; ++t) {
        const size_t begin = t * chunk;
        const size_t end = (std::min)(begin + chunk, total);
        if (begin >= end) continue;
        futures.push_back(std::async(std::launch::async,
            [this, &files, &lowerQuery, &locals, t, begin, end, maxResults]() {
                auto& out = locals[t];
                out.reserve(1024);
                for (size_t i = begin; i < end; ++i) {
                    if ((files[i].attributes & kAttrDeleted) != 0) continue;
                    if (MatchLowerQuery(m_index.GetFileName(i), lowerQuery)) {
                        out.push_back(static_cast<uint32_t>(i));
                        if (maxResults > 0 && out.size() >= maxResults) break;
                    }
                }
            }));
    }
    for (auto& f : futures) f.get();

    size_t totalHits = 0;
    for (auto& v : locals) totalHits += v.size();
    outIndices.reserve((std::min)(totalHits, maxResults > 0 ? maxResults : totalHits));
    for (auto& v : locals) {
        for (uint32_t id : v) {
            outIndices.push_back(id);
            if (maxResults > 0 && outIndices.size() >= maxResults) {
                return outIndices.size();
            }
        }
    }
    return outIndices.size();
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
    return static_cast<uint16_t>(m_index.GetFiles()[fileIndex].attributes & ~kAttrDeleted);
}

void EverythingEngine::EnsureFileMeta(uint32_t fileIndex) {
    std::string path;
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        if (fileIndex >= m_index.GetFiles().size()) return;
        const auto& node = m_index.GetFiles()[fileIndex];
        if ((node.attributes & kAttrDeleted) != 0) return;
        if (node.file_size != 0 || node.date_modified != 0) return;
        path = m_index.GetFilePath(fileIndex);
    }
    if (path.empty()) return;

    int wlen = MultiByteToWideChar(CP_UTF8, 0, path.data(), static_cast<int>(path.size()), nullptr, 0);
    if (wlen <= 0) return;
    std::wstring wpath(static_cast<size_t>(wlen), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, path.data(), static_cast<int>(path.size()), wpath.data(), wlen);

    WIN32_FILE_ATTRIBUTE_DATA fad{};
    if (!GetFileAttributesExW(wpath.c_str(), GetFileExInfoStandard, &fad)) return;

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
