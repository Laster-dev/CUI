#include "MainView.h"
#include "../manager/StartupManager.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/WindowTitleBar.h"
#include "framework/controls/NavigationView.h"
#include "framework/controls/NavigationViewItem.h"
#include "framework/controls/TreeView.h"
#include "framework/controls/StatusBar.h"
#include "framework/controls/TextBox.h"
#include "framework/controls/Button.h"
#include "framework/controls/ToggleSwitch.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/Panel.h"
#include "framework/controls/ScrollViewer.h"
#include "framework/controls/ContextMenu.h"
#include "framework/controls/ToastCenter.h"
#include "framework/controls/SegmentedControl.h"
#include "framework/style/ThemeTokenId.h"

#include <commdlg.h>
#include <shellapi.h>

using namespace CUI;
using namespace CUI::DSL;

namespace AutoGuard {

MainView::MainView(std::shared_ptr<MainViewModel> viewModel, Window* window)
    : m_viewModel(std::move(viewModel)), m_window(window) {
}

std::shared_ptr<UIElement> MainView::Build() {
    auto titleBar = std::make_shared<WindowTitleBar>();
    DSL::Borrow(titleBar).Title("AutoGuard - Windows 启动项安全管理专家");
    DSL::Borrow(titleBar).Height(38.0f);
    SetupMenuBar(titleBar);

    auto themeSwitch = std::make_shared<SegmentedControl>();
    DSL::Borrow(themeSwitch).Width(110.0f);
    themeSwitch->AddItem("Dark");
    themeSwitch->AddItem("Light");
    themeSwitch->OnSelectionChanged().Connect([this](SegmentedControl*, int, const std::string& item) {
        if (m_window) {
            DSL::Borrow(m_window).ThemeMode(item == "Dark" ? ThemeMode::Dark : ThemeMode::Light);
        }
    });
    auto titleRight = std::make_shared<StackPanel>();
    titleRight->Orientation = Orientation::Horizontal;
    titleRight->AddChild(themeSwitch);
    DSL::Borrow(titleBar).RightContent(titleRight);

    auto toolbar = BuildToolbar();
    m_navView = BuildNavigationView();
    m_detailStrip = BuildBottomDetailStrip();
    auto statusBar = BuildStatusBar();

    m_toastCenter = std::make_shared<ToastCenter>();
    DSL::Borrow(m_toastCenter).Id("toastCenter");

    auto root = Column(0.0f, {
        titleBar,
        toolbar,
        Expanded(m_navView),
        m_detailStrip,
        statusBar,
        m_toastCenter
    })
    .BackgroundToken(ThemeTokenId::WindowBackground)
    .Build();

    RefreshContent();
    UpdateStatusBar();
    UpdateDetailPanel();

    return root;
}

void MainView::SetupMenuBar(std::shared_ptr<WindowTitleBar> titleBar) {
    auto& menuBar = titleBar->GetMenuBar();

    // 1. File Menu
    auto fileMenu = menuBar.AddMenu("文件(F)");
    fileMenu->AddItem("保存 / 导出报告 (Ctrl+S)", [this]() {
        wchar_t szFile[MAX_PATH] = L"AutoGuard_Report.csv";
        OPENFILENAMEW ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = m_window ? m_window->GetHWND() : nullptr;
        ofn.lpstrFilter = L"CSV 逗号分隔表格 (*.csv)\0*.csv\0文本文件 (*.txt)\0*.txt\0所有文件 (*.*)\0*.*\0";
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = ARRAYSIZE(szFile);
        ofn.lpstrDefExt = L"csv";
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
        if (GetSaveFileNameW(&ofn)) {
            std::string msg;
            if (m_viewModel->Export(szFile, msg)) {
                ShowToast("报告已成功导出！");
            } else {
                ShowToast("导出失败: " + msg);
            }
        }
    });
    fileMenu->AddSeparator();
    fileMenu->AddItem("退出(X)", "Alt+F4", [this]() {
        if (m_window && m_window->GetHWND()) {
            PostMessage(m_window->GetHWND(), WM_CLOSE, 0, 0);
        }
    });

    // 2. Search Menu
    auto searchMenu = menuBar.AddMenu("搜索(S)");
    searchMenu->AddItem("在 VirusTotal 在线查询 (Ctrl+V)", [this]() {
        if (m_viewModel->SearchOnline()) {
            ShowToast("正在打开 VirusTotal 在线分析…");
        } else {
            ShowToast("请先选择一个启动项。");
        }
    });

    // 3. Entry Operation Menu
    auto entryMenu = menuBar.AddMenu("操作(E)");
    entryMenu->AddItem("重新扫描 (F5)", [this]() {
        m_viewModel->StartScan([this]() {
            RefreshContent();
            UpdateStatusBar();
            UpdateDetailPanel();
            ShowToast("全盘扫描完成！");
        });
        UpdateStatusBar();
        ShowToast("正在后台扫描启动项…");
    });
    entryMenu->AddItem("切换 启用 / 禁用 (Space)", [this]() {
        std::string msg;
        if (m_viewModel->ToggleSelectedStatus(msg)) {
            RefreshContent();
            UpdateStatusBar();
            UpdateDetailPanel();
            ShowToast(msg);
        } else {
            ShowToast(msg);
        }
    });
    entryMenu->AddItem("删除条目 (Del)", [this]() {
        std::string msg;
        if (m_viewModel->DeleteSelected(msg)) {
            RefreshContent();
            UpdateStatusBar();
            UpdateDetailPanel();
            ShowToast(msg);
        } else {
            ShowToast(msg);
        }
    });
    entryMenu->AddSeparator();
    entryMenu->AddItem("转到映像所在目录 (Ctrl+E)", [this]() {
        if (!m_viewModel->JumpToImage()) {
            ShowToast("目标文件不存在或无法打开所在目录。");
        }
    });
    entryMenu->AddItem("转到注册表 / 系统条目 (Ctrl+M)", [this]() {
        if (!m_viewModel->JumpToEntry()) {
            ShowToast("该条目无法直接定位注册表或系统。");
        }
    });
    entryMenu->AddSeparator();
    entryMenu->AddItem("一键清理所有失效项", [this]() {
        size_t count = 0;
        if (m_viewModel->CleanAllMissing(count)) {
            RefreshContent();
            UpdateStatusBar();
            UpdateDetailPanel();
            ShowToast("已清理 " + std::to_string(count) + " 个失效残留项！");
        } else {
            ShowToast("未发现可清理的失效项。");
        }
    });

    // 4. Options Menu
    auto optMenu = menuBar.AddMenu("选项(O)");
    optMenu->AddItem("深色模式", [this]() {
        if (m_window) DSL::Borrow(m_window).ThemeMode(ThemeMode::Dark);
    });
    optMenu->AddItem("浅色模式", [this]() {
        if (m_window) DSL::Borrow(m_window).ThemeMode(ThemeMode::Light);
    });
    optMenu->AddSeparator();
    optMenu->AddItem("低功耗 / 省电渲染模式", [this]() {
        if (m_window) {
            bool current = m_window->IsLowPerformanceMode();
            DSL::Borrow(m_window).LowPerformanceMode(!current);
            ShowToast(!current ? "已开启低功耗渲染模式。" : "已关闭低功耗模式。");
        }
    });

    // 5. Help Menu
    auto helpMenu = menuBar.AddMenu("帮助(H)");
    helpMenu->AddItem("关于 AutoGuard...", [this]() {
        ShowToast("AutoGuard v1.2 - Windows 启动项安全管理专家\n基于 CUI Direct2D 纯 C++ 声明式架构");
    });
}

std::shared_ptr<UIElement> MainView::BuildToolbar() {
    auto makeToolBtn = [](const std::string& label, std::function<void()> onClick) {
        auto btn = std::make_shared<Button>(label);
        DSL::Borrow(btn).Height(28.0f);
        CUI::DSL::Borrow(btn).Margin(Thickness(2, 2, 2, 2));
        btn->OnClick().Connect([onClick](UIElement*) { onClick(); });
        return btn;
    };

    auto btnRefresh = makeToolBtn("🔄 刷新 (F5)", [this]() {
        m_viewModel->StartScan([this]() {
            RefreshContent();
            UpdateStatusBar();
            UpdateDetailPanel();
            ShowToast("扫描已完成！");
        });
        UpdateStatusBar();
    });

    auto btnToggle = makeToolBtn("⏻ 切换启用", [this]() {
        std::string msg;
        if (m_viewModel->ToggleSelectedStatus(msg)) {
            RefreshContent();
            UpdateStatusBar();
            UpdateDetailPanel();
            ShowToast(msg);
        } else {
            ShowToast(msg);
        }
    });

    auto btnImage = makeToolBtn("📂 定位映像", [this]() {
        if (!m_viewModel->JumpToImage()) ShowToast("目标文件不存在或无法打开。");
    });

    auto btnEntry = makeToolBtn("▤ 定位注册表", [this]() {
        if (!m_viewModel->JumpToEntry()) ShowToast("无法直接定位该系统项。");
    });

    auto btnOnline = makeToolBtn("🌐 在线查毒", [this]() {
        if (m_viewModel->SearchOnline()) ShowToast("正在打开在线安全引擎查询…");
        else ShowToast("请先选择一个启动项。");
    });

    auto btnDelete = makeToolBtn("🗑 删除", [this]() {
        std::string msg;
        if (m_viewModel->DeleteSelected(msg)) {
            RefreshContent();
            UpdateStatusBar();
            UpdateDetailPanel();
            ShowToast(msg);
        } else {
            ShowToast(msg);
        }
    });

    auto btnClean = makeToolBtn("🧹 清理失效", [this]() {
        size_t count = 0;
        if (m_viewModel->CleanAllMissing(count)) {
            RefreshContent();
            UpdateStatusBar();
            UpdateDetailPanel();
            ShowToast("已清理 " + std::to_string(count) + " 个失效残留项！");
        } else {
            ShowToast("未检测到失效残留项。");
        }
    });

    auto btnExport = makeToolBtn("💾 导出", [this]() {
        wchar_t szFile[MAX_PATH] = L"AutoGuard_Report.csv";
        OPENFILENAMEW ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = m_window ? m_window->GetHWND() : nullptr;
        ofn.lpstrFilter = L"CSV 逗号分隔表格 (*.csv)\0*.csv\0所有文件 (*.*)\0*.*\0";
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = ARRAYSIZE(szFile);
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
        if (GetSaveFileNameW(&ofn)) {
            std::string msg;
            if (m_viewModel->Export(szFile, msg)) ShowToast("报告已导出！");
            else ShowToast("导出失败: " + msg);
        }
    });

    m_searchBox = std::make_shared<TextBox>();
    DSL::Borrow(m_searchBox).Placeholder("🔍 快速过滤器…");
    DSL::Borrow(m_searchBox).Width(240.0f);
    DSL::Borrow(m_searchBox).Height(26.0f);
    CUI::DSL::Borrow(m_searchBox).Margin(Thickness(4, 2, 8, 2));
    m_searchBox->OnTextChanged().Connect([this](TextBox*, const std::string& text) {
        m_viewModel->SetFilterText(text);
        RefreshContent();
        UpdateStatusBar();
        UpdateDetailPanel();
    });

    auto bar = Row(4.0f, {
        btnRefresh,
        btnToggle,
        btnImage,
        btnEntry,
        btnOnline,
        btnDelete,
        btnClean,
        btnExport,
        Expanded(Row(0, {}).Build()),
        m_searchBox
    })
    .Padding(Thickness(6, 4, 6, 4))
    .BackgroundToken(ThemeTokenId::CardBackground)
    .BorderToken(ThemeTokenId::CardBorder)
    .BorderThickness(1.0f)
    .Build();

    return bar;
}

std::shared_ptr<NavigationView> MainView::BuildNavigationView() {
    auto nav = std::make_shared<NavigationView>();
    DSL::Borrow(nav).PaneDisplayMode(NavigationViewPaneDisplayMode::Left);
    DSL::Borrow(nav).OpenPaneLength(220.0f);
    DSL::Borrow(nav).CompactPaneLength(48.0f);
    DSL::Borrow(nav).Align(Alignment::Stretch);
    DSL::Borrow(nav).FlexGrow(1.0f);

    auto addNavItem = [nav](const std::string& title, const std::string& icon, const std::string& tag) {
        auto item = std::make_shared<NavigationViewItem>(title, icon);
        item->SetTag(tag);
        nav->AddMenuItem(item);
        return item;
    };

    addNavItem("安全总览", "🛡", "overview");
    addNavItem("全部启动项", "🗂", "all");
    nav->AddMenuItem(std::make_shared<NavigationViewItemSeparator>());

    addNavItem("登录自启动", "👤", "logon");
    addNavItem("资源管理器 & COM", "📁", "explorer");
    addNavItem("计划任务", "◷", "tasks");
    addNavItem("系统与驱动服务", "⚙", "services");
    addNavItem("镜像劫持 (IFEO)", "⚠", "ifeo");
    addNavItem("Winlogon 挂钩", "🔑", "winlogon");
    addNavItem("已知 DLL & AppInit", "📦", "dlls");
    addNavItem("Winsock & 网络", "🔌", "network");
    addNavItem("WMI 永久订阅", "◈", "wmi");

    auto settingsItem = std::make_shared<NavigationViewItem>("设置与关于", "⚙");
    settingsItem->SetTag("settings");
    nav->AddFooterMenuItem(settingsItem);

    nav->OnItemInvoked().Connect([this](NavigationView*, const NavigationViewItemInvokedEventArgs& args) {
        if (args.InvokedItem) {
            std::string tag = args.InvokedItem->GetTag();
            if (!tag.empty()) {
                m_currentNavTag = tag;
                RefreshContent();
                UpdateStatusBar();
                UpdateDetailPanel();
            }
        }
    });

    return nav;
}

void MainView::RefreshContent() {
    if (!m_navView) return;

    if (m_currentNavTag == "overview") {
        m_navView->SetContent(BuildOverviewPage());
    } else if (m_currentNavTag == "settings") {
        m_navView->SetContent(BuildSettingsPage());
    } else {
        // Map tag to category
        StartupCategory cat = StartupCategory::All;
        std::string title = "全部启动项";
        std::string desc = "展示 Windows 系统注册表、服务、任务等所有自启动入口。";

        if (m_currentNavTag == "all") {
            cat = StartupCategory::All;
            title = "全部启动项 (Everything)";
            desc = "聚合全系统所有已检测到的自启入口，支持树形展开、快速搜索与右键操作。";
        } else if (m_currentNavTag == "logon") {
            cat = StartupCategory::Logon;
            title = "登录启动 (Logon)";
            desc = "HKCU/HKLM Run、RunOnce、物理启动文件夹与 Active Setup 登录自启组件。";
        } else if (m_currentNavTag == "explorer") {
            cat = StartupCategory::Explorer;
            title = "资源管理器 & COM 扩展 (Explorer)";
            desc = "Windows 资源管理器已批准扩展 (Approved Shell Extensions) 与右键菜单处理程序。";
        } else if (m_currentNavTag == "tasks") {
            cat = StartupCategory::ScheduledTasks;
            title = "计划任务 (Task Scheduler)";
            desc = "Windows 系统与第三方定时计划任务及开机自启触发器。";
        } else if (m_currentNavTag == "services") {
            cat = StartupCategory::Services;
            title = "系统与驱动服务 (Services & Drivers)";
            desc = "服务控制管理器 (SCM) 中配置为自动启动的 Windows 服务与内核驱动。";
        } else if (m_currentNavTag == "ifeo") {
            cat = StartupCategory::ImageHijacks;
            title = "镜像劫持 (IFEO)";
            desc = "Image File Execution Options 映像文件执行选项调试器劫持重定向检测。";
        } else if (m_currentNavTag == "winlogon") {
            cat = StartupCategory::Winlogon;
            title = "Winlogon 登录挂钩";
            desc = "Userinit、Shell、Taskman 核心登录阶段挂钩与用户外壳程序。";
        } else if (m_currentNavTag == "dlls") {
            cat = StartupCategory::KnownDlls;
            title = "已知 DLL & AppInit";
            desc = "系统 KnownDLLs 核心库列表与 AppInit_DLLs 全局注入入口。";
        } else if (m_currentNavTag == "network") {
            cat = StartupCategory::Winsock;
            title = "Winsock 与网络提供商";
            desc = "Winsock 命名空间目录与网络凭据提供程序。";
        } else if (m_currentNavTag == "wmi") {
            cat = StartupCategory::Wmi;
            title = "WMI 永久事件订阅";
            desc = "Windows Management Instrumentation 永久事件过滤器与使用者绑定。";
        }

        m_viewModel->SetCategory(cat);
        m_navView->SetContent(BuildTreeCategoryPage(title, desc));
    }
}

std::shared_ptr<UIElement> MainView::BuildOverviewPage() {
    const auto& summary = m_viewModel->GetSummary();
    const size_t total = summary.entries.size();
    const size_t enabled = summary.enabledCount;
    const size_t disabled = summary.disabledCount;
    const size_t missing = summary.missingCount;
    const size_t risks = summary.suspiciousCount + summary.highRiskCount;

    auto makeCard = [](const std::string& title, const std::string& val, const std::string& sub, Color col) {
        return Column(4.0f, {
            Text(title).FontSize(12.0f).ForegroundToken(ThemeTokenId::TextSecondary),
            Text(val).FontSize(26.0f).FontWeight(FontWeight::SemiBold).Foreground(col),
            Text(sub).FontSize(11.0f).ForegroundToken(ThemeTokenId::TextMuted)
        })
        .MinWidth(170.0f)
        .Padding(14.0f)
        .CornerRadius(6.0f)
        .BackgroundToken(ThemeTokenId::CardBackground)
        .BorderToken(ThemeTokenId::CardBorder)
        .BorderThickness(1.0f)
        .Build();
    };

    auto statRow = Row(10.0f, {
        makeCard("全系统启动项", std::to_string(total), "系统全部注册持久化", Color::Hex("#0078D4")),
        makeCard("已启用项目", std::to_string(enabled), "开机自动加载运行", Color::Hex("#107C41")),
        makeCard("潜在安全风险", std::to_string(risks), risks > 0 ? "需重点排查确认" : "未发现高危项", risks > 0 ? Color::Hex("#E81123") : Color::Hex("#107C41")),
        makeCard("目标失效残留", std::to_string(missing), missing > 0 ? "建议清理无效项" : "文件均有效", missing > 0 ? Color::Hex("#FF8C00") : Color::Hex("#107C41")),
        makeCard("已禁用项", std::to_string(disabled), "已拦截自启生效", Color::Hex("#767676"))
    }).Build();

    auto header = Column(4.0f, {
        Text("🛡 AutoGuard 启动项安全体检中心").FontSize(22.0f).FontWeight(FontWeight::SemiBold).ForegroundToken(ThemeTokenId::TextPrimary),
        Text("基于 Windows 架构深度检测全盘 19 大自启与持久化入口状态。").FontSize(13.0f).ForegroundToken(ThemeTokenId::TextSecondary)
    }).Build();

    auto makeEntranceCard = [this](const std::string& icon, const std::string& title, const std::string& desc, const std::string& tag) {
        auto card = Row(10.0f, {
            Text(icon).FontSize(22.0f).Width(28.0f),
            Column(2.0f, {
                Text(title).FontSize(14.0f).FontWeight(FontWeight::SemiBold).ForegroundToken(ThemeTokenId::TextPrimary),
                Text(desc).FontSize(11.0f).ForegroundToken(ThemeTokenId::TextSecondary)
            }).Build()
        })
        .MinWidth(240.0f)
        .Padding(12.0f)
        .CornerRadius(6.0f)
        .BackgroundToken(ThemeTokenId::CardBackground)
        .BorderToken(ThemeTokenId::CardBorder)
        .BorderThickness(1.0f)
        .Build();

        auto go = [this, tag](UIElement*) {
            m_currentNavTag = tag;
            RefreshContent();
        };
        card->OnClick().Connect(go);
        for (const auto& c : card->GetChildren()) {
            if (c) c->OnClick().Connect(go);
        }
        return card;
    };

    auto entranceWrap = std::make_shared<WrapPanel>(Orientation::Horizontal);
    DSL::Borrow(entranceWrap).Gap(10.0f);
    DSL::Borrow(entranceWrap).Align(Alignment::Stretch);

    entranceWrap->AddChild(makeEntranceCard("👤", "登录自启动", "HKCU/HKLM Run & 物理启动文件夹", "logon"));
    entranceWrap->AddChild(makeEntranceCard("📁", "资源管理器 & COM", "Shell 扩展与右键菜单项", "explorer"));
    entranceWrap->AddChild(makeEntranceCard("◷", "计划任务", "Task Scheduler 开机与定时任务", "tasks"));
    entranceWrap->AddChild(makeEntranceCard("⚙", "系统与驱动服务", "SCM 服务与自启驱动程序", "services"));
    entranceWrap->AddChild(makeEntranceCard("⚠", "镜像劫持 (IFEO)", "映像文件执行重定向与调试器", "ifeo"));
    entranceWrap->AddChild(makeEntranceCard("🔑", "Winlogon 挂钩", "Userinit / Shell 登录外壳挂钩", "winlogon"));
    entranceWrap->AddChild(makeEntranceCard("📦", "已知 DLL & AppInit", "KnownDLLs 与 AppInit 注入", "dlls"));
    entranceWrap->AddChild(makeEntranceCard("🔌", "Winsock & 网络", "网络提供程序与命名空间", "network"));

    auto content = Column(16.0f, {
        header,
        statRow,
        Text("全入口快捷通道").FontSize(15.0f).FontWeight(FontWeight::SemiBold).ForegroundToken(ThemeTokenId::TextPrimary),
        entranceWrap
    })
    .Padding(20.0f)
    .BackgroundToken(ThemeTokenId::WindowBackground)
    .Build();

    auto scroll = std::make_shared<ScrollViewer>();
    DSL::Borrow(scroll).Align(Alignment::Stretch);
    DSL::Borrow(scroll).FlexGrow(1.0f);
    DSL::Borrow(scroll).AddChild(content);
    return scroll;
}

std::shared_ptr<UIElement> MainView::BuildTreeCategoryPage(const std::string& title, const std::string& desc) {
    m_treeView = std::make_shared<TreeView>();
    DSL::Borrow(m_treeView).Align(Alignment::Stretch);
    DSL::Borrow(m_treeView).FlexGrow(1.0f);
    DSL::Borrow(m_treeView).IndentWidth(20.0f);

    // Build TreeView items
    const auto groups = m_viewModel->GetTreeGroups();
    std::vector<std::shared_ptr<TreeViewItem>> rootItems;

    for (const auto& grp : groups) {
        auto groupItem = std::make_shared<TreeViewItem>();
        groupItem->icon = grp.icon;
        groupItem->header = grp.title + "  (" + std::to_string(grp.entries.size()) + " 项)";
        groupItem->isExpanded = true;

        for (const auto& e : grp.entries) {
            auto childItem = std::make_shared<TreeViewItem>();
            childItem->tag = e.id;
            
            std::string statusPrefix;
            if (e.status == StartupStatus::Enabled) statusPrefix = "[✔] ";
            else if (e.status == StartupStatus::Disabled) statusPrefix = "[  ] ";
            else if (e.status == StartupStatus::Missing) statusPrefix = "[✕] ";
            else statusPrefix = "[●] ";

            std::string nodeText = statusPrefix + (e.name.empty() ? "(未命名)" : e.name);
            if (!e.description.empty() && e.description != e.name) {
                nodeText += "  —  " + e.description;
            }
            if (!e.publisher.empty()) {
                nodeText += "  |  " + e.publisher;
            }
            if (!e.fileExists && !e.command.empty()) {
                nodeText += "  |  [File not found: " + e.command + "]";
                childItem->icon = "⚠";
            } else {
                childItem->icon = (e.status == StartupStatus::Enabled ? "●" : (e.status == StartupStatus::Disabled ? "○" : "✕"));
            }

            childItem->header = nodeText;
            childItem->parent = groupItem.get();
            groupItem->children.push_back(childItem);
        }
        rootItems.push_back(groupItem);
    }

    m_treeView->SetItems(rootItems);

    // Selection & double click
    m_treeView->OnSelectionChanged().Connect([this](TreeView*, std::shared_ptr<TreeViewItem> item) {
        if (item && !item->tag.empty()) {
            m_viewModel->SetSelectedId(item->tag);
        } else {
            m_viewModel->SetSelectedId("");
        }
        UpdateDetailPanel();
        UpdateStatusBar();
    });

    m_treeView->OnItemDoubleClicked().Connect([this](TreeView*, std::shared_ptr<TreeViewItem> item) {
        if (item && !item->tag.empty()) {
            m_viewModel->SetSelectedId(item->tag);
            m_viewModel->JumpToImage();
        }
    });

    // Rich Context Menu
    auto contextMenu = std::make_shared<ContextMenu>();
    contextMenu->AddItem("📂 转到映像路径 (Ctrl+E)", [this]() {
        if (!m_viewModel->JumpToImage()) ShowToast("目标文件不存在或无法打开。");
    });
    contextMenu->AddItem("▤ 转到注册表 / 系统条目 (Ctrl+M)", [this]() {
        if (!m_viewModel->JumpToEntry()) ShowToast("无法直接定位该系统项。");
    });
    contextMenu->AddItem("🌐 在线查毒 (VirusTotal) (Ctrl+V)", [this]() {
        if (m_viewModel->SearchOnline()) ShowToast("正在打开在线安全引擎查询…");
        else ShowToast("请先选择一个启动项。");
    });
    contextMenu->AddSeparator();
    contextMenu->AddItem("📋 复制条目名称", [this]() {
        const auto* e = m_viewModel->GetSelectedEntry();
        if (e && m_window) {
            StartupManager::CopyToClipboard(m_window->GetHWND(), e->name);
            ShowToast("已复制条目名称到剪贴板！");
        }
    });
    contextMenu->AddItem("📋 复制映像路径", [this]() {
        if (m_window && m_viewModel->CopyPath(m_window->GetHWND())) {
            ShowToast("已复制映像路径到剪贴板！");
        }
    });
    contextMenu->AddItem("📋 复制完整启动命令", [this]() {
        if (m_window && m_viewModel->CopyCommand(m_window->GetHWND())) {
            ShowToast("已复制完整命令到剪贴板！");
        }
    });
    contextMenu->AddSeparator();
    contextMenu->AddItem("⏻ 切换 启用 / 禁用 (Space)", [this]() {
        std::string msg;
        if (m_viewModel->ToggleSelectedStatus(msg)) {
            RefreshContent();
            UpdateStatusBar();
            UpdateDetailPanel();
            ShowToast(msg);
        } else {
            ShowToast(msg);
        }
    });
    contextMenu->AddItem("🗑 删除条目 (Del)", [this]() {
        std::string msg;
        if (m_viewModel->DeleteSelected(msg)) {
            RefreshContent();
            UpdateStatusBar();
            UpdateDetailPanel();
            ShowToast(msg);
        } else {
            ShowToast(msg);
        }
    });
    contextMenu->AddSeparator();
    contextMenu->AddItem("🧹 清理所有失效项", [this]() {
        size_t count = 0;
        if (m_viewModel->CleanAllMissing(count)) {
            RefreshContent();
            UpdateStatusBar();
            UpdateDetailPanel();
            ShowToast("已清理 " + std::to_string(count) + " 个失效残留项！");
        } else {
            ShowToast("未检测到失效项。");
        }
    });
    contextMenu->AddItem("🔄 刷新全盘 (F5)", [this]() {
        m_viewModel->StartScan([this]() {
            RefreshContent();
            UpdateStatusBar();
            UpdateDetailPanel();
            ShowToast("扫描已完成！");
        });
        UpdateStatusBar();
    });
    m_treeView->SetContextMenu(contextMenu);

    auto header = Column(2.0f, {
        Text(title).FontSize(18.0f).FontWeight(FontWeight::SemiBold).ForegroundToken(ThemeTokenId::TextPrimary),
        Text(desc).FontSize(12.0f).ForegroundToken(ThemeTokenId::TextSecondary)
    }).Build();

    auto pageLayout = Column(8.0f, {
        header,
        Expanded(m_treeView)
    })
    .Padding(14.0f)
    .Align(Alignment::Stretch)
    .FlexGrow(1.0f)
    .BackgroundToken(ThemeTokenId::WindowBackground)
    .Build();

    return pageLayout;
}

std::shared_ptr<UIElement> MainView::BuildSettingsPage() {
    auto makeSettingCard = [](const std::string& t, const std::string& d, const std::shared_ptr<UIElement>& r) {
        return Row(12.0f, {
            Column(3.0f, {
                Text(t).FontSize(14.0f).FontWeight(FontWeight::Medium).ForegroundToken(ThemeTokenId::TextPrimary),
                Text(d).FontSize(12.0f).ForegroundToken(ThemeTokenId::TextSecondary)
            }).Build(),
            Expanded(Row(0, {}).Build()),
            r
        })
        .Padding(14.0f)
        .CornerRadius(6.0f)
        .BackgroundToken(ThemeTokenId::CardBackground)
        .BorderToken(ThemeTokenId::CardBorder)
        .BorderThickness(1.0f)
        .Build();
    };

    auto btnDark = std::make_shared<Button>("🌙 深色模式");
    DSL::Borrow(btnDark).Height(30.0f);
    btnDark->OnClick().Connect([this](UIElement*) {
        if (m_window) DSL::Borrow(m_window).ThemeMode(ThemeMode::Dark);
    });

    auto btnLight = std::make_shared<Button>("☀ 浅色模式");
    DSL::Borrow(btnLight).Height(30.0f);
    btnLight->OnClick().Connect([this](UIElement*) {
        if (m_window) DSL::Borrow(m_window).ThemeMode(ThemeMode::Light);
    });

    auto themeRow = Row(6.0f, { btnDark, btnLight }).Build();
    auto themeCard = makeSettingCard("界面视觉外观", "切换 AutoGuard 深色 / 浅色模式", themeRow);

    auto lowPerfSwitch = std::make_shared<ToggleSwitch>();
    if (m_window) lowPerfSwitch->SetIsOn(m_window->IsLowPerformanceMode());
    lowPerfSwitch->OnToggled().Connect([this](ToggleSwitch*, bool on) {
        if (m_window) {
            DSL::Borrow(m_window).LowPerformanceMode(on);
            ShowToast(on ? "已开启低功耗渲染模式。" : "已关闭低功耗模式。");
        }
    });
    auto lowPerfCard = makeSettingCard("低功耗 / 省电渲染模式", "降低空闲动画帧率，延长移动设备电池续航", lowPerfSwitch);

    auto aboutCard = Column(6.0f, {
        Text("🛡 AutoGuard 启动项安全管理专家 v1.2 Release").FontSize(15.0f).FontWeight(FontWeight::SemiBold).ForegroundToken(ThemeTokenId::TextPrimary),
        Text("基于 CUI Direct2D 纯 C++ 声明式 UI 引擎研发，面向 Windows 10/11 打造。").FontSize(12.0f).ForegroundToken(ThemeTokenId::TextSecondary),
        Text("全面监控注册表 Run/RunOnce、计划任务、系统服务、驱动、IFEO 镜像劫持、Winlogon 挂钩、KnownDLLs、Winsock 与 WMI 订阅。").FontSize(11.0f).ForegroundToken(ThemeTokenId::TextMuted)
    })
    .Padding(16.0f)
    .CornerRadius(6.0f)
    .BackgroundToken(ThemeTokenId::CardBackground)
    .BorderToken(ThemeTokenId::CardBorder)
    .BorderThickness(1.0f)
    .Build();

    auto content = Column(12.0f, {
        Text("设置与关于").FontSize(22.0f).FontWeight(FontWeight::SemiBold).ForegroundToken(ThemeTokenId::TextPrimary),
        themeCard,
        lowPerfCard,
        aboutCard
    })
    .Padding(20.0f)
    .BackgroundToken(ThemeTokenId::WindowBackground)
    .Build();

    auto scroll = std::make_shared<ScrollViewer>();
    DSL::Borrow(scroll).Align(Alignment::Stretch);
    DSL::Borrow(scroll).FlexGrow(1.0f);
    DSL::Borrow(scroll).AddChild(content);
    return scroll;
}

std::shared_ptr<UIElement> MainView::BuildBottomDetailStrip() {
    m_detailIcon = Text("🛡").FontSize(26.0f).Width(36.0f).Build();

    m_detailLine1Name = Text("").FontSize(13.0f).FontWeight(FontWeight::SemiBold).ForegroundToken(ThemeTokenId::TextPrimary).Build();
    m_detailLine1Size = Text("").FontSize(12.0f).ForegroundToken(ThemeTokenId::TextMuted).Build();
    auto line1 = Row(16.0f, { m_detailLine1Name, m_detailLine1Size }).Build();

    m_detailLine2Desc = Text("").FontSize(12.0f).ForegroundToken(ThemeTokenId::TextPrimary).Build();
    m_detailLine2Time = Text("").FontSize(12.0f).ForegroundToken(ThemeTokenId::TextMuted).Build();
    auto line2 = Row(16.0f, { m_detailLine2Desc, m_detailLine2Time }).Build();

    m_detailLine3Pub = Text("").FontSize(12.0f).Foreground(Color::Hex("#107C41")).Build();
    m_detailLine3Ver = Text("").FontSize(12.0f).ForegroundToken(ThemeTokenId::TextMuted).Build();
    auto line3 = Row(16.0f, { m_detailLine3Pub, m_detailLine3Ver }).Build();

    m_detailLine4Cmd = Text("").FontSize(12.0f).ForegroundToken(ThemeTokenId::TextSecondary).Build();

    auto infoCol = Column(2.0f, { line1, line2, line3, m_detailLine4Cmd }).Build();

    m_detailStrip = Row(12.0f, {
        m_detailIcon,
        infoCol
    })
    .Height(0.0f)
    .Opacity(0.0f)
    .Padding(Thickness(14, 6, 14, 6))
    .BackgroundToken(ThemeTokenId::CardBackground)
    .BorderToken(ThemeTokenId::CardBorder)
    .BorderThickness(1.0f)
    .Align(Alignment::Stretch)
    .Build();

    return m_detailStrip;
}

std::shared_ptr<UIElement> MainView::BuildStatusBar() {
    m_statusBar = std::make_shared<StatusBar>();
    m_statusLeftId = m_statusBar->AddTextItem("就绪", StatusBarItemAlignment::Left, 240.0f);
    m_statusCountId = m_statusBar->AddTextItem("条目数: 0", StatusBarItemAlignment::Left, 400.0f);
    m_statusSelectionId = m_statusBar->AddTextItem("未选择项目", StatusBarItemAlignment::Right, 260.0f);
    return m_statusBar;
}

void MainView::UpdateStatusBar() {
    if (!m_statusBar) return;
    m_statusBar->SetItemText(m_statusLeftId, m_viewModel->GetStatusLeftText());
    m_statusBar->SetItemText(m_statusCountId, m_viewModel->GetStatusCountText());
    m_statusBar->SetItemText(m_statusSelectionId, m_viewModel->GetStatusSelectionText());
}

void MainView::UpdateDetailPanel() {
    if (!m_detailStrip) return;

    const auto* entry = m_viewModel->GetSelectedEntry();
    if (!entry) {
        DSL::Borrow(m_detailStrip).Height(0.0f);
        DSL::Borrow(m_detailStrip).Opacity(0.0f);
        return;
    }

    DSL::Borrow(m_detailStrip).Height(78.0f);
    DSL::Borrow(m_detailStrip).Opacity(1.0f);

    ElementBuilder<TextBlock>(m_detailLine1Name).Text(entry->name);
    ElementBuilder<TextBlock>(m_detailLine1Size).Text(entry->fileSizeStr.empty() ? "" : ("大小: " + entry->fileSizeStr));
    
    ElementBuilder<TextBlock>(m_detailLine2Desc).Text(entry->description.empty() ? entry->name : entry->description);
    ElementBuilder<TextBlock>(m_detailLine2Time).Text(entry->fileTimestamp.empty() ? "" : ("时间: " + entry->fileTimestamp));

    std::string pubText = entry->publisher.empty() ? "(未验证签名)" : entry->publisher;
    ElementBuilder<TextBlock>(m_detailLine3Pub).Text(pubText);
    ElementBuilder<TextBlock>(m_detailLine3Ver).Text(entry->fileVersion.empty() ? "" : ("版本: " + entry->fileVersion));

    std::string cmdText = entry->command.empty() ? entry->executablePath : entry->command;
    ElementBuilder<TextBlock>(m_detailLine4Cmd).Text("\"" + cmdText + "\"");
}

void MainView::ShowToast(const std::string& message) {
    if (m_toastCenter) {
        m_toastCenter->ShowToast("AutoGuard", message, ToastType::Info, ToastCorner::BottomRight, 2400);
    }
}

} // namespace AutoGuard
