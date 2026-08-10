#pragma once

#include "FileIndex.h"
#include <string>
#include <vector>
#include <functional>
#include <atomic>

namespace EverythingNEO {

struct IndexProgress {
    char driveLetter = 0;
    size_t filesSeen = 0;
    size_t foldersSeen = 0;
    bool usingUsnEnum = false;
    const char* phase = "";
};

using ProgressCallback = std::function<void(const IndexProgress&)>;

// Disk acquisition layer: NTFS USN enumeration (admin) or FindFirstFile fallback.
class VolumeIndexer {
public:
    // Index one NTFS volume into `index`. Prefer FSCTL_ENUM_USN_DATA when elevated.
    static bool IndexVolume(FileIndexTable& index, char driveLetter,
                            ProgressCallback progress = {},
                            std::atomic<bool>* cancel = nullptr);

    static bool IsElevated();
    static std::vector<char> EnumerateFixedDrives();

private:
    static bool IndexViaUsnEnum(FileIndexTable& index, char driveLetter,
                                ProgressCallback& progress,
                                std::atomic<bool>* cancel);
    static bool IndexViaFindFirst(FileIndexTable& index, char driveLetter,
                                  ProgressCallback& progress,
                                  std::atomic<bool>* cancel);
    static void ScanDirRecursive(FileIndexTable& index, uint32_t parentId,
                                 const std::wstring& dirPath,
                                 ProgressCallback& progress,
                                 IndexProgress& prog,
                                 std::atomic<bool>* cancel);
};

} // namespace EverythingNEO
