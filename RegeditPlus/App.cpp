#include "App.h"
#include "RegIcons.h"
#include "BinaryValueDialog.h"

#include "framework/core/CUIDsl.h"
#include "framework/window/WindowBackdrop.h"
#include "framework/controls/WindowTitleBar.h"
#include "framework/controls/BreadcrumbBar.h"
#include "framework/controls/TreeView.h"
#include "framework/controls/ListView.h"
#include "framework/controls/ContextMenu.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/MessageBox.h"
#include "framework/style/ThemeManager.h"
#include "framework/style/ThemeTokenId.h"

#include <shellapi.h>
#include <cwctype>
#include <cwchar>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <functional>
#include <cstdio>
#include <memory>

using namespace CUI;
using namespace CUI::DSL;

namespace RegeditPlus {

namespace {

std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) return std::string();
    const int cb = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    if (cb <= 0) return std::string();
    std::string out(static_cast<size_t>(cb), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), &out[0], cb, nullptr, nullptr);
    return out;
}

std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return std::wstring();
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (n <= 0) return std::wstring();
    std::wstring out(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), &out[0], n);
    return out;
}

std::wstring ToLower(std::wstring w) {
    std::transform(w.begin(), w.end(), w.begin(), [](wchar_t c) { return static_cast<wchar_t>(towlower(c)); });
    return w;
}

bool ParseRootPath(const std::wstring& fullPath, HKEY& rootOut, std::wstring& subOut) {
    const size_t slash = fullPath.find(L'\\');
    std::wstring rootName = (slash == std::wstring::npos) ? fullPath : fullPath.substr(0, slash);
    subOut = (slash == std::wstring::npos) ? std::wstring() : fullPath.substr(slash + 1);

    HKEY root = nullptr;
    if (rootName == L"HKEY_CLASSES_ROOT") root = HKEY_CLASSES_ROOT;
    else if (rootName == L"HKEY_CURRENT_USER") root = HKEY_CURRENT_USER;
    else if (rootName == L"HKEY_LOCAL_MACHINE") root = HKEY_LOCAL_MACHINE;
    else if (rootName == L"HKEY_USERS") root = HKEY_USERS;
    else if (rootName == L"HKEY_CURRENT_CONFIG") root = HKEY_CURRENT_CONFIG;
    else return false;

    rootOut = root;
    return true;
}

bool OpenKey(const std::wstring& fullPath, REGSAM access, HKEY* outKey) {
    if (!outKey) return false;
    HKEY root = nullptr;
    std::wstring sub;
    if (!ParseRootPath(fullPath, root, sub)) return false;
    const wchar_t* subPtr = sub.empty() ? nullptr : sub.c_str();
    // Prefer the native 64-bit view (matches regedit.exe on x64).
    const REGSAM sam = access | KEY_WOW64_64KEY;
    if (RegOpenKeyExW(root, subPtr, 0, sam, outKey) == ERROR_SUCCESS) return true;
    return RegOpenKeyExW(root, subPtr, 0, access, outKey) == ERROR_SUCCESS;
}

std::vector<std::wstring> SplitPath(const std::wstring& fullPath) {
    std::vector<std::wstring> parts;
    size_t start = 0;
    while (start < fullPath.size()) {
        const size_t slash = fullPath.find(L'\\', start);
        if (slash == std::wstring::npos) {
            parts.push_back(fullPath.substr(start));
            break;
        }
        if (slash > start) parts.push_back(fullPath.substr(start, slash - start));
        start = slash + 1;
    }
    return parts;
}

bool RegNameLess(const std::wstring& a, const std::wstring& b) {
    // Match regedit.exe: locale-aware, case-insensitive name order.
    return CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                          a.c_str(), -1, b.c_str(), -1) == CSTR_LESS_THAN;
}

