#pragma once

#include <string>
#include <windows.h>
#include "PePatcher.h"

namespace Patcher::Core {

enum class UacLevel {
    KeepOriginal = 0,
    AsInvoker = 1,
    RequireAdministrator = 2,
    HighestAvailable = 3
};

enum class SubsystemType {
    KeepOriginal = 0,
    WindowsGui = 1,      // 隐藏控制台黑框 (GUI)
    Console = 2          // 控制台标准输出 (CUI)
};

class PeSecurity {
public:
    // 剥离 Authenticode 签名
    static bool RemoveSignature(const std::wstring& filePath, LogCallback logger = nullptr);

    // 修改/配置 UAC 权限清单
    static bool ModifyUacManifest(const std::wstring& filePath, UacLevel level, LogCallback logger = nullptr);

    // 转换 PE 子系统 (GUI vs Console)
    static bool ConvertSubsystem(const std::wstring& filePath, SubsystemType type, LogCallback logger = nullptr);

    // 抹除或随机化时间戳 (防取证)
    static bool WipeTimeDateStamp(const std::wstring& filePath, DWORD timestamp = 0, LogCallback logger = nullptr);
};

} // namespace Patcher::Core
