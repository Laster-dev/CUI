#include "PePatcher.h"
#include <fstream>
#include <format>
#include <filesystem>

namespace Patcher::Core {

PePatcher::PePatcher(LogCallback logger)
    : m_logger(std::move(logger)) {
}

void PePatcher::Log(CUI::LogLevel level, const std::string& tag, const std::string& msg) {
    if (m_logger) {
        m_logger(level, tag, msg);
    }
}

bool PePatcher::ReadBinaryFile(const std::wstring& filePath, std::vector<uint8_t>& outData) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        Log(CUI::LogLevel::Error, "IO", "无法打开文件进行读取: " + WstrToUtf8(filePath));
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    outData.resize(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(outData.data()), size)) {
        Log(CUI::LogLevel::Error, "IO", "读取文件失败");
        return false;
    }
    return true;
}

bool PePatcher::WriteBinaryFile(const std::wstring& filePath, const std::vector<uint8_t>& data) {
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        Log(CUI::LogLevel::Error, "IO", "无法打开输出文件进行写入: " + WstrToUtf8(filePath));
        return false;
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    if (!file.good()) {
        Log(CUI::LogLevel::Error, "IO", "写入输出文件失败");
        return false;
    }
    return true;
}

bool PePatcher::InspectPe(const std::wstring& pePath, PeFileInfo& outInfo) {
    std::vector<uint8_t> buffer;
    if (!ReadBinaryFile(pePath, buffer) || buffer.size() < sizeof(IMAGE_DOS_HEADER)) {
        return false;
    }

    auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(buffer.data());
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        Log(CUI::LogLevel::Error, "PE", "无效的 DOS 头签名 (非 MZ)");
        return false;
    }

    if (dosHeader->e_lfanew <= 0 || static_cast<size_t>(dosHeader->e_lfanew) + sizeof(DWORD) > buffer.size()) {
        Log(CUI::LogLevel::Error, "PE", "PE 头部偏移超出文件范围");
        return false;
    }

    auto* ntSignature = reinterpret_cast<DWORD*>(buffer.data() + dosHeader->e_lfanew);
    if (*ntSignature != IMAGE_NT_SIGNATURE) {
        Log(CUI::LogLevel::Error, "PE", "无效的 NT 头签名 (非 PE00)");
        return false;
    }

    auto* fileHeader = reinterpret_cast<IMAGE_FILE_HEADER*>(buffer.data() + dosHeader->e_lfanew + sizeof(DWORD));
    outInfo.sectionCount = fileHeader->NumberOfSections;

    WORD optMagic = *reinterpret_cast<WORD*>(buffer.data() + dosHeader->e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER));
    IMAGE_SECTION_HEADER* firstSection = nullptr;

    if (optMagic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        outInfo.is64Bit = true;
        auto* opt64 = reinterpret_cast<IMAGE_OPTIONAL_HEADER64*>(buffer.data() + dosHeader->e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER));
        outInfo.imageBase = opt64->ImageBase;
        outInfo.entryPointRva = opt64->AddressOfEntryPoint;
        outInfo.entryPointVa = opt64->ImageBase + opt64->AddressOfEntryPoint;
        outInfo.subsystem = opt64->Subsystem;
        outInfo.hasSignature = (opt64->DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress != 0 &&
                                opt64->DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size != 0);

        firstSection = reinterpret_cast<IMAGE_SECTION_HEADER*>(
            reinterpret_cast<uint8_t*>(opt64) + fileHeader->SizeOfOptionalHeader
        );
    } else if (optMagic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        outInfo.is64Bit = false;
        auto* opt32 = reinterpret_cast<IMAGE_OPTIONAL_HEADER32*>(buffer.data() + dosHeader->e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER));
        outInfo.imageBase = opt32->ImageBase;
        outInfo.entryPointRva = opt32->AddressOfEntryPoint;
        outInfo.entryPointVa = opt32->ImageBase + opt32->AddressOfEntryPoint;
        outInfo.subsystem = opt32->Subsystem;
        outInfo.hasSignature = (opt32->DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress != 0 &&
                                opt32->DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size != 0);

        firstSection = reinterpret_cast<IMAGE_SECTION_HEADER*>(
            reinterpret_cast<uint8_t*>(opt32) + fileHeader->SizeOfOptionalHeader
        );
    } else {
        Log(CUI::LogLevel::Error, "PE", "未知的 OptionalHeader Magic: 0x" + std::format("{:04X}", optMagic));
        return false;
    }

    // 查找 .text 节
    for (DWORD i = 0; i < fileHeader->NumberOfSections; ++i) {
        IMAGE_SECTION_HEADER* sec = &firstSection[i];
        if (strncmp(reinterpret_cast<const char*>(sec->Name), ".text", 5) == 0) {
            outInfo.textSectionOffset = sec->PointerToRawData;
            outInfo.textSectionSize = sec->SizeOfRawData;
            break;
        }
    }

    return true;
}

