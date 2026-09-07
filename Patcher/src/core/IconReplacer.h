#pragma once

#include <string>
#include <windows.h>
#include "PePatcher.h"

namespace Patcher::Core {

class IconReplacer {
public:
    // 将指定 .ico 或从 .exe 提取的图标应用到目标 PE
    static bool ReplaceIcon(
        const std::wstring& targetExePath,
        const std::wstring& iconOrExePath,
        LogCallback logger = nullptr
    );

    // 从 .exe 中提取图标并保存为 .ico 文件
    static bool ExtractIconFromExe(
        const std::wstring& exePath,
        const std::wstring& outIcoPath,
        LogCallback logger = nullptr
    );

private:
    static bool ApplyIcoToExe(
        const std::wstring& targetExePath,
        const std::wstring& icoPath,
        LogCallback logger = nullptr
    );
};

} // namespace Patcher::Core
