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
    InjectEntryPoint = 1,    // 入口点注入 / 劫持
    EnlargeLastSection = 2,  // 扩容末尾节注入（扩 SizeOfRawData/VirtualSize 并重定向入口点）
    AddNewSection = 3,       // 新增独立节注入（追加节头 + 追加数据并重定向入口点）
    TlsCallback = 4,         // TLS 回调注入（不修改入口点，OEP 之前执行）
    ImportInjection = 5,     // 导入表注入（载荷须为 DLL，由系统加载器加载）
    DllExportPatch = 6       // DLL 导出函数覆盖（白文件须为 DLL，覆盖指定导出函数/DLLMain 的函数体）
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
    bool isDll = false;      // 目标是否为 DLL（Characteristics & IMAGE_FILE_DLL）
};

struct PatchModeAvailability {
    bool canReplaceText = false;
    uint32_t textSectionCapacity = 0;
    bool canInjectOep = false;
    uint32_t oepRemainingCapacity = 0;
    bool canEnlargeLastSection = true;
    bool canAddNewSection = false;
    bool canTlsCallback = true;
    bool canImportInjection = false;
    bool canDllExportPatch = false;   // 白文件须为 DLL
    uintmax_t payloadSize = 0;
};

class PePatcher {
public:
    PePatcher(LogCallback logger = nullptr);
    ~PePatcher() = default;

    void SetLogger(LogCallback logger) { m_logger = std::move(logger); }

    // 检查并解析 PE 文件信息
    bool InspectPe(const std::wstring& pePath, PeFileInfo& outInfo);

    // 评估各项 Patch 模式针对特定白文件和载荷的可用性
    bool EvaluatePatchModes(const std::wstring& whitePath, const std::wstring& payloadPath, PatchModeAvailability& outAvail);

    // 从 PE 文件提取 .text 节数据
    bool ExtractTextSection(const std::wstring& sourcePePath, std::vector<uint8_t>& outData);

    // 枚举 DLL 的导出函数名（用于 AutoSuggestBox 建议列表）
    bool ListDllExports(const std::wstring& dllPath, std::vector<std::string>& outExports);

    // 读取原始文件二进制
    bool ReadBinaryFile(const std::wstring& filePath, std::vector<uint8_t>& outData);

    // 执行核心 Patch 逻辑
    bool ExecutePatch(
        const std::wstring& whitePePath,
        const std::wstring& payloadPath,
        const std::wstring& outputPath,
        PatchMode mode = PatchMode::ReplaceTextSection,
        bool removeSignature = true,
        const std::string& dllFuncName = "DLLMain",  // DllExportPatch 模式：要覆盖的导出函数名（空/DLLMain = 入口点）
        bool disableCfg = true                       // 是否剥离 CFG（GUARD_CF 位与 LOAD_CONFIG 目录）
    );

    // 将数据写入目标文件
    bool WriteBinaryFile(const std::wstring& filePath, const std::vector<uint8_t>& data);

private:
    void Log(CUI::LogLevel level, const std::string& tag, const std::string& msg);

    // ---- 扩展注入模式 ----
    // 定位占据文件末尾的节，扩容 RawSize/VirtualSize/SizeOfImage，返回可安全写入的文件偏移与对应 RVA
    bool ExpandLastSection(std::vector<uint8_t>& buf, size_t neededBytes,
                           uint32_t& outAppendFileOffset, uint32_t& outAppendRVA, DWORD extraCharacteristics);
    // 签名 Overlay 迁移：附加数据前摘除证书块，写出前再挂回文件末尾，避免载荷覆盖证书
    bool DetachSecurityDirectory(std::vector<uint8_t>& buf, std::vector<uint8_t>& certOut);
    bool AttachSecurityDirectory(std::vector<uint8_t>& buf, const std::vector<uint8_t>& certIn);

    bool InjectByEnlargeLastSection(std::vector<uint8_t>& buf, const std::vector<uint8_t>& payload, bool disableCfg);
    bool InjectByNewSection(std::vector<uint8_t>& buf, const std::vector<uint8_t>& payload, bool disableCfg);
    bool InjectByTlsCallback(std::vector<uint8_t>& buf, const std::vector<uint8_t>& payload, bool disableCfg);
    bool InjectByImportTable(std::vector<uint8_t>& buf, const std::wstring& payloadPath);
    bool InjectByDllExportPatch(std::vector<uint8_t>& buf, const std::vector<uint8_t>& payload, const std::string& funcName);

    LogCallback m_logger;
};

} // namespace Patcher::Core
