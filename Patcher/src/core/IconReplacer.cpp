#include "IconReplacer.h"
#include <fstream>
#include <vector>
#include <filesystem>
#include <format>

namespace Patcher::Core {

#pragma pack(push, 2)
struct ICONHEADER {
    WORD idReserved;
    WORD idType;
    WORD idCount;
};

struct ICONDIRENTRY {
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD wPlanes;
    WORD wBitCount;
    DWORD dwBytesInRes;
    DWORD dwImageOffset;
};

struct GRPICONDIRENTRY {
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD wPlanes;
    WORD wBitCount;
    DWORD dwBytesInRes;
    WORD nID;
};

struct GRPICONDIR {
    WORD idReserved;
    WORD idType;
    WORD idCount;
    GRPICONDIRENTRY idEntries[1];
};
#pragma pack(pop)

static void DoLog(LogCallback& logger, CUI::LogLevel level, const std::string& tag, const std::string& msg) {
    if (logger) {
        logger(level, tag, msg);
    }
}

bool IconReplacer::ApplyIcoToExe(const std::wstring& targetExePath, const std::wstring& icoPath, LogCallback logger) {
    std::ifstream file(icoPath, std::ios::binary);
    if (!file.is_open()) {
        DoLog(logger, CUI::LogLevel::Error, "Icon", "[-] 无法打开 .ico 文件: " + WstrToUtf8(icoPath));
        return false;
    }

    ICONHEADER header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (header.idReserved != 0 || header.idType != 1 || header.idCount == 0) {
        DoLog(logger, CUI::LogLevel::Error, "Icon", "[-] 无效的 .ico 文件头");
        return false;
    }

    std::vector<ICONDIRENTRY> entries(header.idCount);
    file.read(reinterpret_cast<char*>(entries.data()), header.idCount * sizeof(ICONDIRENTRY));

    // 构建 RT_GROUP_ICON 资源二进制
    size_t grpSize = sizeof(ICONHEADER) + header.idCount * sizeof(GRPICONDIRENTRY);
    std::vector<uint8_t> grpData(grpSize);

    auto* grpHeader = reinterpret_cast<ICONHEADER*>(grpData.data());
    grpHeader->idReserved = 0;
    grpHeader->idType = 1;
    grpHeader->idCount = header.idCount;

    auto* grpEntries = reinterpret_cast<GRPICONDIRENTRY*>(grpData.data() + sizeof(ICONHEADER));
    std::vector<std::vector<uint8_t>> iconImages(header.idCount);

    for (WORD i = 0; i < header.idCount; ++i) {
        grpEntries[i].bWidth = entries[i].bWidth;
        grpEntries[i].bHeight = entries[i].bHeight;
        grpEntries[i].bColorCount = entries[i].bColorCount;
        grpEntries[i].bReserved = entries[i].bReserved;
        grpEntries[i].wPlanes = entries[i].wPlanes;
        grpEntries[i].wBitCount = entries[i].wBitCount;
        grpEntries[i].dwBytesInRes = entries[i].dwBytesInRes;
        grpEntries[i].nID = i + 1; // 资源 ID 从 1 开始

        iconImages[i].resize(entries[i].dwBytesInRes);
        file.seekg(entries[i].dwImageOffset, std::ios::beg);
        file.read(reinterpret_cast<char*>(iconImages[i].data()), entries[i].dwBytesInRes);
    }

    // 开始更新目标 PE 资源
    HANDLE hUpdate = BeginUpdateResourceW(targetExePath.c_str(), FALSE);
    if (!hUpdate) {
        DoLog(logger, CUI::LogLevel::Error, "Icon", "[-] 无法启动可执行文件资源更新: " + WstrToUtf8(targetExePath));
        return false;
    }

    // 更新各个 RT_ICON 子项
    for (WORD i = 0; i < header.idCount; ++i) {
        if (!UpdateResourceW(
            hUpdate,
            RT_ICON,
            MAKEINTRESOURCEW(i + 1),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
            iconImages[i].data(),
            static_cast<DWORD>(iconImages[i].size())
        )) {
            DoLog(logger, CUI::LogLevel::Error, "Icon", std::format("[-] 写入 RT_ICON #{} 失败", i + 1));
            EndUpdateResourceW(hUpdate, TRUE);
            return false;
        }
    }

    // 更新 RT_GROUP_ICON 组
    if (!UpdateResourceW(
        hUpdate,
        RT_GROUP_ICON,
        MAKEINTRESOURCEW(1),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
        grpData.data(),
        static_cast<DWORD>(grpData.size())
    )) {
        DoLog(logger, CUI::LogLevel::Error, "Icon", "[-] 写入 RT_GROUP_ICON 失败");
        EndUpdateResourceW(hUpdate, TRUE);
        return false;
    }

    if (!EndUpdateResourceW(hUpdate, FALSE)) {
        DoLog(logger, CUI::LogLevel::Error, "Icon", "[-] 提交图标资源失败");
        return false;
    }

    DoLog(logger, CUI::LogLevel::Info, "Icon", std::format("[+] 图标已成功替换 (包含 {} 个尺寸子图)", header.idCount));
    return true;
}

bool IconReplacer::ExtractIconFromExe(const std::wstring& exePath, const std::wstring& outIcoPath, LogCallback logger) {
    DoLog(logger, CUI::LogLevel::Info, "Icon", "正在从可执行文件中提取图标资源: " + WstrToUtf8(exePath));

    HMODULE hModule = LoadLibraryExW(exePath.c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE);
    if (!hModule) {
        DoLog(logger, CUI::LogLevel::Error, "Icon", "[-] 无法加载源 PE 文件以提取图标");
        return false;
    }

    // 寻找 RT_GROUP_ICON
    HRSRC hRes = FindResourceW(hModule, MAKEINTRESOURCEW(1), RT_GROUP_ICON);
    if (!hRes) {
        // 尝试枚举或者使用 ID 128 (常见主图标 ID)
        hRes = FindResourceW(hModule, MAKEINTRESOURCEW(128), RT_GROUP_ICON);
    }

    if (!hRes) {
        DoLog(logger, CUI::LogLevel::Error, "Icon", "[-] 源文件中未找到 RT_GROUP_ICON 资源");
        FreeLibrary(hModule);
        return false;
    }

    HGLOBAL hGlobal = LoadResource(hModule, hRes);
    auto* grpDir = static_cast<const GRPICONDIR*>(LockResource(hGlobal));
    if (!grpDir || grpDir->idCount == 0) {
        FreeLibrary(hModule);
        return false;
    }

    std::ofstream outFile(outIcoPath, std::ios::binary);
    if (!outFile.is_open()) {
        FreeLibrary(hModule);
        return false;
    }

    ICONHEADER outHeader;
    outHeader.idReserved = 0;
    outHeader.idType = 1;
    outHeader.idCount = grpDir->idCount;
    outFile.write(reinterpret_cast<const char*>(&outHeader), sizeof(outHeader));

    DWORD offset = sizeof(ICONHEADER) + grpDir->idCount * sizeof(ICONDIRENTRY);
    std::vector<ICONDIRENTRY> outEntries(grpDir->idCount);

    for (WORD i = 0; i < grpDir->idCount; ++i) {
        const auto& ge = grpDir->idEntries[i];
        outEntries[i].bWidth = ge.bWidth;
        outEntries[i].bHeight = ge.bHeight;
        outEntries[i].bColorCount = ge.bColorCount;
        outEntries[i].bReserved = ge.bReserved;
        outEntries[i].wPlanes = ge.wPlanes;
        outEntries[i].wBitCount = ge.wBitCount;
        outEntries[i].dwBytesInRes = ge.dwBytesInRes;
        outEntries[i].dwImageOffset = offset;
        offset += ge.dwBytesInRes;
    }
    outFile.write(reinterpret_cast<const char*>(outEntries.data()), grpDir->idCount * sizeof(ICONDIRENTRY));

    for (WORD i = 0; i < grpDir->idCount; ++i) {
        HRSRC hIconRes = FindResourceW(hModule, MAKEINTRESOURCEW(grpDir->idEntries[i].nID), RT_ICON);
        if (hIconRes) {
            HGLOBAL hIconData = LoadResource(hModule, hIconRes);
            const void* pData = LockResource(hIconData);
            DWORD size = SizeofResource(hModule, hIconRes);
            outFile.write(reinterpret_cast<const char*>(pData), size);
        }
    }

    outFile.close();
    FreeLibrary(hModule);
    DoLog(logger, CUI::LogLevel::Info, "Icon", "[+] 图标提取成功保存至: " + WstrToUtf8(outIcoPath));
    return true;
}

bool IconReplacer::ReplaceIcon(const std::wstring& targetExePath, const std::wstring& iconOrExePath, LogCallback logger) {
    std::wstring ext = std::filesystem::path(iconOrExePath).extension().wstring();
    for (auto& c : ext) c = towlower(c);

    if (ext == L".ico") {
        return ApplyIcoToExe(targetExePath, iconOrExePath, logger);
    } else if (ext == L".exe") {
        std::wstring tempIco = targetExePath + L".tmp_icon.ico";
        if (ExtractIconFromExe(iconOrExePath, tempIco, logger)) {
            bool ok = ApplyIcoToExe(targetExePath, tempIco, logger);
            std::filesystem::remove(tempIco);
            return ok;
        }
        return false;
    } else {
        DoLog(logger, CUI::LogLevel::Error, "Icon", "[-] 不支持的图标文件格式 (需为 .ico 或包含图标的 .exe)");
        return false;
    }
}

} // namespace Patcher::Core
