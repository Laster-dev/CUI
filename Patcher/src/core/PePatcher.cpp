#include "PePatcher.h"
#include <fstream>
#include <format>
#include <filesystem>
#include <algorithm>
#include <cstring>

namespace Patcher::Core {

PePatcher::PePatcher(LogCallback logger)
    : m_logger(std::move(logger)) {
}

void PePatcher::Log(CUI::LogLevel level, const std::string& tag, const std::string& msg) {
    if (m_logger) {
        m_logger(level, tag, msg);
    }
}

namespace {

inline uint32_t AlignUp(uint32_t value, uint32_t alignment) {
    if (alignment == 0) return value;
    return (value + alignment - 1) / alignment * alignment;
}

// PE 内存视图：封装 32/64 位头部差异访问
struct PeMap {
    IMAGE_DOS_HEADER* dos = nullptr;
    IMAGE_FILE_HEADER* fileHeader = nullptr;
    uint8_t* optBase = nullptr;
    IMAGE_SECTION_HEADER* sections = nullptr;
    bool is64 = false;

    IMAGE_DATA_DIRECTORY* DataDir(int index) {
        return is64
            ? &reinterpret_cast<IMAGE_OPTIONAL_HEADER64*>(optBase)->DataDirectory[index]
            : &reinterpret_cast<IMAGE_OPTIONAL_HEADER32*>(optBase)->DataDirectory[index];
    }
    uint32_t SectionAlignment() const {
        return is64 ? reinterpret_cast<const IMAGE_OPTIONAL_HEADER64*>(optBase)->SectionAlignment
                    : reinterpret_cast<const IMAGE_OPTIONAL_HEADER32*>(optBase)->SectionAlignment;
    }
    uint32_t FileAlignment() const {
        return is64 ? reinterpret_cast<const IMAGE_OPTIONAL_HEADER64*>(optBase)->FileAlignment
                    : reinterpret_cast<const IMAGE_OPTIONAL_HEADER32*>(optBase)->FileAlignment;
    }
    uint32_t SizeOfImage() const {
        return is64 ? reinterpret_cast<const IMAGE_OPTIONAL_HEADER64*>(optBase)->SizeOfImage
                    : reinterpret_cast<const IMAGE_OPTIONAL_HEADER32*>(optBase)->SizeOfImage;
    }
    void SetSizeOfImage(uint32_t v) {
        if (is64) reinterpret_cast<IMAGE_OPTIONAL_HEADER64*>(optBase)->SizeOfImage = v;
        else reinterpret_cast<IMAGE_OPTIONAL_HEADER32*>(optBase)->SizeOfImage = v;
    }
    void SetEntryPoint(uint32_t rva) {
        if (is64) reinterpret_cast<IMAGE_OPTIONAL_HEADER64*>(optBase)->AddressOfEntryPoint = rva;
        else reinterpret_cast<IMAGE_OPTIONAL_HEADER32*>(optBase)->AddressOfEntryPoint = rva;
    }
    uint64_t ImageBase() const {
        return is64 ? reinterpret_cast<const IMAGE_OPTIONAL_HEADER64*>(optBase)->ImageBase
                    : reinterpret_cast<const IMAGE_OPTIONAL_HEADER32*>(optBase)->ImageBase;
    }
    WORD SectionCount() const { return fileHeader->NumberOfSections; }