bool PePatcher::ExtractTextSection(const std::wstring& sourcePePath, std::vector<uint8_t>& outData) {
    std::vector<uint8_t> buffer;
    if (!ReadBinaryFile(sourcePePath, buffer)) {
        return false;
    }

    PeFileInfo info;
    if (!InspectPe(sourcePePath, info)) {
        return false;
    }

    if (info.textSectionOffset == 0 || info.textSectionSize == 0) {
        Log(CUI::LogLevel::Error, "PE", "源 PE 文件中未找到 .text 代码节");
        return false;
    }

    if (info.textSectionOffset + info.textSectionSize > buffer.size()) {
        Log(CUI::LogLevel::Error, "PE", ".text 节偏移超出文件边界");
        return false;
    }

    outData.assign(
        buffer.begin() + info.textSectionOffset,
        buffer.begin() + info.textSectionOffset + info.textSectionSize
    );
    Log(CUI::LogLevel::Info, "PE", std::format("提取 .text 代码段成功，尺寸: {} 字节 (0x{:X})", outData.size(), outData.size()));
    return true;
}

bool PePatcher::ExecutePatch(
    const std::wstring& whitePePath,
    const std::wstring& payloadPath,
    const std::wstring& outputPath,
    PatchMode mode
) {
    Log(CUI::LogLevel::Info, "Patch", "================== 开始执行 Patch 流程 ==================");
    Log(CUI::LogLevel::Info, "Patch", "目标白文件: " + WstrToUtf8(whitePePath));
    Log(CUI::LogLevel::Info, "Patch", "注入载荷: " + WstrToUtf8(payloadPath));

    // 1. 读取并验证目标白文件
    std::vector<uint8_t> whiteBuffer;
    if (!ReadBinaryFile(whitePePath, whiteBuffer)) {
        Log(CUI::LogLevel::Error, "Patch", "[-] 读取目标白文件失败");
        return false;
    }

    PeFileInfo whiteInfo;
    if (!InspectPe(whitePePath, whiteInfo)) {
        Log(CUI::LogLevel::Error, "Patch", "[-] 目标白文件 PE 结构校验失败");
        return false;
    }

    Log(CUI::LogLevel::Info, "PE", std::format(
        "[+] 白文件架构: {}, ImageBase: 0x{:X}, EntryPoint RVA: 0x{:X} (VA: 0x{:X})",
        whiteInfo.is64Bit ? "x64 (64-bit)" : "x86 (32-bit)",
        whiteInfo.imageBase, whiteInfo.entryPointRva, whiteInfo.entryPointVa
    ));

    // 2. 准备载荷数据
    std::vector<uint8_t> payloadData;
    std::wstring ext = std::filesystem::path(payloadPath).extension().wstring();
    for (auto& c : ext) c = towlower(c);

    if (ext == L".exe" || ext == L".dll") {
        Log(CUI::LogLevel::Info, "Payload", "输入载荷为可执行文件，正在解析并提取其 .text 代码段...");
        if (!ExtractTextSection(payloadPath, payloadData)) {
            Log(CUI::LogLevel::Error, "Payload", "[-] 提取载荷代码段失败");
            return false;
        }
    } else {
        Log(CUI::LogLevel::Info, "Payload", "输入载荷为原始二进制数据 (.bin / .text)...");
        if (!ReadBinaryFile(payloadPath, payloadData)) {
            Log(CUI::LogLevel::Error, "Payload", "[-] 读取载荷二进制数据失败");
            return false;
        }
    }

    if (payloadData.empty()) {
        Log(CUI::LogLevel::Error, "Payload", "[-] 载荷数据为空，终止操作");
        return false;
    }
    Log(CUI::LogLevel::Info, "Payload", std::format("[+] 载荷大小: {} 字节", payloadData.size()));

    // 3. 计算注入写入位置
    size_t targetOffset = 0;
    if (mode == PatchMode::ReplaceTextSection) {
        if (whiteInfo.textSectionOffset == 0) {
            Log(CUI::LogLevel::Error, "Patch", "[-] 白文件中不存在 .text 节，无法执行代码段替换");
            return false;
        }
        targetOffset = whiteInfo.textSectionOffset;
        Log(CUI::LogLevel::Info, "Patch", std::format("[+] 模式: .text 代码段覆盖，目标文件偏移: 0x{:X}，白文件段容积: {} 字节",
            targetOffset, whiteInfo.textSectionSize));

        if (payloadData.size() > whiteInfo.textSectionSize) {
            Log(CUI::LogLevel::Warn, "Patch", std::format(
                "[!] 载荷大小 ({} 字节) 超出白文件 .text 原段大小 ({} 字节)，将截断或可能引起节重叠",
                payloadData.size(), whiteInfo.textSectionSize));
        }
    } else {
        // 入口点注入模式
        auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(whiteBuffer.data());
        auto* fileHeader = reinterpret_cast<IMAGE_FILE_HEADER*>(whiteBuffer.data() + dosHeader->e_lfanew + sizeof(DWORD));
        IMAGE_SECTION_HEADER* firstSection = reinterpret_cast<IMAGE_SECTION_HEADER*>(
            whiteBuffer.data() + dosHeader->e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + fileHeader->SizeOfOptionalHeader
        );

        bool foundSec = false;
        for (DWORD i = 0; i < fileHeader->NumberOfSections; ++i) {
            IMAGE_SECTION_HEADER* sec = &firstSection[i];
            if (whiteInfo.entryPointRva >= sec->VirtualAddress &&
                whiteInfo.entryPointRva < sec->VirtualAddress + sec->SizeOfRawData) {
                targetOffset = (whiteInfo.entryPointRva - sec->VirtualAddress) + sec->PointerToRawData;
                foundSec = true;
                break;
            }
        }

        if (!foundSec) {
            Log(CUI::LogLevel::Error, "Patch", "[-] 无法将 EntryPoint RVA 映射到文件偏移");
            return false;
        }
        Log(CUI::LogLevel::Info, "Patch", std::format("[+] 模式: 入口点注入，EntryPoint 文件偏移: 0x{:X}", targetOffset));
    }

    // 4. 拷贝覆盖数据
    if (targetOffset + payloadData.size() > whiteBuffer.size()) {
        // 扩容白文件缓冲区
        whiteBuffer.resize(targetOffset + payloadData.size());
    }
    std::memcpy(whiteBuffer.data() + targetOffset, payloadData.data(), payloadData.size());
    Log(CUI::LogLevel::Info, "Patch", std::format("[+] 数据注入成功，共写入 {} 字节", payloadData.size()));

    // 5. 写出到目标文件
    if (!WriteBinaryFile(outputPath, whiteBuffer)) {
        Log(CUI::LogLevel::Error, "Patch", "[-] 输出修补后文件失败: " + WstrToUtf8(outputPath));
        return false;
    }

    Log(CUI::LogLevel::Info, "Patch", "[+] 文件已成功生成: " + WstrToUtf8(outputPath));
    return true;
}

} // namespace Patcher::Core
