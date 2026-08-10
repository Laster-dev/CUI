#include "EverythingApp.h"

#include "framework/core/CUIDsl.h"
#include "framework/window/WindowBackdrop.h"
#include "framework/controls/WindowTitleBar.h"
#include "framework/controls/TextBox.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/ListView.h"
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

using namespace CUI;
using namespace CUI::DSL;

namespace EverythingNEO {

namespace {

constexpr UINT WM_ENEO_STATUS = WM_APP + 40;
constexpr UINT WM_ENEO_READY = WM_APP + 41;
constexpr UINT WM_ENEO_CHANGED = WM_APP + 42;
constexpr UINT WM_ENEO_ICONS_READY = WM_APP + 44;
constexpr UINT_PTR kSubclassId = 0xE4E0;

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

std::string JoinPath(const std::string& folder, const std::string& name) {
    if (folder.empty()) return name;
    if (folder.back() == '\\' || folder.back() == '/') return folder + name;
    return folder + "\\" + name;
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
    if (m_defaultIcon) {
        DestroyIcon(m_defaultIcon);
        m_defaultIcon = nullptr;
    }
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

HICON FileIconCache::GetIcon(const std::string& fullPath, const std::string& fileName) {
    std::lock_guard<std::mutex> lock(m_mutex);
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

std::string ResultsDataSource::GetName(uint32_t fileIndex) {
    auto it = m_nameCache.find(fileIndex);
    if (it != m_nameCache.end()) return it->second;
    std::string name = engine ? engine->GetFileName(fileIndex) : std::string();
    m_nameCache.emplace(fileIndex, name);
    return name;
}

std::string ResultsDataSource::GetFolder(uint32_t fileIndex) {
    auto it = m_folderCache.find(fileIndex);
    if (it != m_folderCache.end()) return it->second;
    std::string folder = engine ? engine->GetFileFolderPath(fileIndex) : std::string();
    m_folderCache.emplace(fileIndex, folder);
    return folder;
}

std::string ResultsDataSource::GetFullPath(uint32_t fileIndex) {
    auto it = m_pathCache.find(fileIndex);
    if (it != m_pathCache.end()) return it->second;
    std::string path = JoinPath(GetFolder(fileIndex), GetName(fileIndex));
    m_pathCache.emplace(fileIndex, path);
    return path;
}

std::string ResultsDataSource::GetCellText(int row, int col) {
    if (!engine || !results) return {};
    if (row < 0 || row >= static_cast<int>(results->size())) return {};
    uint32_t idx = (*results)[static_cast<size_t>(row)];
    switch (col) {
        case 0: return GetName(idx);
        case 1: return GetFolder(idx);
        case 2: {
            uint64_t size = engine->GetFileSize(idx);
            if (size == 0) return {};
            return FormatBytes(size);
        }
        case 3: return FormatFileTime(engine->GetFileDateModified(idx));
        default: return {};
    }
}

HICON ResultsDataSource::GetRowIcon(int row) {
    if (!engine || !results || !icons) return nullptr;
    if (row < 0 || row >= static_cast<int>(results->size())) return nullptr;
    uint32_t idx = (*results)[static_cast<size_t>(row)];
    return icons->GetIcon(GetFullPath(idx), GetName(idx));
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
    m_dataSource.icons = &m_iconCache;

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

    m_resultsList = std::make_shared<ListView>();
    m_resultsList->SetWidth(-1.0f);
    m_resultsList->SetHeight(-1.0f);
    m_resultsList->SetFlexGrow(1.0f);
    m_resultsList->AddColumn("名称", 280.0f);
    m_resultsList->AddColumn("路径", 420.0f);
    m_resultsList->AddColumn("大小", 100.0f);
    m_resultsList->AddColumn("修改日期", 140.0f);
    m_resultsList->SetRowHeight(24.0f);
    m_resultsList->SetFontFamily("微软雅黑");
    m_resultsList->SetFontSize(13.0f);
    m_resultsList->SetBackground(ThemeManager::Instance().GetColor(ThemeTokenId::WindowBackground));
    m_resultsList->SetVirtualMode(0, &m_dataSource);
    m_resultsList->OnRowDoubleClicked().Connect([this](ListView*, int) { OpenSelected(); });

    auto listMenu = std::make_shared<ContextMenu>();
    listMenu->AddItem("打开(O)", [this]() { OpenSelected(); });
    listMenu->AddItem("打开路径(P)", [this]() { OpenSelectedPath(); });
    listMenu->AddSeparator();
    listMenu->AddItem("复制完整路径(C)", [this]() { CopyFullPath(); });
    m_resultsList->SetContextMenu(listMenu);

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
    searchMenu->AddItem("启用正则表达式(R)", []() {});
    searchMenu->AddItem("匹配路径(P)", []() {});

    auto viewMenu = menuBar.AddMenu("查看(V)");
    viewMenu->AddItem("刷新(R)", [this]() {
        QueueSearch(m_lastQuery);
        RefreshStatusBar();
    });
    viewMenu->AddItem("状态栏(S)", [this]() {
        m_statusBarVisible = !m_statusBarVisible;
        if (m_statusBar) {
            m_statusBar->SetVisibility(m_statusBarVisible ? Visibility::Visible : Visibility::Collapsed);
        }
    });
    viewMenu->AddItem("切换主题(T)", [this]() { ToggleTheme(); });

    auto toolsMenu = menuBar.AddMenu("工具(T)");
    toolsMenu->AddItem("重建索引(O)...", [this]() {
        if (m_statusLeft) m_statusLeft->SetText("正在重建索引...");
        m_engine.RebuildIndexAsync(
            [this](const std::string& s) { OnEngineStatus(s); },
            [this]() {
                HWND hwnd = m_window.GetHWND();
                if (hwnd) PostMessageW(hwnd, WM_ENEO_READY, 0, 0);
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

void EverythingApp::QueueSearch(const std::string& query) {
    m_lastQuery = query;
    m_searchGeneration.fetch_add(1);

    if (query.empty()) {
        ApplySearchResults({}, 0.0, m_searchGeneration.load());
        if (m_statusLeft) m_statusLeft->SetText("就绪");
        return;
    }

    // Everything-style: engine is fast enough — search synchronously on UI thread
    // to avoid thread hop + PostMessage one-frame lag.
    auto t0 = std::chrono::high_resolution_clock::now();
    std::vector<uint32_t> indices;
    m_engine.Search(query, indices, 0);
    auto t1 = std::chrono::high_resolution_clock::now();
    double seconds = std::chrono::duration<double>(t1 - t0).count();
    ApplySearchResults(std::move(indices), seconds, m_searchGeneration.load());
}

void EverythingApp::ApplySearchResults(std::vector<uint32_t>&& indices, double seconds, uint64_t generation) {
    if (generation != m_searchGeneration.load()) return;

    m_dataSource.ClearCaches();
    m_results = std::move(indices);
    if (m_resultsList) {
        if (m_resultsList->IsVirtualMode()) {
            m_resultsList->SetVirtualRowCount(static_cast<int>(m_results.size()));
        } else {
            m_resultsList->SetVirtualMode(static_cast<int>(m_results.size()), &m_dataSource);
        }
    }

    if (!m_lastQuery.empty() && m_statusLeft) {
        std::ostringstream ss;
        ss << m_results.size() << " 个对象";
        if (seconds > 0.0) {
            ss << "  (" << std::fixed << std::setprecision(3) << seconds << " 秒)";
        }
        m_statusLeft->SetText(ss.str());
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
    } else if (m_statusLeft && m_engine.IsReady()) {
        EngineStats s = m_engine.GetStats();
        std::ostringstream ss;
        ss << "就绪 — 已索引 " << s.live_file_count << " 个文件";
        if (s.elevated) ss << " [USN/MFT]";
        else ss << " [标准扫描]";
        m_statusLeft->SetText(ss.str());
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

int EverythingApp::SelectedRow() const {
    if (!m_resultsList || m_results.empty()) return -1;
    const auto& sel = m_resultsList->GetSelectedIndices();
    int row = m_resultsList->GetCaretIndex();
    if (!sel.empty()) row = *sel.begin();
    if (row < 0 || row >= static_cast<int>(m_results.size())) return -1;
    return row;
}

void EverythingApp::OpenSelected() {
    int row = SelectedRow();
    if (row < 0) return;
    std::wstring wpath = Utf8ToWide(m_engine.GetFilePath(m_results[static_cast<size_t>(row)]));
    ShellExecuteW(nullptr, L"open", wpath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void EverythingApp::OpenSelectedPath() {
    int row = SelectedRow();
    if (row < 0) return;
    std::wstring wpath = Utf8ToWide(m_engine.GetFilePath(m_results[static_cast<size_t>(row)]));
    std::wstring params = L"/select,\"" + wpath + L"\"";
    ShellExecuteW(nullptr, L"open", L"explorer.exe", params.c_str(), nullptr, SW_SHOWNORMAL);
}

void EverythingApp::CopyFullPath() {
    int row = SelectedRow();
    if (row < 0) return;
    std::wstring wpath = Utf8ToWide(m_engine.GetFilePath(m_results[static_cast<size_t>(row)]));
    if (!OpenClipboard(m_window.GetHWND())) return;
    EmptyClipboard();
    size_t bytes = (wpath.size() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (hMem) {
        void* ptr = GlobalLock(hMem);
        if (ptr) {
            memcpy(ptr, wpath.c_str(), bytes);
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
