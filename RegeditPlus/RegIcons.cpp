#include "RegIcons.h"

namespace RegeditPlus {

void RegIcons::EnsureLoaded() {
    if (m_loaded) return;
    m_loaded = true;

    wchar_t winDir[MAX_PATH] = {};
    GetWindowsDirectoryW(winDir, MAX_PATH);
    const std::wstring path = std::wstring(winDir) + L"\\regedit.exe";

    const UINT count = ExtractIconExW(path.c_str(), -1, nullptr, nullptr, 0);
    if (count == 0) return;

    std::vector<HICON> large(count, nullptr);
    m_small.assign(count, nullptr);
    ExtractIconExW(path.c_str(), 0, large.data(), m_small.data(), count);

    // We only keep the small icons used by the tree/list chrome.
    for (HICON h : large) {
        if (h) DestroyIcon(h);
    }
}

void RegIcons::Release() {
    for (HICON h : m_small) {
        if (h) DestroyIcon(h);
    }
    m_small.clear();
    m_loaded = false;
}

} // namespace RegeditPlus
