#include "PeSecurity.h"
#include <format>
#include <regex>
#include <imagehlp.h>

#pragma comment(lib, "imagehlp.lib")

namespace Patcher::Core {

static void DoLog(LogCallback& logger, CUI::LogLevel level, const std::string& tag, const std::string& msg) {
    if (logger) {
        logger(level, tag, msg);
    }
}

bool PeSecurity::RemoveSignature(const std::wstring& filePath, LogCallback logger) {
    DoLog(logger, CUI::LogLevel::Info, "Signature", "正在处理数字签名剥离: " + WstrToUtf8(filePath));

    HANDLE hFile = CreateFileW(
        filePath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        DoLog(logger, CUI::LogLevel::Error, "Signature", "无法打开文件进行签名处理");
        return false;
    }

    DWORD bytesRead = 0;
    IMAGE_DOS_HEADER dosHeader;
    if (!ReadFile(hFile, &dosHeader, sizeof(dosHeader), &bytesRead, nullptr) || dosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
        DoLog(logger, CUI::LogLevel::Error, "Signature", "无效的 DOS 头");
        CloseHandle(hFile);
        return false;
    }

    if (SetFilePointer(hFile, dosHeader.e_lfanew, nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        DoLog(logger, CUI::LogLevel::Error, "Signature", "定位 NT 头失败");
        CloseHandle(hFile);
        return false;
    }

    DWORD ntSig = 0;
    ReadFile(hFile, &ntSig, sizeof(ntSig), &bytesRead, nullptr);
    if (ntSig != IMAGE_NT_SIGNATURE) {
        DoLog(logger, CUI::LogLevel::Error, "Signature", "无效的 NT 签名");
        CloseHandle(hFile);
        return false;
    }

    IMAGE_FILE_HEADER fileHeader;
    ReadFile(hFile, &fileHeader, sizeof(fileHeader), &bytesRead, nullptr);

    WORD optMagic = 0;
    ReadFile(hFile, &optMagic, sizeof(optMagic), &bytesRead, nullptr);

    DWORD certOffset = 0;
    DWORD certSize = 0;
    LONG secDirOffset = 0;

    if (optMagic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        secDirOffset = dosHeader.e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) +
                       offsetof(IMAGE_OPTIONAL_HEADER64, DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY]);
    } else if (optMagic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        secDirOffset = dosHeader.e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) +
                       offsetof(IMAGE_OPTIONAL_HEADER32, DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY]);
    } else {
        DoLog(logger, CUI::LogLevel::Error, "Signature", "未知的 PE OptionalHeader 格式");
        CloseHandle(hFile);
        return false;
    }

    SetFilePointer(hFile, secDirOffset, nullptr, FILE_BEGIN);
    IMAGE_DATA_DIRECTORY secDir;
    ReadFile(hFile, &secDir, sizeof(secDir), &bytesRead, nullptr);

    certOffset = secDir.VirtualAddress;
    certSize = secDir.Size;

    if (certOffset == 0 || certSize == 0) {
        DoLog(logger, CUI::LogLevel::Info, "Signature", "该文件未包含 Authenticode 数字签名");
        CloseHandle(hFile);
        return true;
    }

    DoLog(logger, CUI::LogLevel::Info, "Signature", std::format(
        "检测到数字签名: 偏移 0x{:X}, 大小 0x{:X} ({} 字节)",
        certOffset, certSize, certSize
    ));

    // 清零签名数据目录项
    secDir.VirtualAddress = 0;
    secDir.Size = 0;
    SetFilePointer(hFile, secDirOffset, nullptr, FILE_BEGIN);
    DWORD bytesWritten = 0;
    WriteFile(hFile, &secDir, sizeof(secDir), &bytesWritten, nullptr);

    // 检查是否位于文件末尾，若在末尾则直接截断文件
    DWORD fileSize = GetFileSize(hFile, nullptr);
    if (certOffset + certSize <= fileSize && certOffset > 0) {
        SetFilePointer(hFile, certOffset, nullptr, FILE_BEGIN);
        SetEndOfFile(hFile);
        DoLog(logger, CUI::LogLevel::Success, "Signature", std::format(
            "已物理截断文件末尾的签名块，减少文件大小 {} 字节",
            fileSize - certOffset
        ));
    }

    CloseHandle(hFile);
    DoLog(logger, CUI::LogLevel::Success, "Signature", "数字签名已成功剥离");
    return true;
}

