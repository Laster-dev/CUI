#include "StartupScanner.h"

#include <windows.h>
#include <taskschd.h>
#include <wbemidl.h>
#include <oleauto.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <winver.h>
#include <cstdio>
#include <filesystem>
#include <sstream>
#include <map>
#include <iomanip>
#include <algorithm>
#include <cwctype>
#include <chrono>

#pragma comment(lib, "version.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "wbemuuid.lib")

namespace AutoGuard {
namespace {

std::string Narrow(const std::wstring& value) {
    if (value.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size, nullptr, nullptr);
    return result;
}

std::wstring Widen(const std::string& value) {
    if (value.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size);
    return result;
}

std::string EnsureUtf8(const std::string& str) {
    if (str.empty()) return {};
    // Test if str is already valid UTF-8
    int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.data(), static_cast<int>(str.size()), nullptr, 0);
    if (wlen > 0) {
        return str;
    }
    // If not valid UTF-8, it is GBK/ANSI: convert CP_ACP -> UTF-16 -> UTF-8
    wlen = MultiByteToWideChar(CP_ACP, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
    if (wlen <= 0) return str;
    std::wstring wstr(wlen, L'\0');
    MultiByteToWideChar(CP_ACP, 0, str.data(), static_cast<int>(str.size()), wstr.data(), wlen);
    return Narrow(wstr);
}

std::wstring ExpandEnv(const std::wstring& path) {
    if (path.empty()) return {};
    wchar_t expanded[MAX_PATH * 2]{};
    DWORD len = ExpandEnvironmentStringsW(path.c_str(), expanded, ARRAYSIZE(expanded));
    if (len > 0 && len < ARRAYSIZE(expanded)) return expanded;
    return path;
}

std::wstring ExtractExePath(const std::wstring& cmd) {
    if (cmd.empty()) return {};
    std::wstring expanded = ExpandEnv(cmd);

    size_t start = expanded.find_first_not_of(L" \t");
    if (start == std::wstring::npos) return {};
    expanded = expanded.substr(start);

    // Handle \SystemRoot\ prefix (common in services/drivers/session manager)
    if (expanded.rfind(L"\\SystemRoot\\", 0) == 0) {
        wchar_t winDir[MAX_PATH]{};
        GetWindowsDirectoryW(winDir, MAX_PATH);
        expanded = std::wstring(winDir) + expanded.substr(11);
    }
    // Handle \??\ prefix (NT path)
    if (expanded.rfind(L"\\??\\", 0) == 0) {
        expanded = expanded.substr(4);
    }

    if (expanded.front() == L'"') {
        size_t end = expanded.find(L'"', 1);
        if (end != std::wstring::npos) {
            return expanded.substr(1, end - 1);
        }
        return expanded.substr(1);
    }

    // Try whole string first if it exists
    if (GetFileAttributesW(expanded.c_str()) != INVALID_FILE_ATTRIBUTES) {
        return expanded;
    }

    // Search for .exe / .dll / .cmd / .bat / .vbs / .sys / .ocx / .cpl occurrence
    const wchar_t* extensions[] = { L".exe", L".dll", L".cmd", L".bat", L".vbs", L".sys", L".ocx", L".cpl" };
    for (const wchar_t* ext : extensions) {
        std::wstring lower = expanded;
        std::transform(lower.begin(), lower.end(), lower.begin(), towlower);
        size_t pos = lower.find(ext);
        if (pos != std::wstring::npos) {
            std::wstring candidate = expanded.substr(0, pos + wcslen(ext));
            if (GetFileAttributesW(candidate.c_str()) != INVALID_FILE_ATTRIBUTES) {
                return candidate;
            }
            if (candidate.find(L'\\') == std::wstring::npos) {
                wchar_t sysDir[MAX_PATH]{};
                GetSystemDirectoryW(sysDir, MAX_PATH);
                std::wstring inSys = std::wstring(sysDir) + L"\\" + candidate;
                if (GetFileAttributesW(inSys.c_str()) != INVALID_FILE_ATTRIBUTES) {
                    return inSys;
                }
                wchar_t winDir[MAX_PATH]{};
                GetWindowsDirectoryW(winDir, MAX_PATH);
                std::wstring inWin = std::wstring(winDir) + L"\\" + candidate;
                if (GetFileAttributesW(inWin.c_str()) != INVALID_FILE_ATTRIBUTES) {
                    return inWin;
                }
            }
            return candidate;
        }
    }

    size_t end = expanded.find(L' ');
    std::wstring candidate = (end == std::wstring::npos) ? expanded : expanded.substr(0, end);
    if (candidate.find(L'\\') == std::wstring::npos && !candidate.empty()) {
        wchar_t sysDir[MAX_PATH]{};
        GetSystemDirectoryW(sysDir, MAX_PATH);
        std::wstring inSys = std::wstring(sysDir) + L"\\" + candidate;
        if (GetFileAttributesW(inSys.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return inSys;
        }
    }
    return candidate;
}

std::string IdFor(const char* source, const std::string& name) {
    return std::string(source) + ":" + name;
}

// 在 HKCR / HKLM(64位) / HKLM(WOW64) 中解析 CLSID 的进程内服务器 DLL 路径
std::string ResolveClsidServer(const std::string& clsid) {
    char pathBuf[MAX_PATH * 2]{};
    DWORD size = sizeof(pathBuf);
    auto tryRead = [&](HKEY root, const std::string& subKey) -> std::string {
        size = sizeof(pathBuf);
        if (RegGetValueA(root, subKey.c_str(), nullptr, RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ, nullptr, pathBuf, &size) == ERROR_SUCCESS && size > 1) {
            char expanded[MAX_PATH * 2]{};
            ExpandEnvironmentStringsA(pathBuf, expanded, MAX_PATH * 2);
            return expanded;
        }
        return "";
    };
    std::string found = tryRead(HKEY_CLASSES_ROOT, "CLSID\\" + clsid + "\\InprocServer32");
    if (!found.empty()) return found;
    found = tryRead(HKEY_LOCAL_MACHINE, "SOFTWARE\\Classes\\CLSID\\" + clsid + "\\InprocServer32");
    if (!found.empty()) return found;
    found = tryRead(HKEY_LOCAL_MACHINE, "SOFTWARE\\Classes\\WOW6432Node\\CLSID\\" + clsid + "\\InprocServer32");
    if (!found.empty()) return found;
    // 本地服务器 (LocalServer32) 兼容
    found = tryRead(HKEY_CLASSES_ROOT, "CLSID\\" + clsid + "\\LocalServer32");
    if (!found.empty()) return found;
    found = tryRead(HKEY_LOCAL_MACHINE, "SOFTWARE\\Classes\\CLSID\\" + clsid + "\\LocalServer32");
    return found;
}

// 扫描某个注册表键下的单个 REG_SZ 值作为启动项
void ScanSingleRegistryValue(
    HKEY root, bool isHklm, const char* subKey, const char* valueName,
    StartupCategory cat, StartupLocation loc, const std::string& groupTitle,
    RiskLevel risk, const char* riskReason, ScanSummary& result, bool allowDisable = true)
{
    HKEY key = nullptr;
    std::wstring subKeyW = Widen(subKey);
    std::wstring valueNameW = Widen(valueName);
    REGSAM sam = KEY_READ | (isHklm ? KEY_WOW64_64KEY : 0);
    if (RegOpenKeyExW(root, subKeyW.c_str(), 0, sam, &key) != ERROR_SUCCESS) {
        if (RegOpenKeyExW(root, subKeyW.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS) return;
    }
    wchar_t val[2048]{};
    DWORD size = sizeof(val);
    if (RegQueryValueExW(key, valueNameW.c_str(), nullptr, nullptr, reinterpret_cast<BYTE*>(val), &size) == ERROR_SUCCESS && size > sizeof(wchar_t) && val[0] != L'\0') {
        StartupEntry entry;
        entry.id = IdFor("RegValue", std::string(subKey) + "@" + valueName);
        entry.name = valueName;
        entry.command = Narrow(val);
        entry.scope = isHklm ? "本机系统" : "当前用户";
        entry.source = std::string(isHklm ? "HKLM\\" : "HKCU\\") + subKey + "@" + valueName;
        entry.registryKey = subKey;
        entry.registryValue = valueName;
        entry.isHklm = isHklm;
        entry.category = cat;
        entry.location = loc;
        entry.groupTitle = groupTitle;
        entry.status = StartupStatus::Enabled;
        entry.canDisable = allowDisable;
        entry.risk = risk;
        entry.riskReason = riskReason;
        StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
    }
    RegCloseKey(key);
}

// CMD 命令处理器 AutoRun（HKCU/HKLM）：每次打开命令行窗口都会自动执行
void ScanCmdAutoRun(ScanSummary& result) {
    const char* subKey = "Software\\Microsoft\\Command Processor";
    ScanSingleRegistryValue(HKEY_CURRENT_USER, false, subKey, "AutoRun",
        StartupCategory::Logon, StartupLocation::RegistryRun,
        "HKCU\\SOFTWARE\\Microsoft\\Command Processor", RiskLevel::Suspicious,
        "CMD 命令处理器 AutoRun：每次启动命令行都会自动执行该命令，常被用作持久化", result, false);
    ScanSingleRegistryValue(HKEY_LOCAL_MACHINE, true, subKey, "AutoRun",
        StartupCategory::Logon, StartupLocation::RegistryRun,
        "HKLM\\SOFTWARE\\Microsoft\\Command Processor", RiskLevel::Suspicious,
        "CMD 命令处理器 AutoRun：每次启动命令行都会自动执行该命令，常被用作持久化", result, false);
}

std::string FormatTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm buf{};
    localtime_s(&buf, &in_time_t);
    std::ostringstream oss;
    oss << std::put_time(&buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

} // namespace

std::string StartupScanner::FormatFileSize(uint64_t bytes) {
    if (bytes == 0) return "0 K";
    uint64_t k = (bytes + 1023) / 1024;
    std::string s = std::to_string(k);
    int n = static_cast<int>(s.length()) - 3;
    while (n > 0) {
        s.insert(n, ",");
        n -= 3;
    }
    return s + " K";
}

std::string StartupScanner::FormatFileTime(const FILETIME& ft) {
    FILETIME localFt;
    FileTimeToLocalFileTime(&ft, &localFt);
    SYSTEMTIME st;
    FileTimeToSystemTime(&localFt, &st);
    char buf[64]{};
    sprintf_s(buf, "%d/%d/%d %d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
    return buf;
}

void StartupScanner::ExtractVersionInfo(const std::wstring& filePath, std::string& outCompany, std::string& outDesc, std::string& outVersion) {
    outCompany.clear();
    outDesc.clear();
    outVersion.clear();

    if (filePath.empty()) return;
    DWORD handle = 0;
    DWORD size = GetFileVersionInfoSizeW(filePath.c_str(), &handle);
    if (size == 0) return;

    std::vector<BYTE> data(size);
    if (!GetFileVersionInfoW(filePath.c_str(), 0, size, data.data())) return;

    struct LANGANDCODEPAGE {
        WORD wLanguage;
        WORD wCodePage;
    } *lpTranslate = nullptr;
    UINT cbTranslate = 0;

    auto queryVal = [&](const wchar_t* keyName) -> std::string {
        if (VerQueryValueW(data.data(), L"\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate) && cbTranslate >= sizeof(LANGANDCODEPAGE)) {
            wchar_t subBlock[256]{};
            swprintf_s(subBlock, L"\\StringFileInfo\\%04x%04x\\%s", lpTranslate[0].wLanguage, lpTranslate[0].wCodePage, keyName);
            wchar_t* strVal = nullptr;
            UINT len = 0;
            if (VerQueryValueW(data.data(), subBlock, (LPVOID*)&strVal, &len) && len > 0 && strVal) {
                return Narrow(strVal);
            }
        }
        const wchar_t* commonPrefixes[] = { L"\\StringFileInfo\\080404b0\\", L"\\StringFileInfo\\040904b0\\", L"\\StringFileInfo\\000004b0\\" };
        for (const wchar_t* prefix : commonPrefixes) {
            std::wstring block = std::wstring(prefix) + keyName;
            wchar_t* strVal = nullptr;
            UINT len = 0;
            if (VerQueryValueW(data.data(), block.c_str(), (LPVOID*)&strVal, &len) && len > 0 && strVal) {
                return Narrow(strVal);
            }
        }
        return "";
    };

    outCompany = queryVal(L"CompanyName");
    outDesc = queryVal(L"FileDescription");
    outVersion = queryVal(L"FileVersion");
}

ScanSummary StartupScanner::Scan() const {
    ScanSummary result;
    result.scanTimestamp = FormatTimestamp();

    // 1. Logon: Registry HKCU & HKLM Run / RunOnce
    ScanRegistryRoot(HKEY_CURRENT_USER, "当前用户", "Software\\Microsoft\\Windows\\CurrentVersion\\Run", false, StartupCategory::Logon, StartupLocation::RegistryRun, "HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", result);
    ScanRegistryRoot(HKEY_CURRENT_USER, "当前用户", "Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce", false, StartupCategory::Logon, StartupLocation::RegistryRun, "HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce", result);
    ScanRegistryRoot(HKEY_LOCAL_MACHINE, "本机系统", "Software\\Microsoft\\Windows\\CurrentVersion\\Run", true, StartupCategory::Logon, StartupLocation::RegistryRun, "HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", result);
    ScanRegistryRoot(HKEY_LOCAL_MACHINE, "本机系统", "Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce", true, StartupCategory::Logon, StartupLocation::RegistryRun, "HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce", result);
    ScanRegistryRoot(HKEY_LOCAL_MACHINE, "本机系统(32位)", "Software\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Run", true, StartupCategory::Logon, StartupLocation::RegistryRun, "HKLM\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Run", result);
    ScanRegistryRoot(HKEY_LOCAL_MACHINE, "本机系统(32位)", "Software\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\RunOnce", true, StartupCategory::Logon, StartupLocation::RegistryRun, "HKLM\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\RunOnce", result);

    // 1b. Logon: Policies Run (组策略启动项)
    ScanRegistryRoot(HKEY_CURRENT_USER, "当前用户", "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run", false, StartupCategory::Logon, StartupLocation::RegistryRun, "HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run", result);
    ScanRegistryRoot(HKEY_LOCAL_MACHINE, "本机系统", "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run", true, StartupCategory::Logon, StartupLocation::RegistryRun, "HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run", result);

    // 1c. Logon: RunOnceEx
    ScanRunOnceEx(HKEY_CURRENT_USER, false, "Software\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx", "当前用户", result);
    ScanRunOnceEx(HKEY_LOCAL_MACHINE, true, "Software\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx", "本机系统", result);

    // 1d. Logon: CMD AutoRun (命令处理器自动执行)
    ScanCmdAutoRun(result);

    // 1e. Logon: Windows NT 当前版本 Load / Run 值（用户登录时由 winlogon 执行）
    const char* ntKeyPath = "Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows";
    const char* ntGroup = "HKCU\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows";
    ScanSingleRegistryValue(HKEY_CURRENT_USER, false, ntKeyPath, "Load", StartupCategory::Logon, StartupLocation::RegistryRun,
        ntGroup, RiskLevel::Suspicious, "Windows NT 兼容入口 Load 值：登录时静默执行指定程序，极少有合法软件使用", result, false);
    ScanSingleRegistryValue(HKEY_CURRENT_USER, false, ntKeyPath, "Run", StartupCategory::Logon, StartupLocation::RegistryRun,
        ntGroup, RiskLevel::Suspicious, "Windows NT 兼容入口 Run 值：登录时执行指定程序，与标准 Run 键不同且易被滥用", result, false);

    // 2. Logon: Startup Folders
    wchar_t path[MAX_PATH]{};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_STARTUP, nullptr, SHGFP_TYPE_CURRENT, path))) {
        ScanStartupFolder(path, "当前用户", result);
    }
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_COMMON_STARTUP, nullptr, SHGFP_TYPE_CURRENT, path))) {
        ScanStartupFolder(path, "所有用户", result);
    }

