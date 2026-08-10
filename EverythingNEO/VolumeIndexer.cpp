#include "VolumeIndexer.h"
#include <windows.h>
#include <winioctl.h>
#include <unordered_map>
#include <vector>
#include <string>

namespace EverythingNEO {

namespace {

std::string WideToUtf8(const wchar_t* wstr, int len = -1) {
    if (!wstr) return {};
    if (len < 0) len = static_cast<int>(wcslen(wstr));
    if (len <= 0) return {};
    int count = WideCharToMultiByte(CP_UTF8, 0, wstr, len, nullptr, 0, nullptr, nullptr);
    if (count <= 0) return {};
    std::string utf8(static_cast<size_t>(count), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr, len, utf8.data(), count, nullptr, nullptr);
    return utf8;
}

uint64_t FileTimeToU64(const FILETIME& ft) {
    ULARGE_INTEGER u{};
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    return u.QuadPart;
}

HANDLE OpenVolume(char driveLetter) {
    wchar_t path[] = L"\\\\.\\X:";
    path[4] = static_cast<wchar_t>(toupper(static_cast<unsigned char>(driveLetter)));
    return CreateFileW(path, GENERIC_READ,
                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                       nullptr, OPEN_EXISTING, 0, nullptr);
}

void FinalizeVolumeJournal(FileIndexTable& index, char driveLetter) {
    VolumeState* vol = index.FindVolume(driveLetter);
    if (!vol) return;
    HANDLE hVol = OpenVolume(driveLetter);
    if (hVol == INVALID_HANDLE_VALUE) return;
    USN_JOURNAL_DATA_V0 journal{};
    DWORD br = 0;
    if (DeviceIoControl(hVol, FSCTL_QUERY_USN_JOURNAL, nullptr, 0,
                         &journal, sizeof(journal), &br, nullptr)) {
        vol->journalId = journal.UsnJournalID;
        vol->nextUsn = journal.NextUsn;
    }
    CloseHandle(hVol);
}

} // namespace

bool VolumeIndexer::IsElevated() {
    BOOL elevated = FALSE;
    HANDLE token = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION te{};
        DWORD size = 0;
        if (GetTokenInformation(token, TokenElevation, &te, sizeof(te), &size)) {
            elevated = te.TokenIsElevated;
        }
        CloseHandle(token);
    }
    return elevated == TRUE;
}

std::vector<char> VolumeIndexer::EnumerateFixedDrives() {
    std::vector<char> drives;
    DWORD mask = GetLogicalDrives();
    for (int i = 0; i < 26; ++i) {
        if (!(mask & (1u << i))) continue;
        char letter = static_cast<char>('A' + i);
        wchar_t root[] = L"X:\\";
        root[0] = static_cast<wchar_t>(letter);
        if (GetDriveTypeW(root) == DRIVE_FIXED) {
            drives.push_back(letter);
        }
    }
    return drives;
}

bool VolumeIndexer::IndexVolume(FileIndexTable& index, char driveLetter,
                                ProgressCallback progress,
                                std::atomic<bool>* cancel) {
    driveLetter = static_cast<char>(toupper(static_cast<unsigned char>(driveLetter)));

    // Prefer USN MFT enumeration when we can open the volume (typically admin).
    if (IndexViaUsnEnum(index, driveLetter, progress, cancel)) {
        return true;
    }
    return IndexViaFindFirst(index, driveLetter, progress, cancel);
}

