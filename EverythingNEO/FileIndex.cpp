#include "FileIndex.h"
#include <algorithm>
#include <cctype>

namespace EverythingNEO {

uint32_t FileIndexTable::AddFolder(uint32_t parentFolderId, std::string_view name,
                                   uint16_t attributes, uint64_t frn) {
    FolderNode node{};
    node.parent_folder_id = parentFolderId;
    node.name_offset = m_arena.AddString(name);
    node.name_length = static_cast<uint16_t>((std::min)(name.length(), size_t{0x7FFF}));
    node.attributes = static_cast<uint16_t>(attributes & ~kAttrDeleted);

    uint32_t folderId = static_cast<uint32_t>(m_folders.size());
    m_folders.push_back(node);
    m_folderFrns.push_back(frn);
    if (frn != 0) m_folderByFrn[frn] = folderId;
    return folderId;
}

uint32_t FileIndexTable::AddFile(uint32_t parentFolderId, std::string_view name, uint64_t fileSize,
                                 uint16_t attributes, uint64_t dateModified, uint64_t frn) {
    CompactFileNode node{};
    node.parent_folder_id = parentFolderId;
    node.name_offset = m_arena.AddString(name);
    node.name_length = static_cast<uint16_t>((std::min)(name.length(), size_t{0x7FFF}));
    node.file_size = fileSize;
    node.date_modified = dateModified;
    node.attributes = static_cast<uint16_t>(attributes & ~kAttrDeleted);

    uint32_t fileId = static_cast<uint32_t>(m_files.size());
    m_files.push_back(node);
    m_fileFrns.push_back(frn);
    if (frn != 0) m_fileByFrn[frn] = fileId;
    return fileId;
}

uint32_t FileIndexTable::FindFolderByFrn(uint64_t frn) const {
    auto it = m_folderByFrn.find(frn);
    return it == m_folderByFrn.end() ? INVALID_FOLDER_ID : it->second;
}

uint32_t FileIndexTable::FindFileByFrn(uint64_t frn) const {
    auto it = m_fileByFrn.find(frn);
    return it == m_fileByFrn.end() ? INVALID_FILE_ID : it->second;
}

void FileIndexTable::RemapFolderFrn(uint32_t folderId, uint64_t newFrn) {
    if (folderId >= m_folders.size()) return;
    uint64_t old = m_folderFrns[folderId];
    if (old != 0) m_folderByFrn.erase(old);
    m_folderFrns[folderId] = newFrn;
    if (newFrn != 0) m_folderByFrn[newFrn] = folderId;
}

void FileIndexTable::RemapFileFrn(uint32_t fileId, uint64_t newFrn) {
    if (fileId >= m_files.size()) return;
    uint64_t old = m_fileFrns[fileId];
    if (old != 0) m_fileByFrn.erase(old);
    m_fileFrns[fileId] = newFrn;
    if (newFrn != 0) m_fileByFrn[newFrn] = fileId;
}

bool FileIndexTable::IsFileDeleted(uint32_t fileId) const {
    return fileId < m_files.size() && (m_files[fileId].attributes & kAttrDeleted) != 0;
}

bool FileIndexTable::IsFolderDeleted(uint32_t folderId) const {
    return folderId < m_folders.size() && (m_folders[folderId].attributes & kAttrDeleted) != 0;
}

uint64_t FileIndexTable::GetFileFrn(uint32_t fileId) const {
    return fileId < m_fileFrns.size() ? m_fileFrns[fileId] : 0;
}

uint64_t FileIndexTable::GetFolderFrn(uint32_t folderId) const {
    return folderId < m_folderFrns.size() ? m_folderFrns[folderId] : 0;
}

bool FileIndexTable::SoftDeleteFile(uint32_t fileId) {
    if (fileId >= m_files.size()) return false;
    m_files[fileId].attributes = static_cast<uint16_t>(m_files[fileId].attributes | kAttrDeleted);
    uint64_t frn = m_fileFrns[fileId];
    if (frn != 0) m_fileByFrn.erase(frn);
    m_fileFrns[fileId] = 0;
    return true;
}

bool FileIndexTable::SoftDeleteFolder(uint32_t folderId) {
    if (folderId >= m_folders.size()) return false;
    m_folders[folderId].attributes = static_cast<uint16_t>(m_folders[folderId].attributes | kAttrDeleted);
    uint64_t frn = m_folderFrns[folderId];
    if (frn != 0) m_folderByFrn.erase(frn);
    m_folderFrns[folderId] = 0;
    return true;
}

bool FileIndexTable::RenameFile(uint32_t fileId, uint32_t newParentId, std::string_view newName) {
    if (fileId >= m_files.size()) return false;
    auto& node = m_files[fileId];
    node.parent_folder_id = newParentId;
    node.name_offset = m_arena.AddString(newName);
    node.name_length = static_cast<uint16_t>((std::min)(newName.length(), size_t{0x7FFF}));
    node.attributes = static_cast<uint16_t>(node.attributes & ~kAttrDeleted);
    return true;
}

bool FileIndexTable::RenameFolder(uint32_t folderId, uint32_t newParentId, std::string_view newName) {
    if (folderId >= m_folders.size()) return false;
    auto& node = m_folders[folderId];
    node.parent_folder_id = newParentId;
    node.name_offset = m_arena.AddString(newName);
    node.name_length = static_cast<uint16_t>((std::min)(newName.length(), size_t{0x7FFF}));
    node.attributes = static_cast<uint16_t>(node.attributes & ~kAttrDeleted);
    return true;
}

bool FileIndexTable::UpdateFileMeta(uint32_t fileId, uint64_t fileSize, uint64_t dateModified) {
    if (fileId >= m_files.size()) return false;
    m_files[fileId].file_size = fileSize;
    m_files[fileId].date_modified = dateModified;
    return true;
}

std::string_view FileIndexTable::GetFileName(size_t fileIndex) const {
    if (fileIndex >= m_files.size()) return {};
    const auto& file = m_files[fileIndex];
    return m_arena.GetString(file.name_offset, file.name_length);
}

std::string_view FileIndexTable::GetFolderName(uint32_t folderId) const {
    if (folderId >= m_folders.size()) return {};
    const auto& folder = m_folders[folderId];
    return m_arena.GetString(folder.name_offset, folder.name_length);
}

std::string FileIndexTable::GetFolderPath(uint32_t folderId) const {
    if (folderId >= m_folders.size()) return {};

    std::vector<uint32_t> chain;
    chain.reserve(16);
    uint32_t curr = folderId;
    int guard = 0;
    while (curr != INVALID_FOLDER_ID && curr < m_folders.size() && guard++ < 512) {
        chain.push_back(curr);
        curr = m_folders[curr].parent_folder_id;
    }

    std::string path;
    path.reserve(chain.size() * 16);
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        std::string_view name = GetFolderName(*it);
        if (!path.empty() && path.back() != '\\') path += '\\';
        path.append(name.data(), name.length());
    }
    return path;
}

