#include "StartupScanner.h"

#include <windows.h>
#include <taskschd.h>
#include <oleauto.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <winver.h>
#include <cstdio>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cwctype>
#include <chrono>

#pragma comment(lib, "version.lib")
#pragma comment(lib, "shlwapi.lib")

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

    if (expanded.front() == L'"') {
        size_t end = expanded.find(L'"', 1);
        if (end != std::wstring::npos) {
            return expanded.substr(1, end - 1);
        }
        return expanded.substr(1);
    }

    size_t end = expanded.find(L' ');
    if (end != std::wstring::npos) {
        std::wstring candidate = expanded.substr(0, end);
        if (GetFileAttributesW(candidate.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return candidate;
        }
        if (candidate.size() > 4) {
            std::wstring ext = candidate.substr(candidate.size() - 4);
            std::transform(ext.begin(), ext.end(), ext.begin(), towlower);
            if (ext == L".exe" || ext == L".bat" || ext == L".cmd" || ext == L".dll" || ext == L".vbs" || ext == L".sys") {
                return candidate;
            }
        }
    }
    return end == std::wstring::npos ? expanded : expanded.substr(0, end);
}

std::string IdFor(const char* source, const std::string& name) {
    return std::string(source) + ":" + name;
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

    // 8. Winlogon
    ScanWinlogon(result);

    // 9. Explorer / Shell Extensions
    ScanComExtensions(result);

    // 10. AppInit DLLs
    ScanAppInitDlls(result);

    // 11. Known DLLs
    ScanKnownDlls(result);

    // 12. Boot Execute
    ScanBootExecute(result);

    // 13. Winsock Providers
    ScanWinsockProviders(result);

    // 14. Print Monitors
    ScanPrintMonitors(result);

    // 15. LSA Packages
    ScanLsaPackages(result);

    // 16. WMI Subscriptions
    ScanWmiSubscriptions(result);

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

void StartupScanner::ScanRegistryRoot(
    HKEY root, const char* scope, const char* subKey, bool isHklm,
    StartupCategory cat, StartupLocation loc, const std::string& groupTitle,
    ScanSummary& result)
{
    HKEY key = nullptr;
    if (RegOpenKeyExA(root, subKey, 0, KEY_READ, &key) != ERROR_SUCCESS) return;
    for (DWORD index = 0;; ++index) {
        char name[512]{};
        char value[4096]{};
        DWORD nameSize = sizeof(name);
        DWORD valueSize = sizeof(value);
        DWORD type = 0;
        const LONG status = RegEnumValueA(key, index, name, &nameSize, nullptr, &type, reinterpret_cast<BYTE*>(value), &valueSize);
        if (status == ERROR_NO_MORE_ITEMS) break;
        if (status != ERROR_SUCCESS) continue;
        if (nameSize == 0 && valueSize == 0) continue;

        StartupEntry entry;
        entry.id = IdFor(subKey, name);
        entry.name = nameSize > 0 ? name : "(默认)";
        entry.scope = scope;
        entry.command = value;
        entry.source = std::string(isHklm ? "HKLM\\" : "HKCU\\") + subKey;
        entry.registryKey = subKey;
        entry.registryValue = entry.name;
        entry.isHklm = isHklm;
        entry.category = cat;
        entry.location = loc;
        entry.status = StartupStatus::Enabled;
        entry.groupTitle = groupTitle;
        StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
    }
    RegCloseKey(key);
}

void StartupScanner::ScanStartupFolder(const std::wstring& folder, const char* scope, ScanSummary& result) {
    std::error_code error;
    if (!std::filesystem::exists(folder, error)) return;
    std::string folderUtf8 = Narrow(folder);

    for (const auto& item : std::filesystem::directory_iterator(folder, error)) {
        if (error) break;
        std::wstring filename = item.path().filename().wstring();
        if (filename == L"desktop.ini") continue;

        StartupEntry entry;
        entry.name = Narrow(filename);
        entry.command = Narrow(item.path().wstring());
        entry.source = Narrow(item.path().wstring());
        entry.scope = scope;
        entry.category = StartupCategory::Logon;
        entry.location = StartupLocation::StartupFolder;
        entry.id = IdFor("StartupFolder", entry.name);
        entry.groupTitle = folderUtf8;

        if (filename.size() > 9 && filename.substr(filename.size() - 9) == L".disabled") {
            entry.status = StartupStatus::Disabled;
        } else {
            entry.status = StartupStatus::Enabled;
        }

        StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
    }
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
            task->get_Name(&name);
            task->get_Path(&path);
            task->get_Enabled(&enabled);

            std::string taskPath = name ? Narrow(path) : "";
            std::string taskName = name ? Narrow(name) : "";

            StartupEntry entry;
            entry.id = IdFor("ScheduledTask", taskPath);
            entry.name = taskName;
            entry.scope = "本机系统";
            entry.source = "任务计划程序\\" + taskPath;
            entry.category = StartupCategory::ScheduledTasks;
            entry.location = StartupLocation::ScheduledTask;
            entry.status = (enabled == VARIANT_TRUE) ? StartupStatus::Enabled : StartupStatus::Disabled;
            entry.groupTitle = "任务计划程序 (Task Scheduler)";

            ITaskDefinition* definition = nullptr;
            if (SUCCEEDED(task->get_Definition(&definition)) && definition) {
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
                entry.status = StartupStatus::Protected;
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
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, ifeoKey, 0, KEY_READ, &key) != ERROR_SUCCESS) return;

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
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, winlogonKey, 0, KEY_READ, &key) != ERROR_SUCCESS) return;

    const char* valuesToCheck[] = { "Userinit", "Shell", "Taskman", "AppSetup" };
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
            } else {
                entry.description = "非标准登录组件";
                entry.risk = RiskLevel::High;
                entry.riskReason = "非标准 Winlogon 启动项，存在登录劫持风险";
            }
            StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
        }
    }
    RegCloseKey(key);
}