    // 3. Active Setup (Logon)
    ScanActiveSetup(result);

    // 4. Scheduled Tasks
    ScanScheduledTasks(result);

    // 5. Services
    ScanServices(result);

    // 6. Drivers
    ScanDrivers(result);

    // 7. Image Hijacks (IFEO)
    ScanImageHijacks(result);

    // 8. Winlogon (Shell/Userinit/Notify/GinaDLL 等)
    ScanWinlogon(result);

    // 9. Explorer / Shell Extensions
    ScanComExtensions(result);

    // 10. AppInit DLLs
    ScanAppInitDlls(result);

    // 11. Known DLLs
    ScanKnownDlls(result);

    // 12. Boot Execute / SetupExecute / Execute
    ScanBootExecute(result);

    // 13. Winsock Providers
    ScanWinsockProviders(result);

    // 14. Print Monitors
    ScanPrintMonitors(result);

    // 15. LSA Packages (认证/通知/安全包)
    ScanLsaPackages(result);

    // 16. WMI Subscriptions
    ScanWmiSubscriptions(result);

    // 17. Network Providers
    ScanNetworkProviders(result);

    // 18. Explorer ShellServiceObjects & Icon Overlays
    ScanShellServiceObjects(result);

    // 19. IE BHO / Toolbars
    ScanBrowserHelpers(result);

    // 20. Office Add-ins
    ScanOfficeAddins(result);

    // 21. Safe Mode 自启动组件 (Minimal / Network)
    ScanSafeMode(result);

    // 22. 协议 Handler 劫持检测
    ScanProtocolHandlers(result);

    // 23. DirectShow 编解码器过滤器
    ScanDirectShowFilters(result);

    // 24. SharedTaskScheduler（Explorer 共享任务调度 COM 对象）
    ScanSharedTaskSchedulers(result);

    // 25. IE URLSearchHooks
    ScanUrlSearchHooks(result);

    // 26. Drivers32 编解码器驱动
    ScanDrivers32Codecs(result);

    // 27. 打印处理器
    ScanPrintProcessors(result);

    // 28. 安全提供程序 (SecurityProviders)
    ScanSecurityProviders(result);

    // 29. 屏幕保护程序 (SCRNSAVE.EXE)
    ScanScreenSaver(result);

    // 30. AppCertDLLs (进程创建注入)
    ScanAppCertDlls(result);

    // 31. 终端服务器 / RDP 会话启动程序
    ScanTerminalServer(result);

    // 32. UserInitMprLogonScript 登录脚本
    ScanUserInitMprLogonScript(result);

    // 33. 打印提供程序 (Print Providers)
    ScanPrintProviders(result);

    // 34. ShellExecuteHooks (Shell 打开操作挂钩)
    ScanShellExecuteHooks(result);

    // 35. Chromium 浏览器扩展与强制安装策略
    ScanChromiumExtensions(result);

    // 36. Office 启动文件夹加载项 (Word STARTUP / Excel XLSTART)
    ScanOfficeStartupFolders(result);

    // Count statistics
    for (const auto& entry : result.entries) {
        if (entry.status == StartupStatus::Enabled) ++result.enabledCount;
        else if (entry.status == StartupStatus::Disabled) ++result.disabledCount;
        else if (entry.status == StartupStatus::Missing) ++result.missingCount;

        if (entry.risk == RiskLevel::High) ++result.highRiskCount;
        else if (entry.risk == RiskLevel::Suspicious) ++result.suspiciousCount;
        else ++result.safeCount;
    }
    return result;
}

bool IsRegistryStartupDisabledByWindows(HKEY root, bool isHklm, const std::string& subKey, const std::wstring& valueName) {
    std::wstring approvedSubKey;
    if (subKey.find("Wow6432Node") != std::string::npos || subKey.find("WOW6432Node") != std::string::npos) {
        approvedSubKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run32";
    } else {
        approvedSubKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run";
    }

    HKEY hApproved = nullptr;
    REGSAM sam = KEY_READ | (isHklm ? KEY_WOW64_64KEY : 0);
    if (RegOpenKeyExW(root, approvedSubKey.c_str(), 0, sam, &hApproved) == ERROR_SUCCESS ||
        RegOpenKeyExW(root, approvedSubKey.c_str(), 0, KEY_READ, &hApproved) == ERROR_SUCCESS) {
        BYTE data[64]{};
        DWORD dataSize = sizeof(data);
        DWORD type = 0;
        if (RegQueryValueExW(hApproved, valueName.c_str(), nullptr, &type, data, &dataSize) == ERROR_SUCCESS) {
            RegCloseKey(hApproved);
            if (dataSize >= 1) {
                // In Windows Task Manager / StartupApproved:
                // First byte == 0x02, 0x00, 0x06 -> Enabled
                // First byte == 0x03, 0x01, or (byte & 1) != 0 -> Disabled!
                if ((data[0] & 1) != 0 || data[0] == 0x03) {
                    return true;
                }
            }
        } else {
            RegCloseKey(hApproved);
        }
    }
    return false;
}

bool IsStartupFolderItemDisabledByWindows(bool isUser, const std::wstring& itemName) {
    std::wstring approvedSubKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\StartupFolder";
    HKEY root = isUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
    HKEY hApproved = nullptr;
    if (RegOpenKeyExW(root, approvedSubKey.c_str(), 0, KEY_READ, &hApproved) == ERROR_SUCCESS) {
        BYTE data[64]{};
        DWORD dataSize = sizeof(data);
        DWORD type = 0;
        if (RegQueryValueExW(hApproved, itemName.c_str(), nullptr, &type, data, &dataSize) == ERROR_SUCCESS) {
            RegCloseKey(hApproved);
            if (dataSize >= 1) {
                if ((data[0] & 1) != 0 || data[0] == 0x03) {
                    return true;
                }
            }
        } else {
            RegCloseKey(hApproved);
        }
    }
    return false;
}

void StartupScanner::ScanRegistryRoot(
    HKEY root, const char* scope, const char* subKey, bool isHklm,
    StartupCategory cat, StartupLocation loc, const std::string& groupTitle,
    ScanSummary& result)
{
    HKEY key = nullptr;
    std::wstring subKeyW = Widen(subKey);
    REGSAM sam = KEY_READ | (isHklm ? KEY_WOW64_64KEY : 0);
    if (RegOpenKeyExW(root, subKeyW.c_str(), 0, sam, &key) != ERROR_SUCCESS) {
        if (RegOpenKeyExW(root, subKeyW.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS) return;
    }

    for (DWORD index = 0;; ++index) {
        wchar_t name[512]{};
        wchar_t value[4096]{};
        DWORD nameSize = ARRAYSIZE(name);
        DWORD valueSize = sizeof(value);
        DWORD type = 0;
        const LONG status = RegEnumValueW(key, index, name, &nameSize, nullptr, &type, reinterpret_cast<BYTE*>(value), &valueSize);
        if (status == ERROR_NO_MORE_ITEMS) break;
        if (status != ERROR_SUCCESS) continue;
        if (nameSize == 0 && valueSize == 0) continue;

        StartupEntry entry;
        std::string narrowName = Narrow(name);
        std::string narrowVal;
        if (type == REG_SZ || type == REG_EXPAND_SZ) {
            narrowVal = Narrow(value);
        } else {
            narrowVal = narrowName;
        }

        entry.id = IdFor(subKey, narrowName);
        entry.name = narrowName.empty() ? "(默认)" : narrowName;
        entry.scope = scope;
        entry.command = narrowVal;
        entry.source = std::string(isHklm ? "HKLM\\" : "HKCU\\") + subKey;
        entry.registryKey = subKey;
        entry.registryValue = entry.name;
        entry.isHklm = isHklm;
        entry.category = cat;
        entry.location = loc;

        // Query Windows StartupApproved registry to determine exact disabled state
        if (IsRegistryStartupDisabledByWindows(root, isHklm, subKey, name)) {
            entry.status = StartupStatus::Disabled;
        } else {
            entry.status = StartupStatus::Enabled;
        }

        entry.groupTitle = groupTitle;
        StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
    }
    RegCloseKey(key);
}

void StartupScanner::ScanStartupFolder(const std::wstring& folder, const char* scope, ScanSummary& result) {
    std::error_code error;
    if (!std::filesystem::exists(folder, error)) return;
    std::string folderUtf8 = Narrow(folder);

    // 初始化 COM 以便解析 .lnk 快捷方式目标
    HRESULT comInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool shouldUninitCom = SUCCEEDED(comInit);

    auto resolveLnk = [](const std::wstring& lnkPath, std::wstring& outArgs) -> std::wstring {
        IShellLinkW* shellLink = nullptr;
        if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast<void**>(&shellLink))) || !shellLink) return {};
        std::wstring target;
        IPersistFile* persistFile = nullptr;
        if (SUCCEEDED(shellLink->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&persistFile))) && persistFile) {
            if (SUCCEEDED(persistFile->Load(lnkPath.c_str(), STGM_READ))) {
                wchar_t pathBuf[MAX_PATH * 2]{};
                if (SUCCEEDED(shellLink->GetPath(pathBuf, ARRAYSIZE(pathBuf), nullptr, SLGP_RAWPATH)) && pathBuf[0]) {
                    target = pathBuf;
                }
                wchar_t argsBuf[MAX_PATH * 2]{};
                if (SUCCEEDED(shellLink->GetArguments(argsBuf, ARRAYSIZE(argsBuf))) && argsBuf[0]) {
                    outArgs = argsBuf;
                }
            }
            persistFile->Release();
        }
        shellLink->Release();
        return target;
    };

    for (const auto& item : std::filesystem::directory_iterator(folder, error)) {
        if (error) break;
        std::wstring filename = item.path().filename().wstring();
        if (filename == L"desktop.ini") continue;

        StartupEntry entry;
        entry.name = Narrow(filename);
        entry.source = Narrow(item.path().wstring());

        // 解析快捷方式真实目标与参数
        std::wstring lowerName = filename;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), towlower);
        bool isLnk = lowerName.size() > 4 && lowerName.substr(lowerName.size() - 4) == L".lnk";
        std::wstring lnkTarget;
        std::wstring lnkArgs;
        if (isLnk) {
            lnkTarget = resolveLnk(item.path().wstring(), lnkArgs);
            if (!lnkTarget.empty()) {
                entry.command = Narrow(lnkTarget);
                if (!lnkArgs.empty()) entry.command += " " + Narrow(lnkArgs);
            }
        }
        if (entry.command.empty()) {
            entry.command = entry.source;
        }
        entry.scope = scope;
        entry.category = StartupCategory::Logon;
        entry.location = StartupLocation::StartupFolder;
        entry.id = IdFor("StartupFolder", entry.name);
        entry.groupTitle = folderUtf8;

        if ((filename.size() > 9 && filename.substr(filename.size() - 9) == L".disabled") ||
            IsStartupFolderItemDisabledByWindows(scope == std::string("User"), filename)) {
            entry.status = StartupStatus::Disabled;
        } else {
            entry.status = StartupStatus::Enabled;
        }

        StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
    }

    if (shouldUninitCom) CoUninitialize();
}

