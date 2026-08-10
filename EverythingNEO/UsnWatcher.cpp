#include "UsnWatcher.h"
#include <windows.h>
#include <winioctl.h>
#include <string>
#include <vector>
#include <mutex>
#include <shared_mutex>

namespace EverythingNEO {

namespace {

std::string WideToUtf8(const wchar_t* wstr, int len) {
    if (!wstr || len <= 0) return {};
    int count = WideCharToMultiByte(CP_UTF8, 0, wstr, len, nullptr, 0, nullptr, nullptr);
    if (count <= 0) return {};
    std::string utf8(static_cast<size_t>(count), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr, len, utf8.data(), count, nullptr, nullptr);
    return utf8;
}

std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (n <= 0) return {};
    std::wstring out(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n);
    return out;
}

HANDLE OpenVolume(char driveLetter) {
    wchar_t path[] = L"\\\\.\\X:";
    path[4] = static_cast<wchar_t>(toupper(static_cast<unsigned char>(driveLetter)));
    return CreateFileW(path, GENERIC_READ,
                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                       nullptr, OPEN_EXISTING, 0, nullptr);
}

bool QueryUsnJournal(HANDLE hVol, USN_JOURNAL_DATA_V0& journal) {
    DWORD br = 0;
    return DeviceIoControl(hVol, FSCTL_QUERY_USN_JOURNAL, nullptr, 0,
                           &journal, sizeof(journal), &br, nullptr) == TRUE;
}

void StatFileMeta(FileIndexTable* index, std::shared_mutex* mutex,
                  uint32_t fileId, const std::string& path) {
    if (!index || path.empty()) return;
    std::wstring wpath = Utf8ToWide(path);
    WIN32_FILE_ATTRIBUTE_DATA fad{};
    if (!GetFileAttributesExW(wpath.c_str(), GetFileExInfoStandard, &fad)) return;
    ULARGE_INTEGER size{};
    size.LowPart = fad.nFileSizeLow;
    size.HighPart = fad.nFileSizeHigh;
    ULARGE_INTEGER date{};
    date.LowPart = fad.ftLastWriteTime.dwLowDateTime;
    date.HighPart = fad.ftLastWriteTime.dwHighDateTime;
    if (mutex) {
        std::unique_lock<std::shared_mutex> lock(*mutex);
        index->UpdateFileMeta(fileId, size.QuadPart, date.QuadPart);
    } else {
        index->UpdateFileMeta(fileId, size.QuadPart, date.QuadPart);
    }
}

bool ProcessUsnRecords(FileIndexTable* index, std::shared_mutex* mutex,
                       char driveLetter, char* buffer, DWORD bytesReturned,
                       bool& changed, std::vector<std::pair<uint32_t, std::string>>& metaUpdates) {
    if (!index || !mutex || bytesReturned < sizeof(USN)) return false;

    char* p = buffer + sizeof(USN);
    char* end = buffer + bytesReturned;

    while (p < end) {
        auto* rec = reinterpret_cast<USN_RECORD_V2*>(p);
        if (rec->RecordLength == 0) break;

        const wchar_t* namePtr = reinterpret_cast<const wchar_t*>(
            reinterpret_cast<char*>(rec) + rec->FileNameOffset);
        int nameChars = rec->FileNameLength / sizeof(wchar_t);
        std::string name = WideToUtf8(namePtr, nameChars);

        DWORD reason = rec->Reason;
        bool isDir = (rec->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        uint64_t frn = rec->FileReferenceNumber;
        uint64_t parentFrn = rec->ParentFileReferenceNumber;

        std::string pathForMeta;
        uint32_t fileIdForMeta = INVALID_FILE_ID;

        {
            std::unique_lock<std::shared_mutex> lock(*mutex);
            uint32_t parentId = index->FindFolderByFrn(parentFrn);
            if (parentId == INVALID_FOLDER_ID) {
                if (auto* v = index->FindVolume(driveLetter)) {
                    parentId = v->rootFolderId;
                }
            }

            const bool fileDeleted = (reason & USN_REASON_FILE_DELETE) != 0;
            const bool renamedOld = (reason & USN_REASON_RENAME_OLD_NAME) != 0;
            const bool renamedNew = (reason & USN_REASON_RENAME_NEW_NAME) != 0;
            const bool created = (reason & (USN_REASON_FILE_CREATE | USN_REASON_HARD_LINK_CHANGE)) != 0;
            const bool needsMeta = !isDir && !fileDeleted
                && (reason & (USN_REASON_BASIC_INFO_CHANGE | USN_REASON_DATA_EXTEND
                              | USN_REASON_DATA_OVERWRITE | USN_REASON_CLOSE)) != 0;

            if (fileDeleted || renamedOld) {
                if (isDir) {
                    uint32_t id = index->FindFolderByFrn(frn);
                    if (id != INVALID_FOLDER_ID) {
                        index->SoftDeleteFolder(id);
                        changed = true;
                    }
                } else {
                    uint32_t id = index->FindFileByFrn(frn);
                    if (id != INVALID_FILE_ID) {
                        index->SoftDeleteFile(id);
                        changed = true;
                    }
                }
            }

            if (created || renamedNew) {
                if (!name.empty() && name != "." && name != "..") {
                    if (isDir) {
                        uint32_t existing = index->FindFolderByFrn(frn);
                        if (existing == INVALID_FOLDER_ID) {
                            index->AddFolder(parentId, name,
                                             static_cast<uint16_t>(rec->FileAttributes), frn);
                            changed = true;
                        } else {
                            index->RenameFolder(existing, parentId, name);
                            changed = true;
                        }
                    } else {
                        uint32_t existing = index->FindFileByFrn(frn);
                        if (existing == INVALID_FILE_ID) {
                            uint32_t newId = index->AddFile(parentId, name, 0,
                                                            static_cast<uint16_t>(rec->FileAttributes),
                                                            0, frn);
                            changed = true;
                            if (newId != INVALID_FILE_ID) {
                                fileIdForMeta = newId;
                                pathForMeta = index->GetFilePath(newId);
                            }
                        } else {
                            index->RenameFile(existing, parentId, name);
                            changed = true;
                            if (needsMeta) {
                                fileIdForMeta = existing;
                                pathForMeta = index->GetFilePath(existing);
                            }
                        }
                    }
                }
            } else if ((reason & USN_REASON_CLOSE) && !(reason & USN_REASON_FILE_DELETE)) {
                if (!name.empty() && name != "." && name != "..") {
                    if (isDir) {
                        if (index->FindFolderByFrn(frn) == INVALID_FOLDER_ID) {
                            index->AddFolder(parentId, name,
                                             static_cast<uint16_t>(rec->FileAttributes), frn);
                            changed = true;
                        }
                    } else {
                        uint32_t existing = index->FindFileByFrn(frn);
                        if (existing == INVALID_FILE_ID) {
                            uint32_t newId = index->AddFile(parentId, name, 0,
                                                            static_cast<uint16_t>(rec->FileAttributes),
                                                            0, frn);
                            changed = true;
                            if (newId != INVALID_FILE_ID) {
                                fileIdForMeta = newId;
                                pathForMeta = index->GetFilePath(newId);
                            }
                        } else {
                            fileIdForMeta = existing;
                            pathForMeta = index->GetFilePath(existing);
                        }
                    }
                }
            } else if (needsMeta && !isDir) {
                uint32_t id = index->FindFileByFrn(frn);
                if (id != INVALID_FILE_ID) {
                    fileIdForMeta = id;
                    pathForMeta = index->GetFilePath(id);
                }
            }
        }

        if (fileIdForMeta != INVALID_FILE_ID && !pathForMeta.empty()) {
            metaUpdates.emplace_back(fileIdForMeta, std::move(pathForMeta));
        }

        p += rec->RecordLength;
    }

    return true;
}

} // namespace

UsnWatcher::~UsnWatcher() {
    Stop();
}

void UsnWatcher::Start(FileIndexTable& index, std::shared_mutex& indexMutex,
                       ChangeCallback onChanged, RebuildCallback onRebuild) {
    Stop();
    m_index = &index;
    m_mutex = &indexMutex;
    m_onChanged = std::move(onChanged);
    m_onRebuild = std::move(onRebuild);
    m_running.store(true);
    m_thread = std::thread([this]() { ThreadMain(); });
}

void UsnWatcher::Stop() {
    m_running.store(false);
    if (m_thread.joinable()) {
        m_thread.join();
    }
    m_index = nullptr;
    m_mutex = nullptr;
}

size_t UsnWatcher::CatchUpVolumes(FileIndexTable& index, std::shared_mutex& indexMutex,
                                  RebuildCallback onRebuild) {
    UsnWatcher temp;
    temp.m_index = &index;
    temp.m_mutex = &indexMutex;
    temp.m_onRebuild = std::move(onRebuild);

    size_t updates = 0;
    std::vector<VolumeState> volumes;
    {
        std::shared_lock<std::shared_mutex> lock(indexMutex);
        volumes = index.GetVolumes();
    }

    for (auto& vol : volumes) {
        if (vol.journalId == 0) continue;
        if (temp.ProcessVolume(vol.driveLetter, vol, true)) {
            ++updates;
        }
        std::unique_lock<std::shared_mutex> lock(indexMutex);
        if (auto* v = index.FindVolume(vol.driveLetter)) {
            v->nextUsn = vol.nextUsn;
            v->journalId = vol.journalId;
        }
    }
    return updates;
}

void UsnWatcher::ThreadMain() {
    while (m_running.load()) {
        if (!m_index || !m_mutex) break;

        std::vector<VolumeState> volumes;
        {
            std::shared_lock<std::shared_mutex> lock(*m_mutex);
            volumes = m_index->GetVolumes();
        }

        bool anyChanged = false;
        for (auto& vol : volumes) {
            if (!m_running.load()) break;
            if (vol.journalId == 0) continue;

            if (ProcessVolume(vol.driveLetter, vol, true)) {
                anyChanged = true;
            }

            std::unique_lock<std::shared_mutex> lock(*m_mutex);
            if (auto* v = m_index->FindVolume(vol.driveLetter)) {
                v->nextUsn = vol.nextUsn;
                v->journalId = vol.journalId;
            }
        }

        if (anyChanged && m_onChanged) {
            m_onChanged();
        }

        Sleep(anyChanged ? 50 : 250);
    }
}

bool UsnWatcher::ProcessVolume(char driveLetter, VolumeState& vol, bool drainAll) {
    HANDLE hVol = OpenVolume(driveLetter);
    if (hVol == INVALID_HANDLE_VALUE) return false;

    USN_JOURNAL_DATA_V0 journal{};
    if (!QueryUsnJournal(hVol, journal)) {
        CloseHandle(hVol);
        return false;
    }

    if (vol.journalId != 0 && vol.journalId != journal.UsnJournalID) {
        CloseHandle(hVol);
        vol.journalId = journal.UsnJournalID;
        vol.nextUsn = journal.NextUsn;
        if (m_onRebuild) m_onRebuild(driveLetter);
        return false;
    }

    if (vol.journalId == 0) {
        vol.journalId = journal.UsnJournalID;
        vol.nextUsn = journal.NextUsn;
        CloseHandle(hVol);
        return false;
    }

    if (vol.nextUsn == 0) {
        vol.nextUsn = journal.FirstUsn;
    }

    bool changed = false;
    std::vector<std::pair<uint32_t, std::string>> metaUpdates;
    std::vector<char> buffer(256 * 1024);
    DWORD bufferSize = static_cast<DWORD>(buffer.size());

    do {
        READ_USN_JOURNAL_DATA_V0 readData{};
        readData.StartUsn = vol.nextUsn;
        readData.ReasonMask = 0xFFFFFFFF;
        readData.ReturnOnlyOnClose = FALSE;
        readData.Timeout = drainAll ? 0 : 200;
        readData.BytesToWaitFor = drainAll ? 0 : 1;
        readData.UsnJournalID = journal.UsnJournalID;

        DWORD bytesReturned = 0;
        BOOL ok = DeviceIoControl(hVol, FSCTL_READ_USN_JOURNAL,
                                  &readData, sizeof(readData),
                                  buffer.data(), bufferSize,
                                  &bytesReturned, nullptr);
        if (!ok || bytesReturned <= sizeof(USN)) {
            break;
        }

        USN nextUsn = *reinterpret_cast<USN*>(buffer.data());
        ProcessUsnRecords(m_index, m_mutex, driveLetter,
                          buffer.data(), bytesReturned, changed, metaUpdates);
        vol.nextUsn = nextUsn;

        if (!drainAll) break;
        if (bytesReturned < bufferSize) break;
        if (nextUsn >= journal.NextUsn) break;
    } while (drainAll && m_running.load());

    CloseHandle(hVol);

    for (auto& [fileId, path] : metaUpdates) {
        StatFileMeta(m_index, m_mutex, fileId, path);
        changed = true;
    }

    return changed;
}

} // namespace EverythingNEO
