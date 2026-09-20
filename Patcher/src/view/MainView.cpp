#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "MainView.h"
#include "../core/PeSecurity.h"
#include "../core/IconReplacer.h"
#include "framework/controls/WindowTitleBar.h"
#include "framework/controls/Panel.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBlock.h"
#include "framework/core/Widgets.h"
#include "framework/style/ThemeTokenId.h"

#include <shellapi.h>
#include <filesystem>
#include <format>
#include <algorithm>
#include <cmath>

using namespace CUI;
using namespace CUI::DSL;

namespace Patcher::View {

static std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (size <= 1) return L"";
    std::wstring result(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, result.data(), size);
    return result;
}

static std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) return "";
    std::string result(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, result.data(), size, nullptr, nullptr);
    return result;
}

MainView::MainView(CUI::Window* window)
    : m_window(window) {
    m_pePatcher = std::make_unique<Core::PePatcher>();
}

namespace {

// SVG 图标（纯矢量，无 emoji，不依赖文字）
constexpr const char* kSvgPlay =
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M256 160l576 352-576 352V160z\"/>"
    "</svg>";

constexpr const char* kSvgFolderOpen =
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M880 320H480l-64-64H144a48 48 0 0 0-48 48v512a48 48 0 0 0 48 48h736a48 48 0 0 0 48-48V368a48 48 0 0 0-48-48z m-32 480H160V384h704v416z\"/>"
    "</svg>";

constexpr const char* kSvgReset =
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M512 128a384 384 0 1 0 355.2 240H768A288 288 0 1 1 512 224c80 0 152 32 203.2 84.8L624 400h288V112l-104 104A381.76 381.76 0 0 0 512 128z\"/>"
    "</svg>";

} // namespace

std::shared_ptr<UIElement> MainView::Build() {
    auto titleBar = BuildHeader();
    auto filesArea = BuildFilesArea();
    auto optionsArea = BuildOptionsArea();
    auto actionsArea = BuildActionsArea();

    // 日志框初始化：默认展开，直接贴底放，统一使用常驻边框色
    m_logView = Widgets::LogView().Shared();
    m_logView->SetExpanded(true);
    m_logView->SetCornerRadius(0.0f);
    m_logView->SetAlign(Alignment::Stretch);


    // 连接核心日志引擎
    m_pePatcher->SetLogger([this](LogLevel level, const std::string& tag, const std::string& message) {
        if (m_logView) {
            m_logView->Append(level, tag, message);
        }
    });

    m_logView->Append(LogLevel::Info, "System", "程序初始化完成。");

    // 禁止日志栏折叠：用户点击 Header 时强制恢复展开状态
    m_logView->OnExpandedChanged().Connect([](LogView* lv) {
        if (!lv->IsExpanded()) {
            lv->SetExpanded(true);
        }
    });

    // 上部操作与配置区域：无大间距、无嵌套容器、全面左对齐
    auto topArea = Column(8.0f, {
        filesArea,
        optionsArea,
        actionsArea
    })
    .Padding(12.0f, 8.0f, 12.0f, 6.0f)
    .Align(Alignment::Stretch)
    .FlexGrow(1.0f)
    .Build();

    // 贴底布局：标题栏 + 上半部自适应拉伸区域 + 贴底日志框（自身不包 Expanded，紧贴底部）
    auto root = Column(0.0f, {
        titleBar,
        topArea,
        m_logView
    })
    .BackgroundToken(ThemeTokenId::WindowBackground)
    .Align(Alignment::Stretch)
    .Build();

    return root;
}

std::shared_ptr<UIElement> MainView::BuildHeader() {
    auto titleBar = Widgets::WindowTitleBar()
        .Title("PE Patch 工具")
        .Height(32.0f)
        .Shared();

    return titleBar;
}