namespace {

void AppendTaskEntries(ITaskFolder* folder, ScanSummary& result) {
    if (!folder) return;

    IRegisteredTaskCollection* tasks = nullptr;
    if (FAILED(folder->GetTasks(TASK_ENUM_HIDDEN, &tasks)) || !tasks) return;

    LONG taskCount = 0;
    tasks->get_Count(&taskCount);
    for (LONG index = 1; index <= taskCount; ++index) {
        VARIANT item;
        VariantInit(&item);
        item.vt = VT_I4;
        item.lVal = index;

        IRegisteredTask* task = nullptr;
        if (SUCCEEDED(tasks->get_Item(item, &task)) && task) {
            BSTR name = nullptr;
            BSTR path = nullptr;
            VARIANT_BOOL enabled = VARIANT_TRUE;
            TASK_STATE taskState = TASK_STATE_UNKNOWN;
            task->get_Name(&name);
            task->get_Path(&path);
            task->get_Enabled(&enabled);
            task->get_State(&taskState);

            std::string taskPath = path ? Narrow(path) : "";
            std::string taskName = name ? Narrow(name) : "";

            StartupEntry entry;
            entry.id = IdFor("ScheduledTask", taskPath.empty() ? taskName : taskPath);
            entry.name = taskName.empty() ? "Task" : taskName;
            entry.scope = "本机系统";
            entry.source = "任务计划程序\\" + taskPath;
            entry.category = StartupCategory::ScheduledTasks;
            entry.location = StartupLocation::ScheduledTask;

            bool isEnabled = (enabled == VARIANT_TRUE) && (taskState != TASK_STATE_DISABLED);
            entry.status = isEnabled ? StartupStatus::Enabled : StartupStatus::Disabled;
            entry.groupTitle = "任务计划程序 (Task Scheduler)";

            ITaskDefinition* definition = nullptr;
            if (SUCCEEDED(task->get_Definition(&definition)) && definition) {
                // Extract Triggers (启动类型/触发条件)
                ITriggerCollection* triggers = nullptr;
                if (SUCCEEDED(definition->get_Triggers(&triggers)) && triggers) {
                    LONG triggerCount = 0;
                    triggers->get_Count(&triggerCount);
                    std::vector<std::string> trigNames;
                    for (LONG tIdx = 1; tIdx <= triggerCount; ++tIdx) {
                        ITrigger* trig = nullptr;
                        if (SUCCEEDED(triggers->get_Item(tIdx, &trig)) && trig) {
                            TASK_TRIGGER_TYPE2 tType = TASK_TRIGGER_TIME;
                            trig->get_Type(&tType);
                            switch (tType) {
                                case TASK_TRIGGER_EVENT: trigNames.push_back("事件触发"); break;
                                case TASK_TRIGGER_TIME: trigNames.push_back("定时计划"); break;
                                case TASK_TRIGGER_DAILY: trigNames.push_back("每天定时"); break;
                                case TASK_TRIGGER_WEEKLY: trigNames.push_back("每周定时"); break;
                                case TASK_TRIGGER_MONTHLY:
                                case TASK_TRIGGER_MONTHLYDOW: trigNames.push_back("每月定时"); break;
                                case TASK_TRIGGER_IDLE: trigNames.push_back("空闲时触发"); break;
                                case TASK_TRIGGER_REGISTRATION: trigNames.push_back("注册/创建时"); break;
                                case TASK_TRIGGER_BOOT: trigNames.push_back("开机自启"); break;
                                case TASK_TRIGGER_LOGON: trigNames.push_back("登录时触发"); break;
                                case TASK_TRIGGER_SESSION_STATE_CHANGE: trigNames.push_back("会话解锁/变更"); break;
                                default: trigNames.push_back("计划触发"); break;
                            }
                            trig->Release();
                        }
                    }
                    triggers->Release();

                    if (trigNames.empty()) {
                        entry.triggerInfo = "手动作业";
                    } else {
                        for (size_t i = 0; i < trigNames.size(); ++i) {
                            if (i > 0) entry.triggerInfo += ", ";
                            entry.triggerInfo += trigNames[i];
                        }
                    }
                }

                IActionCollection* actions = nullptr;
                if (SUCCEEDED(definition->get_Actions(&actions)) && actions) {
                    LONG actionCount = 0;
                    actions->get_Count(&actionCount);
                    for (LONG actionIndex = 1; actionIndex <= actionCount; ++actionIndex) {
                        VARIANT actionItem;
                        VariantInit(&actionItem);
                        actionItem.vt = VT_I4;
                        actionItem.lVal = actionIndex;
                        IAction* action = nullptr;
                        if (SUCCEEDED(actions->get_Item(actionIndex, &action)) && action) {
                            TASK_ACTION_TYPE actionType = TASK_ACTION_EXEC;
                            action->get_Type(&actionType);
                            if (actionType == TASK_ACTION_EXEC) {
                                IExecAction* execAction = nullptr;
                                if (SUCCEEDED(action->QueryInterface(IID_IExecAction, reinterpret_cast<void**>(&execAction))) && execAction) {
                                    BSTR execPath = nullptr;
                                    BSTR arguments = nullptr;
                                    execAction->get_Path(&execPath);
                                    execAction->get_Arguments(&arguments);
                                    if (execPath) entry.executablePath = Narrow(execPath);
                                    if (arguments) entry.arguments = Narrow(arguments);
                                    entry.command = entry.executablePath;
                                    if (!entry.arguments.empty()) entry.command += " " + entry.arguments;
                                    if (execPath) SysFreeString(execPath);
                                    if (arguments) SysFreeString(arguments);
                                    execAction->Release();
                                }
                            } else if (actionType == TASK_ACTION_COM_HANDLER) {
                                IComHandlerAction* comAction = nullptr;
                                if (SUCCEEDED(action->QueryInterface(IID_IComHandlerAction, reinterpret_cast<void**>(&comAction))) && comAction) {
                                    BSTR classId = nullptr;
                                    BSTR data = nullptr;
                                    comAction->get_ClassId(&classId);
                                    comAction->get_Data(&data);
                                    std::string clsidStr = classId ? Narrow(classId) : "";
                                    std::string dataStr = data ? Narrow(data) : "";
                                    if (classId) SysFreeString(classId);
                                    if (data) SysFreeString(data);
                                    comAction->Release();

                                    std::string dllPath = ResolveClsidServer(clsidStr);
                                    if (!dllPath.empty()) {
                                        entry.executablePath = dllPath;
                                        entry.command = "[COM 处理器] " + clsidStr + " (" + dllPath + ")";
                                    } else {
                                        entry.command = "[COM 处理器] " + clsidStr;
                                    }
                                }
                            }
                            action->Release();
                        }
                        VariantClear(&actionItem);
                    }
                    actions->Release();
                }
                definition->Release();
            }

            if (entry.source.find("\\Microsoft\\Windows\\") != std::string::npos) {
                entry.canDisable = false;
            }
            if (name) SysFreeString(name);
            if (path) SysFreeString(path);
            StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
            task->Release();
        }
        VariantClear(&item);
    }
    tasks->Release();

    ITaskFolderCollection* folders = nullptr;
    if (FAILED(folder->GetFolders(0, &folders)) || !folders) return;
    LONG folderCount = 0;
    folders->get_Count(&folderCount);
    for (LONG index = 1; index <= folderCount; ++index) {
        VARIANT item;
        VariantInit(&item);
        item.vt = VT_I4;
        item.lVal = index;
        ITaskFolder* child = nullptr;
        if (SUCCEEDED(folders->get_Item(item, &child)) && child) {
            AppendTaskEntries(child, result);
            child->Release();
        }
        VariantClear(&item);
    }
    folders->Release();
}

} // namespace

void StartupScanner::ScanScheduledTasks(ScanSummary& result) {
    HRESULT initResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool shouldUninitialize = SUCCEEDED(initResult);
    if (FAILED(initResult) && initResult != RPC_E_CHANGED_MODE) {
        ++result.failedSources;
        return;
    }

    ITaskService* service = nullptr;
    HRESULT resultCode = CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskService, reinterpret_cast<void**>(&service));
    if (SUCCEEDED(resultCode) && service) {
        VARIANT emptyVariant;
        VariantInit(&emptyVariant);
        resultCode = service->Connect(emptyVariant, emptyVariant, emptyVariant, emptyVariant);
        VariantClear(&emptyVariant);
        ITaskFolder* root = nullptr;
        BSTR rootPath = SysAllocString(L"\\");
        const HRESULT folderResult = service->GetFolder(rootPath, &root);
        SysFreeString(rootPath);
        if (SUCCEEDED(resultCode) && SUCCEEDED(folderResult) && root) {
            AppendTaskEntries(root, result);
            root->Release();
        } else {
            ++result.failedSources;
        }
        service->Release();
    } else {
        ++result.failedSources;
    }

    if (shouldUninitialize) CoUninitialize();
}

void StartupScanner::ScanServices(ScanSummary& result) {
    SC_HANDLE manager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
    if (!manager) { ++result.failedSources; return; }
    DWORD needed = 0, count = 0, resume = 0;
    EnumServicesStatusExW(manager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL, nullptr, 0, &needed, &count, &resume, nullptr);
    if (needed == 0) { CloseServiceHandle(manager); return; }
    std::vector<BYTE> buffer(needed);
    if (!EnumServicesStatusExW(manager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL, buffer.data(), static_cast<DWORD>(buffer.size()), &needed, &count, &resume, nullptr)) {
        CloseServiceHandle(manager);
        ++result.failedSources;
        return;
    }
    auto* services = reinterpret_cast<ENUM_SERVICE_STATUS_PROCESSW*>(buffer.data());
    for (DWORD i = 0; i < count; ++i) {
        SC_HANDLE hSvc = OpenServiceW(manager, services[i].lpServiceName, SERVICE_QUERY_CONFIG);
        DWORD startType = SERVICE_DEMAND_START;
        std::wstring binPath;
        if (hSvc) {
            DWORD cfgNeeded = 0;
            QueryServiceConfigW(hSvc, nullptr, 0, &cfgNeeded);
            if (cfgNeeded > 0) {
                std::vector<BYTE> cfgBuf(cfgNeeded);
                auto* cfg = reinterpret_cast<QUERY_SERVICE_CONFIGW*>(cfgBuf.data());
                if (QueryServiceConfigW(hSvc, cfg, cfgNeeded, &cfgNeeded)) {
                    startType = cfg->dwStartType;
                    if (cfg->lpBinaryPathName) binPath = cfg->lpBinaryPathName;
                }
            }
            CloseServiceHandle(hSvc);
        }

        if (startType != SERVICE_AUTO_START && startType != SERVICE_BOOT_START && startType != SERVICE_SYSTEM_START) {
            continue;
        }

        StartupEntry entry;
        entry.id = IdFor("Service", Narrow(services[i].lpServiceName));
        entry.name = Narrow(services[i].lpServiceName);
        entry.description = Narrow(services[i].lpDisplayName);
        entry.scope = "本机系统";
        entry.command = Narrow(binPath);
        entry.source = "HKLM\\System\\CurrentControlSet\\Services\\" + Narrow(services[i].lpServiceName);
        entry.category = StartupCategory::Services;
        entry.location = StartupLocation::Service;
        entry.groupTitle = "HKLM\\System\\CurrentControlSet\\Services";
        entry.status = (services[i].ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING) ? StartupStatus::Enabled : StartupStatus::Disabled;
        entry.canDelete = false;
        StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
    }
    CloseServiceHandle(manager);
}

void StartupScanner::ScanDrivers(ScanSummary& result) {
    SC_HANDLE manager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
    if (!manager) return;
    DWORD needed = 0, count = 0, resume = 0;
    EnumServicesStatusExW(manager, SC_ENUM_PROCESS_INFO, SERVICE_DRIVER, SERVICE_STATE_ALL, nullptr, 0, &needed, &count, &resume, nullptr);
    if (needed == 0) { CloseServiceHandle(manager); return; }
    std::vector<BYTE> buffer(needed);
    if (!EnumServicesStatusExW(manager, SC_ENUM_PROCESS_INFO, SERVICE_DRIVER, SERVICE_STATE_ALL, buffer.data(), static_cast<DWORD>(buffer.size()), &needed, &count, &resume, nullptr)) {
        CloseServiceHandle(manager);
        return;
    }
    auto* services = reinterpret_cast<ENUM_SERVICE_STATUS_PROCESSW*>(buffer.data());
    for (DWORD i = 0; i < count; ++i) {
        SC_HANDLE hSvc = OpenServiceW(manager, services[i].lpServiceName, SERVICE_QUERY_CONFIG);
        DWORD startType = SERVICE_DEMAND_START;
        std::wstring binPath;
        if (hSvc) {
            DWORD cfgNeeded = 0;
            QueryServiceConfigW(hSvc, nullptr, 0, &cfgNeeded);
            if (cfgNeeded > 0) {
                std::vector<BYTE> cfgBuf(cfgNeeded);
                auto* cfg = reinterpret_cast<QUERY_SERVICE_CONFIGW*>(cfgBuf.data());
                if (QueryServiceConfigW(hSvc, cfg, cfgNeeded, &cfgNeeded)) {
                    startType = cfg->dwStartType;
                    if (cfg->lpBinaryPathName) binPath = cfg->lpBinaryPathName;
                }
            }
            CloseServiceHandle(hSvc);
        }

        if (startType != SERVICE_AUTO_START && startType != SERVICE_BOOT_START && startType != SERVICE_SYSTEM_START) {
            continue;
        }

        StartupEntry entry;
        entry.id = IdFor("Driver", Narrow(services[i].lpServiceName));
        entry.name = Narrow(services[i].lpServiceName);
        entry.description = Narrow(services[i].lpDisplayName);
        entry.scope = "本机系统";
        entry.command = Narrow(binPath);
        entry.source = "HKLM\\System\\CurrentControlSet\\Services\\" + Narrow(services[i].lpServiceName);
        entry.category = StartupCategory::Drivers;
        entry.location = StartupLocation::Driver;
        entry.groupTitle = "HKLM\\System\\CurrentControlSet\\Services (Drivers)";
        entry.status = (services[i].ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING) ? StartupStatus::Enabled : StartupStatus::Disabled;
        entry.canDelete = false;
        StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
    }
    CloseServiceHandle(manager);
}

