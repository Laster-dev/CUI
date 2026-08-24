#pragma once

#include "../model/StartupEntry.h"
#include <windows.h>
#include <string>
#include <vector>

namespace AutoGuard {

class StartupManager {
public:
    static bool ToggleStatus(StartupEntry& entry, bool enable, std::string& outMessage);
    static bool DeleteEntry(const StartupEntry& entry, std::string& outMessage);
    static bool JumpToImage(const StartupEntry& entry);
    static bool JumpToEntry(const StartupEntry& entry);
    static bool SearchOnline(const StartupEntry& entry);
    static bool CopyToClipboard(HWND hwnd, const std::string& text);
    static bool ExportReport(const ScanSummary& summary, const std::wstring& outputPath, std::string& outMessage);
};

} // namespace AutoGuard
