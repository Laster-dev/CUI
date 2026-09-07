#include "IconHelper.h"
#include <shlwapi.h>
#include <algorithm>

namespace AutoGuard {

namespace {

std::wstring Widen(const std::string& value) {
    if (value.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size);
    return result;
}

std::wstring CleanFilePath(const std::wstring& cmd) {
    if (cmd.empty()) return {};
    std::wstring s = cmd;
    size_t start = s.find_first_not_of(L" \t\"");
    if (start != std::wstring::npos) s = s.substr(start);
    if (!s.empty() && s.front() == L'"') s = s.substr(1);
    size_t quoteEnd = s.find(L'"');
    if (quoteEnd != std::wstring::npos) {
        s = s.substr(0, quoteEnd);
    } else {
        // If not quoted, look for .exe or space
        size_t exePos = s.find(L".exe");
        if (exePos != std::wstring::npos) {
            s = s.substr(0, exePos + 4);
        } else {
            size_t sp = s.find(L' ');
            if (sp != std::wstring::npos) {
                s = s.substr(0, sp);
            }
        }
    }

    wchar_t expanded[MAX_PATH * 2]{};
    DWORD len = ExpandEnvironmentStringsW(s.c_str(), expanded, ARRAYSIZE(expanded));
    if (len > 0 && len < ARRAYSIZE(expanded)) {
        s = expanded;
    }
    return s;
}

} // namespace

IconHelper& IconHelper::Instance() {
    static IconHelper s_instance;
    return s_instance;
}

IconHelper::~IconHelper() {
    Clear();
}

void IconHelper::Clear() {
    for (auto& pair : m_smallIconCache) {
        if (pair.second) DestroyIcon(pair.second);
    }
    m_smallIconCache.clear();

    for (auto& pair : m_largeIconCache) {
        if (pair.second) DestroyIcon(pair.second);
    }
    m_largeIconCache.clear();
}

HICON IconHelper::GetFileIcon(const std::string& filePath, bool smallIcon) {
    if (filePath.empty()) return GetDefaultExeIcon(smallIcon);

    auto& cache = smallIcon ? m_smallIconCache : m_largeIconCache;
    auto it = cache.find(filePath);
    if (it != cache.end()) {
        return it->second;
    }

    std::wstring pathW = CleanFilePath(Widen(filePath));
    HICON hIcon = nullptr;

    SHFILEINFOW sfi{};
    UINT flags = SHGFI_ICON | (smallIcon ? SHGFI_SMALLICON : SHGFI_LARGEICON);

    if (GetFileAttributesW(pathW.c_str()) != INVALID_FILE_ATTRIBUTES) {
        if (SHGetFileInfoW(pathW.c_str(), 0, &sfi, sizeof(sfi), flags) && sfi.hIcon) {
            hIcon = sfi.hIcon;
        }
    }

    if (!hIcon) {
        // Try extracting via ExtractIconExW
        HICON hLarge = nullptr, hSmall = nullptr;
        if (ExtractIconExW(pathW.c_str(), 0, &hLarge, &hSmall, 1) > 0) {
            if (smallIcon) {
                hIcon = hSmall ? hSmall : hLarge;
                if (hLarge && hLarge != hIcon) DestroyIcon(hLarge);
            } else {
                hIcon = hLarge ? hLarge : hSmall;
                if (hSmall && hSmall != hIcon) DestroyIcon(hSmall);
            }
        }
    }

    if (!hIcon) {
        // Fallback to extension type icon
        std::wstring ext = L".exe";
        size_t dot = pathW.rfind(L'.');
        if (dot != std::wstring::npos) ext = pathW.substr(dot);
        if (SHGetFileInfoW(ext.c_str(), FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi), flags | SHGFI_USEFILEATTRIBUTES) && sfi.hIcon) {
            hIcon = sfi.hIcon;
        }
    }

    if (!hIcon) {
        hIcon = GetDefaultExeIcon(smallIcon);
    }

    cache[filePath] = hIcon;
    return hIcon;
}

HICON IconHelper::GetDefaultExeIcon(bool smallIcon) {
    auto& cache = smallIcon ? m_smallIconCache : m_largeIconCache;
    auto it = cache.find("__default_exe__");
    if (it != cache.end()) return it->second;

    SHFILEINFOW sfi{};
    UINT flags = SHGFI_ICON | (smallIcon ? SHGFI_SMALLICON : SHGFI_LARGEICON) | SHGFI_USEFILEATTRIBUTES;
    HICON hIcon = nullptr;
    if (SHGetFileInfoW(L".exe", FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi), flags) && sfi.hIcon) {
        hIcon = sfi.hIcon;
    }
    cache["__default_exe__"] = hIcon;
    return hIcon;
}

HICON IconHelper::GetFolderIcon(bool smallIcon) {
    auto& cache = smallIcon ? m_smallIconCache : m_largeIconCache;
    auto it = cache.find("__default_folder__");
    if (it != cache.end()) return it->second;

    SHFILEINFOW sfi{};
    UINT flags = SHGFI_ICON | (smallIcon ? SHGFI_SMALLICON : SHGFI_LARGEICON) | SHGFI_USEFILEATTRIBUTES;
    HICON hIcon = nullptr;
    if (SHGetFileInfoW(L"folder", FILE_ATTRIBUTE_DIRECTORY, &sfi, sizeof(sfi), flags) && sfi.hIcon) {
        hIcon = sfi.hIcon;
    }
    cache["__default_folder__"] = hIcon;
    return hIcon;
}

} // namespace AutoGuard