std::shared_ptr<UIElement> MainView::BuildFilesArea() {
    // 1. 白名单目标文件：占满除左边文本外的整行
    m_fpWhite = Widgets::FilePicker()
        .DialogTitle("选择白名单目标文件")
        .Filter("可执行文件 (*.exe;*.dll)", "*.exe;*.dll")
        .FlexGrow(1.0f)
        .Height(28.0f)
        .Placeholder("选择白文件路径 (.exe / .dll)")
        .Shared();
    m_fpWhite->AddFilter("所有文件 (*.*)", "*.*");

    // 2. 注入载荷文件：占满除左边文本外的整行
    m_fpPayload = Widgets::FilePicker()
        .DialogTitle("选择注入载荷文件")
        .Filter("载荷文件 (*.exe;*.dll;*.text;*.bin)", "*.exe;*.dll;*.text;*.bin")
        .FlexGrow(1.0f)
        .Height(28.0f)
        .Placeholder("选择载荷文件路径 (.exe / .dll / .text / .bin)")
        .Shared();
    m_fpPayload->AddFilter("所有文件 (*.*)", "*.*");

    m_fpWhite->OnPathChanged().Connect([this](FilePicker*, const std::string&) {
        UpdateModeAvailability();
    });

    m_fpPayload->OnPathChanged().Connect([this](FilePicker*, const std::string&) {
        UpdateModeAvailability();
    });

    auto makeRow = [](const std::string& label, std::shared_ptr<UIElement> picker) {
        return Row(6.0f, {
            Text(label)
                .FontSize(12.0f)
                .Width(76.0f)
                .AlignHorizontal(Alignment::Start)
                .AlignVertical(Alignment::Center)
                .ForegroundToken(ThemeTokenId::TextPrimary),
            picker
        })
        .Align(Alignment::Stretch)
        .FlexGrow(1.0f);
    };

    auto panel = Column(4.0f, {
        makeRow("白文件:", m_fpWhite),
        makeRow("载荷文件:", m_fpPayload)
    })
    .Align(Alignment::Stretch)
    .Build();

    return panel;
}