void StartupScanner::ScanImageHijacks(ScanSummary& result) {
    const char* ifeoKey = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options";
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, ifeoKey, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS) {
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, ifeoKey, 0, KEY_READ, &key) != ERROR_SUCCESS) return;
    }

    for (DWORD index = 0;; ++index) {
        char subKeyName[256]{};
        DWORD subKeyLen = sizeof(subKeyName);
        if (RegEnumKeyExA(key, index, subKeyName, &subKeyLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;

        HKEY subKey = nullptr;
        if (RegOpenKeyExA(key, subKeyName, 0, KEY_READ, &subKey) == ERROR_SUCCESS) {
            char debuggerVal[1024]{};
            DWORD valSize = sizeof(debuggerVal);
            if (RegQueryValueExA(subKey, "Debugger", nullptr, nullptr, reinterpret_cast<BYTE*>(debuggerVal), &valSize) == ERROR_SUCCESS && valSize > 0) {
                StartupEntry entry;
                entry.id = IdFor("IFEO", subKeyName);
                entry.name = subKeyName;
                entry.description = "IFEO 映像重定向调试器";
                entry.command = debuggerVal;
                entry.scope = "本机系统";
                entry.source = std::string("HKLM\\") + ifeoKey + "\\" + subKeyName;
                entry.category = StartupCategory::ImageHijacks;
                entry.location = StartupLocation::ImageHijack;
                entry.groupTitle = "HKLM\\" + std::string(ifeoKey);
                entry.status = StartupStatus::Enabled;
                entry.risk = RiskLevel::High;
                entry.riskReason = "检测到 IFEO 镜像劫持调试器，启动该程序将触发重定向拦截";
                entry.isHklm = true;
                StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
            }
            RegCloseKey(subKey);
        }
    }
    RegCloseKey(key);
}

void StartupScanner::ScanWinlogon(ScanSummary& result) {
    const char* winlogonKey = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, winlogonKey, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS) {
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, winlogonKey, 0, KEY_READ, &key) != ERROR_SUCCESS) return;
    }

    const char* valuesToCheck[] = { "Userinit", "Shell", "Taskman", "AppSetup", "GinaDLL", "UiHost", "AlternateShell" };
    for (const char* valName : valuesToCheck) {
        char val[1024]{};
        DWORD valSize = sizeof(val);
        if (RegQueryValueExA(key, valName, nullptr, nullptr, reinterpret_cast<BYTE*>(val), &valSize) == ERROR_SUCCESS && valSize > 0) {
            std::string valStr = val;
            StartupEntry entry;
            entry.id = IdFor("Winlogon", valName);
            entry.name = valName;
            entry.command = valStr;
            entry.scope = "本机系统";
            entry.source = std::string("HKLM\\") + winlogonKey + "@" + valName;
            entry.category = StartupCategory::Winlogon;
            entry.location = StartupLocation::Winlogon;
            entry.groupTitle = "HKLM\\" + std::string(winlogonKey);
            entry.status = StartupStatus::Enabled;
            entry.isHklm = true;

            if (strcmp(valName, "Shell") == 0 && (valStr == "explorer.exe" || valStr == "Explorer.exe")) {
                entry.description = "Windows 资源管理器";
                entry.risk = RiskLevel::Safe;
                entry.riskReason = "Windows 标准外壳程序";
                entry.status = StartupStatus::Protected;
                entry.canDisable = false;
            } else if (strcmp(valName, "Userinit") == 0 && valStr.find("userinit.exe") != std::string::npos) {
                entry.description = "用户初始化登录外壳";
                entry.risk = RiskLevel::Safe;
                entry.riskReason = "Windows 标准登录初始化程序";
                entry.status = StartupStatus::Protected;
                entry.canDisable = false;
            } else if (strcmp(valName, "AlternateShell") == 0 && valStr == "cmd.exe") {
                entry.description = "安全模式命令行外壳";
                entry.risk = RiskLevel::Safe;
                entry.riskReason = "Windows 标准安全模式外壳";
                entry.status = StartupStatus::Protected;
                entry.canDisable = false;
            } else if (strcmp(valName, "UiHost") == 0 && valStr.find("explorer.exe") != std::string::npos) {
                entry.description = "登录界面宿主进程";
                entry.risk = RiskLevel::Safe;
                entry.riskReason = "标准登录界面宿主";
                entry.status = StartupStatus::Protected;
                entry.canDisable = false;
            } else {
                entry.description = "非标准登录组件";
                entry.risk = RiskLevel::High;
                entry.riskReason = "非标准 Winlogon 启动项，存在登录劫持风险";
            }
            StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
        }
    }

    // Winlogon\Notify: 登录/注销/启动等事件时被 winlogon 加载的通知包 DLL
    HKEY notifyKey = nullptr;
    std::string notifyPath = std::string(winlogonKey) + "\\Notify";
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, notifyPath.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &notifyKey) == ERROR_SUCCESS ||
        RegOpenKeyExA(HKEY_LOCAL_MACHINE, notifyPath.c_str(), 0, KEY_READ, &notifyKey) == ERROR_SUCCESS) {
        for (DWORD index = 0;; ++index) {
            char pkgName[256]{};
            DWORD nameLen = sizeof(pkgName);
            if (RegEnumKeyExA(notifyKey, index, pkgName, &nameLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;

            HKEY pkgKey = nullptr;
            if (RegOpenKeyExA(notifyKey, pkgName, 0, KEY_READ, &pkgKey) == ERROR_SUCCESS) {
                char dllVal[1024]{};
                DWORD valSize = sizeof(dllVal);
                if (RegQueryValueExA(pkgKey, "DLL", nullptr, nullptr, reinterpret_cast<BYTE*>(dllVal), &valSize) == ERROR_SUCCESS && valSize > 1) {
                    StartupEntry entry;
                    entry.id = IdFor("WinlogonNotify", pkgName);
                    entry.name = pkgName;
                    entry.description = "Winlogon 事件通知包";
                    entry.command = dllVal;
                    entry.scope = "本机系统";
                    entry.source = "HKLM\\" + notifyPath + "\\" + pkgName;
                    entry.category = StartupCategory::Winlogon;
                    entry.location = StartupLocation::Winlogon;
                    entry.groupTitle = "HKLM\\" + notifyPath;
                    entry.status = StartupStatus::Enabled;
                    entry.isHklm = true;
                    entry.canDelete = false;
                    entry.risk = RiskLevel::High;
                    entry.riskReason = "Winlogon Notify DLL 在登录事件时被加载，是常见的权限维持手段";
                    StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
                }
                RegCloseKey(pkgKey);
            }
        }
        RegCloseKey(notifyKey);
    }

    RegCloseKey(key);
}

void StartupScanner::ScanActiveSetup(ScanSummary& result) {
    // 同时扫描 64 位视图与 WOW64（32 位进程的 Active Setup 存根）
    const char* activeKeys[] = {
        "SOFTWARE\\Microsoft\\Active Setup\\Installed Components",
        "SOFTWARE\\Wow6432Node\\Microsoft\\Active Setup\\Installed Components",
    };
    for (const char* activeKey : activeKeys) {
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, activeKey, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS) {
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, activeKey, 0, KEY_READ, &key) != ERROR_SUCCESS) continue;
    }

    for (DWORD index = 0;; ++index) {
        char subKeyName[256]{};
        DWORD subKeyLen = sizeof(subKeyName);
        if (RegEnumKeyExA(key, index, subKeyName, &subKeyLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;

        HKEY subKey = nullptr;
        if (RegOpenKeyExA(key, subKeyName, 0, KEY_READ, &subKey) == ERROR_SUCCESS) {
            char stubPath[1024]{};
            DWORD valSize = sizeof(stubPath);
            if (RegQueryValueExA(subKey, "StubPath", nullptr, nullptr, reinterpret_cast<BYTE*>(stubPath), &valSize) == ERROR_SUCCESS && valSize > 0) {
                char compName[256]{};
                DWORD compLen = sizeof(compName);
                RegQueryValueExA(subKey, nullptr, nullptr, nullptr, reinterpret_cast<BYTE*>(compName), &compLen);

                StartupEntry entry;
                entry.id = IdFor("ActiveSetup", std::string(activeKey) + ":" + subKeyName);
                entry.name = compLen > 0 ? compName : subKeyName;
                entry.command = stubPath;
                entry.scope = "本机系统";
                entry.source = std::string("HKLM\\") + activeKey + "\\" + subKeyName;
                entry.category = StartupCategory::Logon;
                entry.location = StartupLocation::ActiveSetup;
                entry.groupTitle = "HKLM\\" + std::string(activeKey);
                entry.status = StartupStatus::Enabled;
                entry.isHklm = true;
                StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
            }
            RegCloseKey(subKey);
        }
    }
    RegCloseKey(key);
    }
}

void StartupScanner::ScanComExtensions(ScanSummary& result) {
    const char* approvedKey = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved";
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, approvedKey, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS) {
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, approvedKey, 0, KEY_READ, &key) != ERROR_SUCCESS) return;
    }

    for (DWORD index = 0;; ++index) {
        char clsid[256]{};
        char desc[512]{};
        DWORD clsidLen = sizeof(clsid);
        DWORD descLen = sizeof(desc);
        if (RegEnumValueA(key, index, clsid, &clsidLen, nullptr, nullptr, reinterpret_cast<BYTE*>(desc), &descLen) != ERROR_SUCCESS) break;

        std::string clsidStr = clsid;
        std::string dllPath = ResolveClsidServer(clsidStr);

        StartupEntry entry;
        entry.id = IdFor("ShellExt", clsid);
        entry.name = (descLen > 0 && strlen(desc) > 0) ? desc : clsid;
        entry.description = std::string("Shell 扩展组件 (") + clsid + ")";
        entry.command = !dllPath.empty() ? dllPath : clsid;
        entry.executablePath = dllPath;
        entry.scope = "本机系统";
        entry.source = std::string("CLSID: ") + clsid;
        entry.category = StartupCategory::Explorer;
        entry.location = StartupLocation::ComHijack;
        entry.groupTitle = "HKLM\\" + std::string(approvedKey);
        entry.status = StartupStatus::Enabled;
        entry.isHklm = true;
        entry.canDisable = false;
        entry.risk = RiskLevel::Safe;
        entry.riskReason = "已批准的外壳扩展组件";
        StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
    }
    RegCloseKey(key);
}

void StartupScanner::ScanAppInitDlls(ScanSummary& result) {
    // 同时扫描 64 位视图与 WOW64（32 位进程加载的 AppInit_DLLs）
    const char* keyPaths[] = {
        "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows",
        "SOFTWARE\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion\\Windows",
    };
    for (const char* keyPath : keyPaths) {
        HKEY key = nullptr;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ | KEY_WOW64_64KEY, &key) == ERROR_SUCCESS ||
            RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &key) == ERROR_SUCCESS) {
            char val[1024]{};
            DWORD size = sizeof(val);
            if (RegQueryValueExA(key, "AppInit_DLLs", nullptr, nullptr, reinterpret_cast<BYTE*>(val), &size) == ERROR_SUCCESS && size > 0 && strlen(val) > 0) {
                StartupEntry entry;
                entry.id = IdFor("AppInit", keyPath);
                entry.name = "AppInit_DLLs";
                entry.command = val;
                entry.scope = "本机系统";
                entry.source = std::string("HKLM\\") + keyPath + "@AppInit_DLLs";
                entry.category = StartupCategory::AppInit;
                entry.location = StartupLocation::AppInit;
                entry.groupTitle = "HKLM\\" + std::string(keyPath);
                entry.status = StartupStatus::Enabled;
                entry.risk = RiskLevel::High;
                entry.riskReason = "AppInit_DLLs 会被注入到所有载入 User32.dll 的进程中";
                StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
            }
            RegCloseKey(key);
        }
    }
}

void StartupScanner::ScanKnownDlls(ScanSummary& result) {
    const char* keyPath = "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\KnownDLLs";
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &key) == ERROR_SUCCESS) {
        for (DWORD index = 0; index < 20; ++index) {
            char name[256]{};
            char val[512]{};
            DWORD nLen = sizeof(name), vLen = sizeof(val);
            if (RegEnumValueA(key, index, name, &nLen, nullptr, nullptr, reinterpret_cast<BYTE*>(val), &vLen) != ERROR_SUCCESS) break;
            StartupEntry entry;
            entry.id = IdFor("KnownDLLs", name);
            entry.name = name;
            entry.command = val;
            entry.scope = "本机系统";
            entry.source = std::string("HKLM\\") + keyPath;
            entry.category = StartupCategory::KnownDlls;
            entry.location = StartupLocation::KnownDlls;
            entry.groupTitle = "HKLM\\" + std::string(keyPath);
            entry.status = StartupStatus::Protected;
            entry.risk = RiskLevel::Safe;
            entry.canDisable = false;
            StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
        }
        RegCloseKey(key);
    }
}

void StartupScanner::ScanBootExecute(ScanSummary& result) {
    const char* keyPath = "SYSTEM\\CurrentControlSet\\Control\\Session Manager";
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &key) == ERROR_SUCCESS) {
        char val[1024]{};
        DWORD size = sizeof(val);
        if (RegQueryValueExA(key, "BootExecute", nullptr, nullptr, reinterpret_cast<BYTE*>(val), &size) == ERROR_SUCCESS && size > 0) {
            StartupEntry entry;
            entry.id = IdFor("BootExecute", "BootExecute");
            entry.name = "BootExecute";
            entry.command = val;
            entry.scope = "本机系统";
            entry.source = std::string("HKLM\\") + keyPath + "@BootExecute";
            entry.category = StartupCategory::BootExecute;
            entry.location = StartupLocation::BootExecute;
            entry.groupTitle = "HKLM\\" + std::string(keyPath);
            entry.status = StartupStatus::Protected;
            entry.risk = RiskLevel::Safe;
            StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
        }

        // Session Manager 的 SetupExecute / Execute 多字符串值（启动早期执行）
        const char* extraValues[] = { "SetupExecute", "Execute" };
        for (const char* valName : extraValues) {
            char val[2048]{};
            DWORD size = sizeof(val);
            DWORD type = 0;
            if (RegQueryValueExA(key, valName, nullptr, &type, reinterpret_cast<BYTE*>(val), &size) == ERROR_SUCCESS && size > 2) {
                // REG_MULTI_SZ: 将内部分隔的 '\0' 替换为空格以便展示
                if (type == REG_MULTI_SZ) {
                    for (DWORD c = 0; c + 1 < size; ++c) {
                        if (val[c] == '\0' && val[c + 1] != '\0') val[c] = ' ';
                    }
                }
                StartupEntry entry;
                entry.id = IdFor("SessionManager", valName);
                entry.name = valName;
                entry.command = val;
                entry.scope = "本机系统";
                entry.source = std::string("HKLM\\") + keyPath + "@" + valName;
                entry.category = StartupCategory::BootExecute;
                entry.location = StartupLocation::BootExecute;
                entry.groupTitle = "HKLM\\" + std::string(keyPath);
                entry.status = StartupStatus::Enabled;
                entry.canDelete = false;
                entry.risk = RiskLevel::Suspicious;
                entry.riskReason = "Session Manager 会话管理器早期执行入口，非标准值存在引导期劫持风险";
                StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
            }
        }

        RegCloseKey(key);
    }
}

void StartupScanner::ScanWinsockProviders(ScanSummary& result) {
    const char* nsKeyPath = "SYSTEM\\CurrentControlSet\\Services\\WinSock2\\Parameters\\NameSpace_Catalog5_0\\Catalog_Entries";
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, nsKeyPath, 0, KEY_READ, &key) == ERROR_SUCCESS) {
        for (DWORD index = 0; index < 20; ++index) {
            char subKey[256]{};
            DWORD len = sizeof(subKey);
            if (RegEnumKeyExA(key, index, subKey, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;

            // 读取命名空间提供程序的实际 DLL 路径
            char libPath[1024]{};
            DWORD libSize = sizeof(libPath);
            HKEY entryKey = nullptr;
            std::string command = "Winsock 命名空间提供程序";
            if (RegOpenKeyExA(key, subKey, 0, KEY_READ, &entryKey) == ERROR_SUCCESS) {
                RegQueryValueExA(entryKey, "LibraryPath", nullptr, nullptr, reinterpret_cast<BYTE*>(libPath), &libSize);
                RegCloseKey(entryKey);
                if (libSize > 1) command = libPath;
            }

            StartupEntry entry;
            entry.id = IdFor("WinsockNSP", subKey);
            entry.name = subKey;
            entry.command = command;
            entry.scope = "本机系统";
            entry.source = std::string("HKLM\\") + nsKeyPath + "\\" + subKey;
            entry.category = StartupCategory::Winsock;
            entry.location = StartupLocation::WinsockProvider;
            entry.groupTitle = "HKLM\\" + std::string(nsKeyPath);
            entry.status = StartupStatus::Protected;
            entry.canDisable = false;
            entry.risk = RiskLevel::Safe;
            entry.riskReason = "Winsock 命名空间提供程序，域名解析请求会流经该 DLL";
            StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
        }
        RegCloseKey(key);
    }

    // Winsock 协议目录 (LSP 分层服务提供程序)：网络数据流必经的注入点
    const char* protoKeyPath = "SYSTEM\\CurrentControlSet\\Services\\WinSock2\\Parameters\\Protocol_Catalog9\\Catalog_Entries";
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, protoKeyPath, 0, KEY_READ, &key) == ERROR_SUCCESS) {
        for (DWORD index = 0; index < 64; ++index) {
            char subKey[256]{};
            DWORD len = sizeof(subKey);
            if (RegEnumKeyExA(key, index, subKey, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;

            char libPath[1024]{};
            DWORD libSize = sizeof(libPath);
            HKEY entryKey = nullptr;
            std::string command;
            if (RegOpenKeyExA(key, subKey, 0, KEY_READ, &entryKey) == ERROR_SUCCESS) {
                RegQueryValueExA(entryKey, "LibraryPath", nullptr, nullptr, reinterpret_cast<BYTE*>(libPath), &libSize);
                RegCloseKey(entryKey);
                command = (libSize > 1) ? libPath : "(未知)";
            }

            StartupEntry entry;
            entry.id = IdFor("WinsockLSP", std::string(protoKeyPath) + ":" + subKey);
            entry.name = subKey;
            entry.command = command;
            entry.scope = "本机系统";
            entry.source = std::string("HKLM\\") + protoKeyPath + "\\" + subKey;
            entry.category = StartupCategory::Winsock;
            entry.location = StartupLocation::WinsockProvider;
            entry.groupTitle = "HKLM\\" + std::string(protoKeyPath);
            entry.status = StartupStatus::Enabled;
            entry.canDisable = false;
            entry.risk = RiskLevel::Notice;
            entry.riskReason = "WinSock 分层服务提供程序 (LSP)：所有套接字网络数据都会流经该 DLL";
            StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
        }
        RegCloseKey(key);
    }
}

void StartupScanner::ScanPrintMonitors(ScanSummary& result) {
    const char* keyPath = "SYSTEM\\CurrentControlSet\\Control\\Print\\Monitors";
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &key) == ERROR_SUCCESS) {
        for (DWORD index = 0;; ++index) {
            char subKey[256]{};
            DWORD len = sizeof(subKey);
            if (RegEnumKeyExA(key, index, subKey, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
            StartupEntry entry;
            entry.id = IdFor("PrintMonitor", subKey);
            entry.name = subKey;
            entry.command = "打印机驱动监视器";
            entry.scope = "本机系统";
            entry.source = std::string("HKLM\\") + keyPath + "\\" + subKey;
            entry.category = StartupCategory::PrintMonitors;
            entry.location = StartupLocation::PrintMonitor;
            entry.groupTitle = "HKLM\\" + std::string(keyPath);
            entry.status = StartupStatus::Enabled;
            entry.risk = RiskLevel::Safe;
            StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
        }
        RegCloseKey(key);
    }
}

void StartupScanner::ScanLsaPackages(ScanSummary& result) {
    const char* keyPath = "SYSTEM\\CurrentControlSet\\Control\\Lsa";
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &key) == ERROR_SUCCESS) {
        const char* packageValues[] = { "Authentication Packages", "Notification Packages", "Security Packages" };
        for (const char* valName : packageValues) {
            char val[2048]{};
            DWORD size = sizeof(val);
            DWORD type = 0;
            if (RegQueryValueExA(key, valName, nullptr, &type, reinterpret_cast<BYTE*>(val), &size) == ERROR_SUCCESS && size > 2) {
                bool isStandard = (val[0] == '\0');
                if (type == REG_MULTI_SZ) {
                    for (DWORD c = 0; c + 1 < size; ++c) {
                        if (val[c] == '\0' && val[c + 1] != '\0') val[c] = ' ';
                    }
                }
                StartupEntry entry;
                entry.id = IdFor("LSA", valName);
                entry.name = valName;
                entry.command = val;
                entry.scope = "本机系统";
                entry.source = std::string("HKLM\\") + keyPath + "@" + valName;
                entry.category = StartupCategory::Lsa;
                entry.location = StartupLocation::LsaPackage;
                entry.groupTitle = "HKLM\\" + std::string(keyPath);
                entry.status = StartupStatus::Protected;
                entry.canDisable = false;
                entry.risk = RiskLevel::Safe;
                entry.riskReason = "LSA 标准认证/通知/安全包";
                if (strcmp(valName, "Security Packages") == 0 && !isStandard) {
                    entry.risk = RiskLevel::High;
                    entry.riskReason = "自定义 Security Package 可能是凭据窃取（如 mimilib）的持久化手段";
                }
                StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
            }
        }
        RegCloseKey(key);
    }
}

