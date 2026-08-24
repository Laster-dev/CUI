#include "StartupManager.h"

#include <windows.h>
#include <taskschd.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace AutoGuard {
namespace {

std::wstring Widen(const std::string& value) {
    if (value.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size);
    return result;
}

std::string Narrow(const std::wstring& value) {
    if (value.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size, nullptr, nullptr);
    return result;
}

} // namespace

bool StartupManager::ToggleStatus(StartupEntry& entry, bool enable, std::string& outMessage) {
    if (entry.status == StartupStatus::Protected) {
        outMessage = "该项属于 Windows 核心保护组件，不可更改状态。";
        return false;
    }

    if (entry.location == StartupLocation::StartupFolder) {
        std::wstring currentPath = Widen(entry.command);
        if (enable) {
            if (currentPath.size() > 9 && currentPath.substr(currentPath.size() - 9) == L".disabled") {
                std::wstring newPath = currentPath.substr(0, currentPath.size() - 9);
                if (MoveFileW(currentPath.c_str(), newPath.c_str())) {
                    entry.status = StartupStatus::Enabled;
                    entry.command = Narrow(newPath);
                    outMessage = "已启用启动文件夹项目。";
                    return true;
                }
            }
        } else {
            std::wstring newPath = currentPath + L".disabled";
            if (MoveFileW(currentPath.c_str(), newPath.c_str())) {
                entry.status = StartupStatus::Disabled;
                entry.command = Narrow(newPath);
                outMessage = "已禁用启动文件夹项目。";
                return true;
            }
        }
        outMessage = "修改启动文件夹项目状态失败（可能需要管理员权限）。";
        return false;
    }

    if (entry.location == StartupLocation::RegistryRun) {
        HKEY root = entry.isHklm ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
        std::string runKeyPath = entry.registryKey.empty() ? "Software\\Microsoft\\Windows\\CurrentVersion\\Run" : entry.registryKey;
        std::string disabledKeyPath = "Software\\AutoGuard\\DisabledRun";

        if (!enable) {
            HKEY hSrc = nullptr;
            if (RegOpenKeyExA(root, runKeyPath.c_str(), 0, KEY_READ | KEY_WRITE, &hSrc) == ERROR_SUCCESS) {
                char val[4096]{};
                DWORD valSize = sizeof(val);
                DWORD type = 0;
                if (RegQueryValueExA(hSrc, entry.name.c_str(), nullptr, &type, reinterpret_cast<BYTE*>(val), &valSize) == ERROR_SUCCESS) {
                    HKEY hDst = nullptr;
                    if (RegCreateKeyExA(root, disabledKeyPath.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &hDst, nullptr) == ERROR_SUCCESS) {
                        RegSetValueExA(hDst, entry.name.c_str(), 0, type, reinterpret_cast<BYTE*>(val), valSize);
                        RegCloseKey(hDst);
                        RegDeleteValueA(hSrc, entry.name.c_str());
                        RegCloseKey(hSrc);
                        entry.status = StartupStatus::Disabled;
                        outMessage = "已禁用注册表启动项并安全备份。";
                        return true;
                    }
                }
                RegCloseKey(hSrc);
            }
        } else {
            HKEY hDst = nullptr;
            if (RegOpenKeyExA(root, disabledKeyPath.c_str(), 0, KEY_READ | KEY_WRITE, &hDst) == ERROR_SUCCESS) {
                char val[4096]{};
                DWORD valSize = sizeof(val);
                DWORD type = 0;
                if (RegQueryValueExA(hDst, entry.name.c_str(), nullptr, &type, reinterpret_cast<BYTE*>(val), &valSize) == ERROR_SUCCESS) {
                    HKEY hSrc = nullptr;
                    if (RegOpenKeyExA(root, runKeyPath.c_str(), 0, KEY_WRITE, &hSrc) == ERROR_SUCCESS) {
                        RegSetValueExA(hSrc, entry.name.c_str(), 0, type, reinterpret_cast<BYTE*>(val), valSize);
                        RegCloseKey(hSrc);
                        RegDeleteValueA(hDst, entry.name.c_str());
                        RegCloseKey(hDst);
                        entry.status = StartupStatus::Enabled;
                        outMessage = "已重新启用注册表启动项。";
                        return true;
                    }
                }
                RegCloseKey(hDst);
            }
        }
        outMessage = "修改注册表状态失败（可能需要管理员权限）。";
        return false;
    }

    if (entry.location == StartupLocation::ScheduledTask) {
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        ITaskService* service = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&service)) && service) {
            VARIANT empty; VariantInit(&empty);
            service->Connect(empty, empty, empty, empty);
            ITaskFolder* root = nullptr;
            BSTR rootPath = SysAllocString(L"\\");
            if (SUCCEEDED(service->GetFolder(rootPath, &root)) && root) {
                std::string pathStr = entry.id;
                size_t colon = pathStr.find(':');
                if (colon != std::string::npos) pathStr = pathStr.substr(colon + 1);
                BSTR taskPathBstr = SysAllocString(Widen(pathStr).c_str());
                IRegisteredTask* task = nullptr;
                if (SUCCEEDED(root->GetTask(taskPathBstr, &task)) && task) {
                    task->put_Enabled(enable ? VARIANT_TRUE : VARIANT_FALSE);
                    task->Release();
                    entry.status = enable ? StartupStatus::Enabled : StartupStatus::Disabled;
                    outMessage = enable ? "已启用计划任务。" : "已禁用计划任务。";
                    SysFreeString(taskPathBstr);
                    SysFreeString(rootPath);
                    root->Release();
                    service->Release();
                    CoUninitialize();
                    return true;
                }
                SysFreeString(taskPathBstr);
                root->Release();
            }
            SysFreeString(rootPath);
            service->Release();
        }
        CoUninitialize();
        outMessage = "修改计划任务失败。";
        return false;
    }

    if (entry.location == StartupLocation::Service || entry.location == StartupLocation::Driver) {
        std::string svcName = entry.name;
        SC_HANDLE manager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
        if (manager) {
            SC_HANDLE service = OpenServiceW(manager, Widen(svcName).c_str(), SERVICE_CHANGE_CONFIG);
            if (service) {
                DWORD startType = enable ? SERVICE_AUTO_START : SERVICE_DISABLED;
                if (ChangeServiceConfigW(service, SERVICE_NO_CHANGE, startType, SERVICE_NO_CHANGE, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr)) {
                    entry.status = enable ? StartupStatus::Enabled : StartupStatus::Disabled;
                    outMessage = enable ? "已将服务设为自动启动。" : "已禁用系统服务。";
                    CloseServiceHandle(service);
                    CloseServiceHandle(manager);
                    return true;
                }
                CloseServiceHandle(service);
            }
            CloseServiceHandle(manager);
        }
        outMessage = "修改服务配置失败（请以管理员身份运行）。";
        return false;
    }

    outMessage = "该类型启动项暂不支持直接切换状态。";
    return false;
}