std::shared_ptr<UIElement> MainView::BuildOptionsArea() {
    // 0. 目标类型: 自动识别 / 强制 EXE / 强制 DLL（默认自动，按白文件 PE 特征识别）
    m_segTargetType = Widgets::SegmentedControl()
        .Height(26.0f)
        .Width(280.0f)
        .Shared();
    m_segTargetType->AddItem("自动");
    m_segTargetType->AddItem("EXE");
    m_segTargetType->AddItem("DLL");
    m_segTargetType->SetSelectedIndex(0);

    // 1. Patch 模式（EXE / DLL 目标通用，不支持的按可用性变灰）
    m_segPatchMode = Widgets::SegmentedControl()
        .Height(26.0f)
        .Width(480.0f)
        .Shared();
    m_segPatchMode->AddItem(".text覆盖");
    m_segPatchMode->AddItem("OEP覆盖");
    m_segPatchMode->AddItem("末节扩容");
    m_segPatchMode->AddItem("新增节区");
    m_segPatchMode->AddItem("TLS回调");
    m_segPatchMode->AddItem("导入表注入");
    m_segPatchMode->SetSelectedIndex(0);

    // 2. 子系统类型
    m_segSubsystem = Widgets::SegmentedControl()
        .Height(26.0f)
        .Width(280.0f)
        .Shared();
    m_segSubsystem->AddItem("KEEP");
    m_segSubsystem->AddItem("GUI");
    m_segSubsystem->AddItem("CUI");
    m_segSubsystem->SetSelectedIndex(0);

    // 3. UAC 权限清单
    m_segUac = Widgets::SegmentedControl()
        .Height(26.0f)
        .Width(280.0f)
        .Shared();
    m_segUac->AddItem("KEEP");
    m_segUac->AddItem("USER");
    m_segUac->AddItem("ADMIN");
    m_segUac->SetSelectedIndex(0);

    // 4. 开关
    m_swRemoveSig = ToggleSwitchTile("剥离数字签名", true).Build();
    m_swWipeTimestamp = ToggleSwitchTile("抹除时间戳", true).Build();
    m_swDisableCfg = ToggleSwitchTile("禁用CFG", true).Build();

    // 5. 覆盖函数输入框: 与目标类型合并在同一行右侧，仅 DLL 目标时显示（自动识别或手动选择 DLL）
    m_asbDllFunc = Widgets::AutoSuggestBox()
        .Placeholder("要覆盖的导出函数 (默认 DLLMain)")
        .Width(280.0f)
        .Height(24.0f)
        .Shared();
    m_asbDllFunc->SetText("DLLMain");
    m_asbDllFunc->SetSuggestionItems({ "DLLMain" });
    m_asbDllFunc->SetMaxVisibleSuggestions(12);

    auto makeOptionRow = [](const std::string& title, std::shared_ptr<UIElement> ctrl) {
        return Row(6.0f, {
            Text(title)
                .FontSize(12.0f)
                .Width(76.0f)
                .AlignHorizontal(Alignment::Start)
                .AlignVertical(Alignment::Center)
                .ForegroundToken(ThemeTokenId::TextPrimary),
            ctrl
        }).Align(Alignment::Start);
    };

    // 目标类型行: [目标类型: 自动/EXE/DLL] [覆盖函数: 输入框(仅 DLL 时可见)]
    m_dllFuncArea = Row(6.0f, {
        Text("覆盖函数:")
            .FontSize(12.0f)
            .AlignHorizontal(Alignment::Start)
            .AlignVertical(Alignment::Center)
            .ForegroundToken(ThemeTokenId::TextPrimary),
        m_asbDllFunc
    }).Align(Alignment::Start).Build();
    m_dllFuncArea->SetVisibility(Visibility::Collapsed);

    m_targetTypeRow = Row(12.0f, {
        Text("目标类型:")
            .FontSize(12.0f)
            .Width(76.0f)
            .AlignHorizontal(Alignment::Start)
            .AlignVertical(Alignment::Center)
            .ForegroundToken(ThemeTokenId::TextPrimary),
        m_segTargetType,
        m_dllFuncArea
    }).Align(Alignment::Start).Build();

    // 目标类型切换（含自动识别变化）: 即时刷新界面呈现
    m_segTargetType->OnSelectionChanged().Connect([this](SegmentedControl*, int, const std::string&) {
        UpdateModeAvailability();
    });

    auto panel = Column(4.0f, {
        m_targetTypeRow,
        makeOptionRow("Patch模式:", m_segPatchMode),
        makeOptionRow("PE子系统:", m_segSubsystem),
        makeOptionRow("UAC权限:", m_segUac),
        Row(12.0f, {
            Text("选项设置:")
                .FontSize(12.0f)
                .Width(76.0f)
                .AlignHorizontal(Alignment::Start)
                .AlignVertical(Alignment::Center)
                .ForegroundToken(ThemeTokenId::TextPrimary),
            m_swRemoveSig,
            m_swWipeTimestamp,
            m_swDisableCfg
        }).Align(Alignment::Start).Margin(0.0f, 2.0f, 0.0f, 2.0f)
    })
    .Align(Alignment::Start)
    .Build();

    return panel;
}

std::shared_ptr<UIElement> MainView::BuildActionsArea() {
    // 圆形纯图标按钮（36x36，圆角 18，内部居中绘制 SVG 图标，无文字）
    auto btnRun = Widgets::Button("")
        .Icon(kSvgPlay)
        .ToolTip("开始执行")
        .BackgroundToken(ThemeTokenId::AccentColor)
        .ForegroundToken(ThemeTokenId::AccentForeground)
        .Height(36.0f)
        .Width(36.0f)
        .CornerRadius(18.0f)
        .Padding(0.0f)
        .FontSize(18.0f)
        .OnClick([this](UIElement*) { RunPatch(); }).Shared();

    auto btnOpenDir = Widgets::Button("")
        .Icon(kSvgFolderOpen)
        .ToolTip("打开输出目录")
        .Height(36.0f)
        .Width(36.0f)
        .CornerRadius(18.0f)
        .Padding(0.0f)
        .FontSize(18.0f)
        .OnClick([this](UIElement*) { OpenOutputDir(); }).Shared();

    auto btnReset = Widgets::Button("")
        .Icon(kSvgReset)
        .ToolTip("重置")
        .Height(36.0f)
        .Width(36.0f)
        .CornerRadius(18.0f)
        .Padding(0.0f)
        .FontSize(18.0f)
        .OnClick([this](UIElement*) { ResetAll(); }).Shared();

    auto actions = Row(10.0f, {
        btnRun,
        btnOpenDir,
        btnReset
    })
    .Align(Alignment::Start)
    .Build();

    return actions;
}