bool PeSecurity::ModifyUacManifest(const std::wstring& filePath, UacLevel level, LogCallback logger) {
    if (level == UacLevel::KeepOriginal) {
        return true;
    }

    DoLog(logger, CUI::LogLevel::Info, "UAC", "正在检查并修改 UAC 清单 (Manifest)...");

    HMODULE hModule = LoadLibraryExW(filePath.c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE);
    if (!hModule) {
        DoLog(logger, CUI::LogLevel::Warn, "UAC", "无法作为数据文件加载 PE 以读取清单");
        return false;
    }

    HRSRC hRes = FindResourceW(hModule, MAKEINTRESOURCEW(1), RT_MANIFEST);
    if (!hRes) {
        // 尝试 ID 2
        hRes = FindResourceW(hModule, MAKEINTRESOURCEW(2), RT_MANIFEST);
    }

    if (!hRes) {
        DoLog(logger, CUI::LogLevel::Warn, "UAC", "PE 文件中未找到 RT_MANIFEST 清单资源");
        FreeLibrary(hModule);
        return false;
    }

    HGLOBAL hResData = LoadResource(hModule, hRes);
    if (!hResData) {
        FreeLibrary(hModule);
        return false;
    }

    DWORD resSize = SizeofResource(hModule, hRes);
    const char* pResData = static_cast<const char*>(LockResource(hResData));
    if (!pResData || resSize == 0) {
        FreeLibrary(hModule);
        return false;
    }

    std::string manifest(pResData, resSize);
    FreeLibrary(hModule); // 释放只读模块，以便后续以 UpdateResource 写入

    std::string targetLevelStr;
    switch (level) {
    case UacLevel::AsInvoker:
        targetLevelStr = "asInvoker";
        break;
    case UacLevel::RequireAdministrator:
        targetLevelStr = "requireAdministrator";
        break;
    case UacLevel::HighestAvailable:
        targetLevelStr = "highestAvailable";
        break;
    default:
        return true;
    }

    // 替换 requestedExecutionLevel
    std::regex reg(R"(level\s*=\s*["'][^"']+["'])");
    std::string replacement = "level=\"" + targetLevelStr + "\"";
    std::string modifiedManifest = std::regex_replace(manifest, reg, replacement);

    if (modifiedManifest == manifest) {
        DoLog(logger, CUI::LogLevel::Warn, "UAC", "清单中未检索到 requestedExecutionLevel 或内容已一致");
        return true;
    }

    HANDLE hUpdate = BeginUpdateResourceW(filePath.c_str(), FALSE);
    if (!hUpdate) {
        DoLog(logger, CUI::LogLevel::Error, "UAC", "无法启动资源更新");
        return false;
    }

    if (!UpdateResourceW(
        hUpdate,
        RT_MANIFEST,
        MAKEINTRESOURCEW(1),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
        const_cast<char*>(modifiedManifest.c_str()),
        static_cast<DWORD>(modifiedManifest.size())
    )) {
        DoLog(logger, CUI::LogLevel::Error, "UAC", "写入清单资源失败");
        EndUpdateResourceW(hUpdate, TRUE);
        return false;
    }

    if (!EndUpdateResourceW(hUpdate, FALSE)) {
        DoLog(logger, CUI::LogLevel::Error, "UAC", "提交清单资源更新失败");
        return false;
    }

    DoLog(logger, CUI::LogLevel::Success, "UAC", "UAC 清单已更新为: " + targetLevelStr);
    return true;
}