bool StartupManager::DeleteEntry(const StartupEntry& entry, std::string& outMessage) {
    if (entry.status == StartupStatus::Protected) {
        outMessage = "受系统保护的核心项无法删除。";
        return false;
    }

    if (entry.location == StartupLocation::StartupFolder) {
        std::wstring p = Widen(entry.command);
        if (DeleteFileW(p.c_str())) {
            outMessage = "已成功删除启动文件夹中的快捷项。";
            return true;
        }
        outMessage = "删除启动文件失败。";
        return false;
    }

    if (entry.location == StartupLocation::RegistryRun) {
        HKEY root = entry.isHklm ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
        std::string runKeyPath = entry.registryKey.empty() ? "Software\\Microsoft\\Windows\\CurrentVersion\\Run" : entry.registryKey;
        HKEY hKey = nullptr;
        if (RegOpenKeyExA(root, runKeyPath.c_str(), 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            if (RegDeleteValueA(hKey, entry.name.c_str()) == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                outMessage = "已成功从注册表中删除该启动项。";
                return true;
            }
            RegCloseKey(hKey);
        }
        outMessage = "删除注册表键值失败。";
        return false;
    }

    if (entry.location == StartupLocation::ImageHijack) {
        HKEY hKey = nullptr;
        std::string ifeoKey = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\" + entry.name;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, ifeoKey.c_str(), 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            if (RegDeleteValueA(hKey, "Debugger") == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                outMessage = "已成功清除 IFEO 镜像劫持调试器。";
                return true;
            }
            RegCloseKey(hKey);
        }
        outMessage = "清除镜像劫持失败。";
        return false;
    }

    outMessage = "该入口类型暂不支持直接删除。";
    return false;
}