void MainView::ResetAll() {
    if (m_fpWhite) m_fpWhite->SetPath("");
    if (m_fpPayload) m_fpPayload->SetPath("");
    m_lastOutputPath.clear();
    if (m_segTargetType) m_segTargetType->SetSelectedIndex(0); // 目标类型回“自动”
    if (m_segPatchMode) {
        for (int i = 0; i < 6; ++i) {
            m_segPatchMode->SetItemEnabled(i, true);
        }
    }
    if (m_asbDllFunc) {
        m_asbDllFunc->SetText("DLLMain");
        m_asbDllFunc->SetSuggestionItems({ "DLLMain" });
    }
    if (m_swDisableCfg) {
        m_swDisableCfg->SetIsOn(true);
    }
    m_lastTargetDll = false;
    if (m_logView) m_logView->Clear();
    if (m_logView) {
        m_logView->Append(LogLevel::Info, "System", "已重置输入与日志");
    }
}

void MainView::UpdateModeAvailability() {
    if (!m_segPatchMode) return;

    std::string whiteStr = m_fpWhite ? m_fpWhite->GetPath() : "";
    std::string payloadStr = m_fpPayload ? m_fpPayload->GetPath() : "";

    // 白文件变化时同步刷新 DLL 导出建议列表（AutoSuggestBox）
    RefreshDllFunctionSuggestions();

    const bool dllTarget = IsDllTarget();

    // DLL 目标: 目标类型行右侧显示“覆盖函数”输入框；EXE 目标隐藏
    if (m_dllFuncArea) m_dllFuncArea->SetVisibility(dllTarget ? Visibility::Visible : Visibility::Collapsed);

    if (dllTarget != m_lastTargetDll) {
        m_lastTargetDll = dllTarget;
        m_logView->Append(LogLevel::Info, "Target", dllTarget
            ? "目标类型: DLL，Patch 方式为覆盖指定函数；模式列表中 .text覆盖 / OEP覆盖 不适用于 DLL，已变灰"
            : "目标类型: EXE，使用 Patch 模式列表中的注入方式");
    }

    // DLL 目标: .text覆盖 / OEP覆盖 不适用（DLL 的代码 patch 通过“覆盖指定函数”完成），始终变灰
    if (whiteStr.empty() || payloadStr.empty()) {
        m_segPatchMode->SetItemEnabled(0, !dllTarget);
        m_segPatchMode->SetItemEnabled(1, !dllTarget);
        for (int i = 2; i < 6; ++i) {
            m_segPatchMode->SetItemEnabled(i, true);
        }
        return;
    }

    std::wstring whitePath = Utf8ToWide(whiteStr);
    std::wstring payloadPath = Utf8ToWide(payloadStr);

    if (!std::filesystem::exists(whitePath) || !std::filesystem::exists(payloadPath)) {
        return;
    }

    Core::PatchModeAvailability avail;
    if (!m_pePatcher->EvaluatePatchModes(whitePath, payloadPath, avail)) {
        return;
    }

    // 0: .text覆盖  1: OEP覆盖  2: 末节扩容  3: 新增节区  4: TLS回调  5: 导入表注入
    m_segPatchMode->SetItemEnabled(0, !dllTarget && avail.canReplaceText);
    m_segPatchMode->SetItemEnabled(1, !dllTarget && avail.canInjectOep);
    m_segPatchMode->SetItemEnabled(2, avail.canEnlargeLastSection);
    m_segPatchMode->SetItemEnabled(3, avail.canAddNewSection);
    m_segPatchMode->SetItemEnabled(4, avail.canTlsCallback);
    m_segPatchMode->SetItemEnabled(5, avail.canImportInjection);

    // 在日志栏给出明确的容量评估提示
    std::string infoMsg = "载荷大小: " + std::to_string(avail.payloadSize) + " 字节 (";
    char sizeBuf[64];
    snprintf(sizeBuf, sizeof(sizeBuf), "%.2f KB", avail.payloadSize / 1024.0f);
    infoMsg += sizeBuf;
    infoMsg += ") | .text容量: " + std::to_string(avail.textSectionCapacity) + " 字节";
    infoMsg += " | OEP余量: " + std::to_string(avail.oepRemainingCapacity) + " 字节";
    m_logView->Append(LogLevel::Info, "PE", infoMsg);

    std::vector<std::string> disabledModes;
    if (!avail.canReplaceText) disabledModes.push_back(".text覆盖(空间不足)");
    if (!avail.canInjectOep) disabledModes.push_back("OEP覆盖(空间不足)");
    if (!avail.canAddNewSection) disabledModes.push_back("新增节区(节头空间不足)");
    if (!avail.canImportInjection) disabledModes.push_back("导入表注入(需DLL格式)");

    if (!disabledModes.empty()) {
        std::string warnMsg = "已自动禁用不兼容模式: ";
        for (size_t i = 0; i < disabledModes.size(); ++i) {
            if (i > 0) warnMsg += ", ";
            warnMsg += disabledModes[i];
        }
        m_logView->Append(LogLevel::Warn, "Patch", warnMsg);
    }
}

