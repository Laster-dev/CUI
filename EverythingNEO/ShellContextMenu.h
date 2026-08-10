#pragma once

#include <windows.h>
#include <string>
#include <vector>

namespace EverythingNEO {

// Shows the native Explorer file context menu at screen coordinates.
// Returns true if a menu was shown (even if user cancelled).
bool ShowShellContextMenu(HWND owner, const std::vector<std::wstring>& paths, int screenX, int screenY);

} // namespace EverythingNEO
