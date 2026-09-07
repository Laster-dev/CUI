#pragma once

#include <windows.h>
#include <shellapi.h>
#include <string>
#include <unordered_map>
#include <memory>

namespace AutoGuard {

class IconHelper {
public:
    static IconHelper& Instance();

    HICON GetFileIcon(const std::string& filePath, bool smallIcon = true);
    HICON GetDefaultExeIcon(bool smallIcon = true);
    HICON GetFolderIcon(bool smallIcon = true);

    void Clear();

private:
    IconHelper() = default;
    ~IconHelper();
    IconHelper(const IconHelper&) = delete;
    IconHelper& operator=(const IconHelper&) = delete;

private:
    std::unordered_map<std::string, HICON> m_smallIconCache;
    std::unordered_map<std::string, HICON> m_largeIconCache;
};

} // namespace AutoGuard
