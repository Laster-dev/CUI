#include "EverythingApp.h"

#include "ShellContextMenu.h"
#include "framework/core/CUIDsl.h"
#include "framework/window/WindowBackdrop.h"
#include "framework/controls/WindowTitleBar.h"
#include "framework/controls/TextBox.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/ListView.h"
#include "framework/controls/ComboBox.h"
#include "framework/controls/ContextMenu.h"
#include "framework/controls/MessageBox.h"
#include "framework/style/ThemeManager.h"
#include "framework/style/ThemeTokenId.h"

#include <windows.h>
#include <shellapi.h>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <commctrl.h>
#include <cctype>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <shlobj.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

using namespace CUI;
using namespace CUI::DSL;

namespace EverythingNEO {

namespace {

constexpr UINT WM_ENEO_STATUS = WM_APP + 40;
constexpr UINT WM_ENEO_READY = WM_APP + 41;
constexpr UINT WM_ENEO_CHANGED = WM_APP + 42;
constexpr UINT WM_ENEO_ICONS_READY = WM_APP + 44;
constexpr UINT WM_ENEO_SEARCH_RESULTS = WM_APP + 45;
constexpr UINT_PTR kSubclassId = 0xE4E0;

struct SearchResultsMessage {
    std::vector<SearchResultRef> results;
    double search_ms = 0.0;
    double total_ms = 0.0;
    double sort_ms = 0.0;
    bool is_sort_message = false;
    uint64_t generation = 0;
};

std::string FormatBytes(uint64_t bytes) {
    double size = static_cast<double>(bytes);
    const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    int unitIdx = 0;
    while (size >= 1024.0 && unitIdx < 4) {
        size /= 1024.0;
        ++unitIdx;
    }
    std::ostringstream ss;
    if (unitIdx == 0) ss << static_cast<uint64_t>(size) << " " << units[unitIdx];
    else ss << std::fixed << std::setprecision(unitIdx >= 3 ? 2 : 1) << size << " " << units[unitIdx];
    return ss.str();
}

std::string FormatFileTime(uint64_t ft) {
    if (ft == 0) return {};
    FILETIME fileTime{};
    fileTime.dwLowDateTime = static_cast<DWORD>(ft & 0xFFFFFFFFULL);
    fileTime.dwHighDateTime = static_cast<DWORD>(ft >> 32);
    SYSTEMTIME utc{}, local{};
    if (!FileTimeToSystemTime(&fileTime, &utc)) return {};
    if (!SystemTimeToTzSpecificLocalTime(nullptr, &utc, &local)) local = utc;
    char buf[64];
    snprintf(buf, sizeof(buf), "%04d/%02d/%02d %02d:%02d",
             local.wYear, local.wMonth, local.wDay, local.wHour, local.wMinute);
    return buf;
}

std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (n <= 0) return {};
    std::wstring out(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n);
    return out;
}

// Explorer-style name compare (digits as numbers, locale-aware).
int CompareLogicalName(const std::string& a, const std::string& b) {
    return StrCmpLogicalW(Utf8ToWide(a).c_str(), Utf8ToWide(b).c_str());
}

std::string JoinPath(const std::string& folder, const std::string& name) {
    if (folder.empty()) return name;
    if (folder.back() == '\\' || folder.back() == '/') return folder + name;
    return folder + "\\" + name;
}

std::wstring GetFrequentFilesPath() {
    wchar_t appData[MAX_PATH]{};
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, appData))) {
        return L"frequent.txt";
    }
    std::wstring dir = appData;
    dir += L"\\EverythingNEO";
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir + L"\\frequent.txt";
}

} // namespace

// ---------------------------------------------------------------------------
// FileIconCache
// ---------------------------------------------------------------------------
FileIconCache::FileIconCache() {
    m_worker = std::thread([this]() { WorkerLoop(); });
}

FileIconCache::~FileIconCache() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop.store(true);
    }
    m_cv.notify_all();
    if (m_worker.joinable()) m_worker.join();
    Clear();
}

void FileIconCache::SetNotifyHwnd(HWND hwnd, UINT msg) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_notifyHwnd = hwnd;
    m_notifyMsg = msg;
}

void FileIconCache::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [_, icon] : m_byPath) {
        if (icon) DestroyIcon(icon);
    }
    m_byPath.clear();
    for (auto& [_, icon] : m_byExt) {
        if (icon && icon != m_defaultIcon) DestroyIcon(icon);
    }
    m_byExt.clear();
    m_pending.clear();
    m_queue.clear();
    if (m_folderIcon) {
        DestroyIcon(m_folderIcon);
        m_folderIcon = nullptr;
    }
    if (m_defaultIcon) {
        DestroyIcon(m_defaultIcon);
        m_defaultIcon = nullptr;
    }
}

HICON FileIconCache::GetFolderIconUnlocked() {
    if (m_folderIcon) return m_folderIcon;
    SHFILEINFOW sfi{};
    if (SHGetFileInfoW(L"folder", FILE_ATTRIBUTE_DIRECTORY, &sfi, sizeof(sfi),
                       SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES)) {
        m_folderIcon = sfi.hIcon;
    }
    return m_folderIcon;
}

std::string FileIconCache::ExtKey(const std::string& fileName) {
    size_t dot = fileName.find_last_of('.');
    if (dot == std::string::npos || dot + 1 >= fileName.size()) return ".";
    std::string ext = fileName.substr(dot);
    for (char& c : ext) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return ext;
}

HICON FileIconCache::GetExtIconUnlocked(const std::string& fileName) {
    std::string key = ExtKey(fileName);
    auto it = m_byExt.find(key);
    if (it != m_byExt.end()) return it->second;

    std::wstring probe = Utf8ToWide(key == "." ? std::string("file") : ("x" + key));
    SHFILEINFOW sfi{};
    DWORD_PTR ok = SHGetFileInfoW(probe.c_str(), FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi),
                                  SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
    HICON icon = ok ? sfi.hIcon : nullptr;
    if (!icon) {
        if (!m_defaultIcon) {
            SHFILEINFOW def{};
            if (SHGetFileInfoW(L"file", FILE_ATTRIBUTE_NORMAL, &def, sizeof(def),
                               SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES)) {
                m_defaultIcon = def.hIcon;
            }
        }
        icon = m_defaultIcon;
    }
    m_byExt[key] = icon;
    return icon;
}

