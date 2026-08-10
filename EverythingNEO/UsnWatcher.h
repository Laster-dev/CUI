#pragma once

#include "FileIndex.h"
#include <atomic>
#include <thread>
#include <functional>
#include <shared_mutex>

namespace EverythingNEO {

class UsnWatcher {
public:
    using ChangeCallback = std::function<void()>;
    using RebuildCallback = std::function<void(char driveLetter)>;

    UsnWatcher() = default;
    ~UsnWatcher();

    UsnWatcher(const UsnWatcher&) = delete;
    UsnWatcher& operator=(const UsnWatcher&) = delete;

    void Start(FileIndexTable& index, std::shared_mutex& indexMutex,
               ChangeCallback onChanged, RebuildCallback onRebuild = {});
    void Stop();
    bool IsRunning() const { return m_running.load(); }

    // Everything-style startup catch-up: drain USN journal from saved nextUsn.
    static size_t CatchUpVolumes(FileIndexTable& index, std::shared_mutex& indexMutex,
                                 RebuildCallback onRebuild = {});

private:
    void ThreadMain();
    bool ProcessVolume(char driveLetter, VolumeState& vol, bool drainAll);

    FileIndexTable* m_index = nullptr;
    std::shared_mutex* m_mutex = nullptr;
    ChangeCallback m_onChanged;
    RebuildCallback m_onRebuild;
    std::atomic<bool> m_running{ false };
    std::thread m_thread;
};

} // namespace EverythingNEO