bool VolumeIndexer::IndexViaUsnEnum(FileIndexTable& index, char driveLetter,
                                    ProgressCallback& progress,
                                    std::atomic<bool>* cancel) {
    HANDLE hVol = OpenVolume(driveLetter);
    if (hVol == INVALID_HANDLE_VALUE) return false;

    USN_JOURNAL_DATA_V0 journal{};
    DWORD br = 0;
    if (!DeviceIoControl(hVol, FSCTL_QUERY_USN_JOURNAL, nullptr, 0,
                         &journal, sizeof(journal), &br, nullptr)) {
        CloseHandle(hVol);
        return false;
    }

    IndexProgress prog{};
    prog.driveLetter = driveLetter;
    prog.usingUsnEnum = true;
    prog.phase = "USN/MFT enum";

    // Temporary raw records: FRN -> (parentFRN, name, isDir, attrs)
    struct RawEntry {
        uint64_t parentFrn = 0;
        std::string name;
        bool isDir = false;
        uint16_t attrs = 0;
    };
    std::unordered_map<uint64_t, RawEntry> entries;
    entries.reserve(500000);

    MFT_ENUM_DATA_V0 med{};
    med.StartFileReferenceNumber = 0;
    med.LowUsn = 0;
    med.HighUsn = MAXLONGLONG;

    std::vector<char> buffer(1 << 20); // 1 MB

    for (;;) {
        if (cancel && cancel->load()) {
            CloseHandle(hVol);
            return false;
        }

        DWORD bytesReturned = 0;
        BOOL ok = DeviceIoControl(hVol, FSCTL_ENUM_USN_DATA,
                                  &med, sizeof(med),
                                  buffer.data(), static_cast<DWORD>(buffer.size()),
                                  &bytesReturned, nullptr);
        if (!ok) {
            DWORD err = GetLastError();
            if (err == ERROR_HANDLE_EOF) break;
            CloseHandle(hVol);
            return false;
        }
        if (bytesReturned < sizeof(USN)) break;

        // First 8 bytes = next StartFileReferenceNumber
        med.StartFileReferenceNumber = *reinterpret_cast<USN*>(buffer.data());
        char* p = buffer.data() + sizeof(USN);
        char* end = buffer.data() + bytesReturned;

        while (p < end) {
            auto* rec = reinterpret_cast<USN_RECORD_V2*>(p);
            if (rec->RecordLength == 0) break;

            const wchar_t* namePtr = reinterpret_cast<const wchar_t*>(
                reinterpret_cast<char*>(rec) + rec->FileNameOffset);
            int nameChars = rec->FileNameLength / sizeof(wchar_t);
            std::string name = WideToUtf8(namePtr, nameChars);

            // Skip . and ..
            if (name == "." || name == "..") {
                p += rec->RecordLength;
                continue;
            }

            RawEntry e;
            e.parentFrn = rec->ParentFileReferenceNumber;
            e.name = std::move(name);
            e.isDir = (rec->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            e.attrs = static_cast<uint16_t>(rec->FileAttributes);
            entries[rec->FileReferenceNumber] = std::move(e);

            if (e.isDir) ++prog.foldersSeen;
            else ++prog.filesSeen;

            if (progress && ((prog.filesSeen + prog.foldersSeen) % 50000 == 0)) {
                progress(prog);
            }

            p += rec->RecordLength;
        }
    }

    CloseHandle(hVol);

    // Build folder tree: root is "C:" style
    std::string rootName(1, driveLetter);
    rootName += ":";
    uint32_t rootId = index.AddFolder(INVALID_FOLDER_ID, rootName, FILE_ATTRIBUTE_DIRECTORY, 0);

    VolumeState vol{};
    vol.driveLetter = driveLetter;
    vol.rootFolderId = rootId;
    index.AddVolume(vol);

    // Map FRN -> folderId for directories only
    std::unordered_map<uint64_t, uint32_t> frnToFolder;
    frnToFolder.reserve(entries.size() / 4);

    // Multi-pass: create folders when parent is known (or parent missing → attach to root)
    // Simple approach: topological-ish iterative creation
    bool progressed = true;
    while (progressed) {
        progressed = false;
        for (auto& [frn, e] : entries) {
            if (!e.isDir) continue;
            if (frnToFolder.count(frn)) continue;

            uint32_t parentId = rootId;
            auto pit = frnToFolder.find(e.parentFrn);
            if (pit != frnToFolder.end()) {
                parentId = pit->second;
            } else if (entries.count(e.parentFrn)) {
                // Parent folder not yet created
                continue;
            }
            // Parent not in map at all (volume root / deleted) → attach to drive root

            uint32_t id = index.AddFolder(parentId, e.name, e.attrs, frn);
            frnToFolder[frn] = id;
            progressed = true;
        }
    }

    // Remaining orphan directories → root
    for (auto& [frn, e] : entries) {
        if (!e.isDir || frnToFolder.count(frn)) continue;
        uint32_t id = index.AddFolder(rootId, e.name, e.attrs, frn);
        frnToFolder[frn] = id;
    }

    // Files
    for (auto& [frn, e] : entries) {
        if (e.isDir) continue;
        uint32_t parentId = rootId;
        auto pit = frnToFolder.find(e.parentFrn);
        if (pit != frnToFolder.end()) parentId = pit->second;
        index.AddFile(parentId, e.name, 0, e.attrs, 0, frn);
    }

    prog.phase = "USN enum done";
    if (progress) progress(prog);
    FinalizeVolumeJournal(index, driveLetter);
    return true;
}

void VolumeIndexer::ScanDirRecursive(FileIndexTable& index, uint32_t parentId,
                                     const std::wstring& dirPath,
                                     ProgressCallback& progress,
                                     IndexProgress& prog,
                                     std::atomic<bool>* cancel) {
    if (cancel && cancel->load()) return;

    std::wstring pattern = dirPath + L"\\*";
    WIN32_FIND_DATAW fd{};
    HANDLE hFind = FindFirstFileExW(pattern.c_str(), FindExInfoBasic, &fd,
                                    FindExSearchNameMatch, nullptr,
                                    FIND_FIRST_EX_LARGE_FETCH);
    if (hFind == INVALID_HANDLE_VALUE) return;

    std::vector<std::pair<uint32_t, std::wstring>> subDirs;

    do {
        if (cancel && cancel->load()) break;
        if (fd.cFileName[0] == L'.') {
            if (fd.cFileName[1] == L'\0') continue;
            if (fd.cFileName[1] == L'.' && fd.cFileName[2] == L'\0') continue;
        }

        std::string name = WideToUtf8(fd.cFileName);
        if (name.empty()) continue;

        uint16_t attrs = static_cast<uint16_t>(fd.dwFileAttributes);
        ULARGE_INTEGER size{};
        size.LowPart = fd.nFileSizeLow;
        size.HighPart = fd.nFileSizeHigh;
        uint64_t dateMod = FileTimeToU64(fd.ftLastWriteTime);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            uint32_t subId = index.AddFolder(parentId, name, attrs, 0);
            subDirs.emplace_back(subId, dirPath + L"\\" + fd.cFileName);
            ++prog.foldersSeen;
        } else {
            index.AddFile(parentId, name, size.QuadPart, attrs, dateMod, 0);
            ++prog.filesSeen;
        }

        if (progress && ((prog.filesSeen + prog.foldersSeen) % 20000 == 0)) {
            progress(prog);
        }
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);

    for (auto& [subId, subPath] : subDirs) {
        ScanDirRecursive(index, subId, subPath, progress, prog, cancel);
    }
}

bool VolumeIndexer::IndexViaFindFirst(FileIndexTable& index, char driveLetter,
                                      ProgressCallback& progress,
                                      std::atomic<bool>* cancel) {
    IndexProgress prog{};
    prog.driveLetter = driveLetter;
    prog.usingUsnEnum = false;
    prog.phase = "FindFirstFile scan";

    std::string rootName(1, driveLetter);
    rootName += ":";
    uint32_t rootId = index.AddFolder(INVALID_FOLDER_ID, rootName, FILE_ATTRIBUTE_DIRECTORY, 0);

    VolumeState vol{};
    vol.driveLetter = driveLetter;
    vol.rootFolderId = rootId;
    index.AddVolume(vol);

    wchar_t rootPath[] = L"X:";
    rootPath[0] = static_cast<wchar_t>(driveLetter);

    ScanDirRecursive(index, rootId, rootPath, progress, prog, cancel);

    prog.phase = "scan done";
    if (progress) progress(prog);
    FinalizeVolumeJournal(index, driveLetter);
    return !(cancel && cancel->load());
}

} // namespace EverythingNEO