std::vector<std::wstring> EnumerateSubkeys(HKEY hKey) {
    std::vector<std::wstring> result;
    DWORD subCount = 0;
    DWORD maxNameLen = 0;
    if (RegQueryInfoKeyW(hKey, nullptr, nullptr, nullptr, &subCount, &maxNameLen,
                         nullptr, nullptr, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) {
        return result;
    }
    if (maxNameLen == 0) maxNameLen = 1;
    std::vector<wchar_t> buf(maxNameLen + 2, L'\0');
    for (DWORD i = 0; i < subCount; ++i) {
        DWORD len = static_cast<DWORD>(buf.size() - 1);
        if (RegEnumKeyExW(hKey, i, buf.data(), &len, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
            result.emplace_back(buf.data(), len);
        }
    }
    std::sort(result.begin(), result.end(), RegNameLess);
    return result;
}

std::string RegValueTypeName(DWORD type) {
    switch (type) {
        case REG_SZ: return "REG_SZ";
        case REG_EXPAND_SZ: return "REG_EXPAND_SZ";
        case REG_BINARY: return "REG_BINARY";
        case REG_DWORD: return "REG_DWORD";
        case REG_DWORD_BIG_ENDIAN: return "REG_DWORD_BIG_ENDIAN";
        case REG_MULTI_SZ: return "REG_MULTI_SZ";
        case REG_QWORD: return "REG_QWORD";
        case REG_LINK: return "REG_LINK";
        case REG_NONE: return "REG_NONE";
        default: return "REG_UNKNOWN";
    }
}

std::string FormatHex(const BYTE* data, size_t size, size_t maxBytes) {
    std::ostringstream oss;
    oss << std::uppercase << std::hex;
    const size_t show = maxBytes == 0 ? size : (std::min)(size, maxBytes);
    for (size_t i = 0; i < show; ++i) {
        if (i) oss << ' ';
        oss << std::setw(2) << std::setfill('0') << static_cast<unsigned>(data[i]);
    }
    if (maxBytes != 0 && size > show) oss << " ...";
    return oss.str();
}

std::vector<BYTE> HexToBytes(const std::wstring& text) {
    std::vector<BYTE> out;
    std::wstring clean;
    for (wchar_t c : text) if (iswxdigit(c)) clean.push_back(c);
    if (clean.empty()) return out;
    if (clean.size() % 2 == 1) clean = L'0' + clean;
    for (size_t i = 0; i + 1 < clean.size(); i += 2) {
        const wchar_t hi = clean[i];
        const wchar_t lo = clean[i + 1];
        const int hv = iswdigit(hi) ? hi - L'0' : towlower(hi) - L'a' + 10;
        const int lv = iswdigit(lo) ? lo - L'0' : towlower(lo) - L'a' + 10;
        out.push_back(static_cast<BYTE>((hv << 4) | lv));
    }
    return out;
}

std::wstring StringValueFrom(const std::vector<BYTE>& data) {
    const size_t chars = data.size() / sizeof(wchar_t);
    if (chars == 0) return std::wstring();
    const wchar_t* p = reinterpret_cast<const wchar_t*>(data.data());
    size_t end = 0;
    while (end < chars && p[end] != L'\0') ++end;
    return std::wstring(p, end);
}

std::string DataTextFor(DWORD type, const std::vector<BYTE>& data) {
    switch (type) {
        case REG_SZ:
        case REG_EXPAND_SZ: {
            const size_t chars = data.size() / sizeof(wchar_t);
            if (chars == 0) return std::string();
            const wchar_t* p = reinterpret_cast<const wchar_t*>(data.data());
            size_t end = 0;
            while (end < chars && p[end] != L'\0') ++end;
            return WideToUtf8(std::wstring(p, end));
        }
        case REG_MULTI_SZ: {
            std::wstring w;
            const wchar_t* p = reinterpret_cast<const wchar_t*>(data.data());
            const size_t chars = data.size() / sizeof(wchar_t);
            for (size_t i = 0; i < chars; ++i) {
                if (p[i] == L'\0') {
                    if (!w.empty() && w.back() != L'\n') w.push_back(L'\n');
                } else {
                    w.push_back(p[i]);
                }
            }
            return WideToUtf8(w);
        }
        case REG_DWORD: {
            if (data.size() >= 4) {
                const DWORD v = *reinterpret_cast<const DWORD*>(data.data());
                char buf[64];
                snprintf(buf, sizeof(buf), "0x%08x (%u)", v, v);
                return buf;
            }
            return "0x00000000 (0)";
        }
        case REG_QWORD: {
            if (data.size() >= 8) {
                const ULONGLONG v = *reinterpret_cast<const ULONGLONG*>(data.data());
                char buf[64];
                snprintf(buf, sizeof(buf), "0x%016llx (%llu)", v, v);
                return buf;
            }
            return "0x0000000000000000 (0)";
        }
        case REG_BINARY:
        default:
            return FormatHex(data.data(), data.size(), 96);
    }
}

// ---------------------------------------------------------------------------
// Deep registry copy used for key rename.
// ---------------------------------------------------------------------------
bool CopyKeyValues(HKEY src, HKEY dst) {
    DWORD valueCount = 0;
    DWORD maxNameLen = 0;
    DWORD maxDataLen = 0;
    if (RegQueryInfoKeyW(src, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                         &valueCount, &maxNameLen, &maxDataLen, nullptr, nullptr) != ERROR_SUCCESS) {
        return false;
    }
    std::vector<wchar_t> nameBuf(maxNameLen > 0 ? maxNameLen + 2 : 260);
    std::vector<BYTE> dataBuf(maxDataLen > 0 ? maxDataLen + 2 : 64);
    for (DWORD i = 0; i < valueCount; ++i) {
        DWORD nameLen = static_cast<DWORD>(nameBuf.size());
        DWORD type = 0;
        DWORD dataLen = static_cast<DWORD>(dataBuf.size());
        if (RegEnumValueW(src, i, nameBuf.data(), &nameLen, nullptr, &type,
                          dataBuf.data(), &dataLen) != ERROR_SUCCESS) {
            continue;
        }
        dataBuf.resize(dataLen > 0 ? dataLen : 1);
        const LONG r = RegSetValueExW(dst, nameBuf.data(), 0, type,
                                      dataBuf.data(), dataLen);
        if (r != ERROR_SUCCESS) return false;
    }
    return true;
}

bool CopySubtree(HKEY src, HKEY dst, size_t depth) {
    if (depth > 32) return false;
    if (!CopyKeyValues(src, dst)) return false;

    DWORD subCount = 0, maxNameLen = 0;
    if (RegQueryInfoKeyW(src, nullptr, nullptr, nullptr, &subCount, &maxNameLen,
                         nullptr, nullptr, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) {
        return true; // nothing more to copy
    }
    if (maxNameLen == 0) maxNameLen = 1;
    std::vector<wchar_t> buf(maxNameLen + 2, L'\0');
    for (DWORD i = 0; i < subCount; ++i) {
        DWORD len = static_cast<DWORD>(buf.size() - 1);
        if (RegEnumKeyExW(src, i, buf.data(), &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) {
            continue;
        }
        HKEY childSrc = nullptr;
        if (RegOpenKeyExW(src, buf.data(), 0, KEY_READ, &childSrc) != ERROR_SUCCESS) continue;
        HKEY childDst = nullptr;
        if (RegCreateKeyExW(dst, buf.data(), 0, nullptr, REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS, nullptr, &childDst, nullptr) == ERROR_SUCCESS) {
            CopySubtree(childSrc, childDst, depth + 1);
            RegCloseKey(childDst);
        }
        RegCloseKey(childSrc);
    }
    return true;
}

} // namespace

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int RegeditPlusApp::Run() {
    if (!m_window.Create("注册表编辑器 (RegeditPlus)", 1240, 800, false)) {
        return -1;
    }
    m_window.SetThemeMode(ThemeMode::Light);
    m_window.SetBackdropType(BackdropType::None);
    m_window.SetRenderStatsOverlayVisible(false);

    m_icons.EnsureLoaded();
    m_root = BuildRoot();
    m_window.SetRootElement(m_root);
    ApplyNativeRegeditIcon();
    m_window.Show();
    m_window.RunMessageLoop();
    return 0;
}

// ---------------------------------------------------------------------------
// UI construction
// ---------------------------------------------------------------------------
std::shared_ptr<UIElement> RegeditPlusApp::BuildRoot() {
    auto root = Column(0).BackgroundToken(ThemeTokenId::WindowBackground).Build();
    root->SetColorToken(ThemeTokenId::TextPrimary);

    m_titleBar = std::make_shared<WindowTitleBar>();
    m_titleBar->SetTitle("注册表编辑器 (RegeditPlus)");
    BuildMenus();

    m_breadcrumb = std::make_shared<BreadcrumbBar>();
    m_breadcrumb->SetWidth(-1.0f);
    m_breadcrumb->SetHeight(34.0f);
    m_breadcrumb->SetFontFamily("微软雅黑");
    m_breadcrumb->SetFontSize(16.0f);
    m_breadcrumb->SetBackgroundToken(ThemeTokenId::PaneBackground);
    m_breadcrumb->SetBackground(ThemeManager::Instance().GetColor(ThemeTokenId::PaneBackground));
    m_breadcrumb->SetPath({ "计算机" });
    m_breadcrumb->OnItemClicked().Connect([this](BreadcrumbBar*, int index, const std::string&) {
        OnBreadcrumbClicked(index);
    });

    auto body = Row(0).Build();
    body->SetFlexGrow(1.0f);
    body->SetGap(0.0f);

    // Left: tree pane
    auto treePane = Column(0).Build();
    treePane->SetWidth(320.0f);
    treePane->SetBackgroundToken(ThemeTokenId::PaneBackground);
    treePane->SetBackground(ThemeManager::Instance().GetColor(ThemeTokenId::PaneBackground));

    m_tree = std::make_shared<TreeView>();
    m_tree->SetWidth(-1.0f);
    m_tree->SetHeight(-1.0f);
    m_tree->SetFlexGrow(1.0f);
    m_tree->SetBackgroundToken(ThemeTokenId::PaneBackground);
    m_tree->SetBackground(ThemeManager::Instance().GetColor(ThemeTokenId::PaneBackground));
    m_tree->SetIndentWidth(18.0f);
    m_tree->SetFontFamily("微软雅黑");
    m_tree->SetFontSize(16.0f);
    m_tree->SetFontWeight("Normal");
    m_tree->SetItemHeight(28.0f);
    m_tree->OnSelectionChanged().Connect([this](TreeView* tree, std::shared_ptr<TreeViewItem> item) {
        OnTreeSelectionChanged(tree, item);
    });
    m_tree->OnItemToggled().Connect([this](TreeView* tree, std::shared_ptr<TreeViewItem> item) {
        OnTreeNodeToggled(tree, item);
    });
    m_tree->SetContextMenu(BuildTreeContextMenu());
    treePane->AddChild(m_tree);

    auto splitter = SplitterWidget(Orientation::Vertical).Build();

    // Right: list pane
    auto listPane = Column(0).Build();
    listPane->SetFlexGrow(1.0f);
    listPane->SetBackgroundToken(ThemeTokenId::WindowBackground);
    listPane->SetBackground(ThemeManager::Instance().GetColor(ThemeTokenId::WindowBackground));

    m_list = std::make_shared<ListView>();
    m_list->SetWidth(-1.0f);
    m_list->SetHeight(-1.0f);
    m_list->AddColumn("名称", 240.0f);
    m_list->AddColumn("类型", 160.0f);
    m_list->AddColumn("数据", 480.0f);
    m_list->SetRowHeight(30.0f);
    m_list->SetFontFamily("微软雅黑");
    m_list->SetFontSize(16.0f);
    m_list->SetFontWeight("Normal");
    m_list->SetFlexGrow(1.0f);
    m_list->SetBackground(ThemeManager::Instance().GetColor(ThemeTokenId::WindowBackground));
    m_list->OnSelectionChanged().Connect([this](ListView*, int row) {
        m_selectedRow = row;
    });
    m_list->OnRowDoubleClicked().Connect([this](ListView*, int row) {
        m_selectedRow = row;
        ModifySelectedValue();
    });
    m_list->SetContextMenu(BuildListContextMenu());
    listPane->AddChild(m_list);

    body->AddChild(treePane);
    body->AddChild(splitter);
    body->AddChild(listPane);

    // Status bar
    auto statusBar = Column(0).Build();
    statusBar->SetHeight(28.0f);
    statusBar->SetBackgroundToken(ThemeTokenId::PaneBackground);
    statusBar->SetBackground(ThemeManager::Instance().GetColor(ThemeTokenId::PaneBackground));
    statusBar->SetBorderBrush(ThemeManager::Instance().GetColor(ThemeTokenId::CardBorder));
    statusBar->SetBorderThickness(0.0f);
    m_statusBar = statusBar;

    auto statusRow = Row(0).Build();
    statusRow->SetPadding(Thickness(12, 0, 12, 0));
    statusRow->SetFlexGrow(1.0f);

    m_statusPath = Text("就绪").FontSize(16.0f).FontFamily("微软雅黑").FontWeight("Normal").Build();
    m_statusPath->SetColorToken(ThemeTokenId::TextSecondary);
    m_statusPath->SetColor(ThemeManager::Instance().GetColor(ThemeTokenId::TextSecondary));
    m_statusPath->SetTextAlign("Left");
    m_statusPath->SetFlexGrow(1.0f);
    statusRow->AddChild(m_statusPath);

    statusBar->AddChild(statusRow);

    root->AddChild(m_titleBar);
    root->AddChild(m_breadcrumb);
    root->AddChild(body);
    root->AddChild(statusBar);

    BuildInitialTree();
    return root;
}

void RegeditPlusApp::BuildMenus() {
    if (!m_titleBar) return;
    MenuBar& menuBar = m_titleBar->GetMenuBar();
    menuBar.ClearMenus();

    // 文件
    auto fileMenu = menuBar.AddMenu("文件(F)");
    fileMenu->AddItem("导入(E)...", [this]() {
        ShowMessage("导入", "导入 .reg 注册表文件（功能占位）。");
    });
    fileMenu->AddItem("导出(O)...", [this]() {
        ShowMessage("导出", "导出当前项为 .reg 注册表文件（功能占位）。");
    });
    fileMenu->AddSeparator();
    fileMenu->AddItem("加载配置单元(L)...", [this]() {
        ShowMessage("加载配置单元", "从文件加载配置单元（功能占位）。");
    });
    fileMenu->AddItem("卸载配置单元(U)...", [this]() {
        ShowMessage("卸载配置单元", "卸载当前选中的配置单元（功能占位）。");
    });
    fileMenu->AddSeparator();
    fileMenu->AddItem("退出(X)", [this]() {
        if (HWND hwnd = m_window.GetHWND()) {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
        }
    });

    // 编辑
    auto editMenu = menuBar.AddMenu("编辑(E)");
    editMenu->AddItem("撤销(U)", [this]() { ShowMessage("撤销", "本版本暂未实现撤销功能。"); });
    editMenu->AddSeparator();
    editMenu->AddItem("复制项名称(K)", [this]() { CopyKeyName(); });
    editMenu->AddItem("删除(D)", [this]() {
        if (m_tree->GetSelectedItem() && HasValidKey()) DeleteSelectedKey();
        else if (HasValidKey()) DeleteSelectedValue();
    });
    editMenu->AddItem("重命名(M)", [this]() {
        if (m_tree->GetSelectedItem()) RenameSelectedKey();
        else if (HasValidKey()) RenameSelectedValue();
    });
    editMenu->AddItem("修改", [this]() {
        if (m_selectedRow >= 0 && HasValidKey()) ModifySelectedValue();
    });
    editMenu->AddItem("修改二进制数据", [this]() {
        if (m_selectedRow >= 0 && HasValidKey()) ModifySelectedValueAsBinary();
    });

    auto newSub = editMenu->AddSubMenu("新建(R)");
    newSub->AddItem("子项(K)", [this]() { CreateNewKey(); });
    newSub->AddItem("字符串值(S)", [this]() { CreateNewValue(REG_SZ, "新建字符串值"); });
    newSub->AddItem("二进制值(B)", [this]() { CreateNewValue(REG_BINARY, "新建二进制值"); });
    newSub->AddItem("DWORD (32 位)值(D)", [this]() { CreateNewValue(REG_DWORD, "新建DWORD值"); });
    newSub->AddItem("QWORD (64 位)值(Q)", [this]() { CreateNewValue(REG_QWORD, "新建QWORD值"); });
    newSub->AddItem("多字符串值(M)", [this]() { CreateNewValue(REG_MULTI_SZ, "新建多字符串值"); });
    editMenu->AddSeparator();
    editMenu->AddItem("查找(F)...", [this]() { ShowFindDialog(); });
    editMenu->AddItem("查找下一个(N)", [this]() { FindNext(); });

    // 查看
    auto viewMenu = menuBar.AddMenu("查看(V)");
    viewMenu->AddItem("状态栏(S)", [this]() { ToggleStatusBar(); });
    viewMenu->AddItem("切换主题(T)", [this]() { ToggleTheme(); });
    viewMenu->AddItem("刷新(R)", [this]() { RefreshCurrent(); });

    // 收藏夹
    auto favMenu = menuBar.AddMenu("收藏夹(A)");
    favMenu->AddItem("添加到收藏夹(N)...", [this]() { AddToFavorites(); });
    favMenu->AddItem("删除选中的收藏(D)", [this]() { ClearFavorites(); });
    if (!m_favorites.empty()) {
        favMenu->AddSeparator();
        for (const auto& fav : m_favorites) {
            favMenu->AddItem(fav.label, [this, fav]() {
                m_selectedPath = fav.path;
                ReloadCurrent();
                RevealPath(fav.path);
            });
        }
    }

// 帮助
    auto helpMenu = menuBar.AddMenu("帮助(H)");
    helpMenu->AddItem("关于注册表编辑器", [this]() {
        ShowMessage("关于",
                    "注册表编辑器 (RegeditPlus)\n\n"
                    "使用 CUI 声明式框架构建，界面仿 Windows 注册表编辑器。");
    });
}

// ---------------------------------------------------------------------------
// Tree
// ---------------------------------------------------------------------------
// TreeView only shows a chevron / allows toggle when children is non-empty, and
// child visibility is driven by expandAnim (not isExpanded alone). For lazy
// registry loading we plant a placeholder child, then replace it on expand.
static void AttachPlaceholder(const std::shared_ptr<TreeViewItem>& node) {
    if (!node) return;
    auto ph = std::make_shared<TreeViewItem>();
    ph->header.clear();
    ph->parent = node.get();
    node->children.push_back(ph);
}

std::shared_ptr<TreeViewItem> RegeditPlusApp::MakeNode(const std::wstring& fullPath,
                                                       const std::wstring& label,
                                                       bool mayHaveChildren) {
    m_icons.EnsureLoaded();
    auto node = std::make_shared<TreeViewItem>();
    node->header = WideToUtf8(label);
    node->icon.clear();
    node->nativeIcon = fullPath.empty() ? m_icons.Computer() : m_icons.Folder();
    m_nodePaths[node.get()] = fullPath;
    m_nodeLoaded[node.get()] = false;
    if (!fullPath.empty()) {
        m_pathNodes[fullPath] = node;
    }
    if (mayHaveChildren) {
        AttachPlaceholder(node);
    }
    return node;
}

void RegeditPlusApp::EnsureExpanded(const std::shared_ptr<TreeViewItem>& node) {
    if (!node) return;
    node->isExpanded = true;
    node->expandAnim.Reset(1.0f);
}

void RegeditPlusApp::RefreshTreeVisual() {
    if (!m_tree) return;
    m_tree->InvalidateVisibleItems();
    m_tree->InvalidateMeasure();
    m_tree->InvalidateArrange();
}

void RegeditPlusApp::BuildInitialTree() {
    if (!m_tree) return;
    m_tree->ClearItems();
    m_nodePaths.clear();
    m_nodeLoaded.clear();
    m_pathNodes.clear();

    auto computer = MakeNode(L"", L"计算机", false);
    EnsureExpanded(computer);
    m_nodeLoaded[computer.get()] = true;

    const wchar_t* roots[] = {
        L"HKEY_CLASSES_ROOT", L"HKEY_CURRENT_USER", L"HKEY_LOCAL_MACHINE",
        L"HKEY_USERS", L"HKEY_CURRENT_CONFIG"
    };
    std::shared_ptr<TreeViewItem> hkcu;
    for (const wchar_t* r : roots) {
        // Hives always get a placeholder so the chevron is visible before first expand.
        auto hive = MakeNode(r, r, true);
        hive->parent = computer.get();
        if (std::wstring(r) == L"HKEY_CURRENT_USER") hkcu = hive;
        computer->children.push_back(hive);
    }

    m_tree->SetItems({ computer });

    m_selectedPath = L"HKEY_CURRENT_USER";
    if (hkcu) {
        PopulateSubkeys(hkcu);
        EnsureExpanded(hkcu);
        m_tree->SetSelectedItem(hkcu);
    } else {
        m_tree->SetSelectedItem(computer);
    }
    ReloadCurrent();
}

void RegeditPlusApp::PopulateSubkeys(const std::shared_ptr<TreeViewItem>& node) {
    if (!node) return;
    if (m_nodeLoaded[node.get()]) return;

    m_nodeLoaded[node.get()] = true;
    node->children.clear();

    const std::wstring path = m_nodePaths[node.get()];
    if (path.empty()) return;

    HKEY hKey = nullptr;
    if (!OpenKey(path, KEY_READ, &hKey)) return;

    for (const std::wstring& name : EnumerateSubkeys(hKey)) {
        if (name.empty()) continue;
        const std::wstring childPath = path + L"\\" + name;
        // Always plant a placeholder: TreeView hides the chevron when children
        // is empty, so this is required for lazy expand. Leaf keys lose the
        // chevron after the first expand finds no subkeys.
        auto child = MakeNode(childPath, name, true);
        child->parent = node.get();
        node->children.push_back(child);
    }
    RegCloseKey(hKey);
}

void RegeditPlusApp::OnTreeNodeToggled(TreeView* tree, std::shared_ptr<TreeViewItem> item) {
    (void)tree;
    if (!item) return;
    if (item->isExpanded) {
        PopulateSubkeys(item);
        // Keep expandAnim in sync: ToggleItem already set the target, but if we
        // replaced children the visible list must rebuild.
        if (!UIElement::AreAnimationsEnabled()) {
            item->expandAnim.Reset(1.0f);
        }
    }
    RefreshTreeVisual();
}

void RegeditPlusApp::OnTreeSelectionChanged(TreeView* tree, std::shared_ptr<TreeViewItem> item) {
    (void)tree;
    if (!item) return;
    const auto it = m_nodePaths.find(item.get());
    const std::wstring path = (it != m_nodePaths.end()) ? it->second : std::wstring();
    // Ignore placeholder rows (empty path that isn't the computer root).
    if (path.empty() && item->header.empty()) return;
    m_selectedPath = path;
    m_findNextStart = path;
    ReloadCurrent();
}

void RegeditPlusApp::RevealPath(const std::wstring& fullPath) {
    if (fullPath.empty() || !m_tree || m_tree->GetItems().empty()) return;

    const auto parts = SplitPath(fullPath);
    if (parts.empty()) return;

    auto node = m_tree->GetItems().front(); // 计算机
    EnsureExpanded(node);

    std::wstring accum;
    std::shared_ptr<TreeViewItem> target;
    for (const std::wstring& seg : parts) {
        accum = accum.empty() ? seg : (accum + L"\\" + seg);
        PopulateSubkeys(node);

        std::shared_ptr<TreeViewItem> found;
        for (auto& child : node->children) {
            if (!child) continue;
            const auto pit = m_nodePaths.find(child.get());
            if (pit != m_nodePaths.end() && pit->second == accum) {
                found = child;
                break;
            }
        }
        if (!found) break;
        EnsureExpanded(found);
        PopulateSubkeys(found);
        node = found;
        target = found;
    }

    if (target) {
        m_selectedPath = fullPath;
        m_tree->SetSelectedItem(target);
        ReloadCurrent();
    }
    RefreshTreeVisual();
}

// ---------------------------------------------------------------------------
// Values
// ---------------------------------------------------------------------------
void RegeditPlusApp::LoadValues(const std::wstring& path) {
    m_entries.clear();
    m_selectedRow = -1;

    if (!path.empty()) {
        HKEY hKey = nullptr;
        if (OpenKey(path, KEY_READ, &hKey)) {
            DWORD valueCount = 0;
            DWORD maxValueName = 0;
            DWORD maxValueData = 0;
            if (RegQueryInfoKeyW(hKey, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                 &valueCount, &maxValueName, &maxValueData, nullptr, nullptr) == ERROR_SUCCESS) {
                std::vector<wchar_t> nameBuf(maxValueName > 0 ? maxValueName + 2 : 260);
                for (DWORD i = 0; i < valueCount; ++i) {
                    DWORD nameLen = static_cast<DWORD>(nameBuf.size());
                    DWORD type = 0;
                    if (RegEnumValueW(hKey, i, nameBuf.data(), &nameLen, nullptr, &type,
                                      nullptr, nullptr) != ERROR_SUCCESS) {
                        continue;
                    }
                    ValueEntry entry;
                    entry.name.assign(nameBuf.data(), nameLen);
                    entry.type = type;

                    DWORD dataLen = 0;
                    const std::wstring nameStr = entry.name;
                    const wchar_t* namePtr = nameStr.empty() ? nullptr : nameStr.c_str();
                    const LONG q = RegQueryValueExW(hKey, namePtr, nullptr, &type, nullptr, &dataLen);
                    if ((q == ERROR_SUCCESS || q == ERROR_MORE_DATA) && dataLen > 0) {
                        entry.type = type;
                        entry.data.resize(dataLen);
                        DWORD readLen = dataLen;
                        if (RegQueryValueExW(hKey, namePtr, nullptr, &type,
                                             entry.data.data(), &readLen) == ERROR_SUCCESS) {
                            entry.data.resize(readLen);
                            entry.type = type;
                        } else {
                            entry.data.clear();
                        }
                    }
                    m_entries.push_back(std::move(entry));
                }
            }

            // regedit always lists the default value, even when unset.
            bool hasDefault = false;
            for (const auto& e : m_entries) {
                if (e.name.empty()) { hasDefault = true; break; }
            }
            if (!hasDefault) {
                ValueEntry def;
                def.name.clear();
                def.type = REG_SZ;
                DWORD type = REG_SZ;
                DWORD dataLen = 0;
                const LONG st = RegQueryValueExW(hKey, nullptr, nullptr, &type, nullptr, &dataLen);
                if (st == ERROR_SUCCESS || st == ERROR_MORE_DATA) {
                    def.type = type;
                    if (dataLen > 0) {
                        def.data.resize(dataLen);
                        DWORD readLen = dataLen;
                        if (RegQueryValueExW(hKey, nullptr, nullptr, &type,
                                             def.data.data(), &readLen) == ERROR_SUCCESS) {
                            def.data.resize(readLen);
                            def.type = type;
                        } else {
                            def.data.clear();
                        }
                    }
                }
                m_entries.push_back(std::move(def));
            }

            std::sort(m_entries.begin(), m_entries.end(), [](const ValueEntry& a, const ValueEntry& b) {
                if (a.name.empty() != b.name.empty()) return a.name.empty();
                return RegNameLess(a.name, b.name);
            });

            RegCloseKey(hKey);
        }
    }

    if (!m_list) return;
    m_icons.EnsureLoaded();
    m_list->ClearRows();
    std::vector<std::vector<std::string>> rows;
    std::vector<HICON> icons;
    rows.reserve(m_entries.size());
    icons.reserve(m_entries.size());
    for (const ValueEntry& e : m_entries) {
        const std::string name = e.name.empty() ? "(默认)" : WideToUtf8(e.name);
        rows.push_back({ name, TypeName(e.type), DataText(e) });
        icons.push_back(m_icons.ForValueType(e.type));
    }
    m_list->SetRows(rows);
    m_list->SetRowIcons(icons);
    m_list->ClearSelection();
    m_selectedRow = -1;
}

void RegeditPlusApp::ReloadCurrent() {
    if (!m_statusPath) return;
    m_statusPath->SetText(m_selectedPath.empty() ? std::string("就绪") : WideToUtf8(m_selectedPath));
    UpdateBreadcrumb();
    LoadValues(m_selectedPath);
}

void RegeditPlusApp::UpdateBreadcrumb() {
    if (!m_breadcrumb) return;
    std::vector<std::string> nodes;
    nodes.push_back("计算机");
    if (!m_selectedPath.empty()) {
        for (const std::wstring& part : SplitPath(m_selectedPath)) {
            if (!part.empty()) nodes.push_back(WideToUtf8(part));
        }
    }
    m_breadcrumb->SetPath(nodes);
}

void RegeditPlusApp::OnBreadcrumbClicked(int index) {
    if (index < 0) return;
    if (index == 0) {
        m_selectedPath.clear();
        if (m_tree && !m_tree->GetItems().empty()) {
            m_tree->SetSelectedItem(m_tree->GetItems().front());
        }
        ReloadCurrent();
        return;
    }

    const auto parts = SplitPath(m_selectedPath);
    if (parts.empty()) return;
    // Breadcrumb: [0]=计算机, [1]=hive, [2]=...
    const int partCount = static_cast<int>(parts.size());
    const int take = (std::min)(index, partCount);
    std::wstring path;
    for (int i = 0; i < take; ++i) {
        if (!path.empty()) path += L'\\';
        path += parts[static_cast<size_t>(i)];
    }
    m_selectedPath = path;
    RevealPath(path);
    ReloadCurrent();
}

std::string RegeditPlusApp::TypeName(DWORD type) const {
    return RegValueTypeName(type);
}

std::string RegeditPlusApp::DataText(const ValueEntry& entry) const {
    // Unset default value — regedit always shows this placeholder.
    if (entry.name.empty() && entry.data.empty()
        && (entry.type == REG_SZ || entry.type == REG_EXPAND_SZ || entry.type == REG_NONE)) {
        return "(数值未设置)";
    }
    return DataTextFor(entry.type, entry.data);
}

int RegeditPlusApp::SelectedValueRow() const {
    if (m_selectedRow >= 0 && m_selectedRow < static_cast<int>(m_entries.size())) {
        return m_selectedRow;
    }
    return -1;
}

// ---------------------------------------------------------------------------
// Context menus
// ---------------------------------------------------------------------------
std::shared_ptr<ContextMenu> RegeditPlusApp::BuildTreeContextMenu() {
    auto menu = std::make_shared<ContextMenu>();
    auto newSub = menu->AddSubMenu("新建");
    newSub->AddItem("子项(K)", [this]() { CreateNewKey(); });
    newSub->AddItem("字符串值(S)", [this]() { CreateNewValue(REG_SZ, "新建字符串值"); });
    newSub->AddItem("二进制值(B)", [this]() { CreateNewValue(REG_BINARY, "新建二进制值"); });
    newSub->AddItem("DWORD (32 位)值(D)", [this]() { CreateNewValue(REG_DWORD, "新建DWORD值"); });
    newSub->AddItem("QWORD (64 位)值(Q)", [this]() { CreateNewValue(REG_QWORD, "新建QWORD值"); });
    newSub->AddItem("多字符串值(M)", [this]() { CreateNewValue(REG_MULTI_SZ, "新建多字符串值"); });
    menu->AddSeparator();
    menu->AddItem("删除", [this]() { DeleteSelectedKey(); });
    menu->AddItem("重命名", [this]() { RenameSelectedKey(); });
    menu->AddSeparator();
    menu->AddItem("复制项名称", [this]() { CopyKeyName(); });
    menu->AddItem("刷新", [this]() { RefreshCurrent(); });
    return menu;
}

std::shared_ptr<ContextMenu> RegeditPlusApp::BuildListContextMenu() {
    auto menu = std::make_shared<ContextMenu>();
    auto newSub = menu->AddSubMenu("新建");
    newSub->AddItem("子项(K)", [this]() { CreateNewKey(); });
    newSub->AddItem("字符串值(S)", [this]() { CreateNewValue(REG_SZ, "新建字符串值"); });
    newSub->AddItem("二进制值(B)", [this]() { CreateNewValue(REG_BINARY, "新建二进制值"); });
    newSub->AddItem("DWORD (32 位)值(D)", [this]() { CreateNewValue(REG_DWORD, "新建DWORD值"); });
    newSub->AddItem("QWORD (64 位)值(Q)", [this]() { CreateNewValue(REG_QWORD, "新建QWORD值"); });
    newSub->AddItem("多字符串值(M)", [this]() { CreateNewValue(REG_MULTI_SZ, "新建多字符串值"); });
    menu->AddSeparator();
    menu->AddItem("修改", [this]() { ModifySelectedValue(); });
    menu->AddItem("修改二进制数据", [this]() { ModifySelectedValueAsBinary(); });
    menu->AddItem("删除", [this]() { DeleteSelectedValue(); });
    menu->AddItem("重命名", [this]() { RenameSelectedValue(); });
    return menu;
}

// ---------------------------------------------------------------------------
// Key actions
// ---------------------------------------------------------------------------
void RegeditPlusApp::CreateNewKey() {
    if (!HasValidKey()) {
        ShowMessage("新建子项", "请先选中一个键，再新建子项。");
        return;
    }
    PromptText("新建子项", "请键入该子项的名称:", L"", false, [this](const std::wstring& name) {
        if (name.empty() || name.find(L'\\') != std::wstring::npos) return;

        HKEY root = nullptr;
        std::wstring baseSub;
        if (!ParseRootPath(m_selectedPath, root, baseSub)) return;
        const std::wstring full = m_selectedPath + L"\\" + name;
        const std::wstring sub = baseSub.empty() ? name : baseSub + L"\\" + name;

        HKEY hKey = nullptr;
        const LONG st = RegCreateKeyExW(root, sub.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
                                        KEY_ALL_ACCESS | KEY_WOW64_64KEY, nullptr, &hKey, nullptr);
        if (st != ERROR_SUCCESS) {
            const LONG st2 = RegCreateKeyExW(root, sub.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
                                             KEY_ALL_ACCESS, nullptr, &hKey, nullptr);
            if (st2 != ERROR_SUCCESS) {
                ShowMessage("新建子项", "无法创建子项，可能权限不足或名称无效。");
                return;
            }
        }
        if (hKey) RegCloseKey(hKey);
        RefreshCurrent();
        RevealPath(full);
        m_selectedPath = full;
        ReloadCurrent();
    });
}

void RegeditPlusApp::CreateNewValue(DWORD type, const char* defaultName) {
    if (!HasValidKey()) {
        ShowMessage("新建值", "请先选中一个键，再新建值。");
        return;
    }
    PromptText("新建值", "值名称:", Utf8ToWide(defaultName), false, [this, type](const std::wstring& name) {
        if (name.empty()) return;

        HKEY hKey = nullptr;
        if (!OpenKey(m_selectedPath, KEY_SET_VALUE, &hKey)) {
            ShowMessage("新建值", "无法打开键写入，可能权限不足。");
            return;
        }

        const wchar_t* namePtr = name.c_str();
        LONG st = ERROR_SUCCESS;
        switch (type) {
            case REG_SZ: {
                const wchar_t empty = L'\0';
                st = RegSetValueExW(hKey, namePtr, 0, REG_SZ,
                                    reinterpret_cast<const BYTE*>(&empty), sizeof(wchar_t));
                break;
            }
            case REG_MULTI_SZ: {
                const wchar_t dbl[2] = { L'\0', L'\0' };
                st = RegSetValueExW(hKey, namePtr, 0, REG_MULTI_SZ,
                                    reinterpret_cast<const BYTE*>(dbl), sizeof(dbl));
                break;
            }
            case REG_DWORD: {
                DWORD v = 0;
                st = RegSetValueExW(hKey, namePtr, 0, REG_DWORD,
                                    reinterpret_cast<const BYTE*>(&v), sizeof(v));
                break;
            }
            case REG_QWORD: {
                ULONGLONG v = 0;
                st = RegSetValueExW(hKey, namePtr, 0, REG_QWORD,
                                    reinterpret_cast<const BYTE*>(&v), sizeof(v));
                break;
            }
            default: {
                BYTE none = 0;
                st = RegSetValueExW(hKey, namePtr, 0, REG_BINARY, &none, 1);
                break;
            }
        }
        RegCloseKey(hKey);
        if (st != ERROR_SUCCESS) {
            ShowMessage("新建值", "创建值失败，可能权限不足。");
            return;
        }
        ReloadCurrent();
        for (size_t i = 0; i < m_entries.size(); ++i) {
            if (m_entries[i].name == name) {
                m_selectedRow = static_cast<int>(i);
                m_list->SetRowSelected(static_cast<int>(i), true);
                break;
            }
        }
    });
}

void RegeditPlusApp::DeleteSelectedKey() {
    if (!HasValidKey()) return;
    const std::wstring key = m_selectedPath;
    ContentDialog::ShowMessageBox(m_root.get(),
        "确认删除",
        WideToUtf8(key) + "\n\n确实要删除这个项以及它的所有子项吗？",
        [this, key](DialogResult r) {
            if (r != DialogResult::Primary) return;
            HKEY root = nullptr;
            std::wstring sub;
            if (!ParseRootPath(key, root, sub)) return;
            const LONG st = RegDeleteTreeW(root, sub.c_str());
            if (st != ERROR_SUCCESS) {
                ShowMessage("删除失败", "无法删除该项，可能权限不足。");
                return;
            }
            m_selectedPath.clear();
            m_findNextStart.clear();
BuildInitialTree();
        });
}

void RegeditPlusApp::RenameSelectedKey() {
    if (!HasValidKey()) {
        ShowMessage("重命名", "请先选中一个键。");
        return;
    }
    const std::wstring full = m_selectedPath;
    const size_t slash = full.find_last_of(L'\\');
    const std::wstring parentPath = (slash == std::wstring::npos) ? std::wstring() : full.substr(0, slash);
    const std::wstring oldName = (slash == std::wstring::npos) ? full : full.substr(slash + 1);
    if (parentPath.empty()) {
        ShowMessage("重命名", "根键不可重命名。");
        return;
    }

    PromptText("重命名", "新的键名称:", oldName, false, [this, full, parentPath, oldName](const std::wstring& newName) {
        if (newName.empty() || newName == oldName || newName.find(L'\\') != std::wstring::npos) return;

        HKEY parentKey = nullptr;
        if (!OpenKey(parentPath, KEY_ALL_ACCESS, &parentKey)) {
            ShowMessage("重命名", "无法打开父键，可能权限不足。");
            return;
        }

        HKEY srcKey = nullptr;
        LONG st = ERROR_SUCCESS;
        if (!OpenKey(full, KEY_READ, &srcKey)) {
            ShowMessage("重命名", "无法打开原键。");
            RegCloseKey(parentKey);
            return;
        }

        HKEY dstKey = nullptr;
        st = RegCreateKeyExW(parentKey, newName.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS, nullptr, &dstKey, nullptr);
        if (st == ERROR_SUCCESS && dstKey) {
            if (!CopySubtree(srcKey, dstKey, 0)) st = ERROR_WRITE_FAULT;
            RegCloseKey(dstKey);
        }
        RegCloseKey(srcKey);
        RegCloseKey(parentKey);

        if (st != ERROR_SUCCESS) {
            ShowMessage("重命名", "重命名失败，可能权限不足或同名键已存在。");
            return;
        }

        HKEY root = nullptr;
        std::wstring sub;
        if (ParseRootPath(full, root, sub)) {
            RegDeleteTreeW(root, sub.c_str());
        }
        m_selectedPath = parentPath + L"\\" + newName;
        BuildInitialTree();
        ReloadCurrent();
        RevealPath(m_selectedPath);
    });
}

void RegeditPlusApp::CopyKeyName() {
    if (!HasValidKey()) return;
    if (!OpenClipboard(m_window.GetHWND())) return;
    EmptyClipboard();
    const std::wstring text = m_selectedPath;
    const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (hMem) {
        void* p = GlobalLock(hMem);
        if (p) {
            memcpy(p, text.c_str(), bytes);
            GlobalUnlock(hMem);
            if (SetClipboardData(CF_UNICODETEXT, hMem)) {
                // Clipboard owns hMem after a successful SetClipboardData.
                hMem = nullptr;
            }
        }
        if (hMem) GlobalFree(hMem);
    }
    CloseClipboard();
}

// ---------------------------------------------------------------------------
// Value actions
// ---------------------------------------------------------------------------
void RegeditPlusApp::ModifySelectedValue() {
    const int row = SelectedValueRow();
    if (row < 0 || !HasValidKey()) return;
    const ValueEntry e = m_entries[static_cast<size_t>(row)];
    const std::wstring displayName = e.name.empty() ? L"(默认)" : e.name;

    switch (e.type) {
        case REG_SZ:
        case REG_EXPAND_SZ:
        case REG_MULTI_SZ: {
            std::wstring initial;
            if (e.type == REG_MULTI_SZ) {
                const wchar_t* p = reinterpret_cast<const wchar_t*>(e.data.data());
                const size_t chars = e.data.size() / sizeof(wchar_t);
                for (size_t i = 0; i < chars; ++i) {
                    if (p[i] == L'\0') {
                        if (!initial.empty()) initial.push_back(L'\n');
                    } else {
                        initial.push_back(p[i]);
                    }
                }
            } else {
                initial = StringValueFrom(e.data);
            }

            const std::string title = e.type == REG_MULTI_SZ ? "编辑多字符串值" : "编辑字符串值";
            const std::string label = "值名称: " + WideToUtf8(displayName) + "  (数值数据:)";
            const DWORD type = e.type;
            PromptText(title, label, initial, e.type == REG_MULTI_SZ, [this, row, type](const std::wstring& out) {
                std::vector<BYTE> bytes;
                if (type == REG_MULTI_SZ) {
                    std::wstring wide;
                    std::wstring::size_type start = 0;
                    while (start <= out.size()) {
                        const std::wstring::size_type newline = out.find(L'\n', start);
                        std::wstring line;
                        if (newline == std::wstring::npos) {
                            line = out.substr(start);
                            start = out.size() + 1;
                        } else {
                            line = out.substr(start, newline - start);
                            start = newline + 1;
                        }
                        wide.append(line);
                        wide.push_back(L'\0');
                    }
                    wide.push_back(L'\0');
                    bytes.assign(reinterpret_cast<const BYTE*>(wide.data()),
                                 reinterpret_cast<const BYTE*>(wide.data() + wide.size()));
                } else {
                    bytes.assign(reinterpret_cast<const BYTE*>(out.c_str()),
                                 reinterpret_cast<const BYTE*>(out.c_str() + out.size() + 1));
                }
                WriteValueAt(row, type, bytes);
            });
            break;
        }

        case REG_DWORD: {
            std::wstring init;
            if (e.data.size() >= 4) {
                wchar_t buf[40];
                swprintf_s(buf, 40, L"0x%08x", *reinterpret_cast<const DWORD*>(e.data.data()));
                init = buf;
            }
            PromptText("编辑 DWORD (32 位)值", "数值数据 (十六进制):", init, false, [this, row](const std::wstring& out) {
                ULONGLONG v = wcstoull(out.c_str(), nullptr, 0);
                const DWORD dw = static_cast<DWORD>(v);
                std::vector<BYTE> bytes(sizeof(dw), 0);
                memcpy(bytes.data(), &dw, sizeof(dw));
                WriteValueAt(row, REG_DWORD, bytes);
            });
            break;
        }

        case REG_QWORD: {
            std::wstring init;
            if (e.data.size() >= 8) {
                wchar_t buf[40];
                swprintf_s(buf, 40, L"0x%016llx", *reinterpret_cast<const ULONGLONG*>(e.data.data()));
                init = buf;
            }
            PromptText("编辑 QWORD (64 位)值", "数值数据 (十六进制):", init, false, [this, row](const std::wstring& out) {
                ULONGLONG v = wcstoull(out.c_str(), nullptr, 0);
                std::vector<BYTE> bytes(sizeof(v), 0);
                memcpy(bytes.data(), &v, sizeof(v));
                WriteValueAt(row, REG_QWORD, bytes);
            });
            break;
        }

        case REG_BINARY:
        default: {
            ModifySelectedValueAsBinary();
            break;
        }
    }
}

void RegeditPlusApp::ModifySelectedValueAsBinary() {
    const int row = SelectedValueRow();
    if (row < 0 || !HasValidKey()) return;
    const ValueEntry e = m_entries[static_cast<size_t>(row)];
    const DWORD keepType = e.type;
    const std::wstring displayName = e.name.empty() ? L"(默认)" : e.name;

    auto dlg = std::make_shared<BinaryValueDialog>();
    dlg->Show(m_root.get(), displayName, e.data, [this, row, keepType](bool ok, std::vector<BYTE> data) {
        if (!ok) return;
        WriteValueAt(row, keepType, data);
    });
}

void RegeditPlusApp::WriteValueAt(int row, DWORD type, const std::vector<BYTE>& bytes) {
    if (row < 0 || row >= static_cast<int>(m_entries.size())) return;
    const std::wstring name = m_entries[static_cast<size_t>(row)].name;

    HKEY hKey = nullptr;
    if (!OpenKey(m_selectedPath, KEY_SET_VALUE, &hKey)) {
        ShowMessage("写入失败", "无法打开键执行写入，可能权限不足。");
        return;
    }

    const wchar_t* namePtr = name.empty() ? nullptr : name.c_str();
    const DWORD cb = static_cast<DWORD>(bytes.size());
    const BYTE* dataPtr = bytes.empty() ? reinterpret_cast<const BYTE*>(L"") : bytes.data();
    const LONG st = RegSetValueExW(hKey, namePtr, 0, type, dataPtr, cb);
    RegCloseKey(hKey);

    if (st != ERROR_SUCCESS) {
        ShowMessage("写入失败", "写入值失败，可能权限不足。");
        return;
    }
    ReloadCurrent();
    for (size_t i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].name == name) {
            m_selectedRow = static_cast<int>(i);
            m_list->SetRowSelected(static_cast<int>(i), true);
            break;
        }
    }
}

void RegeditPlusApp::DeleteSelectedValue() {
    const int row = SelectedValueRow();
    if (row < 0 || !HasValidKey()) return;
    const std::wstring name = m_entries[static_cast<size_t>(row)].name;

    HKEY hKey = nullptr;
    if (!OpenKey(m_selectedPath, KEY_SET_VALUE, &hKey)) {
        ShowMessage("删除值", "无法打开键，可能权限不足。");
        return;
    }
    const wchar_t* namePtr = name.empty() ? nullptr : name.c_str();
    const LONG st = RegDeleteValueW(hKey, namePtr);
    RegCloseKey(hKey);
    if (st != ERROR_SUCCESS) {
        ShowMessage("删除值", "删除值失败。");
        return;
    }
    ReloadCurrent();
}

void RegeditPlusApp::RenameSelectedValue() {
    const int row = SelectedValueRow();
    if (row < 0 || !HasValidKey()) return;
    const ValueEntry& e = m_entries[static_cast<size_t>(row)];
    const std::wstring oldName = e.name;
    if (oldName.empty()) {
        ShowMessage("重命名值", "(默认) 值无法重命名。");
        return;
    }

    PromptText("重命名值", "新的值名称:", oldName, false, [this, oldName](const std::wstring& newName) {
        if (newName.empty() || newName == oldName) return;

        HKEY hKey = nullptr;
        if (!OpenKey(m_selectedPath, KEY_SET_VALUE, &hKey)) return;

        LONG st = ERROR_SUCCESS;
        DWORD type = 0;
        DWORD dataLen = 0;
        if (RegQueryValueExW(hKey, oldName.c_str(), nullptr, &type, nullptr, &dataLen) == ERROR_SUCCESS) {
            std::vector<BYTE> data(dataLen > 0 ? dataLen : 1, 0);
            DWORD readLen = dataLen;
            RegQueryValueExW(hKey, oldName.c_str(), nullptr, &type, data.data(), &readLen);
            data.resize(readLen);
            st = RegSetValueExW(hKey, newName.c_str(), 0, type, data.data(), static_cast<DWORD>(data.size()));
            if (st == ERROR_SUCCESS) st = RegDeleteValueW(hKey, oldName.c_str());
        } else {
            st = ERROR_FILE_NOT_FOUND;
        }
        RegCloseKey(hKey);
        if (st == ERROR_SUCCESS) ReloadCurrent();
        else ShowMessage("重命名值", "重命名失败。");
    });
}

// ---------------------------------------------------------------------------
// Status / refresh / find / favorites
// ---------------------------------------------------------------------------
void RegeditPlusApp::ToggleStatusBar() {
    if (!m_statusBar) return;
    const Visibility vis = (m_statusBar->GetVisibility() == Visibility::Visible)
                               ? Visibility::Collapsed
                               : Visibility::Visible;
    m_statusBar->SetVisibility(vis);
}

void RegeditPlusApp::ToggleTheme() {
    const ThemeMode currentMode = m_window.GetThemeMode();
    const ThemeMode nextMode = (currentMode == ThemeMode::Light) ? ThemeMode::Dark : ThemeMode::Light;
    ThemeManager::Instance().SetThemeSource(nextMode == ThemeMode::Dark ? ThemeSource::Dark : ThemeSource::Light);
    m_window.SetThemeMode(nextMode);
    ApplyChromeColors();
}

void RegeditPlusApp::ApplyChromeColors() {
    auto& tm = ThemeManager::Instance();
    if (m_root) {
        m_root->SetBackgroundToken(ThemeTokenId::WindowBackground);
        m_root->SetBackground(tm.GetColor(ThemeTokenId::WindowBackground));
    }
    if (m_tree) {
        m_tree->SetBackgroundToken(ThemeTokenId::PaneBackground);
        m_tree->SetBackground(tm.GetColor(ThemeTokenId::PaneBackground));
        m_tree->MarkRenderContentDirty();
    }
    if (m_list) {
        m_list->SetBackground(tm.GetColor(ThemeTokenId::WindowBackground));
        m_list->OnThemeChanged();
    }
    if (m_statusBar) {
        m_statusBar->SetBackgroundToken(ThemeTokenId::PaneBackground);
        m_statusBar->SetBackground(tm.GetColor(ThemeTokenId::PaneBackground));
        m_statusBar->SetBorderBrush(tm.GetColor(ThemeTokenId::CardBorder));
    }
    if (m_statusPath) {
        m_statusPath->SetColorToken(ThemeTokenId::TextSecondary);
        m_statusPath->SetColor(tm.GetColor(ThemeTokenId::TextSecondary));
    }
    if (m_titleBar) {
        m_titleBar->MarkRenderContentDirty();
    }
    if (m_breadcrumb) {
        m_breadcrumb->SetBackgroundToken(ThemeTokenId::PaneBackground);
        m_breadcrumb->SetBackground(tm.GetColor(ThemeTokenId::PaneBackground));
        m_breadcrumb->SetBorderBrush(tm.GetColor(ThemeTokenId::CardBorder));
        m_breadcrumb->SetColor(tm.GetColor(ThemeTokenId::TextSecondary));
        m_breadcrumb->MarkRenderContentDirty();
    }
}

void RegeditPlusApp::ApplyNativeRegeditIcon() {
    wchar_t windir[MAX_PATH] = {};
    if (GetWindowsDirectoryW(windir, MAX_PATH) == 0) return;
    std::wstring path = windir;
    path += L"\\regedit.exe";

    HICON largeIcon = nullptr;
    HICON smallIcon = nullptr;
    const UINT extracted = ExtractIconExW(path.c_str(), 0, &largeIcon, &smallIcon, 1);
    if (extracted == 0) {
        // Fallback: shell association icon for .reg / regedit
        SHFILEINFOW sfi = {};
        if (SHGetFileInfoW(path.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON)) {
            smallIcon = sfi.hIcon;
        }
    }

    if (HWND hwnd = m_window.GetHWND()) {
        if (largeIcon) {
            SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(largeIcon));
        }
        if (smallIcon) {
            SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(smallIcon));
        }
    }

    // Title-bar badge prefers the small icon; take ownership of a copy.
    HICON titleIcon = nullptr;
    if (smallIcon) {
        titleIcon = CopyIcon(smallIcon);
    } else if (largeIcon) {
        titleIcon = CopyIcon(largeIcon);
    }
    if (m_titleBar && titleIcon) {
        m_titleBar->SetNativeIcon(titleIcon, true);
    }

    // ExtractIconEx icons are owned by us unless assigned to the window.
    // Window keeps the icons via WM_SETICON; do not Destroy the ones we sent.
    // If CopyIcon failed and we only have extracted handles already given to
    // WM_SETICON, leave them alone.
    if (!titleIcon) {
        // nothing drawn in title bar
    }
}

