#pragma once

#include "FileIndex.h"
#include "UsnWatcher.h"
#include <string>
#include <vector>
#include <atomic>
#include <shared_mutex>
#include <functional>
#include <cstdint>
#include <unordered_map>
#include <mutex>

namespace EverythingNEO {

struct EngineStats {
    size_t file_count = 0;
    size_t folder_count = 0;
    size_t live_file_count = 0;
    size_t file_nodes_bytes = 0;
    size_t folder_nodes_bytes = 0;
    size_t string_arena_bytes = 0;
    size_t total_memory_bytes = 0;
    double scan_duration_ms = 0.0;
    bool loaded_from_db = false;
    bool usn_active = false;
    bool elevated = false;
};

class EverythingEngine {
public:
    using StatusCallback = std::function<void(const std::string& status)>;
    using ReadyCallback = std::function<void()>;

    EverythingEngine();
    ~EverythingEngine();

    void StartAsync(StatusCallback onStatus = {}, ReadyCallback onReady = {});
    void Stop();
    void RebuildIndexAsync(StatusCallback onStatus = {}, ReadyCallback onReady = {});

    bool SaveDatabase(const std::wstring& path = {});
    bool LoadDatabase(const std::wstring& path = {});

    // Everything-style: match filename only, return compact file indices (no path alloc).
    size_t Search(const std::string& query, std::vector<uint32_t>& outIndices,
                  size_t maxResults = 0);

    std::string GetFileName(uint32_t fileIndex) const;
    std::string GetFilePath(uint32_t fileIndex) const;
    std::string GetFileFolderPath(uint32_t fileIndex) const;
    uint64_t GetFileSize(uint32_t fileIndex) const;
    uint64_t GetFileDateModified(uint32_t fileIndex) const;
    uint16_t GetFileAttrs(uint32_t fileIndex) const;

    void EnsureFileMeta(uint32_t fileIndex);

    EngineStats GetStats() const;
    bool IsIndexing() const { return m_isIndexing.load(); }
    bool IsReady() const { return m_ready.load(); }

    void SetChangeNotify(std::function<void()> cb) { m_changeNotify = std::move(cb); }

private:
    void IndexAllDrives(StatusCallback onStatus);
    void CatchUpUsn();
    void ClearPathCache();

    mutable std::shared_mutex m_mutex;
    mutable std::mutex m_pathCacheMutex;
    mutable std::unordered_map<uint32_t, std::string> m_folderPathCache;
    FileIndexTable m_index;
    UsnWatcher m_usnWatcher;

    std::atomic<bool> m_isIndexing{ false };
    std::atomic<bool> m_ready{ false };
    std::atomic<bool> m_cancel{ false };
    double m_lastScanDurationMs = 0.0;
    bool m_loadedFromDb = false;
    std::function<void()> m_changeNotify;
};

} // namespace EverythingNEO
