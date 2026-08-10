#include "DbSnapshot.h"
#include <windows.h>
#include <vector>

namespace EverythingNEO {

std::wstring DbSnapshot::DefaultPath() {
    wchar_t modulePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    std::wstring path(modulePath);
    size_t slash = path.find_last_of(L"\\/");
    if (slash != std::wstring::npos) {
        path = path.substr(0, slash + 1);
    } else {
        path.clear();
    }
    path += L"Everything.db";
    return path;
}

bool DbSnapshot::Save(const FileIndexTable& index, const std::wstring& path) {
    std::string arenaBlob = index.GetArena().Serialize();

    Header hdr{};
    hdr.magic = kMagic;
    hdr.version = kVersion;
    hdr.volumeCount = static_cast<uint32_t>(index.GetVolumes().size());
    hdr.folderCount = static_cast<uint32_t>(index.GetFolders().size());
    hdr.fileCount = static_cast<uint32_t>(index.GetFiles().size());
    hdr.arenaBytes = arenaBlob.size();

    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    auto writeAll = [&](const void* data, DWORD size) -> bool {
        if (size == 0) return true;
        DWORD written = 0;
        return WriteFile(hFile, data, size, &written, nullptr) && written == size;
    };

    bool ok = writeAll(&hdr, sizeof(hdr));
    if (ok && hdr.volumeCount > 0) {
        ok = writeAll(index.GetVolumes().data(),
                      static_cast<DWORD>(hdr.volumeCount * sizeof(VolumeState)));
    }
    if (ok && hdr.folderCount > 0) {
        ok = writeAll(index.GetFolders().data(),
                      static_cast<DWORD>(hdr.folderCount * sizeof(FolderNode)));
        if (ok) {
            ok = writeAll(index.GetFolderFrns().data(),
                          static_cast<DWORD>(hdr.folderCount * sizeof(uint64_t)));
        }
    }
    if (ok && hdr.fileCount > 0) {
        ok = writeAll(index.GetFiles().data(),
                      static_cast<DWORD>(hdr.fileCount * sizeof(CompactFileNode)));
        if (ok) {
            ok = writeAll(index.GetFileFrns().data(),
                          static_cast<DWORD>(hdr.fileCount * sizeof(uint64_t)));
        }
    }
    if (ok && !arenaBlob.empty()) {
        ok = writeAll(arenaBlob.data(), static_cast<DWORD>(arenaBlob.size()));
    }

    CloseHandle(hFile);
    return ok;
}

bool DbSnapshot::Load(FileIndexTable& index, const std::wstring& path) {
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER fileSize{};
    if (!GetFileSizeEx(hFile, &fileSize) || fileSize.QuadPart < static_cast<LONGLONG>(sizeof(Header))) {
        CloseHandle(hFile);
        return false;
    }

    HANDLE hMap = CreateFileMappingW(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!hMap) {
        CloseHandle(hFile);
        return false;
    }

    const void* view = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (!view) {
        CloseHandle(hMap);
        CloseHandle(hFile);
        return false;
    }

    const uint8_t* ptr = static_cast<const uint8_t*>(view);
    size_t remaining = static_cast<size_t>(fileSize.QuadPart);

    auto take = [&](size_t n) -> const void* {
        if (remaining < n) return nullptr;
        const void* p = ptr;
        ptr += n;
        remaining -= n;
        return p;
    };

    bool ok = false;
    do {
        const Header* hdr = static_cast<const Header*>(take(sizeof(Header)));
        if (!hdr || hdr->magic != kMagic || hdr->version != kVersion) break;

        index.Clear();
        index.Reserve(hdr->fileCount, hdr->folderCount);

        if (hdr->volumeCount > 0) {
            const VolumeState* vols = static_cast<const VolumeState*>(
                take(hdr->volumeCount * sizeof(VolumeState)));
            if (!vols) break;
            for (uint32_t i = 0; i < hdr->volumeCount; ++i) index.AddVolume(vols[i]);
        }

        if (hdr->folderCount > 0) {
            const FolderNode* folders = static_cast<const FolderNode*>(
                take(hdr->folderCount * sizeof(FolderNode)));
            const uint64_t* frns = static_cast<const uint64_t*>(
                take(hdr->folderCount * sizeof(uint64_t)));
            if (!folders || !frns) break;
            index.GetFoldersMutable().assign(folders, folders + hdr->folderCount);
            index.GetFolderFrnsMutable().assign(frns, frns + hdr->folderCount);
        }

        if (hdr->fileCount > 0) {
            const CompactFileNode* files = static_cast<const CompactFileNode*>(
                take(hdr->fileCount * sizeof(CompactFileNode)));
            const uint64_t* frns = static_cast<const uint64_t*>(
                take(hdr->fileCount * sizeof(uint64_t)));
            if (!files || !frns) break;
            index.GetFilesMutable().assign(files, files + hdr->fileCount);
            index.GetFileFrnsMutable().assign(frns, frns + hdr->fileCount);
        }

        if (hdr->arenaBytes > 0) {
            const void* arenaData = take(static_cast<size_t>(hdr->arenaBytes));
            if (!arenaData) break;
            if (!index.GetArenaMutable().Deserialize(arenaData, static_cast<size_t>(hdr->arenaBytes))) {
                break;
            }
        }

        index.RebuildFrnMaps();
        index.ShrinkToFit();
        ok = true;
    } while (false);

    UnmapViewOfFile(view);
    CloseHandle(hMap);
    CloseHandle(hFile);
    return ok;
}

} // namespace EverythingNEO
