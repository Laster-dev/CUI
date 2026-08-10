#pragma once

#include "StringArena.h"
#include <vector>
#include <string>
#include <string_view>
#include <cstdint>
#include <limits>
#include <unordered_map>

namespace EverythingNEO {

constexpr uint32_t INVALID_FOLDER_ID = std::numeric_limits<uint32_t>::max();
constexpr uint32_t INVALID_FILE_ID = std::numeric_limits<uint32_t>::max();
constexpr uint16_t kAttrDeleted = 0x8000; // packed into attributes (not a Win32 flag)

#pragma pack(push, 1)
// Everything-style compact record: name + parent only in hot path; FRN kept aside.
struct CompactFileNode {
    uint32_t parent_folder_id = INVALID_FOLDER_ID;
    uint32_t name_offset = 0;
    uint64_t file_size = 0;
    uint64_t date_modified = 0;
    uint16_t attributes = 0; // low bits = Win32 attrs; kAttrDeleted marks soft-delete
    uint16_t name_length = 0;
};

struct FolderNode {
    uint32_t parent_folder_id = INVALID_FOLDER_ID;
    uint32_t name_offset = 0;
    uint16_t attributes = 0;
    uint16_t name_length = 0;
};
#pragma pack(pop)

static_assert(sizeof(CompactFileNode) == 28, "CompactFileNode must be 28 bytes");
static_assert(sizeof(FolderNode) == 12, "FolderNode must be 12 bytes");

struct VolumeState {
    char driveLetter = 'C';
    uint64_t journalId = 0;
    int64_t nextUsn = 0;
    uint32_t rootFolderId = INVALID_FOLDER_ID;
};

class FileIndexTable {
public:
    FileIndexTable() = default;
    ~FileIndexTable() = default;

    uint32_t AddFolder(uint32_t parentFolderId, std::string_view name, uint16_t attributes,
                       uint64_t frn = 0);
    uint32_t AddFile(uint32_t parentFolderId, std::string_view name, uint64_t fileSize,
                     uint16_t attributes, uint64_t dateModified = 0, uint64_t frn = 0);

    uint32_t FindFolderByFrn(uint64_t frn) const;
    uint32_t FindFileByFrn(uint64_t frn) const;
    void RemapFolderFrn(uint32_t folderId, uint64_t newFrn);
    void RemapFileFrn(uint32_t fileId, uint64_t newFrn);

    bool SoftDeleteFile(uint32_t fileId);
    bool SoftDeleteFolder(uint32_t folderId);
    bool RenameFile(uint32_t fileId, uint32_t newParentId, std::string_view newName);
    bool RenameFolder(uint32_t folderId, uint32_t newParentId, std::string_view newName);
    bool UpdateFileMeta(uint32_t fileId, uint64_t fileSize, uint64_t dateModified);

    bool IsFileDeleted(uint32_t fileId) const;
    bool IsFolderDeleted(uint32_t folderId) const;
    uint64_t GetFileFrn(uint32_t fileId) const;
    uint64_t GetFolderFrn(uint32_t folderId) const;

    std::string_view GetFileName(size_t fileIndex) const;
    std::string_view GetFolderName(uint32_t folderId) const;
    std::string GetFolderPath(uint32_t folderId) const;
    std::string GetFilePath(size_t fileIndex) const;

    const std::vector<CompactFileNode>& GetFiles() const { return m_files; }
    const std::vector<FolderNode>& GetFolders() const { return m_folders; }
    std::vector<CompactFileNode>& GetFilesMutable() { return m_files; }
    std::vector<FolderNode>& GetFoldersMutable() { return m_folders; }
    const std::vector<uint64_t>& GetFileFrns() const { return m_fileFrns; }
    const std::vector<uint64_t>& GetFolderFrns() const { return m_folderFrns; }
    std::vector<uint64_t>& GetFileFrnsMutable() { return m_fileFrns; }
    std::vector<uint64_t>& GetFolderFrnsMutable() { return m_folderFrns; }
    const StringArena& GetArena() const { return m_arena; }
    StringArena& GetArenaMutable() { return m_arena; }

    size_t GetFileCount() const { return m_files.size(); }
    size_t GetFolderCount() const { return m_folders.size(); }
    size_t GetLiveFileCount() const;

    size_t GetFileNodesMemory() const {
        return m_files.capacity() * sizeof(CompactFileNode) + m_fileFrns.capacity() * sizeof(uint64_t);
    }
    size_t GetFolderNodesMemory() const {
        return m_folders.capacity() * sizeof(FolderNode) + m_folderFrns.capacity() * sizeof(uint64_t);
    }
    size_t GetStringArenaMemory() const { return m_arena.GetTotalMemoryUsage(); }
    size_t GetFrnMapMemory() const {
        // rough: node + overhead ~32 bytes/entry
        return (m_folderByFrn.size() + m_fileByFrn.size()) * 40;
    }
    size_t GetTotalMemoryUsage() const {
        return GetFileNodesMemory() + GetFolderNodesMemory() + GetStringArenaMemory() + GetFrnMapMemory();
    }

    void AddVolume(const VolumeState& vol);
    const std::vector<VolumeState>& GetVolumes() const { return m_volumes; }
    std::vector<VolumeState>& GetVolumesMutable() { return m_volumes; }
    VolumeState* FindVolume(char driveLetter);

    void RebuildFrnMaps();
    void Reserve(size_t expectedFiles, size_t expectedFolders);
    void ShrinkToFit();
    void Clear();

private:
    StringArena m_arena;
    std::vector<FolderNode> m_folders;
    std::vector<CompactFileNode> m_files;
    std::vector<uint64_t> m_folderFrns;
    std::vector<uint64_t> m_fileFrns;
    std::vector<VolumeState> m_volumes;
    std::unordered_map<uint64_t, uint32_t> m_folderByFrn;
    std::unordered_map<uint64_t, uint32_t> m_fileByFrn;
};

} // namespace EverythingNEO
