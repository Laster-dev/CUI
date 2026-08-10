#pragma once

#include "FileIndex.h"
#include <string>

namespace EverythingNEO {

// Everything.db — custom binary memory-dump format (not SQLite).
// v2: compact 28-byte file / 12-byte folder nodes + side FRN arrays.
class DbSnapshot {
public:
    static constexpr uint32_t kMagic = 0x4F454E45; // 'ENEO' LE
    static constexpr uint32_t kVersion = 2;

#pragma pack(push, 1)
    struct Header {
        uint32_t magic = kMagic;
        uint32_t version = kVersion;
        uint32_t volumeCount = 0;
        uint32_t folderCount = 0;
        uint32_t fileCount = 0;
        uint64_t arenaBytes = 0;
        uint64_t reserved = 0;
    };
#pragma pack(pop)

    static bool Save(const FileIndexTable& index, const std::wstring& path);
    static bool Load(FileIndexTable& index, const std::wstring& path);

    static std::wstring DefaultPath();
};

} // namespace EverythingNEO