void RegeditPlusApp::RefreshCurrent() {
    if (!HasValidKey()) return;
    auto it = m_pathNodes.find(m_selectedPath);
    if (it != m_pathNodes.end()) {
        if (auto node = it->second.lock()) {
            m_nodeLoaded[node.get()] = false;
            node->children.clear();
            PopulateSubkeys(node);
            EnsureExpanded(node);
            RefreshTreeVisual();
        }
    }
    ReloadCurrent();
}

void RegeditPlusApp::ShowFindDialog() {
    if (!HasValidKey()) {
        ShowMessage("查找", "请先选中一个查找起始键。");
        return;
    }
    PromptText("查找", "查找内容:", m_findQuery, false, [this](const std::wstring& needle) {
        m_findQuery = needle;
        m_findNextStart = m_selectedPath;
        FindNext();
    });
}

void RegeditPlusApp::FindNext() {
    const std::wstring needle = ToLower(m_findQuery);
    if (needle.empty()) {
        ShowMessage("查找", "请输入要查找的内容。");
        return;
    }

    auto walkStart = m_findNextStart.empty() ? m_selectedPath : m_findNextStart;

    // Recursively walk the hives with a simple pre-order DFS.
    std::function<bool(const std::wstring&, std::wstring&)> walk =
        [&](const std::wstring& start, std::wstring& result) -> bool {
        HKEY hKey = nullptr;
        if (!OpenKey(start, KEY_READ, &hKey)) return false;

        bool found = false;
        const size_t slash = start.find_last_of(L'\\');
        const std::wstring leaf = (slash == std::wstring::npos) ? start : start.substr(slash + 1);
        if (ToLower(leaf).find(needle) != std::wstring::npos) {
            result = start;
            found = true;
        }
        if (!found) {
            DWORD valueCount = 0, maxNameLen = 0;
            if (RegQueryInfoKeyW(hKey, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                 &valueCount, &maxNameLen, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
                std::vector<wchar_t> nameBuf(maxNameLen > 0 ? maxNameLen + 2 : 260);
                for (DWORD i = 0; i < valueCount && !found; ++i) {
                    DWORD nameLen = static_cast<DWORD>(nameBuf.size());
                    DWORD type = 0;
                    if (RegEnumValueW(hKey, i, nameBuf.data(), &nameLen, nullptr, &type,
                                      nullptr, nullptr) == ERROR_SUCCESS) {
                        const std::wstring vname(nameBuf.data(), nameLen);
                        if (ToLower(vname).find(needle) != std::wstring::npos) {
                            result = start;
                            found = true;
                        }
                    }
                }
            }
        }
        if (!found) {
            for (const std::wstring& sub : EnumerateSubkeys(hKey)) {
                const std::wstring child = start + L"\\" + sub;
                if (walk(child, result)) {
                    found = true;
                    break;
                }
            }
        }
        RegCloseKey(hKey);
        return found;
    };

    const wchar_t* hives[] = { L"HKEY_CURRENT_USER", L"HKEY_CLASSES_ROOT", L"HKEY_CURRENT_CONFIG" };
    for (const wchar_t* hive : hives) {
        std::wstring result;
        if (walk(hive, result)) {
            m_selectedPath = result;
            m_findNextStart = result;
            m_findQuery = ToLower(m_findQuery);
            ReloadCurrent();
            RevealPath(result);
            return;
        }
    }
m_findNextStart.clear();
    ShowMessage("查找", "搜索完毕，未找到匹配项。");
}

void RegeditPlusApp::AddToFavorites() {
    if (!HasValidKey()) {
        ShowMessage("收藏夹", "请先选中一个键。");
        return;
    }
    for (const Favorite& fav : m_favorites) {
        if (fav.path == m_selectedPath) return;
    }
    m_favorites.push_back({ WideToUtf8(m_selectedPath), m_selectedPath });
    BuildMenus();
}

void RegeditPlusApp::ClearFavorites() {
    m_favorites.clear();
    BuildMenus();
}

// ---------------------------------------------------------------------------
// Dialog helpers
// ---------------------------------------------------------------------------
void RegeditPlusApp::ShowMessage(const std::string& title, const std::string& message) {
    if (m_root) {
        ContentDialog::ShowMessageBox(m_root.get(), title, message);
    }
}

void RegeditPlusApp::PromptText(const std::string& title, const std::string& label,
                                const std::wstring& initial, bool multiline,
                                std::function<void(const std::wstring&)> onOk) {
    if (!m_root || !onOk) return;
    ContentDialog::ShowInputBox(
        m_root.get(),
        title,
        label,
        WideToUtf8(initial),
        multiline,
        [onOk](DialogResult r, const std::string& text) {
            if (r != DialogResult::Primary) return;
            onOk(Utf8ToWide(text));
        });
}

} // namespace RegeditPlus
