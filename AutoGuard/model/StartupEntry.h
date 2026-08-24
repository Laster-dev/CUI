#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace AutoGuard {

enum class StartupCategory {
    All = 0,
    Logon,
    Explorer,
    InternetExplorer,
    ScheduledTasks,
    Services,
    Drivers,
    Codecs,
    BootExecute,
    ImageHijacks,
    AppInit,
    KnownDlls,
    Winlogon,
    Winsock,
    PrintMonitors,
    Lsa,
    NetworkProviders,
    Wmi,
    Office
};

enum class StartupLocation {
    RegistryRun,
    StartupFolder,
    ScheduledTask,
    Service,
    Driver,
    ImageHijack,
    WmiSubscription,
    Winlogon,
    ActiveSetup,
    ComHijack,
    AppInit,
    KnownDlls,
    BootExecute,
    WinsockProvider,
    PrintMonitor,
    LsaPackage,
    Unknown
};

enum class StartupStatus {
    Enabled,
    Disabled,
    Missing,
    Protected,
    Unknown
};

enum class RiskLevel {
    Safe,
    Notice,
    Suspicious,
    High
};

enum class SignatureStatus {
    Valid,
    Invalid,
    Unsigned,
    Unavailable,
    NotApplicable
};

struct StartupEntry {
    std::string id;
    std::string name;           // 条目名称（例如 Docker Desktop）
    std::string description;    // 文件描述（例如 Docker Desktop Launcher）
    StartupCategory category = StartupCategory::Logon;
    StartupLocation location = StartupLocation::Unknown;
    std::string scope;          // "当前用户" / "本机系统"
    std::string command;        // 完整启动命令行
    std::string executablePath; // 解析出的可执行文件真实路径
    std::string arguments;      // 参数
    std::string source;         // 注册表键路径 / 文件路径 / 任务路径
    std::string registryKey;    // 如果是注册表：键名（例如 Run）
    std::string registryValue;  // 如果是注册表：项名
    bool isHklm = false;        // 是否是 HKLM
    StartupStatus status = StartupStatus::Enabled;
    RiskLevel risk = RiskLevel::Notice;
    SignatureStatus signature = SignatureStatus::Unavailable;
    std::string publisher;      // 发布者 / 厂商 (例如 (Verified) Microsoft Corporation)
    std::string riskReason;     // 风险分析理由
    std::string fileSizeStr;    // 文件大小 (例如 4,809 K)
    std::string fileVersion;    // 文件版本 (例如 152.0.4191.34)
    std::string fileTimestamp;  // 文件时间 (例如 2026/8/17 6:58)
    bool fileExists = true;     // 文件是否存在
    bool canDisable = true;
    bool canDelete = true;

    // 分组支持
    bool isGroupHeader = false; // 是否是分组标题行（如 HKCU\...\Run）
    std::string groupTitle;     // 分组标题
    std::string groupIcon;      // 分组图标
};

struct ScanSummary {
    std::vector<StartupEntry> entries;
    size_t enabledCount = 0;
    size_t disabledCount = 0;
    size_t missingCount = 0;
    size_t suspiciousCount = 0;
    size_t highRiskCount = 0;
    size_t safeCount = 0;
    size_t failedSources = 0;
    std::string scanTimestamp;
};

inline const char* CategoryName(StartupCategory cat) {
    switch (cat) {
    case StartupCategory::All: return "所有";
    case StartupCategory::Logon: return "登陆";
    case StartupCategory::Explorer: return "资源管理器";
    case StartupCategory::InternetExplorer: return "IE 浏览器";
    case StartupCategory::ScheduledTasks: return "计划任务";
    case StartupCategory::Services: return "服务";
    case StartupCategory::Drivers: return "驱动";
    case StartupCategory::Codecs: return "编解码器";
    case StartupCategory::BootExecute: return "引导执行";
    case StartupCategory::ImageHijacks: return "镜像劫持";
    case StartupCategory::AppInit: return "应用初始化";
    case StartupCategory::KnownDlls: return "已知 DLL";
    case StartupCategory::Winlogon: return "Win 登陆";
    case StartupCategory::Winsock: return "Winsock 提供商";
    case StartupCategory::PrintMonitors: return "打印监视器";
    case StartupCategory::Lsa: return "本地安全认证";
    case StartupCategory::NetworkProviders: return "网络提供商";
    case StartupCategory::Wmi: return "WMI";
    case StartupCategory::Office: return "Office";
    default: return "其他";
    }
}