    IMAGE_SECTION_HEADER* LastFileSection() { // 占据文件末尾的节
        IMAGE_SECTION_HEADER* last = sections;
        for (WORD i = 1; i < fileHeader->NumberOfSections; ++i) {
            auto* s = &sections[i];
            if (s->PointerToRawData + s->SizeOfRawData > last->PointerToRawData + last->SizeOfRawData) {
                last = s;
            }
        }
        return last;
    }
};

bool MapPeBuffer(std::vector<uint8_t>& buf, PeMap& m) {
    if (buf.size() < sizeof(IMAGE_DOS_HEADER)) return false;
    m.dos = reinterpret_cast<IMAGE_DOS_HEADER*>(buf.data());
    if (m.dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
    const size_t fhOff = static_cast<size_t>(m.dos->e_lfanew) + sizeof(DWORD);
    if (fhOff + sizeof(IMAGE_FILE_HEADER) > buf.size()) return false;
    m.fileHeader = reinterpret_cast<IMAGE_FILE_HEADER*>(buf.data() + fhOff);
    const size_t optOff = fhOff + sizeof(IMAGE_FILE_HEADER);
    if (optOff >= buf.size()) return false;
    m.optBase = buf.data() + optOff;
    const WORD magic = *reinterpret_cast<WORD*>(m.optBase);
    if (magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) m.is64 = true;
    else if (magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) return false;
    const size_t secTableEnd = optOff + m.fileHeader->SizeOfOptionalHeader +
                               sizeof(IMAGE_SECTION_HEADER) * m.fileHeader->NumberOfSections;
    if (secTableEnd > buf.size()) return false;
    m.sections = reinterpret_cast<IMAGE_SECTION_HEADER*>(buf.data() + optOff + m.fileHeader->SizeOfOptionalHeader);
    return true;
}

uint32_t RvaToOffset(const PeMap& m, const std::vector<uint8_t>& buf, uint32_t rva) {
    if (rva == 0) return 0;
    for (WORD i = 0; i < m.fileHeader->NumberOfSections; ++i) {
        const IMAGE_SECTION_HEADER* s = &m.sections[i];
        if (s->PointerToRawData == 0) continue;
        const uint32_t span = (std::max)(s->Misc.VirtualSize, s->SizeOfRawData);
        if (rva >= s->VirtualAddress && rva < s->VirtualAddress + span) {
            const size_t off = static_cast<size_t>(s->PointerToRawData) + (rva - s->VirtualAddress);
            return off < buf.size() ? static_cast<uint32_t>(off) : 0;
        }
    }
    return 0;
}

} // namespace

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

// 定位占据文件末尾的节并扩容，返回可安全写入的文件偏移与对应 RVA
bool PePatcher::ExpandLastSection(std::vector<uint8_t>& buf, size_t neededBytes,
                                  uint32_t& outAppendFileOffset, uint32_t& outAppendRVA, DWORD extraCharacteristics) {
    outAppendFileOffset = 0;
    outAppendRVA = 0;
    PeMap m;
    if (!MapPeBuffer(buf, m) || m.SectionCount() == 0) {
        Log(CUI::LogLevel::Error, "Patch", "PE 结构解析失败，无法扩容末尾节");
        return false;
    }
    IMAGE_SECTION_HEADER* last = m.LastFileSection();
    if (last->PointerToRawData == 0 || last->SizeOfRawData == 0) {
        Log(CUI::LogLevel::Error, "Patch", "末节没有原始数据，无法扩容");
        return false;
    }
    const uint32_t secAlign = m.SectionAlignment();
    const uint32_t fileAlign = m.FileAlignment();
    if (secAlign == 0 || fileAlign == 0) {
        Log(CUI::LogLevel::Error, "Patch", "无效的 PE 对齐参数");
        return false;
    }

    // 写入位置：旧原始数据末尾 (SizeOfRawData 已按 FileAlignment 对齐)
    outAppendFileOffset = last->PointerToRawData + last->SizeOfRawData;
    outAppendRVA = last->VirtualAddress + (outAppendFileOffset - last->PointerToRawData);

    const uint32_t oldRawSize = last->SizeOfRawData;
    const uint32_t oldVirtSize = last->Misc.VirtualSize;
    const uint32_t oldSizeOfImage = m.SizeOfImage();
    last->Misc.VirtualSize = AlignUp(oldVirtSize + static_cast<uint32_t>(neededBytes), secAlign);
    last->SizeOfRawData = AlignUp(oldRawSize + static_cast<uint32_t>(neededBytes), fileAlign);
    last->Characteristics |= extraCharacteristics;
    m.SetSizeOfImage((std::max)(oldSizeOfImage,
        AlignUp(last->VirtualAddress + last->Misc.VirtualSize, secAlign)));

    const size_t newFileSize = static_cast<size_t>(last->PointerToRawData) + last->SizeOfRawData;
    if (buf.size() < newFileSize) {
        buf.resize(newFileSize, 0);
    }
    Log(CUI::LogLevel::Info, "Patch", std::format(
        "已扩容末节: RawSize 0x{:X} -> 0x{:X}, VirtualSize 0x{:X} -> 0x{:X}, SizeOfImage 0x{:X} -> 0x{:X}",
        oldRawSize, last->SizeOfRawData, oldVirtSize, last->Misc.VirtualSize, oldSizeOfImage, m.SizeOfImage()));
    return true;
}

bool PePatcher::DetachSecurityDirectory(std::vector<uint8_t>& buf, std::vector<uint8_t>& certOut) {
    certOut.clear();
    PeMap m;
    if (!MapPeBuffer(buf, m)) {
        return true;
    }
    IMAGE_DATA_DIRECTORY* sec = m.DataDir(IMAGE_DIRECTORY_ENTRY_SECURITY);
    if (sec->VirtualAddress == 0 || sec->Size == 0) {
        return true;
    }
    const uint32_t certOff = sec->VirtualAddress;
    const uint32_t certSize = sec->Size;
    if (certOff >= buf.size() || certSize > buf.size() - certOff) {
        Log(CUI::LogLevel::Warn, "Patch", "签名 Overlay 超出文件范围，已忽略");
        sec->VirtualAddress = 0;
        sec->Size = 0;
        return true;
    }
    certOut.assign(buf.begin() + certOff, buf.begin() + certOff + certSize);
    std::memset(buf.data() + certOff, 0, certSize);
    sec->VirtualAddress = 0;
    sec->Size = 0;
    Log(CUI::LogLevel::Info, "Patch", std::format(
        "检测到签名 Overlay (偏移 0x{:X}, {} 字节)，已摘除并将在写出时挂回文件末尾", certOff, certSize));
    return true;
}

bool PePatcher::AttachSecurityDirectory(std::vector<uint8_t>& buf, const std::vector<uint8_t>& certIn) {
    if (certIn.empty()) {
        return true;
    }
    PeMap m;
    if (!MapPeBuffer(buf, m)) {
        Log(CUI::LogLevel::Warn, "Patch", "无法挂回签名 Overlay (PE 结构解析失败)");
        return true;
    }
    const uint32_t certOff = AlignUp(static_cast<uint32_t>(buf.size()), 8);
    buf.resize(static_cast<size_t>(certOff) + certIn.size(), 0);
    std::memcpy(buf.data() + certOff, certIn.data(), certIn.size());
    IMAGE_DATA_DIRECTORY* sec = m.DataDir(IMAGE_DIRECTORY_ENTRY_SECURITY);
    sec->VirtualAddress = certOff;
    sec->Size = static_cast<uint32_t>(certIn.size());
    Log(CUI::LogLevel::Info, "Patch", std::format(
        "签名 Overlay 已迁移至文件末尾 (偏移 0x{:X}, {} 字节)", certOff, certIn.size()));
    return true;
}

// 模式 1: 扩容末尾节并追加载荷，入口点指向载荷
bool PePatcher::InjectByEnlargeLastSection(std::vector<uint8_t>& buf, const std::vector<uint8_t>& payload) {
    uint32_t appendOff = 0;
    uint32_t appendRVA = 0;
    const DWORD chars = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
    if (!ExpandLastSection(buf, payload.size(), appendOff, appendRVA, chars)) {
        return false;
    }
    std::memcpy(buf.data() + appendOff, payload.data(), payload.size());
    PeMap m;
    if (!MapPeBuffer(buf, m)) {
        return false;
    }
    m.SetEntryPoint(appendRVA);
    Log(CUI::LogLevel::Success, "Patch", std::format(
        "载荷已追加至末节 (文件偏移 0x{:X}，RVA 0x{:X})，入口点已重定向至载荷", appendOff, appendRVA));
    return true;
}

// 模式 2: 追加新节头 + 末尾数据，入口点指向新节
bool PePatcher::InjectByNewSection(std::vector<uint8_t>& buf, const std::vector<uint8_t>& payload) {
    PeMap m;
    if (!MapPeBuffer(buf, m) || m.SectionCount() == 0) {
        Log(CUI::LogLevel::Error, "Patch", "PE 结构解析失败，无法新增节");
        return false;
    }
    const WORD nsec = m.SectionCount();
    if (nsec >= 96) {
        Log(CUI::LogLevel::Error, "Patch", "节数量已达上限，无法新增节");
        return false;
    }

    // 节表末尾与首个节实体之间必须有足够空间容纳新节头 (40 字节)
    uint32_t firstRaw = 0xFFFFFFFFu;
    for (WORD i = 0; i < nsec; ++i) {
        if (m.sections[i].PointerToRawData != 0) {
            firstRaw = (std::min)(firstRaw, m.sections[i].PointerToRawData);
        }
    }
    const size_t secTableEnd = reinterpret_cast<const uint8_t*>(&m.sections[nsec]) - buf.data();
    if (secTableEnd > buf.size()) {
        Log(CUI::LogLevel::Error, "Patch", "节表超出文件范围");
        return false;
    }
    const size_t freeSpace = (firstRaw == 0xFFFFFFFFu ? buf.size() : firstRaw) - secTableEnd;
    if (freeSpace < sizeof(IMAGE_SECTION_HEADER)) {
        Log(CUI::LogLevel::Error, "Patch", std::format(
            "节表后空闲空间不足 (需要 {} 字节，仅 {} 字节)，无法新增节头，可改用扩容末节模式",
            sizeof(IMAGE_SECTION_HEADER), freeSpace));
        return false;
    }

    const uint32_t secAlign = m.SectionAlignment();
    const uint32_t fileAlign = m.FileAlignment();
    uint32_t maxEndRVA = 0;
    for (WORD i = 0; i < nsec; ++i) {
        const IMAGE_SECTION_HEADER* s = &m.sections[i];
        maxEndRVA = (std::max)(maxEndRVA,
            AlignUp(s->VirtualAddress + (std::max)(s->Misc.VirtualSize, s->SizeOfRawData), secAlign));
    }

    IMAGE_SECTION_HEADER* ns = &m.sections[nsec];
    std::memset(ns, 0, sizeof(IMAGE_SECTION_HEADER));
    std::memcpy(ns->Name, ".patch", sizeof(".patch") - 1);
    ns->VirtualAddress = maxEndRVA;
    ns->Misc.VirtualSize = static_cast<uint32_t>(payload.size());
    ns->SizeOfRawData = AlignUp(static_cast<uint32_t>(payload.size()), fileAlign);
    ns->PointerToRawData = AlignUp(static_cast<uint32_t>(buf.size()), fileAlign);
    ns->Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;

    m.fileHeader->NumberOfSections = static_cast<WORD>(nsec + 1);
    m.SetSizeOfImage((std::max)(m.SizeOfImage(),
        AlignUp(ns->VirtualAddress + AlignUp(ns->Misc.VirtualSize, secAlign), secAlign)));

    const size_t newFileSize = static_cast<size_t>(ns->PointerToRawData) + ns->SizeOfRawData;
    buf.resize(newFileSize, 0);
    std::memcpy(buf.data() + ns->PointerToRawData, payload.data(), payload.size());
    m.SetEntryPoint(ns->VirtualAddress);
    Log(CUI::LogLevel::Success, "Patch", std::format(
        "新节 .patch 已建立 (RVA 0x{:X}，文件偏移 0x{:X}，共 {} 节)，入口点已重定向至新节",
        ns->VirtualAddress, ns->PointerToRawData, nsec + 1));
    return true;
}

// 模式 3: 构建 TLS 目录与回调数组，入口点保持不变
bool PePatcher::InjectByTlsCallback(std::vector<uint8_t>& buf, const std::vector<uint8_t>& payload) {
    PeMap m;
    if (!MapPeBuffer(buf, m)) {
        Log(CUI::LogLevel::Error, "Patch", "PE 结构解析失败，无法注入 TLS 回调");
        return false;
    }
    IMAGE_DATA_DIRECTORY* tlsDir = m.DataDir(IMAGE_DIRECTORY_ENTRY_TLS);
    if (tlsDir->VirtualAddress != 0) {
        Log(CUI::LogLevel::Error, "Patch", "白文件已存在 TLS 目录，暂不支持合并回调，请改用其他模式");
        return false;
    }
    const bool is64 = m.is64;
    const uint32_t ptrSize = is64 ? 8 : 4;
    const uint32_t tlsStructSize = is64 ? sizeof(IMAGE_TLS_DIRECTORY64) : sizeof(IMAGE_TLS_DIRECTORY32);
    // 布局: [TLS 目录][对齐填充][回调数组: 载荷指针 + 终止][TLS 索引变量][载荷代码]
    const size_t needed = AlignUp(tlsStructSize, 16) + 2 * ptrSize + ptrSize + payload.size() + 16;
    uint32_t baseOff = 0;
    uint32_t baseRVA = 0;
    const DWORD chars = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
    if (!ExpandLastSection(buf, needed, baseOff, baseRVA, chars)) {
        return false;
    }

    const uint32_t dirOff = baseOff;
    const uint32_t cbArrOff = dirOff + AlignUp(tlsStructSize, 16);
    const uint32_t indexOff = cbArrOff + 2 * ptrSize;
    const uint32_t payOff = indexOff + ptrSize;
    const uint32_t dirRVA = baseRVA + (dirOff - baseOff);
    const uint32_t cbArrRVA = baseRVA + (cbArrOff - baseOff);
    const uint32_t idxRVA = baseRVA + (indexOff - baseOff);
    const uint32_t payRVA = baseRVA + (payOff - baseOff);
    const uint64_t imageBase = m.ImageBase();

    if (is64) {
        auto* dir = reinterpret_cast<IMAGE_TLS_DIRECTORY64*>(buf.data() + dirOff);
        dir->StartAddressOfRawData = 0;
        dir->EndAddressOfRawData = 0;
        dir->AddressOfIndex = imageBase + idxRVA;
        dir->AddressOfCallBacks = imageBase + cbArrRVA;
        dir->SizeOfZeroFill = 0;
        dir->Characteristics = 0;
        auto* callbacks = reinterpret_cast<uint64_t*>(buf.data() + cbArrOff);
        callbacks[0] = imageBase + payRVA;
        callbacks[1] = 0;
    } else {
        auto* dir = reinterpret_cast<IMAGE_TLS_DIRECTORY32*>(buf.data() + dirOff);
        dir->StartAddressOfRawData = 0;
        dir->EndAddressOfRawData = 0;
        dir->AddressOfIndex = static_cast<DWORD>(imageBase + idxRVA);
        dir->AddressOfCallBacks = static_cast<DWORD>(imageBase + cbArrRVA);
        dir->SizeOfZeroFill = 0;
        dir->Characteristics = 0;
        auto* callbacks = reinterpret_cast<DWORD*>(buf.data() + cbArrOff);
        callbacks[0] = static_cast<DWORD>(imageBase + payRVA);
        callbacks[1] = 0;
    }
    std::memset(buf.data() + indexOff, 0, ptrSize);
    std::memcpy(buf.data() + payOff, payload.data(), payload.size());

    tlsDir->VirtualAddress = dirRVA;
    tlsDir->Size = tlsStructSize;
    Log(CUI::LogLevel::Success, "Patch", std::format(
        "TLS 回调已注册 (回调 RVA 0x{:X}，载荷 RVA 0x{:X})，入口点保持不变，载荷将在 OEP 之前执行",
        cbArrRVA, payRVA));
    return true;
}

// 模式 4: 追加导入描述符，让系统加载器在白文件启动时加载载荷 DLL
bool PePatcher::InjectByImportTable(std::vector<uint8_t>& buf, const std::wstring& payloadPath) {
    // 1. 读取载荷 DLL 并解析导出表，选取一个真实导出项
    std::vector<uint8_t> dll;
    if (!ReadBinaryFile(payloadPath, dll)) {
        Log(CUI::LogLevel::Error, "Import", "读取载荷 DLL 失败");
        return false;
    }
    PeMap dm;
    if (!MapPeBuffer(dll, dm)) {
        Log(CUI::LogLevel::Error, "Import", "载荷不是有效的 PE 文件 (导入表注入要求载荷为 DLL)");
        return false;
    }
    IMAGE_DATA_DIRECTORY* expDir = dm.DataDir(IMAGE_DIRECTORY_ENTRY_EXPORT);
    if (expDir->VirtualAddress == 0 || expDir->Size == 0) {
        Log(CUI::LogLevel::Error, "Import", "载荷 DLL 不包含导出表，无法生成导入项");
        return false;
    }
    const uint32_t expOff = RvaToOffset(dm, dll, expDir->VirtualAddress);
    if (expOff == 0 || expOff + sizeof(IMAGE_EXPORT_DIRECTORY) > dll.size()) {
        Log(CUI::LogLevel::Error, "Import", "载荷 DLL 导出表损坏");
        return false;
    }
    auto* exp = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(dll.data() + expOff);
    std::string funcName;
    uint32_t hint = 0;
    uint32_t ordinal = 0;
    bool byName = false;
    if (exp->NumberOfNames > 0) {
        const uint32_t namesOff = RvaToOffset(dm, dll, exp->AddressOfNames);
        if (namesOff == 0 || namesOff + sizeof(DWORD) > dll.size()) {
            Log(CUI::LogLevel::Error, "Import", "载荷 DLL 导出名称表损坏");
            return false;
        }
        const uint32_t nameRva = *reinterpret_cast<const DWORD*>(dll.data() + namesOff);
        const uint32_t ibnOff = RvaToOffset(dm, dll, nameRva);
        if (ibnOff == 0 || ibnOff + sizeof(IMAGE_IMPORT_BY_NAME) > dll.size()) {
            Log(CUI::LogLevel::Error, "Import", "载荷 DLL 导出名损坏");
            return false;
        }
        auto* ibn = reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(dll.data() + ibnOff);
        hint = ibn->Hint;
        funcName = reinterpret_cast<const char*>(ibn->Name);
        byName = true;
    } else if (exp->NumberOfFunctions > 0) {
        ordinal = exp->Base;
    } else {
        Log(CUI::LogLevel::Error, "Import", "载荷 DLL 没有任何导出函数，无法生成导入项");
        return false;
    }
    std::string dllName = WstrToUtf8(std::filesystem::path(payloadPath).filename().wstring());
    if (dllName.empty()) {
        dllName = "payload.dll";
    }

    // 2. 解析白文件现有导入描述符数量
    PeMap m;
    if (!MapPeBuffer(buf, m)) {
        Log(CUI::LogLevel::Error, "Import", "PE 结构解析失败，无法注入导入表");
        return false;
    }
    IMAGE_DATA_DIRECTORY* impDir = m.DataDir(IMAGE_DIRECTORY_ENTRY_IMPORT);
    uint32_t descCount = 0;
    if (impDir->VirtualAddress != 0 && impDir->Size >= sizeof(IMAGE_IMPORT_DESCRIPTOR)) {
        const uint32_t endRva = impDir->VirtualAddress + impDir->Size;
        for (uint32_t walkRva = impDir->VirtualAddress;
             walkRva + sizeof(IMAGE_IMPORT_DESCRIPTOR) <= endRva;
             walkRva += sizeof(IMAGE_IMPORT_DESCRIPTOR)) {
            const uint32_t dOff = RvaToOffset(m, buf, walkRva);
            if (dOff == 0 || dOff + sizeof(IMAGE_IMPORT_DESCRIPTOR) > buf.size()) break;
            auto* d = reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(buf.data() + dOff);
            if (d->Name == 0) break;
            ++descCount;
        }
        if (descCount == 0) {
            Log(CUI::LogLevel::Error, "Import", "现有导入表解析失败，无法注入");
            return false;
        }
    }

    // 3. 布局并扩容末节
    const bool is64 = m.is64;
    const uint32_t thunkSize = is64 ? 8 : 4;
    const size_t descBytes = (static_cast<size_t>(descCount) + 2) * sizeof(IMAGE_IMPORT_DESCRIPTOR);
    const size_t hintNameBytes = byName ? (2 + funcName.size() + 1) : 0;
    const size_t needed = descBytes + 4 * thunkSize + dllName.size() + 1 + hintNameBytes + 16;
    uint32_t baseOff = 0;
    uint32_t baseRVA = 0;
    const DWORD chars = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
    if (!ExpandLastSection(buf, needed, baseOff, baseRVA, chars)) {
        return false;
    }

    // 4. 依次布置: 描述符数组 / INT / FT / DLL 名 / Hint-Name
    const uint32_t descArrOff = baseOff;
    const uint32_t intArrOff = descArrOff + static_cast<uint32_t>(descBytes);
    const uint32_t ftArrOff = intArrOff + 2 * thunkSize;
    const uint32_t nameOff = ftArrOff + 2 * thunkSize;
    const uint32_t hintNameOff = AlignUp(nameOff + static_cast<uint32_t>(dllName.size()) + 1, 4);
    const uint32_t descArrRVA = baseRVA + (descArrOff - baseOff);
    const uint32_t intArrRVA = baseRVA + (intArrOff - baseOff);
    const uint32_t ftArrRVA = baseRVA + (ftArrOff - baseOff);
    const uint32_t nameRVA = baseRVA + (nameOff - baseOff);
    const uint32_t hintNameRVA = baseRVA + (hintNameOff - baseOff);

    // 5. 复制旧描述符，追加新描述符与全零终止符
    for (uint32_t i = 0; i < descCount; ++i) {
        const uint32_t srcOff = RvaToOffset(m, buf, impDir->VirtualAddress + i * sizeof(IMAGE_IMPORT_DESCRIPTOR));
        if (srcOff == 0) {
            Log(CUI::LogLevel::Error, "Import", "读取旧导入描述符失败");
            return false;
        }
        std::memcpy(buf.data() + descArrOff + i * sizeof(IMAGE_IMPORT_DESCRIPTOR),
                    buf.data() + srcOff, sizeof(IMAGE_IMPORT_DESCRIPTOR));
    }
    auto* newDesc = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(
        buf.data() + descArrOff + descCount * sizeof(IMAGE_IMPORT_DESCRIPTOR));
    std::memset(newDesc, 0, sizeof(IMAGE_IMPORT_DESCRIPTOR));
    newDesc->OriginalFirstThunk = intArrRVA;
    newDesc->TimeDateStamp = 0;
    newDesc->ForwarderChain = 0;
    newDesc->Name = nameRVA;
    newDesc->FirstThunk = ftArrRVA;
    std::memset(buf.data() + descArrOff + (descCount + 1) * sizeof(IMAGE_IMPORT_DESCRIPTOR),
                0, sizeof(IMAGE_IMPORT_DESCRIPTOR));

    // 6. INT/FT 各写一条 Thunk + 终止符
    const uint64_t thunkValue = byName
        ? static_cast<uint64_t>(hintNameRVA)
        : (is64 ? (IMAGE_ORDINAL_FLAG64 | ordinal) : (IMAGE_ORDINAL_FLAG32 | ordinal));
    for (int which = 0; which < 2; ++which) {
        uint8_t* arrBase = buf.data() + (which == 0 ? intArrOff : ftArrOff);
        if (is64) {
            auto* thunk = reinterpret_cast<uint64_t*>(arrBase);
            thunk[0] = thunkValue;
            thunk[1] = 0;
        } else {
            auto* thunk = reinterpret_cast<DWORD*>(arrBase);
            thunk[0] = static_cast<DWORD>(thunkValue);
            thunk[1] = 0;
        }
    }

    // 7. DLL 名称与 Hint/Name
    std::memcpy(buf.data() + nameOff, dllName.c_str(), dllName.size() + 1);
    if (byName) {
        auto* ibn = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(buf.data() + hintNameOff);
        ibn->Hint = static_cast<WORD>(hint);
        std::memcpy(ibn->Name, funcName.c_str(), funcName.size() + 1);
    }

    // 8. 重定向导入目录
    impDir->VirtualAddress = descArrRVA;
    impDir->Size = static_cast<DWORD>(descBytes);

    Log(CUI::LogLevel::Success, "Import", std::format(
        "导入表已注入: {} ! {} ({})，白文件启动时将由系统加载器自动加载载荷 DLL",
        dllName, byName ? funcName : ("Ordinal#" + std::to_string(ordinal)), is64 ? "x64" : "x86"));
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
        Log(CUI::LogLevel::Error, "Patch", "读取目标白文件失败");
        return false;
    }

