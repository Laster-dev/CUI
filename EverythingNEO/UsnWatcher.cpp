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

HANDLE OpenVolume(char driveLetter) {
    wchar_t path[] = L"\\\\.\\X:";
    path[4] = static_cast<wchar_t>(toupper(static_cast<unsigned char>(driveLetter)));
    return CreateFileW(path, GENERIC_READ,
                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                       nullptr, OPEN_EXISTING, 0, nullptr);
}

} // namespace

UsnWatcher::~UsnWatcher() {
    Stop();
}

void UsnWatcher::Start(FileIndexTable& index, std::shared_mutex& indexMutex, ChangeCallback onChanged) {
    Stop();
    m_index = &index;
    m_mutex = &indexMutex;
    m_onChanged = std::move(onChanged);
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

void UsnWatcher::ThreadMain() {
    while (m_running.load()) {
        if (!m_index || !m_mutex) break;

        std::vector<VolumeState> volumes;
        {
            std::unique_lock<std::shared_mutex> lock(*m_mutex);
            volumes = m_index->GetVolumes();
        }

        bool any = false;
        for (auto& vol : volumes) {
            if (!m_running.load()) break;
            if (vol.journalId == 0) continue;
            WatchVolume(vol.driveLetter, vol);
            any = true;

            // Persist nextUsn back
            std::unique_lock<std::shared_mutex> lock(*m_mutex);
            if (auto* v = m_index->FindVolume(vol.driveLetter)) {
                v->nextUsn = vol.nextUsn;
                v->journalId = vol.journalId;
            }
        }

        // Idle wait — USN watchers poll periodically when no waitable IO
        Sleep(any ? 500 : 2000);
    }
}

void UsnWatcher::WatchVolume(char driveLetter, VolumeState& vol) {
    HANDLE hVol = OpenVolume(driveLetter);
    if (hVol == INVALID_HANDLE_VALUE) return;

    USN_JOURNAL_DATA_V0 journal{};
    DWORD br = 0;
    if (!DeviceIoControl(hVol, FSCTL_QUERY_USN_JOURNAL, nullptr, 0,
                         &journal, sizeof(journal), &br, nullptr)) {
        CloseHandle(hVol);
        return;
    }

    // Journal recreated → skip (caller should rebuild)
    if (vol.journalId != 0 && vol.journalId != journal.UsnJournalID) {
        vol.journalId = journal.UsnJournalID;
        vol.nextUsn = journal.NextUsn;
        CloseHandle(hVol);
        return;
    }
    vol.journalId = journal.UsnJournalID;

    READ_USN_JOURNAL_DATA_V0 readData{};
    readData.StartUsn = vol.nextUsn;
    readData.ReasonMask = 0xFFFFFFFF;
    readData.ReturnOnlyOnClose = FALSE;
    readData.Timeout = 0;
    readData.BytesToWaitFor = 0;
    readData.UsnJournalID = journal.UsnJournalID;

    std::vector<char> buffer(64 * 1024);
    DWORD bytesReturned = 0;
    BOOL ok = DeviceIoControl(hVol, FSCTL_READ_USN_JOURNAL,
                              &readData, sizeof(readData),
                              buffer.data(), static_cast<DWORD>(buffer.size()),
                              &bytesReturned, nullptr);
    if (!ok || bytesReturned < sizeof(USN)) {
        CloseHandle(hVol);
        return;
    }

    USN nextUsn = *reinterpret_cast<USN*>(buffer.data());
    char* p = buffer.data() + sizeof(USN);
    char* end = buffer.data() + bytesReturned;
    bool changed = false;

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

        {
            std::unique_lock<std::shared_mutex> lock(*m_mutex);
            uint32_t parentId = m_index->FindFolderByFrn(parentFrn);
            if (parentId == INVALID_FOLDER_ID) {
                // Attach under volume root if known
                if (auto* v = m_index->FindVolume(driveLetter)) {
                    parentId = v->rootFolderId;
                }
            }

            const bool created = (reason & (USN_REASON_FILE_CREATE | USN_REASON_HARD_LINK_CHANGE)) != 0;
            const bool deleted = (reason & (USN_REASON_FILE_DELETE | USN_REASON_CLOSE)) != 0
                                 && (reason & USN_REASON_FILE_DELETE) != 0;
            const bool renamedNew = (reason & USN_REASON_RENAME_NEW_NAME) != 0;
            const bool renamedOld = (reason & USN_REASON_RENAME_OLD_NAME) != 0;

            if (deleted || renamedOld) {
                if (isDir) {
                    uint32_t id = m_index->FindFolderByFrn(frn);
                    if (id != INVALID_FOLDER_ID) {
                        m_index->SoftDeleteFolder(id);
                        changed = true;
                    }
                } else {
                    uint32_t id = m_index->FindFileByFrn(frn);
                    if (id != INVALID_FILE_ID) {
                        m_index->SoftDeleteFile(id);
                        changed = true;
                    }
                }
            }

            if (created || renamedNew) {
                if (name.empty() || name == "." || name == "..") {
                    p += rec->RecordLength;
                    continue;
                }
                if (isDir) {
                    uint32_t existing = m_index->FindFolderByFrn(frn);
                    if (existing == INVALID_FOLDER_ID) {
                        m_index->AddFolder(parentId, name,
                                           static_cast<uint16_t>(rec->FileAttributes), frn);
                        changed = true;
                    } else {
                        m_index->RenameFolder(existing, parentId, name);
                        changed = true;
                    }
                } else {
                    uint32_t existing = m_index->FindFileByFrn(frn);
                    if (existing == INVALID_FILE_ID) {
                        m_index->AddFile(parentId, name, 0,
                                         static_cast<uint16_t>(rec->FileAttributes), 0, frn);
                        changed = true;
                    } else {
                        m_index->RenameFile(existing, parentId, name);
                        changed = true;
                    }
                }
            }
        }

        p += rec->RecordLength;
    }

    vol.nextUsn = nextUsn;
    CloseHandle(hVol);

    if (changed && m_onChanged) {
        m_onChanged();
    }
}

} // namespace EverythingNEO