void StartupScanner::ScanWmiSubscriptions(ScanSummary& result) {
    HRESULT initResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool shouldUninitialize = SUCCEEDED(initResult);
    if (FAILED(initResult) && initResult != RPC_E_CHANGED_MODE) {
        ++result.failedSources;
        return;
    }

    IWbemLocator* locator = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, reinterpret_cast<void**>(&locator));
    if (FAILED(hr) || !locator) {
        if (shouldUninitialize) CoUninitialize();
        ++result.failedSources;
        return;
    }

    IWbemServices* services = nullptr;
    BSTR ns = SysAllocString(L"ROOT\\SUBSCRIPTION");
    hr = locator->ConnectServer(ns, nullptr, nullptr, nullptr, 0, nullptr, nullptr, &services);
    SysFreeString(ns);
    locator->Release();
    if (FAILED(hr) || !services) {
        if (shouldUninitialize) CoUninitialize();
        ++result.failedSources;
        return;
    }
    CoSetProxyBlanket(services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
        RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);

    auto getString = [](IWbemClassObject* obj, const wchar_t* prop) -> std::wstring {
        VARIANT v;
        VariantInit(&v);
        std::wstring out;
        if (SUCCEEDED(obj->Get(prop, 0, &v, nullptr, nullptr))) {
            if (v.vt == VT_BSTR && v.bstrVal) out = v.bstrVal;
            else if (v.vt == VT_NULL) out.clear();
            VariantClear(&v);
        }
        return out;
    };

    auto execQuery = [&](const wchar_t* q, IEnumWbemClassObject** penum) -> bool {
        BSTR lang = SysAllocString(L"WQL");
        BSTR query = SysAllocString(q);
        const HRESULT r = services->ExecQuery(lang, query, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, penum);
        SysFreeString(lang);
        SysFreeString(query);
        return SUCCEEDED(r) && *penum;
    };

    // 第一遍：枚举所有永久事件消费者
    struct ConsumerInfo {
        std::wstring relPath;   // 例如 __EventConsumer.Name="X"
        std::wstring className;
        std::wstring name;
        std::wstring command;   // CommandLineTemplate / ScriptText
        size_t filterCount = 0;
    };
    std::vector<ConsumerInfo> consumers;

    IEnumWbemClassObject* enumObjects = nullptr;
    if (execQuery(L"SELECT * FROM __EventConsumer", &enumObjects)) {
        for (;;) {
            IWbemClassObject* obj = nullptr;
            ULONG returned = 0;
            if (enumObjects->Next(WBEM_INFINITE, 1, &obj, &returned) != WBEM_S_NO_ERROR || returned == 0 || !obj) break;

            ConsumerInfo info;
            info.className = getString(obj, L"__CLASS");
            info.name = getString(obj, L"Name");
            info.command = getString(obj, L"CommandLineTemplate");
            if (info.command.empty()) info.command = getString(obj, L"ScriptText");
            info.relPath = getString(obj, L"__RELPATH");
            consumers.push_back(info);
            obj->Release();
        }
        enumObjects->Release();
    }

    auto findConsumer = [&consumers](const std::wstring& consumerPath) -> ConsumerInfo* {
        for (auto& c : consumers) {
            if (!c.relPath.empty() && consumerPath.find(c.relPath) != std::wstring::npos) return &c;
            if (!c.name.empty() && consumerPath.find(c.name) != std::wstring::npos) return &c;
        }
        return nullptr;
    };

    // 第二遍：枚举 FilterToConsumerBinding，把过滤器查询语句附加到对应消费者上
    if (execQuery(L"SELECT * FROM __FilterToConsumerBinding", &enumObjects)) {
        for (;;) {
            IWbemClassObject* obj = nullptr;
            ULONG returned = 0;
            if (enumObjects->Next(WBEM_INFINITE, 1, &obj, &returned) != WBEM_S_NO_ERROR || returned == 0 || !obj) break;

            std::wstring consumerPath = getString(obj, L"Consumer");
            std::wstring filterPath = getString(obj, L"Filter");
            obj->Release();

            ConsumerInfo* consumer = findConsumer(consumerPath);
            if (!consumer) continue;
            ++consumer->filterCount;

            // 解析过滤器的 WQL 查询，拼接到描述中
            if (!filterPath.empty()) {
                IWbemClassObject* filterObj = nullptr;
                BSTR fp = SysAllocString(filterPath.c_str());
                if (SUCCEEDED(services->GetObject(fp, 0, nullptr, &filterObj, nullptr)) && filterObj) {
                    std::wstring query = getString(filterObj, L"Query");
                    filterObj->Release();
                    if (!query.empty()) consumer->command += L"   [触发条件: " + query + L"]";
                }
                SysFreeString(fp);
            }
        }
        enumObjects->Release();
    }

    for (const auto& c : consumers) {
        StartupEntry entry;
        entry.id = IdFor("WMISub", Narrow(c.className + L":" + (c.name.empty() ? c.relPath : c.name)));
        entry.name = (c.name.empty() ? Narrow(c.relPath) : Narrow(c.name));
        entry.description = "WMI 永久事件订阅消费者 (" + Narrow(c.className) + ")";
        entry.command = Narrow(c.command);
        entry.executablePath.clear();
        entry.scope = "本机系统";
        entry.source = "ROOT\\SUBSCRIPTION: " + Narrow(c.relPath);
        entry.category = StartupCategory::Wmi;
        entry.location = StartupLocation::WmiSubscription;
        entry.groupTitle = "ROOT\\SUBSCRIPTION (WMI 永久事件订阅)";
        entry.status = StartupStatus::Enabled;
        entry.canDisable = false;
        entry.canDelete = false;
        if (c.className.find(L"ActiveScript") != std::wstring::npos) {
            entry.risk = RiskLevel::High;
            entry.riskReason = "ActiveScriptEventConsumer 可执行任意脚本代码，是常见的无文件持久化手段";
        } else {
            entry.risk = RiskLevel::Suspicious;
            entry.riskReason = "WMI 永久事件订阅在系统事件触发时自动执行命令，难以通过常规手段发现";
        }
        StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
    }

    services->Release();
    if (shouldUninitialize) CoUninitialize();
}

void StartupScanner::ScanNetworkProviders(ScanSummary& result) {
    const char* keyPath = "SYSTEM\\CurrentControlSet\\Control\\NetworkProvider\\Order";
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &key) != ERROR_SUCCESS) return;

    char val[2048]{};
    DWORD size = sizeof(val);
    if (RegQueryValueExA(key, "ProviderOrder", nullptr, nullptr, reinterpret_cast<BYTE*>(val), &size) == ERROR_SUCCESS && size > 1) {
        // ProviderOrder 是逗号分隔的服务名列表，逐个解析其 DLL 路径
        char* context = nullptr;
        char* token = strtok_s(val, ", ", &context);
        while (token) {
            std::string providerSvcPath = std::string("SYSTEM\\CurrentControlSet\\Services\\") + token + "\\NetworkProvider";
            HKEY provKey = nullptr;
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, providerSvcPath.c_str(), 0, KEY_READ, &provKey) == ERROR_SUCCESS) {
                char dll[1024]{};
                DWORD dllSize = sizeof(dll);
                if (RegQueryValueExA(provKey, "ProviderPath", nullptr, nullptr, reinterpret_cast<BYTE*>(dll), &dllSize) == ERROR_SUCCESS && dllSize > 1) {
                    StartupEntry entry;
                    entry.id = IdFor("NetProvider", token);
                    entry.name = token;
                    entry.description = "网络提供商 DLL";
                    entry.command = dll;
                    entry.scope = "本机系统";
                    entry.source = "HKLM\\" + providerSvcPath + "@ProviderPath";
                    entry.category = StartupCategory::NetworkProviders;
                    entry.location = StartupLocation::NetworkProvider;
                    entry.groupTitle = "HKLM\\" + std::string(keyPath);
                    entry.status = StartupStatus::Enabled;
                    entry.canDisable = false;
                    entry.canDelete = false;
                    entry.risk = RiskLevel::Safe;
                    entry.riskReason = "已注册的网络提供商（多供应商路由器 MPR 会加载其 DLL）";
                    StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
                }
                RegCloseKey(provKey);
            }
            token = strtok_s(nullptr, ", ", &context);
        }
    }
    RegCloseKey(key);
}