    PeFileInfo whiteInfo;
    if (!InspectPe(whitePePath, whiteInfo)) {
        Log(CUI::LogLevel::Error, "Patch", "目标白文件 PE 结构校验失败");
        return false;
    }

    Log(CUI::LogLevel::Info, "PE", std::format(
        "白文件架构: {}, ImageBase: 0x{:X}, EntryPoint RVA: 0x{:X} (VA: 0x{:X})",
        whiteInfo.is64Bit ? "x64 (64-bit)" : "x86 (32-bit)",
        whiteInfo.imageBase, whiteInfo.entryPointRva, whiteInfo.entryPointVa
    ));

    // 2. 准备载荷数据（导入表注入不嵌入载荷，仅引用其导出函数）
    std::vector<uint8_t> payloadData;
    if (mode != PatchMode::ImportInjection) {
        std::wstring ext = std::filesystem::path(payloadPath).extension().wstring();
        for (auto& c : ext) c = towlower(c);

        if (ext == L".exe" || ext == L".dll") {
            Log(CUI::LogLevel::Info, "Payload", "输入载荷为可执行文件，正在解析并提取其 .text 代码段...");
            if (!ExtractTextSection(payloadPath, payloadData)) {
                Log(CUI::LogLevel::Error, "Payload", "提取载荷代码段失败");
                return false;
            }
        } else {
            Log(CUI::LogLevel::Info, "Payload", "输入载荷为原始二进制数据 (.bin / .text)...");
            if (!ReadBinaryFile(payloadPath, payloadData)) {
                Log(CUI::LogLevel::Error, "Payload", "读取载荷二进制数据失败");
                return false;
            }
        }

        if (payloadData.empty()) {
            Log(CUI::LogLevel::Error, "Payload", "载荷数据为空，终止操作");
            return false;
        }
        Log(CUI::LogLevel::Info, "Payload", std::format("载荷大小: {} 字节", payloadData.size()));
    } else {
        Log(CUI::LogLevel::Info, "Patch", "模式: 导入表注入，载荷 DLL 不会被嵌入，需随主程序一同分发");
    }

