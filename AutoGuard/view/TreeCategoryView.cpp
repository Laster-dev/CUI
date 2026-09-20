#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "TreeCategoryView.h"
#include "IconHelper.h"
#include "../manager/StartupManager.h"
#include "framework/controls/Expander.h"
#include "framework/controls/ListView.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/Panel.h"
#include "framework/controls/ScrollViewer.h"
#include "framework/controls/ContextMenu.h"
#include "framework/core/Widgets.h"
#include "framework/style/ThemeTokenId.h"

#include <vector>
#include <map>

using namespace CUI;
using namespace CUI::DSL;

namespace AutoGuard {

namespace {

std::string ToLower(std::string s) {
    for (char& c : s) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return s;
}

std::shared_ptr<ContextMenu> BuildSpecializedContextMenu(
    StartupLocation loc,
    std::shared_ptr<MainViewModel> viewModel,
    Window* window,
    std::function<void(const std::string&)> onShowToast,
    std::function<void()> onRefresh) {

    CUI::Widgets::Ref menu = CUI::Widgets::ContextMenu().Shared();

    // 1. Scheduled Tasks
    if (loc == StartupLocation::ScheduledTask) {
        menu->AddItem("▶ 立即运行此任务", [viewModel, onShowToast]() {
            const auto* e = viewModel->GetSelectedEntry();
            if (e) {
                std::string msg;
                if (StartupManager::RunScheduledTask(e->name, msg)) {
                    if (onShowToast) onShowToast(msg);
                } else if (onShowToast) {
                    onShowToast(msg);
                }
            }
        });
        menu->AddItem("◷ 打开任务计划程序 (taskschd.msc)", []() {
            StartupManager::OpenTaskScheduler();
        });
        menu->AddItem("📂 打开任务映像路径 (Ctrl+E)", [viewModel, onShowToast]() {
            if (!viewModel->JumpToImage() && onShowToast) onShowToast("目标文件不存在或无法打开。");
        });
        menu->AddSeparator();
        menu->AddItem("📋 复制任务名称", [viewModel, window, onShowToast]() {
            const auto* e = viewModel->GetSelectedEntry();
            if (e && window) {
                StartupManager::CopyToClipboard(window->GetHWND(), e->name);
                if (onShowToast) onShowToast("已复制任务名称到剪贴板！");
            }
        });
        menu->AddItem("📋 复制启动程序路径", [viewModel, window, onShowToast]() {
            if (window && viewModel->CopyPath(window->GetHWND())) {
                if (onShowToast) onShowToast("已复制路径到剪贴板！");
            }
        });
        menu->AddSeparator();
        menu->AddItem("⏻ 切换 启用 / 禁用 (Space)", [viewModel, onRefresh, onShowToast]() {
            std::string msg;
            if (viewModel->ToggleSelectedStatus(msg)) {
                if (onRefresh) onRefresh();
                if (onShowToast) onShowToast(msg);
            } else if (onShowToast) onShowToast(msg);
        });
        menu->AddItem("🗑 从计划任务中注销/删除 (Del)", [viewModel, onRefresh, onShowToast]() {
            std::string msg;
            if (viewModel->DeleteSelected(msg)) {
                if (onRefresh) onRefresh();
                if (onShowToast) onShowToast(msg);
            } else if (onShowToast) onShowToast(msg);
        });
        return menu;
    }

    // 2. Services & Drivers
    if (loc == StartupLocation::Service || loc == StartupLocation::Driver) {
        menu->AddItem("▶ 启动服务", [viewModel, onShowToast]() {
            const auto* e = viewModel->GetSelectedEntry();
            if (e) {
                std::string msg;
                StartupManager::ControlService(e->name, "start", msg);
                if (onShowToast) onShowToast(msg);
            }
        });
        menu->AddItem("⏹ 停止服务", [viewModel, onShowToast]() {
            const auto* e = viewModel->GetSelectedEntry();
            if (e) {
                std::string msg;
                StartupManager::ControlService(e->name, "stop", msg);
                if (onShowToast) onShowToast(msg);
            }
        });
        menu->AddItem("🔄 重启服务", [viewModel, onShowToast]() {
            const auto* e = viewModel->GetSelectedEntry();
            if (e) {
                std::string msg;
                StartupManager::ControlService(e->name, "restart", msg);
                if (onShowToast) onShowToast(msg);
            }
        });
        menu->AddItem("⚙ 打开服务管理器 (services.msc)", []() {
            StartupManager::OpenServiceManager();
        });
        menu->AddItem("📂 打开服务二进制所在路径 (Ctrl+E)", [viewModel, onShowToast]() {
            if (!viewModel->JumpToImage() && onShowToast) onShowToast("服务二进制文件不存在。");
        });
        menu->AddSeparator();
        menu->AddItem("📋 复制服务短名", [viewModel, window, onShowToast]() {
            const auto* e = viewModel->GetSelectedEntry();
            if (e && window) {
                StartupManager::CopyToClipboard(window->GetHWND(), e->name);
                if (onShowToast) onShowToast("已复制服务名到剪贴板！");
            }
        });
        menu->AddItem("📋 复制二进制命令行", [viewModel, window, onShowToast]() {
            if (window && viewModel->CopyCommand(window->GetHWND())) {
                if (onShowToast) onShowToast("已复制命令行到剪贴板！");
            }
        });
        menu->AddSeparator();
        menu->AddItem("⏻ 切换启动类型 (自动 / 禁用)", [viewModel, onRefresh, onShowToast]() {
            std::string msg;
            if (viewModel->ToggleSelectedStatus(msg)) {
                if (onRefresh) onRefresh();
                if (onShowToast) onShowToast(msg);
            } else if (onShowToast) onShowToast(msg);
        });
        return menu;
    }

    // 3. Startup Folder
    if (loc == StartupLocation::StartupFolder) {
        menu->AddItem("📂 打开快捷方式目标所在目录 (Ctrl+E)", [viewModel, onShowToast]() {
            if (!viewModel->JumpToImage() && onShowToast) onShowToast("目标文件不存在或无法打开。");
        });
        menu->AddItem("📁 打开当前用户自启文件夹", []() {
            StartupManager::OpenStartupFolder(true);
        });
        menu->AddItem("📁 打开全系统自启文件夹", []() {
            StartupManager::OpenStartupFolder(false);
        });
        menu->AddSeparator();
        menu->AddItem("📋 复制快捷方式目标路径", [viewModel, window, onShowToast]() {
            if (window && viewModel->CopyPath(window->GetHWND())) {
                if (onShowToast) onShowToast("已复制路径到剪贴板！");
            }
        });
        menu->AddSeparator();
        menu->AddItem("⏻ 切换 启用 / 禁用 (.disabled)", [viewModel, onRefresh, onShowToast]() {
            std::string msg;
            if (viewModel->ToggleSelectedStatus(msg)) {
                if (onRefresh) onRefresh();
                if (onShowToast) onShowToast(msg);
            } else if (onShowToast) onShowToast(msg);
        });
        menu->AddItem("🗑 删除快捷方式 (Del)", [viewModel, onRefresh, onShowToast]() {
            std::string msg;
            if (viewModel->DeleteSelected(msg)) {
                if (onRefresh) onRefresh();
                if (onShowToast) onShowToast(msg);
            } else if (onShowToast) onShowToast(msg);
        });
        return menu;
    }

    // 4. IFEO (Image File Execution Options)
    if (loc == StartupLocation::ImageHijack) {
        menu->AddItem("⚠ 清除镜像劫持调试器 (恢复默认)", [viewModel, onRefresh, onShowToast]() {
            std::string msg;
            if (viewModel->DeleteSelected(msg)) {
                if (onRefresh) onRefresh();
                if (onShowToast) onShowToast(msg);
            } else if (onShowToast) onShowToast(msg);
        });
        menu->AddItem("▤ 在注册表定位 IFEO 劫持项 (Ctrl+M)", [viewModel, onShowToast]() {
            if (!viewModel->JumpToEntry() && onShowToast) onShowToast("无法直接定位注册表。");
        });
        menu->AddItem("📂 打开被劫持程序目录 (Ctrl+E)", [viewModel, onShowToast]() {
            if (!viewModel->JumpToImage() && onShowToast) onShowToast("目标文件不存在。");
        });
        menu->AddSeparator();
        menu->AddItem("📋 复制调试器重定向命令", [viewModel, window, onShowToast]() {
            if (window && viewModel->CopyCommand(window->GetHWND())) {
                if (onShowToast) onShowToast("已复制重定向命令到剪贴板！");
            }
        });
        return menu;
    }

    // 5. Explorer / COM Extensions
    if (loc == StartupLocation::ComHijack) {
        menu->AddItem("📂 打开扩展 DLL 所在目录 (Ctrl+E)", [viewModel, onShowToast]() {
            if (!viewModel->JumpToImage() && onShowToast) onShowToast("目标 DLL 不存在。");
        });
        menu->AddItem("▤ 定位 CLSID 扩展注册表 (Ctrl+M)", [viewModel, onShowToast]() {
            if (!viewModel->JumpToEntry() && onShowToast) onShowToast("无法定位 CLSID 注册表项。");
        });
        menu->AddSeparator();
        menu->AddItem("📋 复制 CLSID 标识符", [viewModel, window, onShowToast]() {
            const auto* e = viewModel->GetSelectedEntry();
            if (e && window) {
                StartupManager::CopyToClipboard(window->GetHWND(), e->name);
                if (onShowToast) onShowToast("已复制 CLSID 到剪贴板！");
            }
        });
        menu->AddItem("📋 复制 DLL 模块路径", [viewModel, window, onShowToast]() {
            if (window && viewModel->CopyPath(window->GetHWND())) {
                if (onShowToast) onShowToast("已复制模块路径到剪贴板！");
            }
        });
        menu->AddSeparator();
        menu->AddItem("⏻ 切换 启用 / 禁用 (Approved)", [viewModel, onRefresh, onShowToast]() {
            std::string msg;
            if (viewModel->ToggleSelectedStatus(msg)) {
                if (onRefresh) onRefresh();
                if (onShowToast) onShowToast(msg);
            } else if (onShowToast) onShowToast(msg);
        });
        return menu;
    }

    // 6. Network & Winsock
    if (loc == StartupLocation::WinsockProvider) {
        menu->AddItem("🔌 打开网络连接设置 (ncpa.cpl)", []() {
            StartupManager::OpenNetworkConnections();
        });
        menu->AddItem("▤ 在注册表定位 Winsock 目录 (Ctrl+M)", [viewModel, onShowToast]() {
            if (!viewModel->JumpToEntry() && onShowToast) onShowToast("无法定位注册表。");
        });
        menu->AddItem("📂 打开网络提供程序模块目录 (Ctrl+E)", [viewModel, onShowToast]() {
            if (!viewModel->JumpToImage() && onShowToast) onShowToast("目标模块不存在。");
        });
        menu->AddSeparator();
        menu->AddItem("📋 复制提供程序路径", [viewModel, window, onShowToast]() {
            if (window && viewModel->CopyPath(window->GetHWND())) {
                if (onShowToast) onShowToast("已复制路径到剪贴板！");
            }
        });
        return menu;
    }

    // 7. Standard Registry Run / Default
    menu->AddItem("▤ 在注册表编辑器中定位 (Ctrl+M)", [viewModel, onShowToast]() {
        if (!viewModel->JumpToEntry() && onShowToast) onShowToast("该条目无法直接定位注册表。");
    });
    menu->AddItem("📂 打开映像所在目录 (Ctrl+E)", [viewModel, onShowToast]() {
        if (!viewModel->JumpToImage() && onShowToast) onShowToast("目标文件不存在或无法打开所在目录。");
    });
    menu->AddSeparator();
    menu->AddItem("📋 复制条目名称", [viewModel, window, onShowToast]() {
        const auto* e = viewModel->GetSelectedEntry();
        if (e && window) {
            StartupManager::CopyToClipboard(window->GetHWND(), e->name);
            if (onShowToast) onShowToast("已复制条目名称到剪贴板！");
        }
    });
    menu->AddItem("📋 复制映像完整路径", [viewModel, window, onShowToast]() {
        if (window && viewModel->CopyPath(window->GetHWND())) {
            if (onShowToast) onShowToast("已复制映像路径到剪贴板！");
        }
    });
    menu->AddItem("📋 复制启动命令行", [viewModel, window, onShowToast]() {
        if (window && viewModel->CopyCommand(window->GetHWND())) {
            if (onShowToast) onShowToast("已复制完整命令到剪贴板！");
        }
    });
    menu->AddSeparator();
    menu->AddItem("⏻ 切换 启用 / 禁用 (Space)", [viewModel, onRefresh, onShowToast]() {
        std::string msg;
        if (viewModel->ToggleSelectedStatus(msg)) {
            if (onRefresh) onRefresh();
            if (onShowToast) onShowToast(msg);
        } else if (onShowToast) onShowToast(msg);
    });
    menu->AddItem("🗑 删除该启动项 (Del)", [viewModel, onRefresh, onShowToast]() {
        std::string msg;
        if (viewModel->DeleteSelected(msg)) {
            if (onRefresh) onRefresh();
            if (onShowToast) onShowToast(msg);
        } else if (onShowToast) onShowToast(msg);
    });
    menu->AddSeparator();
    menu->AddItem("🧹 清理所有失效项", [viewModel, onRefresh, onShowToast]() {
        size_t count = 0;
        if (viewModel->CleanAllMissing(count)) {
            if (onRefresh) onRefresh();
            if (onShowToast) onShowToast("已清理 " + std::to_string(count) + " 个失效残留项！");
        } else if (onShowToast) onShowToast("未检测到失效项。");
    });
    menu->AddItem("🔄 重新全盘扫描 (F5)", [viewModel, onRefresh, onShowToast]() {
        viewModel->StartScan([onRefresh, onShowToast]() {
            if (onRefresh) onRefresh();
            if (onShowToast) onShowToast("扫描已完成！");
        });
    });
    return menu;
}

} // namespace

TreeCategoryView::TreeCategoryView(
    std::shared_ptr<MainViewModel> viewModel,
    Window* window,
    std::function<void()> onSelectionChanged,
    std::function<void(const std::string&)> onShowToast,
    std::function<void()> onRefresh)
    : m_viewModel(std::move(viewModel))
    , m_window(window)
    , m_onSelectionChanged(std::move(onSelectionChanged))
    , m_onShowToast(std::move(onShowToast))
    , m_onRefresh(std::move(onRefresh)) {
}

std::shared_ptr<UIElement> TreeCategoryView::Build(const std::string& title, const std::string& desc) {
    auto viewModel = m_viewModel;
    auto window = m_window;
    auto onSelectionChanged = m_onSelectionChanged;
    auto onShowToast = m_onShowToast;
    auto onRefresh = m_onRefresh;

    const auto& summary = viewModel->GetSummary();
    const std::string q = ToLower(viewModel->GetFilterText());
    StartupCategory activeCategory = viewModel->GetCategory();

    // Group items
    struct GroupDef {
        std::string title;
        std::string icon;
        StartupLocation location = StartupLocation::Unknown;
        std::vector<StartupEntry> items;
    };
    std::vector<GroupDef> groups;
    std::map<std::string, size_t> groupMap;

    for (const auto& entry : summary.entries) {
        if (activeCategory != StartupCategory::All && entry.category != activeCategory) {
            continue;
        }

        if (!q.empty()) {
            if (ToLower(entry.name).find(q) == std::string::npos &&
                ToLower(entry.description).find(q) == std::string::npos &&
                ToLower(entry.publisher).find(q) == std::string::npos &&
                ToLower(entry.command).find(q) == std::string::npos &&
                ToLower(entry.executablePath).find(q) == std::string::npos &&
                ToLower(entry.source).find(q) == std::string::npos) {
                continue;
            }
        }

        std::string grpName = entry.groupTitle.empty() ? LocationName(entry.location) : entry.groupTitle;
        auto it = groupMap.find(grpName);
        if (it == groupMap.end()) {
            GroupDef gd;
            gd.title = grpName;
            gd.icon = LocationIcon(entry.location);
            gd.location = entry.location;
            gd.items.push_back(entry);
            size_t idx = groups.size();
            groups.push_back(std::move(gd));
            groupMap[grpName] = idx;
        } else {
            groups[it->second].items.push_back(entry);
        }
    }

    std::shared_ptr<UIElement> contentArea = nullptr;

    if (groups.empty()) {
        contentArea = Column(8.0f, {
            Text("✅ 当前分类下未检测到自启动项").FontSize(14.0f).ForegroundToken(ThemeTokenId::TextSecondary).Padding(Thickness(10, 20, 10, 20))
        }).Build();
    } else if (groups.size() == 1) {
        // Single group: no Expander needed! Render full-page table directly!
        const auto& grp = groups[0];
        CUI::Widgets::Ref listView = Widgets::ListView().Columns(6).RowHeight(24.0f).Align(Alignment::Stretch).FlexGrow(1.0f).Shared();
        listView.SelectionMode(ListViewSelectionMode::Single);
        listView->AddColumn("自动运行条目", 190.0f);
        listView->AddColumn("状态", 65.0f);
        listView->AddColumn("描述", 160.0f);
        listView->AddColumn("出版商 / 签名", 140.0f);
        listView->AddColumn("镜像路径 / 启动命令", 200.0f);
        listView->AddColumn("时间戳", 95.0f);

        std::vector<std::vector<ListViewCellData>> tableRows;
        std::vector<HICON> rowIcons;
        std::vector<std::string> rowTags;
        tableRows.reserve(grp.items.size());
        rowIcons.reserve(grp.items.size());
        rowTags.reserve(grp.items.size());

        for (const auto& e : grp.items) {
            std::string statusText;
            Color statusColor = Color::Hex("#D8A000");
            if (e.status == StartupStatus::Enabled) {
                statusText = "已启用";
                statusColor = Color::Hex("#107C41");
            } else if (e.status == StartupStatus::Disabled) {
                statusText = "已禁用";
                statusColor = Color::Hex("#D8A000");
            } else if (e.status == StartupStatus::Missing) {
                statusText = "失效";
                statusColor = Color::Hex("#E81123");
            } else {
                statusText = "未知";
                statusColor = Color::Hex("#D8A000");
            }

            std::string nameText = e.name.empty() ? "(未命名)" : e.name;
            std::string descText = e.description.empty() ? "--" : e.description;
            if (!e.triggerInfo.empty()) {
                descText = "[" + e.triggerInfo + "] " + (e.description.empty() ? "--" : e.description);
            }
            std::string pubText = e.publisher.empty() ? "--" : e.publisher;
            
            Color pubColor = Color::Transparent;
            if (pubText.find("(Verified)") != std::string::npos || pubText.find("Microsoft") != std::string::npos || pubText.find("Google") != std::string::npos) {
                pubColor = Color::Hex("#107C41");
            } else if (pubText.find("Not verified") != std::string::npos || pubText.find("未") != std::string::npos) {
                pubColor = Color::Hex("#D8A000");
            }

            std::string pathText = e.executablePath.empty() ? e.command : e.executablePath;
            Color pathColor = Color::Transparent;
            if (!e.fileExists && !e.command.empty()) {
                pathText = "[File not found: " + e.command + "]";
                pathColor = Color::Hex("#E81123");
            }
            std::string timeText = e.fileTimestamp.empty() ? "--" : e.fileTimestamp;

            ListViewCellData c0{ nameText, nullptr, !e.fileExists ? Color::Hex("#E81123") : Color::Transparent };
            ListViewCellData c1{ statusText, nullptr, statusColor };
            ListViewCellData c2{ descText, nullptr, Color::Transparent };
            ListViewCellData c3{ pubText, nullptr, pubColor };
            ListViewCellData c4{ pathText, nullptr, pathColor };
            ListViewCellData c5{ timeText, nullptr, Color::Transparent };

            tableRows.push_back({ c0, c1, c2, c3, c4, c5 });

            HICON hIcon = nullptr;
            std::string targetIconPath = e.executablePath.empty() ? e.command : e.executablePath;
            if (!targetIconPath.empty()) {
                hIcon = IconHelper::Instance().GetFileIcon(targetIconPath, true);
            }
            if (!hIcon) {
                hIcon = IconHelper::Instance().GetDefaultExeIcon(true);
            }
            rowIcons.push_back(hIcon);
            rowTags.push_back(e.id);
        }
        listView.Rows(tableRows);
        listView.RowIcons(rowIcons);
        listView.RowTags(rowTags);

        listView->OnSelectionChanged().Connect([viewModel, listView, onSelectionChanged](ListView*, int idx) {
            if (idx >= 0) {
                viewModel->SetSelectedId(listView->GetRowTag(idx));
            } else {
                viewModel->SetSelectedId("");
            }
            if (onSelectionChanged) onSelectionChanged();
        });

        listView->OnRowDoubleClicked().Connect([viewModel, listView](ListView*, int idx) {
            if (idx >= 0) {
                std::string selId = listView->GetRowTag(idx);
                if (!selId.empty()) {
                    viewModel->SetSelectedId(selId);
                    viewModel->JumpToImage();
                }
            }
        });

        listView.ContextMenu(BuildSpecializedContextMenu(grp.location, viewModel, window, onShowToast, onRefresh));
        contentArea = listView;
    } else {
        // Multiple groups: Use collapsible Expander cards!
        CUI::Widgets::Ref groupListColumn = CUI::Widgets::StackPanel().Shared();
        groupListColumn->Orientation = Orientation::Vertical;
                groupListColumn.Gap(8.0f);
                groupListColumn.Align(Alignment::Stretch);

        for (const auto& grp : groups) {
            CUI::Widgets::Ref expander = CUI::Widgets::Expander(grp.icon + "  " + grp.title).Shared();
                        expander.Subtitle(std::to_string(grp.items.size()) + " 个自启动项");
                        expander.IsExpanded(true);
                        expander.Align(Alignment::Stretch);

            // Clamped height inside Expander so it does not grow indefinitely
            float itemHeight = 36.0f + static_cast<float>(grp.items.size()) * 24.0f;
            float listHeight = (std::min)(340.0f, (std::max)(60.0f, itemHeight));

            CUI::Widgets::Ref listView = Widgets::ListView().Columns(6).RowHeight(24.0f).Height(listHeight).Align(Alignment::Stretch).ShowScrollBars(true).Shared();
            listView.SelectionMode(ListViewSelectionMode::Single);
            listView->AddColumn("自动运行条目", 190.0f);
            listView->AddColumn("状态", 65.0f);
            listView->AddColumn("描述", 160.0f);
            listView->AddColumn("出版商 / 签名", 140.0f);
            listView->AddColumn("镜像路径 / 启动命令", 200.0f);
            listView->AddColumn("时间戳", 95.0f);

            std::vector<std::vector<ListViewCellData>> tableRows;
            std::vector<HICON> rowIcons;
            std::vector<std::string> rowTags;
            tableRows.reserve(grp.items.size());
            rowIcons.reserve(grp.items.size());
            rowTags.reserve(grp.items.size());

            for (const auto& e : grp.items) {
                std::string statusText;
                Color statusColor = Color::Hex("#D8A000");
                if (e.status == StartupStatus::Enabled) {
                    statusText = "已启用";
                    statusColor = Color::Hex("#107C41");
                } else if (e.status == StartupStatus::Disabled) {
                    statusText = "已禁用";
                    statusColor = Color::Hex("#D8A000");
                } else if (e.status == StartupStatus::Missing) {
                    statusText = "失效";
                    statusColor = Color::Hex("#E81123");
                } else {
                    statusText = "未知";
                    statusColor = Color::Hex("#D8A000");
                }

                std::string nameText = e.name.empty() ? "(未命名)" : e.name;
                std::string descText = e.description.empty() ? "--" : e.description;
                if (!e.triggerInfo.empty()) {
                    descText = "[" + e.triggerInfo + "] " + (e.description.empty() ? "--" : e.description);
                }
                std::string pubText = e.publisher.empty() ? "--" : e.publisher;
                
                Color pubColor = Color::Transparent;
                if (pubText.find("(Verified)") != std::string::npos || pubText.find("Microsoft") != std::string::npos || pubText.find("Google") != std::string::npos) {
                    pubColor = Color::Hex("#107C41");
                } else if (pubText.find("Not verified") != std::string::npos || pubText.find("未") != std::string::npos) {
                    pubColor = Color::Hex("#D8A000");
                }

                std::string pathText = e.executablePath.empty() ? e.command : e.executablePath;
                Color pathColor = Color::Transparent;
                if (!e.fileExists && !e.command.empty()) {
                    pathText = "[File not found: " + e.command + "]";
                    pathColor = Color::Hex("#E81123");
                }
                std::string timeText = e.fileTimestamp.empty() ? "--" : e.fileTimestamp;

                ListViewCellData c0{ nameText, nullptr, !e.fileExists ? Color::Hex("#E81123") : Color::Transparent };
                ListViewCellData c1{ statusText, nullptr, statusColor };
                ListViewCellData c2{ descText, nullptr, Color::Transparent };
                ListViewCellData c3{ pubText, nullptr, pubColor };
                ListViewCellData c4{ pathText, nullptr, pathColor };
                ListViewCellData c5{ timeText, nullptr, Color::Transparent };

                tableRows.push_back({ c0, c1, c2, c3, c4, c5 });

                HICON hIcon = nullptr;
                std::string targetIconPath = e.executablePath.empty() ? e.command : e.executablePath;
                if (!targetIconPath.empty()) {
                    hIcon = IconHelper::Instance().GetFileIcon(targetIconPath, true);
                }
                if (!hIcon) {
                    hIcon = IconHelper::Instance().GetDefaultExeIcon(true);
                }
                rowIcons.push_back(hIcon);
                rowTags.push_back(e.id);
            }
            listView.Rows(tableRows);
            listView.RowIcons(rowIcons);
            listView.RowTags(rowTags);

            listView->OnSelectionChanged().Connect([viewModel, listView, onSelectionChanged](ListView*, int idx) {
                if (idx >= 0) {
                    viewModel->SetSelectedId(listView->GetRowTag(idx));
                } else {
                    viewModel->SetSelectedId("");
                }
                if (onSelectionChanged) onSelectionChanged();
            });

            listView->OnRowDoubleClicked().Connect([viewModel, listView](ListView*, int idx) {
                if (idx >= 0) {
                    std::string selId = listView->GetRowTag(idx);
                    if (!selId.empty()) {
                        viewModel->SetSelectedId(selId);
                        viewModel->JumpToImage();
                    }
                }
            });

            listView.ContextMenu(BuildSpecializedContextMenu(grp.location, viewModel, window, onShowToast, onRefresh));

                        expander.Content(listView);
            groupListColumn->AddChild(expander);
        }
        contentArea = groupListColumn;
    }

    auto header = Row(8.0f, {
        Text(title).FontSize(16.0f).FontWeight(FontWeight::SemiBold).ForegroundToken(ThemeTokenId::TextPrimary),
        Text(desc).FontSize(12.0f).ForegroundToken(ThemeTokenId::TextSecondary).Padding(Thickness(0, 2, 0, 0))
    }).Build();

    auto pageLayout = Column(6.0f, {
        header,
        contentArea
    })
    .Padding(Thickness(10, 8, 10, 8))
    .Align(Alignment::Stretch)
    .FlexGrow(1.0f)
    .BackgroundToken(ThemeTokenId::WindowBackground)
    .Build();

    // If single group or empty, return pageLayout directly so ListView takes exact window height with internal scrolling
    if (groups.size() <= 1) {
        return pageLayout;
    }

    CUI::Widgets::Ref scroll = CUI::Widgets::ScrollViewer().Shared();
        scroll.Align(Alignment::Stretch);
        scroll.FlexGrow(1.0f);
        scroll->AddChild(pageLayout);

    return scroll;
}

} // namespace AutoGuard