bool PeSecurity::ConvertSubsystem(const std::wstring& filePath, SubsystemType type, LogCallback logger) {
    if (type == SubsystemType::KeepOriginal) {
        return true;
    }

    WORD targetSubsystem = (type == SubsystemType::WindowsGui) ? IMAGE_SUBSYSTEM_WINDOWS_GUI : IMAGE_SUBSYSTEM_WINDOWS_CUI;
    std::string desc = (type == SubsystemType::WindowsGui) ? "Windows GUI (隐藏控制台窗口)" : "控制台 (Console)";
    DoLog(logger, CUI::LogLevel::Info, "Subsystem", "正在转换 PE 子系统为: " + desc);

    HANDLE hFile = CreateFileW(
        filePath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        DoLog(logger, CUI::LogLevel::Error, "Subsystem", "无法打开文件修改子系统");
        return false;
    }

    DWORD bytesRead = 0;
    IMAGE_DOS_HEADER dosHeader;
    ReadFile(hFile, &dosHeader, sizeof(dosHeader), &bytesRead, nullptr);
    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
        CloseHandle(hFile);
        return false;
    }

    SetFilePointer(hFile, dosHeader.e_lfanew, nullptr, FILE_BEGIN);
    DWORD ntSig = 0;
    ReadFile(hFile, &ntSig, sizeof(ntSig), &bytesRead, nullptr);
    if (ntSig != IMAGE_NT_SIGNATURE) {
        CloseHandle(hFile);
        return false;
    }

    IMAGE_FILE_HEADER fileHeader;
    ReadFile(hFile, &fileHeader, sizeof(fileHeader), &bytesRead, nullptr);

    WORD optMagic = 0;
    ReadFile(hFile, &optMagic, sizeof(optMagic), &bytesRead, nullptr);

    LONG subsystemOffset = 0;
    if (optMagic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        subsystemOffset = dosHeader.e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) +
                          offsetof(IMAGE_OPTIONAL_HEADER64, Subsystem);
    } else if (optMagic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        subsystemOffset = dosHeader.e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) +
                          offsetof(IMAGE_OPTIONAL_HEADER32, Subsystem);
    } else {
        CloseHandle(hFile);
        return false;
    }

    SetFilePointer(hFile, subsystemOffset, nullptr, FILE_BEGIN);
    DWORD bytesWritten = 0;
    WriteFile(hFile, &targetSubsystem, sizeof(targetSubsystem), &bytesWritten, nullptr);
    CloseHandle(hFile);

    DoLog(logger, CUI::LogLevel::Success, "Subsystem", "子系统已更新为: " + desc);
    return true;
}

bool PeSecurity::WipeTimeDateStamp(const std::wstring& filePath, DWORD timestamp, LogCallback logger) {
    DoLog(logger, CUI::LogLevel::Info, "AntiForensics", std::format("正在重设 PE 时间戳为 0x{:08X}...", timestamp));

    HANDLE hFile = CreateFileW(
        filePath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD bytesRead = 0;
    IMAGE_DOS_HEADER dosHeader;
    ReadFile(hFile, &dosHeader, sizeof(dosHeader), &bytesRead, nullptr);
    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
        CloseHandle(hFile);
        return false;
    }

    LONG timestampOffset = dosHeader.e_lfanew + sizeof(DWORD) + offsetof(IMAGE_FILE_HEADER, TimeDateStamp);
    SetFilePointer(hFile, timestampOffset, nullptr, FILE_BEGIN);

    DWORD bytesWritten = 0;
    WriteFile(hFile, &timestamp, sizeof(timestamp), &bytesWritten, nullptr);
    CloseHandle(hFile);

    DoLog(logger, CUI::LogLevel::Success, "AntiForensics", "PE 编译时间戳擦除成功");
    return true;
}

} // namespace Patcher::Core
