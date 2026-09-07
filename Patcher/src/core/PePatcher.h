#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <windows.h>
#include "framework/controls/LogView.h"

namespace Patcher::Core {

using LogCallback = std::function<void(CUI::LogLevel level, const std::string& tag, const std::string& message)>;

inline std::string WstrToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) return "";
    std::string result(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, result.data(), size, nullptr, nullptr);
    return result;
}

enum class PatchMode {
    ReplaceTextSection = 0,  // 覆盖 .text 代码段
    InjectEntryPoint = 1     // 入口点注入 / 劫持
};

struct PeFileInfo {
    bool is64Bit = false;
    uint64_t imageBase = 0;
    uint32_t entryPointRva = 0;
    uint64_t entryPointVa = 0;
    uint32_t sectionCount = 0;
    uint32_t textSectionOffset = 0;
    uint32_t textSectionSize = 0;
    uint32_t subsystem = 0;
    bool hasSignature = false;
};

class PePatcher {
public:
    PePatcher(LogCallback logger = nullptr);
    ~PePatcher() = default;

    void SetLogger(LogCallback logger) { m_logger = std::move(logger); }

    // 检查并解析 PE 文件信息
    bool InspectPe(const std::wstring& pePath, PeFileInfo& outInfo);

    // 从 PE 文件提取 .text 节数据
    bool ExtractTextSection(const std::wstring& sourcePePath, std::vector<uint8_t>& outData);

    // 读取原始文件二进制
    bool ReadBinaryFile(const std::wstring& filePath, std::vector<uint8_t>& outData);

    // 执行核心 Patch 逻辑
    bool ExecutePatch(
        const std::wstring& whitePePath,
        const std::wstring& payloadPath,
        const std::wstring& outputPath,
        PatchMode mode = PatchMode::ReplaceTextSection
    );

    // 将数据写入目标文件
    bool WriteBinaryFile(const std::wstring& filePath, const std::vector<uint8_t>& data);

private:
    void Log(CUI::LogLevel level, const std::string& tag, const std::string& msg);

    LogCallback m_logger;
};

} // namespace Patcher::Core