void StartupScanner::ScanRunOnceEx(HKEY root, bool isHklm, const char* subKey, const char* scope, ScanSummary& result) {
    HKEY key = nullptr;
    REGSAM sam = KEY_READ | (isHklm ? KEY_WOW64_64KEY : 0);
    if (RegOpenKeyExA(root, subKey, 0, sam, &key) != ERROR_SUCCESS) {
        if (RegOpenKeyExA(root, subKey, 0, KEY_READ, &key) != ERROR_SUCCESS) return;
    }
    const std::string groupTitle = std::string(isHklm ? "HKLM\\" : "HKCU\\") + subKey;

    auto addValueEntries = [&](HKEY hKey, const std::string& parentPath) {
        for (DWORD index = 0;; ++index) {
            char name[256]{};
            char value[2048]{};
            DWORD nameLen = sizeof(name);
            DWORD valLen = sizeof(value);
            DWORD type = 0;
            if (RegEnumValueA(hKey, index, name, &nameLen, nullptr, &type, reinterpret_cast<BYTE*>(value), &valLen) != ERROR_SUCCESS) break;
            if ((type != REG_SZ && type != REG_EXPAND_SZ) || valLen <= 1) continue;
            if (_stricmp(name, "Flags") == 0 || _stricmp(name, "Title") == 0) continue;

            StartupEntry entry;
            entry.id = IdFor("RunOnceEx", parentPath + ":" + name);
            entry.name = name;
            entry.description = "RunOnceEx 待执行命令";
            entry.command = value;
            entry.scope = scope;
            entry.source = groupTitle + "\\" + parentPath;
            entry.registryKey = subKey;
            entry.registryValue = name;
            entry.isHklm = isHklm;
            entry.category = StartupCategory::Logon;
            entry.location = StartupLocation::RegistryRun;
            entry.groupTitle = groupTitle + "\\" + parentPath;
            entry.status = StartupStatus::Enabled;
            entry.canDisable = false;
            entry.canDelete = false;
            entry.risk = RiskLevel::Suspicious;
            entry.riskReason = "RunOnceEx 条目会在下次登录时执行一次，常被安装器与恶意软件滥用";
            StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
        }
    };

    addValueEntries(key, "");

    for (DWORD index = 0;; ++index) {
        char subKeyName[256]{};
        DWORD nameLen = sizeof(subKeyName);
        if (RegEnumKeyExA(key, index, subKeyName, &nameLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
        HKEY child = nullptr;
        if (RegOpenKeyExA(key, subKeyName, 0, KEY_READ, &child) == ERROR_SUCCESS) {
            addValueEntries(child, subKeyName);
            RegCloseKey(child);
        }
    }

    RegCloseKey(key);
}

void StartupScanner::ScanShellServiceObjects(ScanSummary& result) {
    struct Root { HKEY root; bool isHklm; const char* scope; const char* prefix; };
    const Root roots[] = {
        { HKEY_LOCAL_MACHINE, true, "本机系统", "HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellServiceObjects" },
        { HKEY_CURRENT_USER, false, "当前用户", "HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellServiceObjects" },
    };

    for (const Root& r : roots) {
        HKEY key = nullptr;
        REGSAM sam = KEY_READ | (r.isHklm ? KEY_WOW64_64KEY : 0);
        if (RegOpenKeyExA(r.root, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellServiceObjects", 0, sam, &key) != ERROR_SUCCESS) continue;

        for (DWORD index = 0;; ++index) {
            char clsid[256]{};
            DWORD len = sizeof(clsid);
            if (RegEnumKeyExA(key, index, clsid, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;

            std::string dllPath = ResolveClsidServer(clsid);
            StartupEntry entry;
            entry.id = IdFor("ShellSvcObj", std::string(r.prefix) + clsid);
            entry.name = clsid;
            entry.description = "资源管理器 Shell 服务对象（启动时自动创建的 COM 对象）";
            entry.command = !dllPath.empty() ? dllPath : clsid;
            entry.executablePath = dllPath;
            entry.scope = r.scope;
            entry.source = std::string(r.prefix) + "\\" + clsid;
            entry.category = StartupCategory::Explorer;
            entry.location = StartupLocation::ComHijack;
            entry.groupTitle = r.prefix;
            entry.status = StartupStatus::Enabled;
            entry.isHklm = r.isHklm;
            entry.canDisable = false;
            entry.canDelete = false;
            entry.risk = RiskLevel::Safe;
            entry.riskReason = "Explorer 启动时会自动实例化此处注册的 COM 服务对象";
            StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
        }
        RegCloseKey(key);
    }

    // ShellIconOverlayIdentifiers：资源管理器启动时加载的图标覆盖处理程序
    HKEY overlayKey = nullptr;
    const char* overlayPath = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers";
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, overlayPath, 0, KEY_READ | KEY_WOW64_64KEY, &overlayKey) == ERROR_SUCCESS) {
        for (DWORD index = 0;; ++index) {
            char displayName[256]{};
            char clsid[256]{};
            DWORD nameLen = sizeof(displayName);
            DWORD clsidLen = sizeof(clsid);
            if (RegEnumValueA(overlayKey, index, displayName, &nameLen, nullptr, nullptr, reinterpret_cast<BYTE*>(clsid), &clsidLen) != ERROR_SUCCESS) break;

            std::string trimmed = displayName;
            size_t first = trimmed.find_first_not_of(" \t");
            if (first == std::string::npos) continue;
            trimmed = trimmed.substr(first);

            std::string dllPath = ResolveClsidServer(clsid);
            StartupEntry entry;
            entry.id = IdFor("IconOverlay", trimmed + ":" + clsid);
            entry.name = trimmed;
            entry.description = "图标覆盖处理程序 (Shell Icon Overlay Identifier)";
            entry.command = !dllPath.empty() ? dllPath : clsid;
            entry.executablePath = dllPath;
            entry.scope = "本机系统";
            entry.source = std::string("HKLM\\") + overlayPath + ":" + trimmed;
            entry.category = StartupCategory::Explorer;
            entry.location = StartupLocation::ComHijack;
            entry.groupTitle = "HKLM\\" + std::string(overlayPath);
            entry.status = StartupStatus::Enabled;
            entry.isHklm = true;
            entry.canDisable = false;
            entry.canDelete = false;
            entry.risk = RiskLevel::Safe;
            entry.riskReason = "Explorer 启动时加载的图标覆盖扩展（如网盘同步状态图标）";
            StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
        }
        RegCloseKey(overlayKey);
    }
}

void StartupScanner::ScanBrowserHelpers(ScanSummary& result) {
    // Browser Helper Objects (BHO)：随 IE/WebView 加载的浏览器辅助对象
    const char* bhoKeys[] = {
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects",
        "SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects",
    };
    for (const char* bhoKey : bhoKeys) {
        HKEY key = nullptr;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, bhoKey, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS &&
            RegOpenKeyExA(HKEY_LOCAL_MACHINE, bhoKey, 0, KEY_READ, &key) != ERROR_SUCCESS) continue;

        for (DWORD index = 0;; ++index) {
            char clsid[256]{};
            DWORD len = sizeof(clsid);
            if (RegEnumKeyExA(key, index, clsid, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;

            std::string dllPath = ResolveClsidServer(clsid);
            StartupEntry entry;
            entry.id = IdFor("BHO", std::string(bhoKey) + clsid);
            entry.name = clsid;
            entry.description = "浏览器帮助对象 (BHO)";
            entry.command = !dllPath.empty() ? dllPath : clsid;
            entry.executablePath = dllPath;
            entry.scope = "本机系统";
            entry.source = std::string("HKLM\\") + bhoKey + "\\" + clsid;
            entry.category = StartupCategory::InternetExplorer;
            entry.location = StartupLocation::ComHijack;
            entry.groupTitle = "HKLM\\" + std::string(bhoKey);
            entry.status = StartupStatus::Enabled;
            entry.isHklm = true;
            entry.canDisable = false;
            entry.canDelete = false;
            entry.risk = RiskLevel::Safe;
            entry.riskReason = "浏览器启动时加载的辅助对象，可监控并修改所有网页请求";
            StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
        }
        RegCloseKey(key);
    }

    // IE 工具栏：键值名即为 CLSID
    const char* toolbarKeys[] = {
        "SOFTWARE\\Microsoft\\Internet Explorer\\Toolbar",
        "SOFTWARE\\Wow6432Node\\Microsoft\\Internet Explorer\\Toolbar",
    };
    for (const char* tbKey : toolbarKeys) {
        HKEY key = nullptr;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, tbKey, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS) continue;

        for (DWORD index = 0;; ++index) {
            char clsid[256]{};
            char data[512]{};
            DWORD clsidLen = sizeof(clsid);
            DWORD dataLen = sizeof(data);
            if (RegEnumValueA(key, index, clsid, &clsidLen, nullptr, nullptr, reinterpret_cast<BYTE*>(data), &dataLen) != ERROR_SUCCESS) break;
            if (clsidLen == 0 || clsid[0] != '{') continue;

            std::string dllPath = ResolveClsidServer(clsid);
            StartupEntry entry;
            entry.id = IdFor("IEToolbar", std::string(tbKey) + clsid);
            entry.name = clsid;
            entry.description = "IE 浏览器工具栏组件";
            entry.command = !dllPath.empty() ? dllPath : clsid;
            entry.executablePath = dllPath;
            entry.scope = "本机系统";
            entry.source = std::string("HKLM\\") + tbKey + ":" + clsid;
            entry.category = StartupCategory::InternetExplorer;
            entry.location = StartupLocation::ComHijack;
            entry.groupTitle = "HKLM\\" + std::string(tbKey);
            entry.status = StartupStatus::Enabled;
            entry.isHklm = true;
            entry.canDisable = false;
            entry.canDelete = false;
            entry.risk = RiskLevel::Safe;
            entry.riskReason = "浏览器工具栏扩展，随 IE 窗口创建加载";
            StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
        }
        RegCloseKey(key);
    }
}

void StartupScanner::ScanOfficeAddins(ScanSummary& result) {
    const char* apps[] = { "Word", "Excel", "PowerPoint", "Outlook", "Access" };
    const char* versions[] = { "", "16.0", "15.0", "14.0" };
    struct Root { HKEY root; bool isHklm; const char* scope; const char* prefix; };
    const Root roots[] = {
        { HKEY_LOCAL_MACHINE, true, "本机系统", "HKLM" },
        { HKEY_CURRENT_USER, false, "当前用户", "HKCU" },
    };

    for (const Root& r : roots) {
        REGSAM sam = KEY_READ | (r.isHklm ? KEY_WOW64_64KEY : 0);
        for (const char* version : versions) {
            for (const char* app : apps) {
                std::string addinPath = std::string("SOFTWARE\\Microsoft\\Office")
                    + (version[0] ? (std::string("\\") + version) : "")
                    + "\\" + app + "\\Addins";

                HKEY key = nullptr;
                if (RegOpenKeyExA(r.root, addinPath.c_str(), 0, sam, &key) != ERROR_SUCCESS) continue;

                const std::string groupTitle = std::string(r.prefix) + "\\" + addinPath;
                for (DWORD index = 0;; ++index) {
                    char progId[256]{};
                    DWORD len = sizeof(progId);
                    if (RegEnumKeyExA(key, index, progId, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;

                    HKEY addinKey = nullptr;
                    if (RegOpenKeyExA(key, progId, 0, KEY_READ, &addinKey) == ERROR_SUCCESS) {
                        char friendly[256]{};
                        char loadBehavior[32]{};
                        DWORD friendlyLen = sizeof(friendly);
                        DWORD lbLen = sizeof(loadBehavior);
                        RegQueryValueExA(addinKey, "FriendlyName", nullptr, nullptr, reinterpret_cast<BYTE*>(friendly), &friendlyLen);
                        RegQueryValueExA(addinKey, "LoadBehavior", nullptr, nullptr, reinterpret_cast<BYTE*>(loadBehavior), &lbLen);
                        RegCloseKey(addinKey);

                        // LoadBehavior 含位 0x2 时为启动加载；0x9 表示已卸载/出错
                        int lb = 2;
                        try { lb = std::stoi(loadBehavior); } catch (...) { lb = 2; }
                        bool enabled = (lb & 0x2) != 0;

                        std::string dllPath = ResolveClsidServer(progId);
                        StartupEntry entry;
                        entry.id = IdFor("OfficeAddin", groupTitle + ":" + progId);
                        entry.name = friendlyLen > 0 ? friendly : progId;
                        entry.description = std::string("Office ") + app + " COM 加载项 (" + progId + ")";
                        entry.command = !dllPath.empty() ? dllPath : progId;
                        entry.executablePath = dllPath;
                        entry.scope = r.scope;
                        entry.source = groupTitle + "\\" + progId;
                        entry.category = StartupCategory::Office;
                        entry.location = StartupLocation::ComHijack;
                        entry.groupTitle = groupTitle;
                        entry.status = enabled ? StartupStatus::Enabled : StartupStatus::Disabled;
                        entry.isHklm = r.isHklm;
                        entry.canDisable = false;
                        entry.canDelete = false;
                        entry.risk = RiskLevel::Safe;
                        entry.riskReason = "Office 应用启动时自动加载的 COM 加载项 (LoadBehavior=" + std::to_string(lb) + ")";
                        StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
                    }
                }
                RegCloseKey(key);
            }
        }
    }
}

void StartupScanner::AddEntry(ScanSummary& result, StartupEntry entry) {
    entry.name = EnsureUtf8(entry.name);
    entry.description = EnsureUtf8(entry.description);
    entry.publisher = EnsureUtf8(entry.publisher);
    entry.command = EnsureUtf8(entry.command);
    entry.executablePath = EnsureUtf8(entry.executablePath);
    entry.source = EnsureUtf8(entry.source);
    entry.groupTitle = EnsureUtf8(entry.groupTitle);
    entry.scope = EnsureUtf8(entry.scope);
    entry.riskReason = EnsureUtf8(entry.riskReason);
    entry.registryKey = EnsureUtf8(entry.registryKey);
    entry.registryValue = EnsureUtf8(entry.registryValue);
    entry.fileVersion = EnsureUtf8(entry.fileVersion);
    entry.fileTimestamp = EnsureUtf8(entry.fileTimestamp);
    entry.triggerInfo = EnsureUtf8(entry.triggerInfo);
    result.entries.push_back(std::move(entry));
}

StartupEntry StartupScanner::Analyze(StartupEntry entry) {
    std::wstring cmdW = Widen(entry.command);
    std::wstring exePathW = ExtractExePath(cmdW);

    if (!exePathW.empty()) {
        entry.executablePath = Narrow(exePathW);
        WIN32_FILE_ATTRIBUTE_DATA fileData{};
        if (GetFileAttributesExW(exePathW.c_str(), GetFileExInfoStandard, &fileData)) {
            entry.fileExists = true;
            uint64_t fileSize = (static_cast<uint64_t>(fileData.nFileSizeHigh) << 32) | fileData.nFileSizeLow;
            entry.fileSizeStr = FormatFileSize(fileSize);
            entry.fileTimestamp = FormatFileTime(fileData.ftLastWriteTime);

            std::string comp, desc, ver;
            ExtractVersionInfo(exePathW, comp, desc, ver);
            if (!comp.empty()) {
                entry.publisher = "(Verified) " + comp;
                entry.signature = SignatureStatus::Valid;
            } else {
                entry.publisher = "(Not verified)";
                entry.signature = SignatureStatus::Unsigned;
            }
            if (entry.description.empty() && !desc.empty()) {
                entry.description = desc;
            }
            if (!ver.empty()) {
                entry.fileVersion = ver;
            }

            if (entry.risk == RiskLevel::Notice || entry.risk == RiskLevel::Safe) {
                if (entry.publisher.find("Microsoft") != std::string::npos) {
                    entry.risk = RiskLevel::Safe;
                    entry.riskReason = "微软官方已签名程序";
                } else {
                    entry.risk = RiskLevel::Notice;
                    entry.riskReason = "已验证签名的第三方程序";
                }
            }
        } else {
            entry.fileExists = false;
            entry.fileSizeStr = "--";
            entry.description = "File not found: " + entry.executablePath;
            entry.publisher = "";
            if (entry.status != StartupStatus::Disabled) {
                entry.status = StartupStatus::Missing;
                entry.risk = RiskLevel::Suspicious;
                entry.riskReason = "启动目标文件不存在或已被卸载清理";
            }
        }
    } else {
        entry.fileExists = false;
        entry.fileSizeStr = "--";
        if (entry.description.empty()) entry.description = "系统内部组件指令";
        if (entry.publisher.empty()) entry.publisher = "(Verified) Microsoft Windows";
    }

    if (entry.description.empty()) {
        entry.description = entry.name;
    }

    return entry;
}

void StartupScanner::ScanSafeMode(ScanSummary& result) {
    struct Mode { const char* keyPath; const char* modeName; };
    const Mode modes[] = {
        { "SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Minimal", "安全模式" },
        { "SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Network", "带网络的安全模式" },
    };

    for (const Mode& mode : modes) {
        HKEY key = nullptr;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, mode.keyPath, 0, KEY_READ, &key) != ERROR_SUCCESS) continue;

        for (DWORD index = 0;; ++index) {
            char svcName[256]{};
            DWORD nameLen = sizeof(svcName);
            if (RegEnumKeyExA(key, index, svcName, &nameLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;

            // 仅处理在 Services 下有对应配置的项（跳过 AlternateShell 等选项子键）
            std::string svcPath = std::string("SYSTEM\\CurrentControlSet\\Services\\") + svcName;
            HKEY svcKey = nullptr;
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, svcPath.c_str(), 0, KEY_READ, &svcKey) != ERROR_SUCCESS) continue;

            DWORD startType = SERVICE_DEMAND_START;
            DWORD startSize = sizeof(startType);
            char imagePath[1024]{};
            DWORD imgSize = sizeof(imagePath);
            RegQueryValueExA(svcKey, "Start", nullptr, nullptr, reinterpret_cast<BYTE*>(&startType), &startSize);
            bool hasImage = RegQueryValueExA(svcKey, "ImagePath", nullptr, nullptr, reinterpret_cast<BYTE*>(imagePath), &imgSize) == ERROR_SUCCESS && imgSize > 1;
            RegCloseKey(svcKey);

            if (!hasImage || startType == SERVICE_DISABLED) continue;

            std::string imageLower = imagePath;
            std::transform(imageLower.begin(), imageLower.end(), imageLower.begin(), tolower);
            const bool isDriver = imageLower.find(".sys") != std::string::npos;

            StartupEntry entry;
            entry.id = IdFor("SafeMode", std::string(mode.modeName) + ":" + svcName);
            entry.name = svcName;
            entry.description = std::string(mode.modeName) + "下仍会加载的" + (isDriver ? "驱动" : "服务");
            entry.command = imagePath;
            entry.scope = "本机系统";
            entry.source = "HKLM\\" + svcPath + " (SafeBoot)";
            entry.category = isDriver ? StartupCategory::Drivers : StartupCategory::Services;
            entry.location = isDriver ? StartupLocation::Driver : StartupLocation::Service;
            entry.groupTitle = "HKLM\\" + std::string(mode.keyPath);
            entry.status = StartupStatus::Enabled;
            entry.canDelete = false;
            entry.risk = RiskLevel::Notice;
            entry.riskReason = std::string(mode.modeName) + "下仍会自动加载，恶意软件常利用安全模式驻留逃避常规清理";
            StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
        }
        RegCloseKey(key);
    }
}

void StartupScanner::ScanProtocolHandlers(ScanSummary& result) {
    struct Root { HKEY root; bool isHklm; const char* scope; const char* prefix; };
    const Root roots[] = {
        { HKEY_LOCAL_MACHINE, true, "本机系统", "HKLM" },
        { HKEY_CURRENT_USER, false, "当前用户", "HKCU" },
    };
    const char* handlerPaths[] = {
        "SOFTWARE\\Classes\\PROTOCOLS\\Handler",
        "SOFTWARE\\Classes\\PROTOCOLS\\Filter",
    };

    for (const char* handlerPath : handlerPaths) {
    for (const Root& r : roots) {
        HKEY key = nullptr;
        REGSAM sam = KEY_READ | (r.isHklm ? KEY_WOW64_64KEY : 0);
        if (RegOpenKeyExA(r.root, handlerPath, 0, sam, &key) != ERROR_SUCCESS) continue;

        for (DWORD index = 0;; ++index) {
            char scheme[128]{};
            DWORD len = sizeof(scheme);
            if (RegEnumKeyExA(key, index, scheme, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;

            HKEY schemeKey = nullptr;
            if (RegOpenKeyExA(key, scheme, 0, KEY_READ, &schemeKey) != ERROR_SUCCESS) continue;

            // 每个协议 Handler 子键下以 CLSID 命名的值即为其实现 DLL
            for (DWORD valIndex = 0;; ++valIndex) {
                char clsid[256]{};
                DWORD clsidLen = sizeof(clsid);
                if (RegEnumValueA(schemeKey, valIndex, clsid, &clsidLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
                if (clsidLen == 0 || clsid[0] != '{') continue;

                std::string dllPath = ResolveClsidServer(clsid);
                StartupEntry entry;
                entry.id = IdFor("ProtoHandler", std::string(r.prefix) + ":" + scheme + ":" + clsid);
                entry.name = scheme;
                entry.description = std::string("URL 协议处理程序 (") + clsid + ")";
                entry.command = !dllPath.empty() ? dllPath : clsid;
                entry.executablePath = dllPath;
                entry.scope = r.scope;
                entry.source = std::string(r.prefix) + "\\" + handlerPath + "\\" + scheme;
                entry.category = StartupCategory::InternetExplorer;
                entry.location = StartupLocation::ComHijack;
                entry.groupTitle = std::string(r.prefix) + "\\" + handlerPath;
                entry.status = StartupStatus::Enabled;
                entry.isHklm = r.isHklm;
                entry.canDisable = false;
                entry.canDelete = false;
                entry.risk = RiskLevel::Notice;
                entry.riskReason = "访问对应协议时自动加载该 DLL，可拦截或篡改网络请求";
                StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
            }
            RegCloseKey(schemeKey);
        }
        RegCloseKey(key);
    }
    }
}

void StartupScanner::ScanDirectShowFilters(ScanSummary& result) {
    // DirectShow 过滤器类别管理器：系统视图与 32 位兼容视图各扫一遍
    const char* instancePaths[] = {
        "SOFTWARE\\Classes\\CLSID\\{083863F1-70DE-11d0-BD40-00A0C911CE86}\\Instance",
        "SOFTWARE\\Classes\\WOW6432Node\\CLSID\\{083863F1-70DE-11d0-BD40-00A0C911CE86}\\Instance",
    };

    for (const char* instancePath : instancePaths) {
        HKEY key = nullptr;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, instancePath, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS &&
            RegOpenKeyExA(HKEY_LOCAL_MACHINE, instancePath, 0, KEY_READ, &key) != ERROR_SUCCESS) continue;

        for (DWORD index = 0;; ++index) {
            char filterGuid[256]{};
            DWORD len = sizeof(filterGuid);
            if (RegEnumKeyExA(key, index, filterGuid, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;

            HKEY filterKey = nullptr;
            if (RegOpenKeyExA(key, filterGuid, 0, KEY_READ, &filterKey) != ERROR_SUCCESS) continue;

            char friendly[256]{};
            char clsid[256]{};
            DWORD friendlyLen = sizeof(friendly);
            DWORD clsidLen = sizeof(clsid);
            RegQueryValueExA(filterKey, "FriendlyName", nullptr, nullptr, reinterpret_cast<BYTE*>(friendly), &friendlyLen);
            RegQueryValueExA(filterKey, "CLSID", nullptr, nullptr, reinterpret_cast<BYTE*>(clsid), &clsidLen);
            RegCloseKey(filterKey);

            std::string clsidStr = (clsidLen > 1) ? clsid : filterGuid;
            std::string dllPath = ResolveClsidServer(clsidStr);

            StartupEntry entry;
            entry.id = IdFor("DShowFilter", std::string(instancePath) + ":" + filterGuid);
            entry.name = (friendlyLen > 0) ? friendly : filterGuid;
            entry.description = "DirectShow 过滤器 / 编解码器";
            entry.command = !dllPath.empty() ? dllPath : clsidStr;
            entry.executablePath = dllPath;
            entry.scope = "本机系统";
            entry.source = std::string("HKLM\\") + instancePath + "\\" + filterGuid;
            entry.category = StartupCategory::Codecs;
            entry.location = StartupLocation::ComHijack;
            entry.groupTitle = "HKLM\\" + std::string(instancePath);
            entry.status = StartupStatus::Enabled;
            entry.isHklm = true;
            entry.canDisable = false;
            entry.canDelete = false;
            entry.risk = RiskLevel::Notice;
            entry.riskReason = "媒体播放加载时自动注入的解码器 DLL，恶意编解码器可借此执行代码";
            StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
        }
        RegCloseKey(key);
    }
}

void StartupScanner::ScanSharedTaskSchedulers(ScanSummary& result) {
    const char* stsPaths[] = {
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedTaskScheduler",
        "SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\SharedTaskScheduler",
    };
    for (const char* stsPath : stsPaths) {
        HKEY key = nullptr;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, stsPath, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS &&
            RegOpenKeyExA(HKEY_LOCAL_MACHINE, stsPath, 0, KEY_READ, &key) != ERROR_SUCCESS) continue;

        for (DWORD index = 0;; ++index) {
            char clsid[256]{};
            char desc[512]{};
            DWORD clsidLen = sizeof(clsid);
            DWORD descLen = sizeof(desc);
            if (RegEnumValueA(key, index, clsid, &clsidLen, nullptr, nullptr, reinterpret_cast<BYTE*>(desc), &descLen) != ERROR_SUCCESS) break;
            if (clsidLen == 0 || clsid[0] != '{') continue;

            std::string dllPath = ResolveClsidServer(clsid);
            StartupEntry entry;
            entry.id = IdFor("SharedTask", std::string(stsPath) + ":" + clsid);
            entry.name = (descLen > 0) ? desc : clsid;
            entry.description = "Explorer 共享任务调度 COM 对象";
            entry.command = !dllPath.empty() ? dllPath : clsid;
            entry.executablePath = dllPath;
            entry.scope = "本机系统";
            entry.source = std::string("HKLM\\") + stsPath + ":" + clsid;
            entry.category = StartupCategory::Explorer;
            entry.location = StartupLocation::ComHijack;
            entry.groupTitle = "HKLM\\" + std::string(stsPath);
            entry.status = StartupStatus::Enabled;
            entry.isHklm = true;
            entry.canDisable = false;
            entry.canDelete = false;
            entry.risk = RiskLevel::Notice;
            entry.riskReason = "Explorer 启动时自动预加载的共享任务 COM 对象";
            StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
        }
        RegCloseKey(key);
    }
}

void StartupScanner::ScanUrlSearchHooks(ScanSummary& result) {
    const char* hookPath = "SOFTWARE\\Microsoft\\Internet Explorer\\URLSearchHooks";
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, hookPath, 0, KEY_READ, &key) != ERROR_SUCCESS) return;

    for (DWORD index = 0;; ++index) {
        char clsid[256]{};
        DWORD clsidLen = sizeof(clsid);
        if (RegEnumValueA(key, index, clsid, &clsidLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
        if (clsidLen == 0 || clsid[0] != '{') continue;

        std::string dllPath = ResolveClsidServer(clsid);
        StartupEntry entry;
        entry.id = IdFor("UrlSearchHook", clsid);
        entry.name = clsid;
        entry.description = "IE 地址栏搜索钩子";
        entry.command = !dllPath.empty() ? dllPath : clsid;
        entry.executablePath = dllPath;
        entry.scope = "当前用户";
        entry.source = std::string("HKCU\\") + hookPath + ":" + clsid;
        entry.category = StartupCategory::InternetExplorer;
        entry.location = StartupLocation::ComHijack;
        entry.groupTitle = "HKCU\\" + std::string(hookPath);
        entry.status = StartupStatus::Enabled;
        entry.canDisable = false;
        entry.canDelete = false;
        entry.risk = RiskLevel::Notice;
        entry.riskReason = "地址栏搜索请求会经过该 DLL，可重定向搜索流量";
        StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
    }
    RegCloseKey(key);
}

void StartupScanner::ScanDrivers32Codecs(ScanSummary& result) {
    // Drivers32 是传统多媒体编解码器驱动表（vidc./msacm./vids. 等）
    const char* drivers32Paths[] = {
        "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32",
        "SOFTWARE\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32",
    };
    for (const char* codecPath : drivers32Paths) {
        HKEY key = nullptr;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, codecPath, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS &&
            RegOpenKeyExA(HKEY_LOCAL_MACHINE, codecPath, 0, KEY_READ, &key) != ERROR_SUCCESS) continue;

        for (DWORD index = 0;; ++index) {
            char name[128]{};
            char value[1024]{};
            DWORD nameLen = sizeof(name);
            DWORD valLen = sizeof(value);
            if (RegEnumValueA(key, index, name, &nameLen, nullptr, nullptr, reinterpret_cast<BYTE*>(value), &valLen) != ERROR_SUCCESS) break;
            if (!strchr(name, '.') || valLen <= 1) continue; // 跳过 wave/midi/timer 等系统内置项

            StartupEntry entry;
            entry.id = IdFor("Drivers32", std::string(codecPath) + ":" + name);
            entry.name = name;
            entry.description = "多媒体编解码器驱动 (Drivers32)";
            entry.command = value;
            entry.executablePath.clear();
            entry.scope = "本机系统";
            entry.source = std::string("HKLM\\") + codecPath + ":" + name;
            entry.category = StartupCategory::Codecs;
            entry.location = StartupLocation::CodecDriver;
            entry.groupTitle = "HKLM\\" + std::string(codecPath);
            entry.status = StartupStatus::Enabled;
            entry.isHklm = true;
            entry.canDisable = false;
            entry.canDelete = false;
            entry.risk = RiskLevel::Notice;
            entry.riskReason = "播放对应格式媒体时自动加载的编解码器 DLL";
            StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
        }
        RegCloseKey(key);
    }
}

void StartupScanner::ScanPrintProcessors(ScanSummary& result) {
    const char* envBase = "SYSTEM\\CurrentControlSet\\Control\\Print\\Environments";
    HKEY envRoot = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, envBase, 0, KEY_READ, &envRoot) != ERROR_SUCCESS) return;

    for (DWORD envIndex = 0;; ++envIndex) {
        char envName[128]{};
        DWORD envLen = sizeof(envName);
        if (RegEnumKeyExA(envRoot, envIndex, envName, &envLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;

        std::string procPath = std::string(envBase) + "\\" + envName + "\\Print Processors";
        HKEY procKey = nullptr;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, procPath.c_str(), 0, KEY_READ, &procKey) != ERROR_SUCCESS) continue;

        for (DWORD index = 0;; ++index) {
            char procName[256]{};
            DWORD len = sizeof(procName);
            if (RegEnumKeyExA(procKey, index, procName, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;

            char driverDll[1024]{};
            DWORD drvSize = sizeof(driverDll);
            HKEY itemKey = nullptr;
            std::string command = "打印处理器";
            if (RegOpenKeyExA(procKey, procName, 0, KEY_READ, &itemKey) == ERROR_SUCCESS) {
                RegQueryValueExA(itemKey, "Driver", nullptr, nullptr, reinterpret_cast<BYTE*>(driverDll), &drvSize);
                RegCloseKey(itemKey);
                if (drvSize > 1) command = driverDll;
            }

            StartupEntry entry;
            entry.id = IdFor("PrintProcessor", std::string(envName) + ":" + procName);
            entry.name = procName;
            entry.description = "打印处理器 (" + std::string(envName) + ")";
            entry.command = command;
            entry.scope = "本机系统";
            entry.source = "HKLM\\" + procPath + "\\" + procName;
            entry.category = StartupCategory::PrintMonitors;
            entry.location = StartupLocation::PrintMonitor;
            entry.groupTitle = "HKLM\\" + procPath;
            entry.status = StartupStatus::Enabled;
            entry.isHklm = true;
            entry.canDisable = false;
            entry.canDelete = false;
            entry.risk = RiskLevel::Notice;
            entry.riskReason = "打印后台服务 (spooler) 启动时加载的打印处理器 DLL";
            StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
        }
        RegCloseKey(procKey);
    }
    RegCloseKey(envRoot);
}

void StartupScanner::ScanSecurityProviders(ScanSummary& result) {
    const char* keyPath = "SYSTEM\\CurrentControlSet\\Control\\SecurityProviders";
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS &&
        RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &key) != ERROR_SUCCESS) return;

    char val[2048]{};
    DWORD size = sizeof(val);
    if (RegQueryValueExA(key, "SecurityProviders", nullptr, nullptr, reinterpret_cast<BYTE*>(val), &size) == ERROR_SUCCESS && size > 1) {
        StartupEntry entry;
        entry.id = IdFor("SecProvider", "SecurityProviders");
        entry.name = "SecurityProviders";
        entry.description = "SSPI 安全提供程序 DLL 列表";
        entry.command = val;
        entry.executablePath.clear();
        entry.scope = "本机系统";
        entry.source = std::string("HKLM\\") + keyPath + "@SecurityProviders";
        entry.category = StartupCategory::Lsa;
        entry.location = StartupLocation::LsaPackage;
        entry.groupTitle = "HKLM\\" + std::string(keyPath);
        entry.status = StartupStatus::Enabled;
        entry.isHklm = true;
        entry.canDisable = false;
        entry.canDelete = false;
        entry.risk = RiskLevel::High;
        entry.riskReason = "自定义安全提供程序 DLL 会被 lsass 进程加载，是凭据窃取的高价值持久化点";
        StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
    }
    RegCloseKey(key);
}

void StartupScanner::ScanScreenSaver(ScanSummary& result) {
    // 屏幕保护程序：空闲时由系统自动启动的经典持久化点
    ScanSingleRegistryValue(HKEY_CURRENT_USER, false, "Control Panel\\Desktop", "SCRNSAVE.EXE",
        StartupCategory::Explorer, StartupLocation::ScreenSaver,
        "HKCU\\Control Panel\\Desktop", RiskLevel::Notice,
        "当前用户的屏幕保护程序：系统空闲超时后自动执行该 .scr 文件", result, false);
}

void StartupScanner::ScanAppCertDlls(ScanSummary& result) {
    // AppCertDLLs：进程创建通知 API 钩子，会注入到每个新启动的进程
    const char* keyPath = "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\AppCertDLLs";
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS &&
        RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &key) != ERROR_SUCCESS) return;

    for (DWORD index = 0;; ++index) {
        char name[128]{};
        char val[1024]{};
        DWORD nameLen = sizeof(name);
        DWORD valLen = sizeof(val);
        if (RegEnumValueA(key, index, name, &nameLen, nullptr, nullptr, reinterpret_cast<BYTE*>(val), &valLen) != ERROR_SUCCESS) break;
        if (valLen <= 1) continue;

        StartupEntry entry;
        entry.id = IdFor("AppCert", std::string(keyPath) + ":" + name);
        entry.name = nameLen > 0 ? name : val;
        entry.description = "进程创建证书 DLL (AppCertDLLs)";
        entry.command = val;
        entry.scope = "本机系统";
        entry.source = std::string("HKLM\\") + keyPath + ":" + name;
        entry.registryKey = keyPath;
        entry.registryValue = name;
        entry.isHklm = true;
        entry.category = StartupCategory::BootExecute;
        entry.location = StartupLocation::AppCertDlls;
        entry.groupTitle = "HKLM\\" + std::string(keyPath);
        entry.status = StartupStatus::Enabled;
        entry.canDisable = false;
        entry.canDelete = false;
        entry.risk = RiskLevel::High;
        entry.riskReason = "AppCertDLLs 会注入到每个新创建的进程中，是极隐蔽的持久化与监控手段";
        StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
    }
    RegCloseKey(key);
}

void StartupScanner::ScanTerminalServer(ScanSummary& result) {
    // 远程桌面 / 终端服务器会话登录时执行的程序
    ScanSingleRegistryValue(HKEY_LOCAL_MACHINE, true,
        "SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\Wds\\rdpwd", "StartupPrograms",
        StartupCategory::Logon, StartupLocation::RegistryRun,
        "HKLM\\SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\Wds\\rdpwd", RiskLevel::Suspicious,
        "RDP 会话登录时自动执行的程序（rdpwd StartupPrograms），极少有合法软件使用", result, false);
    ScanSingleRegistryValue(HKEY_LOCAL_MACHINE, true,
        "SYSTEM\\CurrentControlSet\\Control\\Terminal Server", "InitialProgram",
        StartupCategory::Logon, StartupLocation::RegistryRun,
        "HKLM\\SYSTEM\\CurrentControlSet\\Control\\Terminal Server", RiskLevel::Suspicious,
        "终端服务器 InitialProgram：每个远程会话建立时都会执行", result, false);

    // 每用户终端服务安装模式下的 Run 键（兼容旧版应用安装）
    const char* tsRun = "Software\\Microsoft\\Windows NT\\CurrentVersion\\Terminal Server\\Install\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, tsRun, 0, KEY_READ, &key) == ERROR_SUCCESS) {
        for (DWORD index = 0;; ++index) {
            char name[256]{};
            char val[1024]{};
            DWORD nameLen = sizeof(name);
            DWORD valLen = sizeof(val);
            if (RegEnumValueA(key, index, name, &nameLen, nullptr, nullptr, reinterpret_cast<BYTE*>(val), &valLen) != ERROR_SUCCESS) break;
            if (valLen <= 1) continue;

            StartupEntry entry;
            entry.id = IdFor("TsRun", std::string(name));
            entry.name = name;
            entry.description = "终端服务器安装模式 Run 值";
            entry.command = val;
            entry.scope = "当前用户";
            entry.source = std::string("HKCU\\") + tsRun + ":" + name;
            entry.registryKey = tsRun;
            entry.registryValue = name;
            entry.category = StartupCategory::Logon;
            entry.location = StartupLocation::RegistryRun;
            entry.groupTitle = "HKCU\\" + std::string(tsRun);
            entry.status = StartupStatus::Enabled;
            entry.risk = RiskLevel::Suspicious;
            entry.riskReason = "终端服务器安装模式下遗留的 Run 值，位置罕见且易被滥用";
            StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
        }
        RegCloseKey(key);
    }
}

void StartupScanner::ScanUserInitMprLogonScript(ScanSummary& result) {
    // UserInitMprLogonScript：用户环境变量形式的登录脚本持久化点
    ScanSingleRegistryValue(HKEY_CURRENT_USER, false, "Environment", "UserInitMprLogonScript",
        StartupCategory::Winlogon, StartupLocation::Winlogon,
        "HKCU\\Environment", RiskLevel::High,
        "UserInitMprLogonScript 环境变量：用户登录时由 winlogon 自动执行，是隐蔽的脚本持久化点", result, false);
}

void StartupScanner::ScanPrintProviders(ScanSummary& result) {
    const char* keyPath = "SYSTEM\\CurrentControlSet\\Control\\Print\\Providers";
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS &&
        RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &key) != ERROR_SUCCESS) return;

    for (DWORD index = 0;; ++index) {
        char provName[256]{};
        DWORD len = sizeof(provName);
        if (RegEnumKeyExA(key, index, provName, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;

        char dll[1024]{};
        DWORD dllSize = sizeof(dll);
        HKEY itemKey = nullptr;
        if (RegOpenKeyExA(key, provName, 0, KEY_READ, &itemKey) == ERROR_SUCCESS) {
            RegQueryValueExA(itemKey, "Name", nullptr, nullptr, reinterpret_cast<BYTE*>(dll), &dllSize);
            RegCloseKey(itemKey);
        }

        StartupEntry entry;
        entry.id = IdFor("PrintProvider", provName);
        entry.name = provName;
        entry.description = "打印提供程序 DLL";
        entry.command = dllSize > 1 ? dll : "打印提供程序";
        entry.scope = "本机系统";
        entry.source = "HKLM\\" + std::string(keyPath) + "\\" + provName;
        entry.category = StartupCategory::PrintMonitors;
        entry.location = StartupLocation::PrintMonitor;
        entry.groupTitle = "HKLM\\" + std::string(keyPath);
        entry.status = StartupStatus::Enabled;
        entry.isHklm = true;
        entry.canDisable = false;
        entry.canDelete = false;
        entry.risk = RiskLevel::Notice;
        entry.riskReason = "打印后台服务 (spooler) 启动时加载的打印提供程序 DLL";
        StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
    }
    RegCloseKey(key);
}

void StartupScanner::ScanShellExecuteHooks(ScanSummary& result) {
    // ShellExecuteHooks：每次通过 Shell 执行打开操作都会加载的 COM 对象
    struct Root { HKEY root; bool isHklm; const char* scope; const char* prefix; };
    const Root roots[] = {
        { HKEY_LOCAL_MACHINE, true, "本机系统", "HKLM" },
        { HKEY_CURRENT_USER, false, "当前用户", "HKCU" },
    };
    const char* hookPath = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellExecuteHooks";

    for (const Root& r : roots) {
        REGSAM sam = KEY_READ | (r.isHklm ? KEY_WOW64_64KEY : 0);
        HKEY key = nullptr;
        if (RegOpenKeyExA(r.root, hookPath, 0, sam, &key) != ERROR_SUCCESS) continue;
        const std::string groupTitle = std::string(r.prefix) + "\\" + hookPath;

        for (DWORD index = 0;; ++index) {
            char clsid[128]{};
            DWORD len = sizeof(clsid);
            if (RegEnumKeyExA(key, index, clsid, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;

            std::string dllPath = ResolveClsidServer(clsid);
            StartupEntry entry;
            entry.id = IdFor("ShellExecHook", groupTitle + ":" + clsid);
            entry.name = "CLSID: " + std::string(clsid);
            entry.description = "ShellExecute 挂钩 COM 对象";
            entry.command = !dllPath.empty() ? dllPath : clsid;
            entry.executablePath = dllPath;
            entry.scope = r.scope;
            entry.source = "CLSID: " + std::string(clsid);
            entry.category = StartupCategory::Explorer;
            entry.location = StartupLocation::ComHijack;
            entry.groupTitle = groupTitle;
            entry.status = StartupStatus::Enabled;
            entry.isHklm = r.isHklm;
            entry.canDisable = false;
            entry.canDelete = false;
            entry.risk = RiskLevel::Notice;
            entry.riskReason = "每次 Shell 打开文件/程序时都会加载的挂钩 DLL，可被用于全局监控或劫持";
            StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
        }
        RegCloseKey(key);
    }
}

void StartupScanner::ScanChromiumExtensions(ScanSummary& result) {
    // Chromium 内核浏览器的扩展强制安装策略与用户扩展注册表入口
    struct Browser { const char* vendor; const char* label; };
    const Browser browsers[] = {
        { "Google\\Chrome", "Chrome" },
        { "Microsoft\\Edge", "Edge" },
        { "BraveSoftware\\Brave-Browser", "Brave" },
    };
    struct Root { HKEY root; bool isHklm; const char* scope; const char* prefix; };
    const Root roots[] = {
        { HKEY_LOCAL_MACHINE, true, "本机系统", "HKLM" },
        { HKEY_CURRENT_USER, false, "当前用户", "HKCU" },
    };

    for (const Browser& b : browsers) {
        for (const Root& r : roots) {
            // ExtensionInstallForcelist：策略强制安装的扩展（企业策略常被滥用来强制植入扩展）
            std::string forcelistPath = std::string("SOFTWARE\\Policies\\") + b.vendor + "\\ExtensionInstallForcelist";
            REGSAM sam = KEY_READ | (r.isHklm ? KEY_WOW64_64KEY : 0);
            HKEY key = nullptr;
            if (RegOpenKeyExA(r.root, forcelistPath.c_str(), 0, sam, &key) == ERROR_SUCCESS) {
                const std::string groupTitle = std::string(r.prefix) + "\\" + forcelistPath;
                for (DWORD index = 0;; ++index) {
                    char name[64]{};
                    char val[1024]{};
                    DWORD nameLen = sizeof(name);
                    DWORD valLen = sizeof(val);
                    if (RegEnumValueA(key, index, name, &nameLen, nullptr, nullptr, reinterpret_cast<BYTE*>(val), &valLen) != ERROR_SUCCESS) break;
                    if (valLen <= 1) continue;

                    StartupEntry entry;
                    entry.id = IdFor("ExtForce", groupTitle + ":" + name);
                    entry.name = std::string(b.label) + " 强制安装扩展 " + name;
                    entry.description = std::string(b.label) + " 企业策略强制安装的浏览器扩展";
                    entry.command = val;
                    entry.scope = r.scope;
                    entry.source = groupTitle + ":" + name;
                    entry.category = StartupCategory::InternetExplorer;
                    entry.location = StartupLocation::BrowserExtension;
                    entry.groupTitle = groupTitle;
                    entry.status = StartupStatus::Enabled;
                    entry.isHklm = r.isHklm;
                    entry.canDisable = false;
                    entry.canDelete = false;
                    entry.risk = RiskLevel::Notice;
                    entry.riskReason = "通过组策略强制安装的浏览器扩展会在每次启动浏览器时加载，无法被普通用户卸载";
                    StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
                }
                RegCloseKey(key);
            }

            // 用户已安装扩展注册表项
            std::string extPath = std::string("SOFTWARE\\") + b.vendor + "\\Extensions";
            if (RegOpenKeyExA(r.root, extPath.c_str(), 0, sam, &key) != ERROR_SUCCESS) continue;
            const std::string groupTitle2 = std::string(r.prefix) + "\\" + extPath;
            for (DWORD index = 0;; ++index) {
                char extId[64]{};
                DWORD len = sizeof(extId);
                if (RegEnumKeyExA(key, index, extId, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;

                char ver[64]{};
                DWORD verSize = sizeof(ver);
                HKEY extKey = nullptr;
                if (RegOpenKeyExA(key, extId, 0, KEY_READ, &extKey) == ERROR_SUCCESS) {
                    RegQueryValueExA(extKey, "Path", nullptr, nullptr, reinterpret_cast<BYTE*>(ver), &verSize);
                    RegCloseKey(extKey);
                }

                StartupEntry entry;
                entry.id = IdFor("ChromiumExt", groupTitle2 + ":" + extId);
                entry.name = std::string(b.label) + " 扩展 " + std::string(extId);
                entry.description = std::string(b.label) + " 已注册的浏览器扩展";
                entry.command = verSize > 1 ? ver : extId;
                entry.scope = r.scope;
                entry.source = groupTitle2 + "\\" + extId;
                entry.category = StartupCategory::InternetExplorer;
                entry.location = StartupLocation::BrowserExtension;
                entry.groupTitle = groupTitle2;
                entry.status = StartupStatus::Enabled;
                entry.isHklm = r.isHklm;
                entry.canDisable = false;
                entry.canDelete = false;
                entry.risk = RiskLevel::Safe;
                entry.riskReason = "浏览器启动时加载的扩展，拥有对应站点权限";
                StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
            }
            RegCloseKey(key);
        }
    }
}

void StartupScanner::ScanOfficeStartupFolders(ScanSummary& result) {
    // Word STARTUP 与 Excel XLSTART：放入其中的加载宏/DLL 会在 Office 应用启动时自动加载
    struct Folder { const int csidl; const char* subFolder; const char* appName; };
    const Folder folders[] = {
        { CSIDL_APPDATA, "Microsoft\\Word\\STARTUP", "Word" },
        { CSIDL_APPDATA, "Microsoft\\Excel\\XLSTART", "Excel" },
        { CSIDL_COMMON_APPDATA, "Microsoft\\Excel\\XLSTART", "Excel (所有用户)" },
    };

    for (const Folder& f : folders) {
        wchar_t base[MAX_PATH]{};
        if (FAILED(SHGetFolderPathW(nullptr, f.csidl, nullptr, SHGFP_TYPE_CURRENT, base))) continue;
        std::wstring dir = std::wstring(base) + L"\\" + Widen(f.subFolder);

        WIN32_FIND_DATAW fd{};
        HANDLE find = FindFirstFileW((dir + L"\\*").c_str(), &fd);
        if (find == INVALID_HANDLE_VALUE) continue;

        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            std::wstring fileName(fd.cFileName);
            std::wstring full = dir + L"\\" + fileName;
            std::string narrowFull = Narrow(full);

            StartupEntry entry;
            entry.id = IdFor("OfficeStartup", narrowFull);
            entry.name = Narrow(fileName);
            entry.description = std::string(f.appName) + " 启动文件夹加载项";
            entry.command = narrowFull;
            entry.executablePath = narrowFull;
            entry.scope = f.csidl == CSIDL_COMMON_APPDATA ? "所有用户" : "当前用户";
            entry.source = narrowFull;
            entry.category = StartupCategory::Office;
            entry.location = StartupLocation::OfficeAddin;
            entry.groupTitle = Narrow(dir);
            entry.status = StartupStatus::Enabled;
            entry.canDisable = false;
            entry.canDelete = false;
            entry.risk = RiskLevel::Notice;
            entry.riskReason = std::string("Office ") + f.appName + " 启动时会自动加载该文件夹内的加载项文件";
            StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
        } while (FindNextFileW(find, &fd));
        FindClose(find);
    }
}

} // namespace AutoGuard