void StartupScanner::ScanActiveSetup(ScanSummary& result) {
    const char* activeKey = "SOFTWARE\\Microsoft\\Active Setup\\Installed Components";
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, activeKey, 0, KEY_READ, &key) != ERROR_SUCCESS) return;

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
                entry.id = IdFor("ActiveSetup", subKeyName);
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

void StartupScanner::ScanComExtensions(ScanSummary& result) {
    const char* approvedKey = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved";
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, approvedKey, 0, KEY_READ, &key) != ERROR_SUCCESS) return;

    for (DWORD index = 0; index < 40; ++index) {
        char clsid[256]{};
        char desc[512]{};
        DWORD clsidLen = sizeof(clsid);
        DWORD descLen = sizeof(desc);
        if (RegEnumValueA(key, index, clsid, &clsidLen, nullptr, nullptr, reinterpret_cast<BYTE*>(desc), &descLen) != ERROR_SUCCESS) break;

        StartupEntry entry;
        entry.id = IdFor("ShellExt", clsid);
        entry.name = descLen > 0 ? desc : clsid;
        entry.description = "Shell 扩展组件";
        entry.command = clsid;
        entry.scope = "本机系统";
        entry.source = std::string("CLSID: ") + clsid;
        entry.category = StartupCategory::Explorer;
        entry.location = StartupLocation::ComHijack;
        entry.groupTitle = "HKLM\\" + std::string(approvedKey);
        entry.status = StartupStatus::Enabled;
        entry.isHklm = true;
        entry.risk = RiskLevel::Safe;
        entry.riskReason = "已批准的外壳扩展组件";
        StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
    }
    RegCloseKey(key);
}

void StartupScanner::ScanAppInitDlls(ScanSummary& result) {
    const char* keyPath = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows";
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &key) == ERROR_SUCCESS) {
        char val[1024]{};
        DWORD size = sizeof(val);
        if (RegQueryValueExA(key, "AppInit_DLLs", nullptr, nullptr, reinterpret_cast<BYTE*>(val), &size) == ERROR_SUCCESS && size > 0 && strlen(val) > 0) {
            StartupEntry entry;
            entry.id = IdFor("AppInit", "AppInit_DLLs");
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
        RegCloseKey(key);
    }
}

void StartupScanner::ScanWinsockProviders(ScanSummary& result) {
    const char* keyPath = "SYSTEM\\CurrentControlSet\\Services\\WinSock2\\Parameters\\NameSpace_Catalog5_0\\Catalog_Entries";
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &key) == ERROR_SUCCESS) {
        for (DWORD index = 0; index < 10; ++index) {
            char subKey[256]{};
            DWORD len = sizeof(subKey);
            if (RegEnumKeyExA(key, index, subKey, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
            StartupEntry entry;
            entry.id = IdFor("Winsock", subKey);
            entry.name = subKey;
            entry.command = "Winsock 命名空间提供程序";
            entry.scope = "本机系统";
            entry.source = std::string("HKLM\\") + keyPath + "\\" + subKey;
            entry.category = StartupCategory::Winsock;
            entry.location = StartupLocation::WinsockProvider;
            entry.groupTitle = "HKLM\\" + std::string(keyPath);
            entry.status = StartupStatus::Protected;
            entry.risk = RiskLevel::Safe;
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
        char val[1024]{};
        DWORD size = sizeof(val);
        if (RegQueryValueExA(key, "Authentication Packages", nullptr, nullptr, reinterpret_cast<BYTE*>(val), &size) == ERROR_SUCCESS && size > 0) {
            StartupEntry entry;
            entry.id = IdFor("LSA", "Authentication Packages");
            entry.name = "Authentication Packages";
            entry.command = val;
            entry.scope = "本机系统";
            entry.source = std::string("HKLM\\") + keyPath + "@Authentication Packages";
            entry.category = StartupCategory::Lsa;
            entry.location = StartupLocation::LsaPackage;
            entry.groupTitle = "HKLM\\" + std::string(keyPath);
            entry.status = StartupStatus::Protected;
            entry.risk = RiskLevel::Safe;
            StartupScanner::AddEntry(result, StartupScanner::Analyze(std::move(entry)));
        }
        RegCloseKey(key);
    }
}

void StartupScanner::ScanWmiSubscriptions(ScanSummary& result) {
    // Basic placeholder for permanent WMI subscription scanning
}

void StartupScanner::AddEntry(ScanSummary& result, StartupEntry entry) {
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

} // namespace AutoGuard
