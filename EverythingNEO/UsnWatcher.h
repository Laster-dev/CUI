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

    UsnWatcher() = default;
    ~UsnWatcher();

    UsnWatcher(const UsnWatcher&) = delete;
    UsnWatcher& operator=(const UsnWatcher&) = delete;

    void Start(FileIndexTable& index, std::shared_mutex& indexMutex, ChangeCallback onChanged = {});
    void Stop();
    bool IsRunning() const { return m_running.load(); }

private:
    void ThreadMain();
    void WatchVolume(char driveLetter, VolumeState& vol);

    FileIndexTable* m_index = nullptr;
    std::shared_mutex* m_mutex = nullptr;
    ChangeCallback m_onChanged;
    std::atomic<bool> m_running{ false };
    std::thread m_thread;
};

} // namespace EverythingNEO