HICON FileIconCache::GetIcon(const std::string& fullPath, const std::string& fileName, bool isFolder) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (isFolder) return GetFolderIconUnlocked();
    if (!fullPath.empty()) {
        auto it = m_byPath.find(fullPath);
        if (it != m_byPath.end()) return it->second;

        if (m_pending.find(fullPath) == m_pending.end()) {
            m_pending.insert(fullPath);
            m_queue.push_back(fullPath);
            m_cv.notify_one();
        }
    }
    return GetExtIconUnlocked(fileName);
}

void FileIconCache::WorkerLoop() {
    for (;;) {
        std::string path;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]() { return m_stop.load() || !m_queue.empty(); });
            if (m_stop.load() && m_queue.empty()) break;
            path = std::move(m_queue.front());
            m_queue.pop_front();
        }

        std::wstring wpath = Utf8ToWide(path);
        SHFILEINFOW sfi{};
        // Real path — extracts embedded .exe/.dll icons (no USEFILEATTRIBUTES).
        DWORD_PTR ok = SHGetFileInfoW(wpath.c_str(), 0, &sfi, sizeof(sfi),
                                      SHGFI_ICON | SHGFI_SMALLICON);

        HWND notifyHwnd = nullptr;
        UINT notifyMsg = 0;
        bool shouldPost = false;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_pending.erase(path);
            if (ok && sfi.hIcon) {
                auto old = m_byPath.find(path);
                if (old != m_byPath.end() && old->second) DestroyIcon(old->second);
                m_byPath[path] = sfi.hIcon;
            } else if (sfi.hIcon) {
                DestroyIcon(sfi.hIcon);
            }
            notifyHwnd = m_notifyHwnd;
            notifyMsg = m_notifyMsg;
            if (ok && sfi.hIcon && notifyHwnd && notifyMsg && !m_notifyPosted.exchange(true)) {
                shouldPost = true;
            }
        }
        if (shouldPost) {
            PostMessageW(notifyHwnd, notifyMsg, 0, 0);
        }
    }
}

// ---------------------------------------------------------------------------
// ResultsDataSource — per-file display cache (paint must not re-lock engine)
// ---------------------------------------------------------------------------
void ResultsDataSource::ClearCaches() {
    m_nameCache.clear();
    m_folderCache.clear();
    m_pathCache.clear();
}

std::string ResultsDataSource::GetName(const SearchResultRef& r) {
    uint64_t key = CacheKey(r);
    auto it = m_nameCache.find(key);
    if (it != m_nameCache.end()) return it->second;
    std::string name = engine ? engine->GetResultName(r) : std::string();
    m_nameCache.emplace(key, name);
    return name;
}

std::string ResultsDataSource::GetFolder(const SearchResultRef& r) {
    uint64_t key = CacheKey(r);
    auto it = m_folderCache.find(key);
    if (it != m_folderCache.end()) return it->second;
    std::string folder = engine ? engine->GetResultFolderPath(r) : std::string();
    m_folderCache.emplace(key, folder);
    return folder;
}

std::string ResultsDataSource::GetFullPath(const SearchResultRef& r) {
    uint64_t key = CacheKey(r);
    auto it = m_pathCache.find(key);
    if (it != m_pathCache.end()) return it->second;
    std::string path = engine ? engine->GetResultPath(r) : std::string();
    m_pathCache.emplace(key, path);
    return path;
}

std::string ResultsDataSource::BaseNameFromPath(const std::string& path) {
    size_t pos = path.find_last_of("\\/");
    if (pos == std::string::npos) return path;
    return path.substr(pos + 1);
}

std::string ResultsDataSource::FolderFromPath(const std::string& path) {
    size_t pos = path.find_last_of("\\/");
    if (pos == std::string::npos) return {};
    return path.substr(0, pos);
}

