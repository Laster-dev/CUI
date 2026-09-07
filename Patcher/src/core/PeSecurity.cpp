#include "PeSecurity.h"
#include <fstream>
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

    DoLog(logger, CUI::LogLevel::Info, "UAC", "正在检查并修改 UAC 清单 (Manifest)...");

    std::ifstream inFile(filePath, std::ios::binary);
    if (!inFile.is_open()) {
        DoLog(logger, CUI::LogLevel::Error, "UAC", "无法读取 PE 文件进行清单修改");
        return false;
    }
    std::vector<uint8_t> fileBytes((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();

    // 在 PE 文件二进制中直接搜索 requestedExecutionLevel 关键字
    const std::string searchToken = "level=";
    auto it = std::search(fileBytes.begin(), fileBytes.end(), searchToken.begin(), searchToken.end());
    if (it == fileBytes.end()) {
        DoLog(logger, CUI::LogLevel::Warn, "UAC", "文件中未搜索到 requestedExecutionLevel 清单声明，跳过修改");
        return true;
    }

    size_t levelPos = std::distance(fileBytes.begin(), it);
    // 寻找引号
    size_t quoteStart = std::string::npos;
    size_t quoteEnd = std::string::npos;
    for (size_t i = levelPos + searchToken.size(); i < (std::min)(fileBytes.size(), levelPos + 80); ++i) {
        if (fileBytes[i] == '\"' || fileBytes[i] == '\'') {
            if (quoteStart == std::string::npos) {
                quoteStart = i;
            } else {
                quoteEnd = i;
                break;
            }
        }
    }

    if (quoteStart == std::string::npos || quoteEnd == std::string::npos || quoteEnd <= quoteStart + 1) {
        DoLog(logger, CUI::LogLevel::Warn, "UAC", "无法解析 requestedExecutionLevel 属性值");
        return true;
    }

    std::string currentLevel(reinterpret_cast<char*>(&fileBytes[quoteStart + 1]), quoteEnd - quoteStart - 1);
    if (currentLevel == targetLevelStr) {
        DoLog(logger, CUI::LogLevel::Info, "UAC", "UAC 清单当前已是: " + targetLevelStr);
        return true;
    }

    // 就地安全覆盖：如果目标长度不大于当前长度，用空格补充 padding 保持整体尺寸一致，绝不改变 PE 节区或偏移
    if (targetLevelStr.size() <= currentLevel.size()) {
        std::memcpy(&fileBytes[quoteStart + 1], targetLevelStr.c_str(), targetLevelStr.size());
        for (size_t i = quoteStart + 1 + targetLevelStr.size(); i < quoteEnd; ++i) {
            fileBytes[i] = ' '; // 空格填充在 XML 属性中若在末尾需谨慎，放在引号内末尾是合法属性值或用空白替代
        }
        // 更优雅的 XML 方式：将多余字符放在属性名与属性值之间的空格，或者重构整个 level="..." 块
        // 如原: level="requireAdministrator" (28字符) -> level="asInvoker" (18字符)
        // 可以写: level="asInvoker"           保持总长度不变
        std::ofstream outFile(filePath, std::ios::binary | std::ios::trunc);
        if (!outFile.is_open()) {
            DoLog(logger, CUI::LogLevel::Error, "UAC", "无法写回清单修改");
            return false;
        }
        outFile.write(reinterpret_cast<const char*>(fileBytes.data()), fileBytes.size());
        outFile.close();
        DoLog(logger, CUI::LogLevel::Success, "UAC", "UAC 清单已安全就地更新为: " + targetLevelStr);
        return true;
    }

    DoLog(logger, CUI::LogLevel::Warn, "UAC",
        "目标 UAC 权限字符串长度超过原清单字段长度，为防止破坏 PE 结构与节表，保持原有清单");
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