void MainView::RefreshDllFunctionSuggestions() {
    if (!m_asbDllFunc) return;

    std::vector<std::string> suggestions = { "DLLMain" };
    std::string whiteStr = m_fpWhite ? m_fpWhite->GetPath() : "";
    if (!whiteStr.empty()) {
        std::wstring whitePath = Utf8ToWide(whiteStr);
        if (std::filesystem::exists(whitePath)) {
            std::wstring ext = std::filesystem::path(whitePath).extension().wstring();
            for (auto& c : ext) c = towlower(c);
            if (ext == L".dll") {
                std::vector<std::string> exports;
                if (m_pePatcher->ListDllExports(whitePath, exports)) {
                    suggestions.insert(suggestions.end(), exports.begin(), exports.end());
                }
            }
        }
    }
    m_asbDllFunc->SetSuggestionItems(suggestions);
}

// 判断当前生效的目标类型: 手动 EXE/DLL 强制；自动模式按白文件 PE 特征识别（退化按扩展名）
bool MainView::IsDllTarget() {
    int tIdx = m_segTargetType ? m_segTargetType->GetSelectedIndex() : 0;
    if (tIdx == 1) return false; // 强制 EXE
    if (tIdx == 2) return true;  // 强制 DLL

    // 自动识别
    std::string whiteStr = m_fpWhite ? m_fpWhite->GetPath() : "";
    if (whiteStr.empty()) return false;
    std::wstring whitePath = Utf8ToWide(whiteStr);
    if (!std::filesystem::exists(whitePath)) return false;

    Core::PeFileInfo info;
    if (m_pePatcher->InspectPe(whitePath, info)) {
        return info.isDll;
    }
    std::wstring ext = std::filesystem::path(whitePath).extension().wstring();
    for (auto& c : ext) c = towlower(c);
    return ext == L".dll";
}

