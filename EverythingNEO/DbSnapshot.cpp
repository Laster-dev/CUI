#include "DbSnapshot.h"
#include <windows.h>
#include <shlobj.h>
#include <vector>

#pragma comment(lib, "shell32.lib")

namespace EverythingNEO {
namespace {

std::wstring ModuleDirectory() {
    wchar_t modulePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    std::wstring path(modulePath);
    size_t slash = path.find_last_of(L"\\/");
    if (slash != std::wstring::npos) {
        return path.substr(0, slash + 1);
    }
    return {};
}

bool FileExists(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

bool CopyFileBestEffort(const std::wstring& src, const std::wstring& dst) {
    if (src == dst) return true;
    if (!DbSnapshot::EnsureParentDir(dst)) return false;
    return CopyFileW(src.c_str(), dst.c_str(), FALSE) != 0;
}

} // namespace

std::wstring DbSnapshot::DefaultPath() {
    wchar_t localAppData[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData))) {
        std::wstring path(localAppData);
        if (!path.empty() && path.back() != L'\\' && path.back() != L'/') {
            path += L'\\';
        }
        path += L"EverythingNEO\\Everything.db";
        return path;
    }
    return ModuleDirectory() + L"Everything.db";
}

bool DbSnapshot::EnsureParentDir(const std::wstring& filePath) {
    size_t slash = filePath.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return true;
    std::wstring dir = filePath.substr(0, slash);
    if (dir.empty()) return true;
    size_t start = 0;
    if (dir.size() >= 2 && dir[1] == L':') start = 2;
    while (start < dir.size() && (dir[start] == L'\\' || dir[start] == L'/')) ++start;
    for (size_t i = start; i <= dir.size(); ++i) {
        if (i == dir.size() || dir[i] == L'\\' || dir[i] == L'/') {
            if (i > start) {
                std::wstring sub = dir.substr(0, i);
                CreateDirectoryW(sub.c_str(), nullptr);
            }
        }
    }
    return true;
}

bool DbSnapshot::Save(const FileIndexTable& index, const std::wstring& path) {
    if (!EnsureParentDir(path)) return false;

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

bool DbSnapshot::LoadPreferred(FileIndexTable& index, std::wstring* loadedFrom) {
    const std::wstring primary = DefaultPath();
    std::vector<std::wstring> candidates;
    candidates.push_back(primary);

    const std::wstring moduleDir = ModuleDirectory();
    if (!moduleDir.empty()) {
        candidates.push_back(moduleDir + L"Everything.db");

        // Climb parents looking for solution-style x64\{Release|Debug}\Everything.db
        // (VS often builds to EverythingNEO\x64\Release while an older DB sits in
        //  $(SolutionDir)x64\Release).
        std::wstring walk = moduleDir;
        for (int i = 0; i < 6; ++i) {
            if (walk.size() <= 3) break;
            if (walk.back() == L'\\' || walk.back() == L'/') {
                walk.pop_back();
            }
            size_t slash = walk.find_last_of(L"\\/");
            if (slash == std::wstring::npos) break;
            std::wstring parent = walk.substr(0, slash + 1);
            candidates.push_back(parent + L"x64\\Release\\Everything.db");
            candidates.push_back(parent + L"x64\\Debug\\Everything.db");
            walk = parent;
        }
    }

    std::wstring chosen;
    for (const auto& path : candidates) {
        if (path.empty() || !FileExists(path)) continue;
        if (!Load(index, path)) continue;
        chosen = path;
        break;
    }

    if (chosen.empty()) return false;

    if (loadedFrom) *loadedFrom = chosen;

    // Keep a canonical copy under LocalAppData so every build output shares one DB.
    if (_wcsicmp(chosen.c_str(), primary.c_str()) != 0) {
        CopyFileBestEffort(chosen, primary);
    }
    return true;
}

} // namespace EverythingNEO