std::string FileIndexTable::GetFilePath(size_t fileIndex) const {
    if (fileIndex >= m_files.size()) return {};
    const auto& file = m_files[fileIndex];
    std::string path = GetFolderPath(file.parent_folder_id);
    if (!path.empty() && path.back() != '\\') path += '\\';
    std::string_view fileName = GetFileName(fileIndex);
    path.append(fileName.data(), fileName.length());
    return path;
}

size_t FileIndexTable::GetLiveFileCount() const {
    size_t n = 0;
    for (const auto& f : m_files) {
        if ((f.attributes & kAttrDeleted) == 0) ++n;
    }
    return n;
}

void FileIndexTable::AddVolume(const VolumeState& vol) {
    m_volumes.push_back(vol);
}

VolumeState* FileIndexTable::FindVolume(char driveLetter) {
    char upper = static_cast<char>(toupper(static_cast<unsigned char>(driveLetter)));
    for (auto& v : m_volumes) {
        if (static_cast<char>(toupper(static_cast<unsigned char>(v.driveLetter))) == upper) {
            return &v;
        }
    }
    return nullptr;
}

void FileIndexTable::RebuildFrnMaps() {
    m_folderByFrn.clear();
    m_fileByFrn.clear();
    m_folderByFrn.reserve(m_folders.size() / 4 + 16);
    m_fileByFrn.reserve(m_fileFrns.size() / 8 + 16);
    for (uint32_t i = 0; i < m_folders.size(); ++i) {
        if ((m_folders[i].attributes & kAttrDeleted) == 0 && m_folderFrns[i] != 0) {
            m_folderByFrn[m_folderFrns[i]] = i;
        }
    }
    for (uint32_t i = 0; i < m_files.size(); ++i) {
        if ((m_files[i].attributes & kAttrDeleted) == 0 && m_fileFrns[i] != 0) {
            m_fileByFrn[m_fileFrns[i]] = i;
        }
    }
}

void FileIndexTable::Reserve(size_t expectedFiles, size_t expectedFolders) {
    m_files.reserve(expectedFiles);
    m_fileFrns.reserve(expectedFiles);
    m_folders.reserve(expectedFolders);
    m_folderFrns.reserve(expectedFolders);
}

void FileIndexTable::ShrinkToFit() {
    m_files.shrink_to_fit();
    m_fileFrns.shrink_to_fit();
    m_folders.shrink_to_fit();
    m_folderFrns.shrink_to_fit();
    m_volumes.shrink_to_fit();
}

void FileIndexTable::Clear() {
    m_files.clear();
    m_fileFrns.clear();
    m_folders.clear();
    m_folderFrns.clear();
    m_volumes.clear();
    m_folderByFrn.clear();
    m_fileByFrn.clear();
    m_arena.Clear();
}

void FileIndexTable::Swap(FileIndexTable& other) noexcept {
    m_arena.Swap(other.m_arena);
    m_folders.swap(other.m_folders);
    m_files.swap(other.m_files);
    m_folderFrns.swap(other.m_folderFrns);
    m_fileFrns.swap(other.m_fileFrns);
    m_volumes.swap(other.m_volumes);
    m_folderByFrn.swap(other.m_folderByFrn);
    m_fileByFrn.swap(other.m_fileByFrn);
}

} // namespace EverythingNEO
