#include "MainView.h"
#include "OverviewView.h"
#include "TreeCategoryView.h"
#include "SettingsView.h"
#include "IconHelper.h"
#include "../manager/StartupManager.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/WindowTitleBar.h"
#include "framework/controls/NavigationViewItem.h"
#include "framework/controls/Panel.h"
#include "framework/controls/SegmentedControl.h"
#include "framework/style/ThemeTokenId.h"

#include <commdlg.h>
#include <shellapi.h>

using namespace CUI;
using namespace CUI::DSL;

namespace AutoGuard {

MainView::MainView(std::shared_ptr<MainViewModel> viewModel, Window* window)
    : m_viewModel(std::move(viewModel)), m_window(window) {
    m_detailStripView = std::make_shared<DetailStripView>(m_viewModel);
}

std::shared_ptr<UIElement> MainView::Build() {
    CUI::Widgets::Ref titleBar = CUI::Widgets::WindowTitleBar().Shared();
        titleBar.Title("AutoGuard - Windows 启动项安全管理专家");
        titleBar.Height(36.0f);

    m_navView = BuildNavigationView();
    auto detailElement = m_detailStripView->Build();
    auto statusBar = BuildStatusBar();

    m_toastCenter = std::make_shared<ToastCenter>();
        m_toastCenter->SetId("toastCenter");

    auto root = Column(0.0f, {
        titleBar,
        Expanded(m_navView),
        detailElement,
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

std::shared_ptr<NavigationView> MainView::BuildNavigationView() {
    auto nav = std::make_shared<NavigationView>();
        nav->SetPaneDisplayMode(NavigationViewPaneDisplayMode::Left);
        nav->SetOpenPaneLength(200.0f);
        nav->SetCompactPaneLength(44.0f);
        nav->SetIsSettingsVisible(false);
        nav->SetAlign(Alignment::Stretch);
        nav->SetFlexGrow(1.0f);

    auto addNavItem = [nav](const std::string& title, const std::string& icon, const std::string& tag) {
        auto item = std::make_shared<NavigationViewItem>(title, icon);
        item->SetTag(tag);
        nav->AddMenuItem(item);
        return item;
    };

    addNavItem("安全总览", "🛡", "overview");
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
        OverviewView overview(m_viewModel, [this](const std::string& tag) {
            m_currentNavTag = tag;
            RefreshContent();
        });
        m_navView->SetContent(overview.Build());
    } else if (m_currentNavTag == "settings") {
        SettingsView settings(m_window, [this](const std::string& msg) {
            ShowToast(msg);
        });
        m_navView->SetContent(settings.Build());
    } else {
        StartupCategory cat = StartupCategory::All;
        std::string title = "全部启动项";
        std::string desc = "聚合全系统所有已检测到的自启入口，支持树形展开与右键操作。";

        if (m_currentNavTag == "all") {
            cat = StartupCategory::All;
            title = "全部启动项 (Everything)";
            desc = "聚合全系统所有已检测到的自启入口，支持树形展开与右键操作。";
        } else if (m_currentNavTag == "logon") {
            cat = StartupCategory::Logon;
            title = "登录启动 (Logon)";
            desc = "HKCU/HKLM Run、RunOnce、物理启动文件夹与 Active Setup 登录自启组件。";
        } else if (m_currentNavTag == "explorer") {
            cat = StartupCategory::Explorer;
            title = "资源管理器 & COM 扩展 (Explorer)";
            desc = "Windows 资源管理器已批准扩展与右键菜单处理程序。";
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

        TreeCategoryView categoryView(
            m_viewModel,
            m_window,
            [this]() {
                UpdateDetailPanel();
                UpdateStatusBar();
            },
            [this](const std::string& msg) {
                ShowToast(msg);
            },
            [this]() {
                RefreshContent();
                UpdateStatusBar();
                UpdateDetailPanel();
            }
        );
        m_navView->SetContent(categoryView.Build(title, desc));
    }
}

std::shared_ptr<UIElement> MainView::BuildStatusBar() {
    m_statusBar = std::make_shared<StatusBar>();
    m_statusLeftId = m_statusBar->AddTextItem("就绪", StatusBarItemAlignment::Left, 200.0f);
    m_statusCountId = m_statusBar->AddTextItem("条目数: 0", StatusBarItemAlignment::Left, 380.0f);
    m_statusSelectionId = m_statusBar->AddTextItem("未选择项目", StatusBarItemAlignment::Right, 240.0f);
    return m_statusBar;
}

void MainView::UpdateStatusBar() {
    if (!m_statusBar) return;
    m_statusBar.ItemText(m_statusLeftId, m_viewModel->GetStatusLeftText());
    m_statusBar.ItemText(m_statusCountId, m_viewModel->GetStatusCountText());
    m_statusBar.ItemText(m_statusSelectionId, m_viewModel->GetStatusSelectionText());
}

void MainView::UpdateDetailPanel() {
    if (m_detailStripView) {
        m_detailStripView->Update();
    }
}

void MainView::ShowToast(const std::string& message) {
    if (m_toastCenter) {
        m_toastCenter->ShowToast("AutoGuard", message, ToastType::Info, ToastCorner::BottomRight, 2400);
    }
}

} // namespace AutoGuard