bool ResultsDataSource::QueryPathMeta(const std::string& path, uint64_t& size, uint64_t& date,
                                    bool& isFolder) {
    if (path.empty()) return false;
    std::wstring wpath = Utf8ToWide(path);
    WIN32_FILE_ATTRIBUTE_DATA fad{};
    if (!GetFileAttributesExW(wpath.c_str(), GetFileExInfoStandard, &fad)) return false;
    isFolder = (fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    ULARGE_INTEGER sz{};
    sz.LowPart = fad.nFileSizeLow;
    sz.HighPart = fad.nFileSizeHigh;
    size = isFolder ? 0 : sz.QuadPart;
    ULARGE_INTEGER dt{};
    dt.LowPart = fad.ftLastWriteTime.dwLowDateTime;
    dt.HighPart = fad.ftLastWriteTime.dwHighDateTime;
    date = dt.QuadPart;
    return true;
}

std::string ResultsDataSource::GetCellText(int row, int col) {
    if (displayMode && *displayMode == ListDisplayMode::FrequentFiles) {
        if (!frequentFiles || row < 0 || row >= static_cast<int>(frequentFiles->size())) return {};
        const FrequentFileEntry& entry = (*frequentFiles)[static_cast<size_t>(row)];
        if (col == 0) return BaseNameFromPath(entry.path);
        if (col == 1) return FolderFromPath(entry.path);
        if (col == 2) {
            uint64_t size = 0, date = 0;
            bool isFolder = entry.is_folder;
            QueryPathMeta(entry.path, size, date, isFolder);
            if (isFolder) return "<文件夹>";
            return FormatBytes(size);
        }
        if (col == 3) {
            uint64_t size = 0, date = 0;
            bool isFolder = entry.is_folder;
            if (QueryPathMeta(entry.path, size, date, isFolder)) return FormatFileTime(date);
        }
        return {};
    }
    if (!engine || !results) return {};
    if (row < 0 || row >= static_cast<int>(results->size())) return {};
    const SearchResultRef& r = (*results)[static_cast<size_t>(row)];
    switch (col) {
        case 0: return GetName(r);
        case 1: return r.is_folder ? engine->GetResultPath(r) : GetFolder(r);
        case 2: {
            if (r.is_folder) return "<文件夹>";
            engine->EnsureFileMeta(r.index);
            return FormatBytes(engine->GetResultSize(r));
        }
        case 3: {
            if (r.is_folder) {
                uint64_t size = 0, date = 0;
                bool isFolder = true;
                if (QueryPathMeta(engine->GetResultPath(r), size, date, isFolder)) {
                    return FormatFileTime(date);
                }
                return {};
            }
            engine->EnsureFileMeta(r.index);
            return FormatFileTime(engine->GetResultDateModified(r));
        }
        default: return {};
    }
}

HICON ResultsDataSource::GetRowIcon(int row) {
    if (!icons) return nullptr;
    if (displayMode && *displayMode == ListDisplayMode::FrequentFiles) {
        if (!frequentFiles || row < 0 || row >= static_cast<int>(frequentFiles->size())) return nullptr;
        const FrequentFileEntry& entry = (*frequentFiles)[static_cast<size_t>(row)];
        return icons->GetIcon(entry.path, BaseNameFromPath(entry.path), entry.is_folder);
    }
    if (!engine || !results) return nullptr;
    if (row < 0 || row >= static_cast<int>(results->size())) return nullptr;
    const SearchResultRef& r = (*results)[static_cast<size_t>(row)];
    return icons->GetIcon(GetFullPath(r), GetName(r), r.is_folder);
}

// ---------------------------------------------------------------------------
// EverythingApp
// ---------------------------------------------------------------------------
EverythingApp::~EverythingApp() {
    m_searchGeneration.fetch_add(1);
    m_engine.Stop();
    if (HWND hwnd = m_window.GetHWND()) {
        RemoveWindowSubclass(hwnd, WndSubclassProc, kSubclassId);
    }
}

LRESULT CALLBACK EverythingApp::WndSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                                UINT_PTR /*id*/, DWORD_PTR refData) {
    auto* self = reinterpret_cast<EverythingApp*>(refData);
    if (!self) return DefSubclassProc(hwnd, msg, wParam, lParam);

    if (msg == WM_ENEO_STATUS) {
        auto* str = reinterpret_cast<std::string*>(lParam);
        if (str) {
            if (self->m_statusLeft) self->m_statusLeft->SetText(*str);
            delete str;
        }
        return 0;
    }
    if (msg == WM_ENEO_ICONS_READY) {
        self->m_iconCache.ResetNotifyFlag();
        if (self->m_resultsList) self->m_resultsList->RefreshRows();
        return 0;
    }
    if (msg == WM_ENEO_SEARCH_RESULTS) {
        auto* payload = reinterpret_cast<SearchResultsMessage*>(lParam);
        if (payload) {
            self->ApplySearchResults(std::move(payload->results), payload->search_ms, payload->total_ms, payload->sort_ms, payload->is_sort_message, payload->generation);
            delete payload;
        }
        return 0;
    }
    if (msg == WM_ENEO_READY || msg == WM_ENEO_CHANGED) {
        self->OnEngineReady();
        return 0;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

int EverythingApp::Run() {
    if (!m_window.Create("Everything", 1100, 680, false)) {
        return -1;
    }
    m_window.SetThemeMode(ThemeMode::Light);
    m_window.SetBackdropType(BackdropType::None);
    m_window.SetRenderStatsOverlayVisible(false);

    m_dataSource.engine = &m_engine;
    m_dataSource.results = &m_results;
    m_dataSource.frequentFiles = &m_frequentFiles;
    m_dataSource.displayMode = &m_displayMode;
    m_dataSource.icons = &m_iconCache;

    LoadFrequentFiles();

    m_root = BuildRoot();
    m_window.SetRootElement(m_root);
    ApplyChromeColors();

    HWND hwnd = m_window.GetHWND();
    SetWindowSubclass(hwnd, WndSubclassProc, kSubclassId, reinterpret_cast<DWORD_PTR>(this));
    m_iconCache.SetNotifyHwnd(hwnd, WM_ENEO_ICONS_READY);

    m_engine.SetChangeNotify([this]() {
        HWND h = m_window.GetHWND();
        if (h) PostMessageW(h, WM_ENEO_CHANGED, 0, 0);
    });

    m_window.Show();

    m_engine.StartAsync(
        [this](const std::string& status) { OnEngineStatus(status); },
        [this]() {
            HWND h = m_window.GetHWND();
            if (h) PostMessageW(h, WM_ENEO_READY, 0, 0);
        });

    m_window.RunMessageLoop();
    m_searchGeneration.fetch_add(1);
    m_engine.Stop();
    return 0;
}

std::shared_ptr<UIElement> EverythingApp::BuildRoot() {
    auto root = Column(0).BackgroundToken(ThemeTokenId::WindowBackground).Build();
    root->SetColorToken(ThemeTokenId::TextPrimary);

    m_titleBar = std::make_shared<WindowTitleBar>();
    m_titleBar->SetTitle("Everything");
    BuildMenus();

    auto searchRow = Row(0).Build();
    searchRow->SetHeight(40.0f);
    searchRow->SetPadding(Thickness(6, 4, 6, 4));
    searchRow->SetBackgroundToken(ThemeTokenId::PaneBackground);
    searchRow->SetBackground(ThemeManager::Instance().GetColor(ThemeTokenId::PaneBackground));

    m_searchBox = std::make_shared<TextBox>();
    m_searchBox->SetPlaceholder("");
    m_searchBox->SetHeight(32.0f);
    m_searchBox->SetFlexGrow(1.0f);
    m_searchBox->SetFontFamily("微软雅黑");
    m_searchBox->SetFontSize(14.0f);
    m_searchBox->SetPadding(Thickness(8, 2, 8, 2));
    m_searchBox->OnTextChanged().Connect([this](TextBox*, const std::string& query) {
        QueueSearch(query);
    });
    searchRow->AddChild(m_searchBox);

    m_typeFilter = std::make_shared<ComboBox>();
    m_typeFilter->SetWidth(150.0f);
    m_typeFilter->SetHeight(32.0f);
    m_typeFilter->SetMargin(Thickness(6, 0, 0, 0));
    m_typeFilter->SetFontFamily("微软雅黑");
    m_typeFilter->SetFontSize(13.0f);
    m_typeFilter->AddItem("文件+文件夹");
    m_typeFilter->AddItem("文件");
    m_typeFilter->AddItem("文件夹");
    m_typeFilter->SetSelectedIndex(0);
    m_typeFilter->OnSelectionChanged().Connect([this](ComboBox*, int index, const std::string&) {
        switch (index) {
            case 1: m_searchOptions.result_kind = SearchResultKind::FilesOnly; break;
            case 2: m_searchOptions.result_kind = SearchResultKind::FoldersOnly; break;
            default: m_searchOptions.result_kind = SearchResultKind::FilesAndFolders; break;
        }
        if (!m_lastQuery.empty()) QueueSearch(m_lastQuery);
    });
    searchRow->AddChild(m_typeFilter);

    m_resultsList = std::make_shared<ListView>();
    m_resultsList->SetWidth(-1.0f);
    m_resultsList->SetHeight(-1.0f);
    m_resultsList->SetFlexGrow(1.0f);
    m_resultsList->AddColumn("名称", 280.0f);
    m_resultsList->AddColumn("路径", 420.0f);
    m_resultsList->AddColumn("大小", 100.0f);
    m_resultsList->AddColumn("修改日期", 140.0f);
    m_resultsList->SetRowHeight(24.0f);
    m_resultsList->SetShowGridLines(false);
    m_resultsList->SetFontFamily("微软雅黑");
    m_resultsList->SetFontSize(13.0f);
    m_resultsList->SetBackground(ThemeManager::Instance().GetColor(ThemeTokenId::WindowBackground));
    m_resultsList->SetVirtualMode(0, &m_dataSource);
    m_resultsList->OnRowDoubleClicked().Connect([this](ListView*, int row) {
        if (m_displayMode == ListDisplayMode::FrequentFiles) {
            if (row >= 0 && row < static_cast<int>(m_frequentFiles.size())) {
                const FrequentFileEntry& entry = m_frequentFiles[static_cast<size_t>(row)];
                RecordFileAccessByPath(entry.path, entry.is_folder);
                std::wstring wpath = Utf8ToWide(entry.path);
                ShellExecuteW(nullptr, L"open", wpath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            }
            return;
        }
        OpenSelected();
    });
    m_resultsList->OnColumnHeaderClicked().Connect([this](ListView*, int column, bool ascending) {
        SortResults(column, ascending);
    });
    m_resultsList->SetShellContextMenuHandler([this](Point pt, const std::vector<int>& rows) {
        std::vector<std::wstring> paths;
        paths.reserve(rows.size());
        if (m_displayMode == ListDisplayMode::FrequentFiles) {
            for (int row : rows) {
                if (row < 0 || row >= static_cast<int>(m_frequentFiles.size())) continue;
                paths.push_back(Utf8ToWide(m_frequentFiles[static_cast<size_t>(row)].path));
            }
        } else {
            for (int row : rows) {
                if (row < 0 || row >= static_cast<int>(m_results.size())) continue;
                paths.push_back(Utf8ToWide(m_engine.GetResultPath(m_results[static_cast<size_t>(row)])));
            }
        }
        if (paths.empty()) return false;

        ShellMenuActions actions;
        actions.showOpenPath = true;
        actions.showCopyFullPath = true;
        actions.showRename = (rows.size() == 1);
        actions.openPath = [this]() { OpenSelectedPath(); };
        actions.copyFullPath = [this]() { CopyFullPath(); };
        actions.rename = [this]() { RenameSelected(); };

        m_fileContextMenu = BuildShellContextMenu(m_window.GetHWND(), paths, actions);
        if (!m_fileContextMenu) return false;
        m_resultsList->SetContextMenu(m_fileContextMenu);
        m_fileContextMenu->ShowAt(pt.x, pt.y);
        return true;
    });

    m_statusBar = Row(0).Build();
    m_statusBar->SetHeight(26.0f);
    m_statusBar->SetPadding(Thickness(10, 2, 10, 2));
    m_statusBar->SetBackgroundToken(ThemeTokenId::PaneBackground);
    m_statusBar->SetBackground(ThemeManager::Instance().GetColor(ThemeTokenId::PaneBackground));

    m_statusLeft = Text("正在初始化...").FontSize(12.0f).FontFamily("微软雅黑").Build();
    m_statusLeft->SetColorToken(ThemeTokenId::TextSecondary);
    m_statusLeft->SetFlexGrow(1.0f);
    m_statusLeft->SetTextAlign("Left");

    m_statusRight = Text("").FontSize(12.0f).FontFamily("微软雅黑").Build();
    m_statusRight->SetColorToken(ThemeTokenId::TextSecondary);
    m_statusRight->SetTextAlign("Right");

    m_statusBar->AddChild(m_statusLeft);
    m_statusBar->AddChild(m_statusRight);

    root->AddChild(m_titleBar);
    root->AddChild(searchRow);
    root->AddChild(m_resultsList);
    root->AddChild(m_statusBar);
    return root;
}

void EverythingApp::BuildMenus() {
    if (!m_titleBar) return;
    MenuBar& menuBar = m_titleBar->GetMenuBar();
    menuBar.ClearMenus();

    auto fileMenu = menuBar.AddMenu("文件(F)");
    fileMenu->AddItem("首页(H)", [this]() {
        if (m_searchBox) {
            m_searchBox->SetText("");
            QueueSearch("");
        }
    });
    fileMenu->AddSeparator();
    fileMenu->AddItem("退出(X)", [this]() {
        if (HWND hwnd = m_window.GetHWND()) PostMessage(hwnd, WM_CLOSE, 0, 0);
    });

    auto editMenu = menuBar.AddMenu("编辑(E)");
    editMenu->AddItem("复制(C)", [this]() { CopyFullPath(); });
    editMenu->AddSeparator();
    editMenu->AddItem("全选(A)", [this]() {
        if (m_resultsList) m_resultsList->SelectAll();
    });

    auto searchMenu = menuBar.AddMenu("搜索(S)");
    m_menuRegex = searchMenu->AddItem("启用正则表达式(R)", [this]() {
        ToggleSearchOption(&SearchOptions::use_regex, "regex");
    });
    m_menuMatchPath = searchMenu->AddItem("匹配路径(P)", [this]() {
        ToggleSearchOption(&SearchOptions::match_path, "path");
    });
    m_menuWholeWord = searchMenu->AddItem("匹配全字(W)", [this]() {
        ToggleSearchOption(&SearchOptions::match_whole_word, "word");
    });
    m_menuMatchCase = searchMenu->AddItem("匹配大小写(C)", [this]() {
        ToggleSearchOption(&SearchOptions::match_case, "case");
    });
    RefreshSearchMenuChecks();

    auto viewMenu = menuBar.AddMenu("查看(V)");
    viewMenu->AddItem("刷新(R)", [this]() {
        QueueSearch(m_lastQuery);
        RefreshStatusBar();
    });
    m_menuStatusBar = viewMenu->AddItem("状态栏(S)", [this]() {
        m_statusBarVisible = !m_statusBarVisible;
        if (m_statusBar) {
            m_statusBar->SetVisibility(m_statusBarVisible ? Visibility::Visible : Visibility::Collapsed);
        }
        RefreshSearchMenuChecks();
    });
    m_menuStatusBar->SetChecked(m_statusBarVisible);
    viewMenu->AddItem("切换主题(T)", [this]() { ToggleTheme(); });

    auto toolsMenu = menuBar.AddMenu("工具(T)");
    toolsMenu->AddItem("重建索引(O)...", [this]() {
        ContentDialog::ShowMessageBox(
            m_root.get(),
            "重建索引",
            "重建索引可能需要几分钟，期间仍可操作界面。\n\n确定要继续吗？",
            [this](DialogResult result) {
                if (result != DialogResult::Primary) return;
                if (m_statusLeft) m_statusLeft->SetText("正在重建索引...");
                m_engine.RebuildIndexAsync(
                    [this](const std::string& s) { OnEngineStatus(s); },
                    [this]() {
                        HWND hwnd = m_window.GetHWND();
                        if (hwnd) PostMessageW(hwnd, WM_ENEO_READY, 0, 0);
                    });
            });
    });
    toolsMenu->AddItem("保存数据库(S)", [this]() {
        bool ok = m_engine.SaveDatabase();
        if (m_statusLeft) {
            m_statusLeft->SetText(ok ? "Everything.db 已保存" : "保存失败（可能需要管理员权限）");
        }
    });

    auto helpMenu = menuBar.AddMenu("帮助(H)");
    helpMenu->AddItem("关于 Everything NEO", [this]() {
        EngineStats s = m_engine.GetStats();
        std::ostringstream oss;
        oss << "Everything NEO\n\n"
            << "纯内存二进制索引引擎（非 SQLite）\n"
            << "索引文件: " << s.live_file_count << "\n"
            << "文件夹: " << s.folder_count << "\n"
            << "内存占用: " << FormatBytes(s.total_memory_bytes) << "\n"
            << "权限: " << (s.elevated ? "已提升 (USN/MFT)" : "标准用户 (FindFirstFile)") << "\n"
            << "USN 监视: " << (s.usn_active ? "开启" : "关闭");
        ContentDialog::ShowMessageBox(m_root.get(), "关于 Everything NEO", oss.str());
    });
}

void EverythingApp::ToggleSearchOption(bool SearchOptions::* flag, const char* /*label*/) {
    bool current = m_searchOptions.*flag;
    m_searchOptions.*flag = !current;
    RefreshSearchMenuChecks();
    if (!m_lastQuery.empty()) QueueSearch(m_lastQuery);
}

void EverythingApp::RefreshSearchMenuChecks() {
    if (m_menuRegex) m_menuRegex->SetChecked(m_searchOptions.use_regex);
    if (m_menuMatchPath) m_menuMatchPath->SetChecked(m_searchOptions.match_path);
    if (m_menuWholeWord) m_menuWholeWord->SetChecked(m_searchOptions.match_whole_word);
    if (m_menuMatchCase) m_menuMatchCase->SetChecked(m_searchOptions.match_case);
    if (m_menuStatusBar) m_menuStatusBar->SetChecked(m_statusBarVisible);
}

void EverythingApp::SortResults(int column, bool ascending) {
    m_sortColumn = column;
    m_sortAscending = ascending;

    auto cmpLogical = [ascending](const std::string& a, const std::string& b) {
        const int cmp = CompareLogicalName(a, b);
        return ascending ? cmp < 0 : cmp > 0;
    };

    if (m_displayMode == ListDisplayMode::FrequentFiles) {
        std::sort(m_frequentFiles.begin(), m_frequentFiles.end(),
                  [&](const FrequentFileEntry& fa, const FrequentFileEntry& fb) {
                      // Folders always before files (Everything-style).
                      if (fa.is_folder != fb.is_folder) return fa.is_folder && !fb.is_folder;
                      switch (column) {
                          case 0: return cmpLogical(ResultsDataSource::BaseNameFromPath(fa.path),
                                                    ResultsDataSource::BaseNameFromPath(fb.path));
                          case 1: return cmpLogical(ResultsDataSource::FolderFromPath(fa.path),
                                                    ResultsDataSource::FolderFromPath(fb.path));
                          case 2: {
                              uint64_t sa = 0, da = 0, sb = 0, db = 0;
                              bool ia = fa.is_folder, ib = fb.is_folder;
                              ResultsDataSource::QueryPathMeta(fa.path, sa, da, ia);
                              ResultsDataSource::QueryPathMeta(fb.path, sb, db, ib);
                              return ascending ? sa < sb : sa > sb;
                          }
                          case 3: {
                              uint64_t sa = 0, da = 0, sb = 0, db = 0;
                              bool ia = fa.is_folder, ib = fb.is_folder;
                              ResultsDataSource::QueryPathMeta(fa.path, sa, da, ia);
                              ResultsDataSource::QueryPathMeta(fb.path, sb, db, ib);
                              return ascending ? da < db : da > db;
                          }
                          default: return false;
                      }
                  });
        m_dataSource.ClearCaches();
        if (m_resultsList) {
            m_resultsList->RefreshRows();
        }
        return;
    }

    // Sort search results asynchronously in a background thread — never block UI.
    auto resultsCopy = m_results;  // copy so UI stays responsive
    const uint64_t generation = m_searchGeneration.load();

    std::thread([this, results = std::move(resultsCopy), column, ascending, generation]() mutable {
        const auto t0 = std::chrono::high_resolution_clock::now();
        m_engine.SortSearchResults(results, column, ascending);
        const auto t1 = std::chrono::high_resolution_clock::now();
        const double sort_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        if (generation != m_searchGeneration.load()) return;  // search changed, discard

        HWND hwnd = m_window.GetHWND();
        if (!hwnd) return;
        auto* payload = new SearchResultsMessage{ std::move(results), 0.0, sort_ms, sort_ms, true, generation };
        if (!PostMessageW(hwnd, WM_ENEO_SEARCH_RESULTS, 0, reinterpret_cast<LPARAM>(payload))) {
            delete payload;
        }
    }).detach();
}

void EverythingApp::QueueSearch(const std::string& query) {
    m_lastQuery = query;
    const uint64_t generation = ++m_searchGeneration;

    if (query.empty()) {
        ShowFrequentFiles();
        return;
    }

    const SearchOptions opts = m_searchOptions;
    const auto t_input = std::chrono::high_resolution_clock::now();

    std::thread([this, query, opts, generation, t_input]() {
        const auto t0 = std::chrono::high_resolution_clock::now();
        std::vector<SearchResultRef> results;
        // Pass generation counter so search aborts early if user types another char
        m_engine.Search(query, opts, results, 0, &m_searchGeneration, generation);
        const auto t1 = std::chrono::high_resolution_clock::now();
        const double search_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        const double total_ms = std::chrono::duration<double, std::milli>(t1 - t_input).count();
        if (generation != m_searchGeneration.load()) return;

        HWND hwnd = m_window.GetHWND();
        if (!hwnd) return;

        auto* payload = new SearchResultsMessage{ std::move(results), search_ms, total_ms, 0.0, false, generation };
        if (!PostMessageW(hwnd, WM_ENEO_SEARCH_RESULTS, 0, reinterpret_cast<LPARAM>(payload))) {
            delete payload;
        }
    }).detach();
}

void EverythingApp::ShowFrequentFiles() {
    m_displayMode = ListDisplayMode::FrequentFiles;
    m_dataSource.ClearCaches();
    if (m_resultsList) {
        if (m_resultsList->IsVirtualMode()) {
            m_resultsList->SetVirtualRowCount(static_cast<int>(m_frequentFiles.size()));
        } else {
            m_resultsList->SetVirtualMode(static_cast<int>(m_frequentFiles.size()), &m_dataSource);
        }
    }
    if (m_statusLeft) {
        m_statusLeft->SetText(m_frequentFiles.empty() ? "就绪" : "常用文件");
    }
}

void EverythingApp::RecordFileAccess(const SearchResultRef& ref) {
    RecordFileAccessByPath(m_engine.GetResultPath(ref), ref.is_folder);
}

void EverythingApp::RecordFileAccessByPath(const std::string& path, bool isFolder) {
    if (path.empty()) return;
    auto it = std::find_if(m_frequentFiles.begin(), m_frequentFiles.end(),
                           [&](const FrequentFileEntry& e) { return _stricmp(e.path.c_str(), path.c_str()) == 0; });
    if (it != m_frequentFiles.end()) {
        it->use_count++;
        it->is_folder = isFolder;
        if (it != m_frequentFiles.begin()) {
            FrequentFileEntry entry = *it;
            m_frequentFiles.erase(it);
            m_frequentFiles.insert(m_frequentFiles.begin(), entry);
        }
    } else {
        FrequentFileEntry entry{};
        entry.path = path;
        entry.is_folder = isFolder;
        entry.use_count = 1;
        m_frequentFiles.insert(m_frequentFiles.begin(), entry);
    }
    if (m_frequentFiles.size() > 30) m_frequentFiles.resize(30);
    SaveFrequentFiles();
    if (m_displayMode == ListDisplayMode::FrequentFiles && m_resultsList) {
        m_resultsList->SetVirtualRowCount(static_cast<int>(m_frequentFiles.size()));
        m_resultsList->RefreshRows();
    }
}

void EverythingApp::LoadFrequentFiles() {
    m_frequentFiles.clear();
    const std::wstring storePath = GetFrequentFilesPath();
    std::ifstream in{std::filesystem::path(storePath)};
    if (!in) return;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        size_t t1 = line.find('\t');
        if (t1 == std::string::npos) continue;
        size_t t2 = line.find('\t', t1 + 1);
        if (t2 == std::string::npos) continue;
        FrequentFileEntry entry{};
        entry.path = line.substr(0, t1);
        entry.is_folder = (line[t1 + 1] == '1');
        entry.use_count = static_cast<uint32_t>(std::stoul(line.substr(t2 + 1)));
        if (!entry.path.empty()) m_frequentFiles.push_back(entry);
    }
}

void EverythingApp::SaveFrequentFiles() {
    const std::wstring storePath = GetFrequentFilesPath();
    std::ofstream out(std::filesystem::path(storePath), std::ios::trunc);
    if (!out) return;
    for (const auto& entry : m_frequentFiles) {
        out << entry.path << '\t' << (entry.is_folder ? '1' : '0') << '\t' << entry.use_count << '\n';
    }
}

void EverythingApp::ApplySearchResults(std::vector<SearchResultRef>&& results, double searchMs,
                                       double totalMs, double sortMs, bool isSortMessage, uint64_t generation) {
    if (generation != m_searchGeneration.load()) return;

    const auto t_ui0 = std::chrono::high_resolution_clock::now();

    m_displayMode = ListDisplayMode::SearchResults;
    m_dataSource.ClearCaches();
    m_results = std::move(results);
    if (m_resultsList) {
        if (m_resultsList->IsVirtualMode()) {
            m_resultsList->SetVirtualRowCount(static_cast<int>(m_results.size()));
        } else {
            m_resultsList->SetVirtualMode(static_cast<int>(m_results.size()), &m_dataSource);
        }
        m_resultsList->RefreshRows();
    }

    const auto t_ui1 = std::chrono::high_resolution_clock::now();
    const double ui_ms = std::chrono::duration<double, std::milli>(t_ui1 - t_ui0).count();

    if (!m_lastQuery.empty() && m_statusLeft) {
        std::ostringstream ss;
        ss << m_results.size() << " 个对象";
        if (isSortMessage) {
            ss << " | [排序]: " << std::fixed << std::setprecision(1) << sortMs << " ms"
               << " | [UI刷新]: " << std::setprecision(1) << ui_ms << " ms";
        } else {
            const double dispatch_ms = (totalMs > searchMs) ? (totalMs - searchMs) : 0.0;
            ss << " | [检索]: " << std::fixed << std::setprecision(1) << searchMs << " ms"
               << " | [调度]: " << std::setprecision(1) << dispatch_ms << " ms"
               << " | [UI渲染]: " << std::setprecision(1) << ui_ms << " ms"
               << " | [总计]: " << std::setprecision(1) << (totalMs + ui_ms) << " ms";
        }
        m_statusLeft->SetText(ss.str());
    }

    // Force immediate Direct2D frame rendering & DWM Present (bypass WM_PAINT queue delay)
    if (HWND hwnd = m_window.GetHWND()) {
        UpdateWindow(hwnd);
    }
}

void EverythingApp::RefreshStatusBar() {
    EngineStats s = m_engine.GetStats();
    std::ostringstream ss;
    ss << "已索引 " << s.live_file_count << " 个文件";
    if (s.folder_count) ss << ", " << s.folder_count << " 个文件夹";
    if (s.total_memory_bytes) ss << "  |  " << FormatBytes(s.total_memory_bytes);
    if (s.loaded_from_db) ss << "  |  DB";
    if (s.usn_active) ss << "  |  USN";
    if (m_statusRight) m_statusRight->SetText(ss.str());
}

void EverythingApp::OnEngineReady() {
    RefreshStatusBar();
    if (!m_lastQuery.empty()) {
        QueueSearch(m_lastQuery);
    } else {
        ShowFrequentFiles();
        if (m_statusLeft && m_engine.IsReady() && m_frequentFiles.empty()) {
            EngineStats s = m_engine.GetStats();
            std::ostringstream ss;
            ss << "就绪 — 已索引 " << s.live_file_count << " 个文件";
            if (s.elevated) ss << " [USN/MFT]";
            else ss << " [标准扫描]";
            m_statusLeft->SetText(ss.str());
        }
    }
}

void EverythingApp::OnEngineStatus(const std::string& status) {
    HWND hwnd = m_window.GetHWND();
    if (!hwnd) return;
    auto* heapStr = new std::string(status);
    if (!PostMessageW(hwnd, WM_ENEO_STATUS, 0, reinterpret_cast<LPARAM>(heapStr))) {
        delete heapStr;
    }
}

std::vector<int> EverythingApp::SelectedRows() const {
    std::vector<int> rows;
    if (!m_resultsList) return rows;
    const auto& sel = m_resultsList->GetSelectedIndices();
    if (!sel.empty()) {
        rows.assign(sel.begin(), sel.end());
        return rows;
    }
    int row = m_resultsList->GetCaretIndex();
    if (row >= 0) rows.push_back(row);
    return rows;
}

void EverythingApp::OpenSelected() {
    if (m_displayMode == ListDisplayMode::FrequentFiles) {
        for (int row : SelectedRows()) {
            if (row < 0 || row >= static_cast<int>(m_frequentFiles.size())) continue;
            const FrequentFileEntry& entry = m_frequentFiles[static_cast<size_t>(row)];
            RecordFileAccessByPath(entry.path, entry.is_folder);
            std::wstring wpath = Utf8ToWide(entry.path);
            ShellExecuteW(nullptr, L"open", wpath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
        return;
    }
    for (int row : SelectedRows()) {
        if (row < 0 || row >= static_cast<int>(m_results.size())) continue;
        const SearchResultRef& ref = m_results[static_cast<size_t>(row)];
        RecordFileAccess(ref);
        std::wstring wpath = Utf8ToWide(m_engine.GetResultPath(ref));
        ShellExecuteW(nullptr, L"open", wpath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
}

void EverythingApp::OpenSelectedPath() {
    if (m_displayMode == ListDisplayMode::FrequentFiles) {
        for (int row : SelectedRows()) {
            if (row < 0 || row >= static_cast<int>(m_frequentFiles.size())) continue;
            const FrequentFileEntry& entry = m_frequentFiles[static_cast<size_t>(row)];
            std::wstring wpath = Utf8ToWide(entry.path);
            std::wstring params = L"/select,\"" + wpath + L"\"";
            ShellExecuteW(nullptr, L"open", L"explorer.exe", params.c_str(), nullptr, SW_SHOWNORMAL);
        }
        return;
    }
    for (int row : SelectedRows()) {
        if (row < 0 || row >= static_cast<int>(m_results.size())) continue;
        const SearchResultRef& ref = m_results[static_cast<size_t>(row)];
        RecordFileAccess(ref);
        std::wstring wpath = Utf8ToWide(m_engine.GetResultPath(ref));
        std::wstring params = L"/select,\"" + wpath + L"\"";
        ShellExecuteW(nullptr, L"open", L"explorer.exe", params.c_str(), nullptr, SW_SHOWNORMAL);
    }
}

void EverythingApp::RenameSelected() {
    auto rows = SelectedRows();
    if (rows.size() != 1) return;
    const int row = rows[0];

    std::string fullPath;
    bool isFolder = false;
    if (m_displayMode == ListDisplayMode::FrequentFiles) {
        if (row < 0 || row >= static_cast<int>(m_frequentFiles.size())) return;
        fullPath = m_frequentFiles[static_cast<size_t>(row)].path;
        isFolder = m_frequentFiles[static_cast<size_t>(row)].is_folder;
    } else {
        if (row < 0 || row >= static_cast<int>(m_results.size())) return;
        const SearchResultRef& ref = m_results[static_cast<size_t>(row)];
        fullPath = m_engine.GetResultPath(ref);
        isFolder = ref.is_folder;
    }
    if (fullPath.empty()) return;

    std::string baseName = ResultsDataSource::BaseNameFromPath(fullPath);
    std::string folder = ResultsDataSource::FolderFromPath(fullPath);

    ContentDialog::ShowInputBox(
        m_root.get(),
        "重命名",
        isFolder ? "输入新文件夹名：" : "输入新文件名：",
        baseName,
        false,
        [this, fullPath, folder, baseName](DialogResult result, const std::string& newName) {
            if (result != DialogResult::Primary) return;
            std::string trimmed = newName;
            while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t')) trimmed.pop_back();
            while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t')) trimmed.erase(trimmed.begin());
            if (trimmed.empty() || trimmed == baseName) return;
            if (trimmed.find_first_of("\\/:*?\"<>|") != std::string::npos) {
                ContentDialog::ShowMessageBox(m_root.get(), "重命名", "名称包含非法字符。");
                return;
            }

            const std::wstring src = Utf8ToWide(fullPath);
            const std::wstring dst = Utf8ToWide(folder.empty() ? trimmed : (folder + "\\" + trimmed));
            if (!MoveFileW(src.c_str(), dst.c_str())) {
                ContentDialog::ShowMessageBox(m_root.get(), "重命名", "重命名失败（文件可能被占用或权限不足）。");
                return;
            }

            // Update frequent-list entry if present.
            for (auto& entry : m_frequentFiles) {
                if (_stricmp(entry.path.c_str(), fullPath.c_str()) == 0) {
                    entry.path = folder.empty() ? trimmed : (folder + "\\" + trimmed);
                    break;
                }
            }
            SaveFrequentFiles();

            if (!m_lastQuery.empty()) {
                QueueSearch(m_lastQuery);
            } else {
                ShowFrequentFiles();
            }
        });
}

void EverythingApp::CopyFullPath() {
    if (m_displayMode == ListDisplayMode::FrequentFiles) {
        std::wstring combined;
        for (int row : SelectedRows()) {
            if (row < 0 || row >= static_cast<int>(m_frequentFiles.size())) continue;
            if (!combined.empty()) combined += L"\r\n";
            combined += Utf8ToWide(m_frequentFiles[static_cast<size_t>(row)].path);
        }
        if (combined.empty()) return;
        if (!OpenClipboard(m_window.GetHWND())) return;
        EmptyClipboard();
        size_t bytes = (combined.size() + 1) * sizeof(wchar_t);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (hMem) {
            void* ptr = GlobalLock(hMem);
            if (ptr) {
                memcpy(ptr, combined.c_str(), bytes);
                GlobalUnlock(hMem);
                SetClipboardData(CF_UNICODETEXT, hMem);
            }
        }
        CloseClipboard();
        return;
    }
    std::wstring combined;
    for (int row : SelectedRows()) {
        if (row < 0 || row >= static_cast<int>(m_results.size())) continue;
        if (!combined.empty()) combined += L"\r\n";
        combined += Utf8ToWide(m_engine.GetResultPath(m_results[static_cast<size_t>(row)]));
    }
    if (combined.empty()) return;
    if (!OpenClipboard(m_window.GetHWND())) return;
    EmptyClipboard();
    size_t bytes = (combined.size() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (hMem) {
        void* ptr = GlobalLock(hMem);
        if (ptr) {
            memcpy(ptr, combined.c_str(), bytes);
            GlobalUnlock(hMem);
            SetClipboardData(CF_UNICODETEXT, hMem);
        }
    }
    CloseClipboard();
}

void EverythingApp::ToggleTheme() {
    ThemeMode mode = ThemeManager::Instance().GetThemeMode();
    m_window.SetThemeMode(mode == ThemeMode::Dark ? ThemeMode::Light : ThemeMode::Dark);
    ApplyChromeColors();
}

void EverythingApp::ApplyChromeColors() {
    auto& tm = ThemeManager::Instance();
    if (m_resultsList) {
        m_resultsList->SetBackground(tm.GetColor(ThemeTokenId::WindowBackground));
    }
    if (m_statusBar) {
        m_statusBar->SetBackground(tm.GetColor(ThemeTokenId::PaneBackground));
    }
}

} // namespace EverythingNEO