void MainView::OpenOutputDir() {
    std::filesystem::path targetDir;
    if (!m_lastOutputPath.empty() && std::filesystem::exists(m_lastOutputPath)) {
        // 定位并选中最后生成的文件
        std::wstring selectCmd = L"/select,\"" + m_lastOutputPath + L"\"";
        ShellExecuteW(nullptr, L"open", L"explorer.exe", selectCmd.c_str(), nullptr, SW_SHOW);
        return;
    }

    targetDir = std::filesystem::current_path() / L"out";
    if (!std::filesystem::exists(targetDir)) {
        std::error_code ec;
        std::filesystem::create_directories(targetDir, ec);
    }

    ShellExecuteW(nullptr, L"open", targetDir.c_str(), nullptr, nullptr, SW_SHOW);
}

void MainView::RunPatch() {
    std::string whiteStr = m_fpWhite ? m_fpWhite->GetPath() : "";
    std::string payloadStr = m_fpPayload ? m_fpPayload->GetPath() : "";

    // 校验输入
    if (whiteStr.empty() || payloadStr.empty()) {
        m_logView->Append(LogLevel::Error, "Validate", "白文件或载荷路径未填写");
        return;
    }

    std::wstring whitePath = Utf8ToWide(whiteStr);
    std::wstring payloadPath = Utf8ToWide(payloadStr);

    if (!std::filesystem::exists(whitePath)) {
        m_logView->Append(LogLevel::Error, "Validate", "白文件不存在: " + whiteStr);
        return;
    }
    if (!std::filesystem::exists(payloadPath)) {
        m_logView->Append(LogLevel::Error, "Validate", "载荷文件不存在: " + payloadStr);
        return;
    }

    // 自动构建统一输出路径：.\out\<原文件名>_Patch.exe (DLL 白文件则输出 _Patch.dll)
    std::filesystem::path outDir = std::filesystem::current_path() / L"out";
    std::error_code ec;
    std::filesystem::create_directories(outDir, ec);

    std::filesystem::path p = whitePath;
    std::wstring whiteExt = p.extension().wstring();
    for (auto& c : whiteExt) c = towlower(c);
    std::wstring outFileName = p.stem().wstring() + L".exe";
    if (whiteExt == L".dll") {
        outFileName = p.stem().wstring() + L".dll";
    }
    std::filesystem::path outPath = outDir / outFileName;
    m_lastOutputPath = outPath.wstring();
    std::string outStr = WideToUtf8(m_lastOutputPath);

    // 读取选项
    int patchModeIdx = m_segPatchMode ? m_segPatchMode->GetSelectedIndex() : 0;
    Core::PatchMode patchMode = Core::PatchMode::ReplaceTextSection;
    std::string dllFuncName = "DLLMain";

    const bool dllTarget = IsDllTarget();
    if (dllTarget) {
        // DLL 目标: 默认 Patch 方式 = 覆盖指定函数；也可选择附加式模式（末节扩容/新增节区/TLS回调/导入表注入）
        switch (patchModeIdx) {
        case 2: patchMode = Core::PatchMode::EnlargeLastSection; break;
        case 3: patchMode = Core::PatchMode::AddNewSection; break;
        case 4: patchMode = Core::PatchMode::TlsCallback; break;
        case 5: patchMode = Core::PatchMode::ImportInjection; break;
        default:
            patchMode = Core::PatchMode::DllExportPatch;
            if (m_asbDllFunc) {
                dllFuncName = m_asbDllFunc->GetText();
                if (dllFuncName.empty()) dllFuncName = "DLLMain";
            }
            break;
        }
        // 预检: 目标为 DLL 但白文件不是 DLL
        Core::PeFileInfo whiteInfo;
        if (m_pePatcher->InspectPe(whitePath, whiteInfo) && !whiteInfo.isDll) {
            m_logView->Append(LogLevel::Error, "Validate", "目标类型为 DLL，但白文件不是 DLL 文件");
            return;
        }
    } else {
        // EXE 目标: 使用 Patch 模式列表（patch 入口 / 节注入）
        switch (patchModeIdx) {
        case 1: patchMode = Core::PatchMode::InjectEntryPoint; break;
        case 2: patchMode = Core::PatchMode::EnlargeLastSection; break;
        case 3: patchMode = Core::PatchMode::AddNewSection; break;
        case 4: patchMode = Core::PatchMode::TlsCallback; break;
        case 5: patchMode = Core::PatchMode::ImportInjection; break;
        default: break;
        }
    }

    int subIdx = m_segSubsystem ? m_segSubsystem->GetSelectedIndex() : 0;
    auto subType = Core::SubsystemType::KeepOriginal;
    if (subIdx == 1) subType = Core::SubsystemType::WindowsGui;
    else if (subIdx == 2) subType = Core::SubsystemType::Console;

    int uacIdx = m_segUac ? m_segUac->GetSelectedIndex() : 0;
    auto uacLevel = Core::UacLevel::KeepOriginal;
    if (uacIdx == 1) uacLevel = Core::UacLevel::AsInvoker;
    else if (uacIdx == 2) uacLevel = Core::UacLevel::RequireAdministrator;

    bool bRemoveSig = m_swRemoveSig && m_swRemoveSig->GetIsOn();
    bool bWipeTimestamp = m_swWipeTimestamp && m_swWipeTimestamp->GetIsOn();
    bool bDisableCfg = m_swDisableCfg && m_swDisableCfg->GetIsOn();

    m_logView->Append(LogLevel::Info, "CFG", bDisableCfg
        ? "禁用CFG: 开，将剥离 GUARD_CF 标志并清空 LOAD_CONFIG 目录"
        : "禁用CFG: 关，保留目标文件原有 CFG 设置");

    // 执行核心 Patch 逻辑（数字签名在 Patch 阶段统一安全处理）
    bool success = m_pePatcher->ExecutePatch(whitePath, payloadPath, m_lastOutputPath, patchMode, bRemoveSig, dllFuncName, bDisableCfg);
    if (!success) {
        m_logView->Append(LogLevel::Error, "Patch", "Patch 失败，请检查上方日志详情");
        return;
    }

    auto logger = [this](LogLevel lvl, const std::string& tag, const std::string& msg) {
        if (m_logView) {
            m_logView->Append(lvl, tag, msg);
        }
    };

    // 1. 子系统转换
    if (subType != Core::SubsystemType::KeepOriginal) {
        Core::PeSecurity::ConvertSubsystem(m_lastOutputPath, subType, logger);
    }

    // 3. 修改 UAC Manifest
    if (uacLevel != Core::UacLevel::KeepOriginal) {
        Core::PeSecurity::ModifyUacManifest(m_lastOutputPath, uacLevel, logger);
    }

    // 4. 擦除时间戳
    if (bWipeTimestamp) {
        Core::PeSecurity::WipeTimeDateStamp(m_lastOutputPath, 0, logger);
    }

    // 导入表注入模式: 载荷 DLL 未被嵌入，需复制到输出目录与主程序一同分发
    if (patchMode == Core::PatchMode::ImportInjection) {
        std::filesystem::path payloadFs = payloadPath;
        const std::wstring dllName = payloadFs.filename().wstring();
        std::error_code copyEc;
        std::filesystem::copy_file(payloadFs, outDir / dllName, std::filesystem::copy_options::overwrite_existing, copyEc);
        if (!copyEc) {
            m_logView->Append(LogLevel::Success, "Complete", "载荷 DLL 已复制到输出目录，分发时请与主程序放在同一目录");
        } else {
            m_logView->Append(LogLevel::Warn, "Complete", "载荷 DLL 复制到输出目录失败，请手动分发");
        }
    }

    m_logView->Append(LogLevel::Success, "Complete", "Patch 处理完成。");
    m_logView->Append(LogLevel::Info, "Complete", "输出文件: " + outStr);

    // Patch 完成后自动弹出资源管理器定位选中该文件
    std::wstring selectCmd = L"/select,\"" + m_lastOutputPath + L"\"";
    ShellExecuteW(nullptr, L"open", L"explorer.exe", selectCmd.c_str(), nullptr, SW_SHOW);
}

} // namespace Patcher::View