    // 3. 计算注入写入位置
    size_t targetOffset = 0;
    if (mode == PatchMode::ReplaceTextSection) {
        if (whiteInfo.textSectionOffset == 0) {
            Log(CUI::LogLevel::Error, "Patch", "白文件中不存在 .text 节，无法执行代码段替换");
            return false;
        }
        targetOffset = whiteInfo.textSectionOffset;
        Log(CUI::LogLevel::Info, "Patch", std::format("模式: .text 代码段覆盖，目标文件偏移: 0x{:X}，白文件段容积: {} 字节",
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
            Log(CUI::LogLevel::Error, "Patch", "无法将 EntryPoint RVA 映射到文件偏移");
            return false;
        }
        Log(CUI::LogLevel::Info, "Patch", std::format("模式: 入口点注入，EntryPoint 文件偏移: 0x{:X}", targetOffset));
    }

    // 4. 拷贝覆盖数据
    if (targetOffset + payloadData.size() > whiteBuffer.size()) {
        // 扩容白文件缓冲区
        whiteBuffer.resize(targetOffset + payloadData.size());
    }
    std::memcpy(whiteBuffer.data() + targetOffset, payloadData.data(), payloadData.size());
    Log(CUI::LogLevel::Info, "Patch", std::format("数据注入成功，共写入 {} 字节", payloadData.size()));

    // 5. 写出到目标文件
    if (!WriteBinaryFile(outputPath, whiteBuffer)) {
        Log(CUI::LogLevel::Error, "Patch", "输出修补后文件失败: " + WstrToUtf8(outputPath));
        return false;
    }

    Log(CUI::LogLevel::Info, "Patch", "文件已成功生成: " + WstrToUtf8(outputPath));
    return true;
}

} // namespace Patcher::Core
