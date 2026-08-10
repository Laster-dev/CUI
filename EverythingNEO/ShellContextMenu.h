#pragma once

#include "framework/controls/ContextMenu.h"
#include <windows.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace EverythingNEO {

// App-provided verbs merged into the shell menu (Everything-style).
struct ShellMenuActions {
    std::function<void()> openPath;
    std::function<void()> copyFullPath;
    std::function<void()> rename;
    bool showOpenPath = true;
    bool showCopyFullPath = true;
    bool showRename = true; // typically single selection only
};

// Query the real Explorer IContextMenu, convert items into a CUI ContextMenu,
// and inject Everything extras (打开路径 / 复制完整路径 / 重命名).
// Caller must keep the returned shared_ptr alive while the menu is open.
std::shared_ptr<CUI::ContextMenu> BuildShellContextMenu(
    HWND owner,
    const std::vector<std::wstring>& paths,
    const ShellMenuActions& actions);

} // namespace EverythingNEO