inline const char* CategoryIcon(StartupCategory cat) {
    switch (cat) {
    case StartupCategory::All: return "🗂";
    case StartupCategory::Logon: return "👤";
    case StartupCategory::Explorer: return "📁";
    case StartupCategory::InternetExplorer: return "🌐";
    case StartupCategory::ScheduledTasks: return "◷";
    case StartupCategory::Services: return "⚙";
    case StartupCategory::Drivers: return "🖹";
    case StartupCategory::Codecs: return "🎬";
    case StartupCategory::BootExecute: return "🚀";
    case StartupCategory::ImageHijacks: return "⚠";
    case StartupCategory::AppInit: return "🧩";
    case StartupCategory::KnownDlls: return "📦";
    case StartupCategory::Winlogon: return "🔑";
    case StartupCategory::Winsock: return "🔌";
    case StartupCategory::PrintMonitors: return "🖨";
    case StartupCategory::Lsa: return "🛡";
    case StartupCategory::NetworkProviders: return "🌐";
    case StartupCategory::Wmi: return "◈";
    case StartupCategory::Office: return "📑";
    default: return "📄";
    }
}

inline const char* LocationName(StartupLocation value) {
    switch (value) {
    case StartupLocation::RegistryRun: return "注册表 Run";
    case StartupLocation::StartupFolder: return "启动文件夹";
    case StartupLocation::ScheduledTask: return "计划任务";
    case StartupLocation::Service: return "系统服务";
    case StartupLocation::Driver: return "驱动服务";
    case StartupLocation::ImageHijack: return "镜像劫持 (IFEO)";
    case StartupLocation::WmiSubscription: return "WMI 订阅";
    case StartupLocation::Winlogon: return "Winlogon 挂钩";
    case StartupLocation::ActiveSetup: return "Active Setup";
    case StartupLocation::ComHijack: return "COM 劫持与扩展";
    case StartupLocation::AppInit: return "AppInit DLLs";
    case StartupLocation::KnownDlls: return "KnownDLLs";
    case StartupLocation::BootExecute: return "BootExecute";
    case StartupLocation::WinsockProvider: return "Winsock Provider";
    case StartupLocation::PrintMonitor: return "Print Monitor";
    case StartupLocation::LsaPackage: return "LSA Package";
    default: return "其他入口";
    }
}

inline const char* LocationIcon(StartupLocation value) {
    switch (value) {
    case StartupLocation::RegistryRun: return "▤";
    case StartupLocation::StartupFolder: return "📁";
    case StartupLocation::ScheduledTask: return "◷";
    case StartupLocation::Service: return "⚙";
    case StartupLocation::Driver: return "🖹";
    case StartupLocation::ImageHijack: return "⚠";
    case StartupLocation::WmiSubscription: return "◈";
    case StartupLocation::Winlogon: return "◆";
    case StartupLocation::ActiveSetup: return "◇";
    case StartupLocation::ComHijack: return "◉";
    case StartupLocation::AppInit: return "🧩";
    case StartupLocation::KnownDlls: return "📦";
    case StartupLocation::BootExecute: return "🚀";
    case StartupLocation::WinsockProvider: return "🔌";
    case StartupLocation::PrintMonitor: return "🖨";
    case StartupLocation::LsaPackage: return "🛡";
    default: return "📄";
    }
}

inline const char* StatusName(StartupStatus value) {
    switch (value) {
    case StartupStatus::Enabled: return "已启用";
    case StartupStatus::Disabled: return "已禁用";
    case StartupStatus::Missing: return "目标失效";
    case StartupStatus::Protected: return "系统保护";
    default: return "未知";
    }
}

inline const char* RiskName(RiskLevel value) {
    switch (value) {
    case RiskLevel::Safe: return "安全";
    case RiskLevel::Notice: return "提示";
    case RiskLevel::Suspicious: return "可疑";
    case RiskLevel::High: return "高风险";
    default: return "未知";
    }
}

inline const char* SignatureName(SignatureStatus value) {
    switch (value) {
    case SignatureStatus::Valid: return "(Verified)";
    case SignatureStatus::Invalid: return "(Invalid)";
    case SignatureStatus::Unsigned: return "(Not verified)";
    case SignatureStatus::NotApplicable: return "";
    default: return "(Unknown)";
    }
}

} // namespace AutoGuard