bool StartupManager::JumpToImage(const StartupEntry& entry) {
    std::string filePath = entry.executablePath.empty() ? entry.command : entry.executablePath;
    if (filePath.empty()) return false;
    std::wstring pathW = Widen(filePath);
    if (GetFileAttributesW(pathW.c_str()) != INVALID_FILE_ATTRIBUTES) {
        std::wstring params = L"/select,\"" + pathW + L"\"";
        ShellExecuteW(nullptr, L"open", L"explorer.exe", params.c_str(), nullptr, SW_SHOWNORMAL);
        return true;
    }
    try {
        std::filesystem::path p(pathW);
        std::filesystem::path parent = p.parent_path();
        if (std::filesystem::exists(parent)) {
            ShellExecuteW(nullptr, L"open", parent.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            return true;
        }
    } catch (...) {}
    return false;
}

bool StartupManager::JumpToEntry(const StartupEntry& entry) {
    if (entry.location == StartupLocation::RegistryRun || entry.location == StartupLocation::ImageHijack ||
        entry.location == StartupLocation::Winlogon || entry.location == StartupLocation::ActiveSetup ||
        entry.location == StartupLocation::AppInit || entry.location == StartupLocation::KnownDlls ||
        entry.location == StartupLocation::BootExecute || entry.location == StartupLocation::WinsockProvider ||
        entry.location == StartupLocation::PrintMonitor || entry.location == StartupLocation::LsaPackage ||
        entry.location == StartupLocation::ComHijack) {

        std::string keyPath = entry.source;
        size_t atPos = keyPath.find('@');
        if (atPos != std::string::npos) keyPath = keyPath.substr(0, atPos);
        if (keyPath.find("CLSID: ") == 0) {
            keyPath = "HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved";
        }

        std::wstring fullKey = L"Computer\\" + Widen(keyPath);
        if (fullKey.find(L"Computer\\HKCU\\") == 0) {
            fullKey = L"Computer\\HKEY_CURRENT_USER\\" + fullKey.substr(14);
        } else if (fullKey.find(L"Computer\\HKLM\\") == 0) {
            fullKey = L"Computer\\HKEY_LOCAL_MACHINE\\" + fullKey.substr(14);
        }
        HKEY hKey = nullptr;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Regedit", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            RegSetValueExW(hKey, L"LastKey", 0, REG_SZ, reinterpret_cast<const BYTE*>(fullKey.c_str()), static_cast<DWORD>((fullKey.size() + 1) * sizeof(wchar_t)));
            RegCloseKey(hKey);
        }
        ShellExecuteW(nullptr, L"open", L"regedit.exe", nullptr, nullptr, SW_SHOWNORMAL);
        return true;
    }

    if (entry.location == StartupLocation::ScheduledTask) {
        ShellExecuteW(nullptr, L"open", L"taskschd.msc", nullptr, nullptr, SW_SHOWNORMAL);
        return true;
    }

    if (entry.location == StartupLocation::Service || entry.location == StartupLocation::Driver) {
        ShellExecuteW(nullptr, L"open", L"services.msc", nullptr, nullptr, SW_SHOWNORMAL);
        return true;
    }

    if (entry.location == StartupLocation::StartupFolder) {
        return JumpToImage(entry);
    }

    return false;
}

bool StartupManager::SearchOnline(const StartupEntry& entry) {
    std::string searchTarget = entry.name;
    if (!entry.executablePath.empty()) {
        try {
            searchTarget = std::filesystem::path(Widen(entry.executablePath)).filename().string();
        } catch (...) {}
    }
    std::wstring url = L"https://www.virustotal.com/gui/search/" + Widen(searchTarget);
    ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return true;
}

bool StartupManager::CopyToClipboard(HWND hwnd, const std::string& text) {
    if (text.empty()) return false;
    std::wstring wText = Widen(text);
    if (!OpenClipboard(hwnd)) return false;
    EmptyClipboard();
    size_t bytes = (wText.size() + 1) * sizeof(wchar_t);
    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (hGlob) {
        memcpy(GlobalLock(hGlob), wText.c_str(), bytes);
        GlobalUnlock(hGlob);
        SetClipboardData(CF_UNICODETEXT, hGlob);
    }
    CloseClipboard();
    return true;
}

bool StartupManager::ExportReport(const ScanSummary& summary, const std::wstring& outputPath, std::string& outMessage) {
    std::ofstream file(outputPath, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        outMessage = "无法创建报告文件。";
        return false;
    }
    file << "\xEF\xBB\xBF";
    file << "================================================================================\n";
    file << "                     AutoGuard Windows 启动项安全扫描审计报告                     \n";
    file << "================================================================================\n";
    file << "扫描时间: " << summary.scanTimestamp << "\n";
    file << "启动项总数: " << summary.entries.size() << " | 已启用: " << summary.enabledCount
         << " | 已禁用: " << summary.disabledCount << " | 目标失效: " << summary.missingCount
         << " | 潜在风险: " << (summary.suspiciousCount + summary.highRiskCount) << "\n\n";

    file << "自动运行条目,描述,出版商,状态,风险等级,镜像路径,时间戳,版本\n";
    for (const auto& entry : summary.entries) {
        file << "\"" << entry.name << "\","
             << "\"" << entry.description << "\","
             << "\"" << entry.publisher << "\","
             << "\"" << StatusName(entry.status) << "\","
             << "\"" << RiskName(entry.risk) << "\","
             << "\"" << entry.command << "\","
             << "\"" << entry.fileTimestamp << "\","
             << "\"" << entry.fileVersion << "\"\n";
    }

    outMessage = "报告已成功导出！";
    return true;
}

} // namespace AutoGuard
