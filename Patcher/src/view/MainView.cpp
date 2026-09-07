#include "MainView.h"
#include "../core/PeSecurity.h"
#include "../core/IconReplacer.h"
#include "framework/controls/WindowTitleBar.h"
#include "framework/controls/Panel.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBlock.h"
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
    m_logView = LogViewWidget().Build();
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
    auto titleBar = ElementBuilder<WindowTitleBar>()
        .Title("PE Patch 工具")
        .Height(32.0f);

    return titleBar.Build();
}

std::shared_ptr<UIElement> MainView::BuildFilesArea() {
    // 1. 白名单目标文件：占满除左边文本外的整行
    m_fpWhite = FilePickerWidget()
        .DialogTitle("选择白名单目标文件")
        .Filter("可执行文件 (*.exe;*.dll)", "*.exe;*.dll")
        .FlexGrow(1.0f)
        .Height(28.0f)
        .Placeholder("选择白文件路径 (.exe / .dll)")
        .Build();
    m_fpWhite->AddFilter("所有文件 (*.*)", "*.*");

    // 2. 注入载荷文件：占满除左边文本外的整行
    m_fpPayload = FilePickerWidget()
        .DialogTitle("选择注入载荷文件")
        .Filter("载荷文件 (*.exe;*.dll;*.text;*.bin)", "*.exe;*.dll;*.text;*.bin")
        .FlexGrow(1.0f)
        .Height(28.0f)
        .Placeholder("选择载荷文件路径 (.exe / .dll / .text / .bin)")
        .Build();
    m_fpPayload->AddFilter("所有文件 (*.*)", "*.*");

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
    // 1. Patch 模式
    m_segPatchMode = ElementBuilder<SegmentedControl>()
        .AddItem("覆盖 .text 代码段")
        .AddItem("入口点注入Payload")
        .Height(26.0f)
        .Width(260.0f)
        .Build();
    m_segPatchMode->SetSelectedIndex(0);

    // 2. 子系统类型
    m_segSubsystem = ElementBuilder<SegmentedControl>()
        .AddItem("KEEP")
        .AddItem("GUI")
        .AddItem("CUI")
        .Height(26.0f)
        .Width(280.0f)
        .Build();
    m_segSubsystem->SetSelectedIndex(1);

    // 3. UAC 权限清单
    m_segUac = ElementBuilder<SegmentedControl>()
        .AddItem("KEEP")
        .AddItem("USER")
        .AddItem("ADMIN")
        .Height(26.0f)
        .Width(280.0f)
        .Build();
    m_segUac->SetSelectedIndex(1);

    // 4. 开关
    m_swRemoveSig = ToggleSwitchTile("剥离数字签名", true).Build();
    m_swWipeTimestamp = ToggleSwitchTile("抹除时间戳", true).Build();

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

    auto panel = Column(4.0f, {
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
            m_swWipeTimestamp
        }).Align(Alignment::Start).Margin(0.0f, 2.0f, 0.0f, 2.0f)
    })
    .Align(Alignment::Start)
    .Build();

    return panel;
}

std::shared_ptr<UIElement> MainView::BuildActionsArea() {
    // 圆形纯图标按钮（36x36，圆角 18，内部居中绘制 SVG 图标，无文字）
    auto btnRun = Fluent::Button("")
        .Icon(kSvgPlay)
        .ToolTip("开始执行")
        .BackgroundToken(ThemeTokenId::AccentColor)
        .ForegroundToken(ThemeTokenId::AccentForeground)
        .Height(36.0f)
        .Width(36.0f)
        .CornerRadius(18.0f)
        .Padding(0.0f)
        .FontSize(18.0f)
        .OnClick([this](UIElement*) { RunPatch(); });

    auto btnOpenDir = Fluent::Button("")
        .Icon(kSvgFolderOpen)
        .ToolTip("打开输出目录")
        .Height(36.0f)
        .Width(36.0f)
        .CornerRadius(18.0f)
        .Padding(0.0f)
        .FontSize(18.0f)
        .OnClick([this](UIElement*) { OpenOutputDir(); });

    auto btnReset = Fluent::Button("")
        .Icon(kSvgReset)
        .ToolTip("重置")
        .Height(36.0f)
        .Width(36.0f)
        .CornerRadius(18.0f)
        .Padding(0.0f)
        .FontSize(18.0f)
        .OnClick([this](UIElement*) { ResetAll(); });

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
    if (m_logView) m_logView->Clear();
    if (m_logView) {
        m_logView->Append(LogLevel::Info, "System", "已重置输入与日志");
    }
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

    // 自动构建统一输出路径：.\out\<原文件名>_Patch.exe
    std::filesystem::path outDir = std::filesystem::current_path() / L"out";
    std::error_code ec;
    std::filesystem::create_directories(outDir, ec);

    std::filesystem::path p = whitePath;
    std::wstring outFileName = p.stem().wstring() + L"_Patch.exe";
    std::filesystem::path outPath = outDir / outFileName;
    m_lastOutputPath = outPath.wstring();
    std::string outStr = WideToUtf8(m_lastOutputPath);

    // 读取选项
    int patchModeIdx = m_segPatchMode ? m_segPatchMode->GetSelectedIndex() : 0;
    auto patchMode = (patchModeIdx == 1) ? Core::PatchMode::InjectEntryPoint : Core::PatchMode::ReplaceTextSection;

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

    // 执行核心 Patch 逻辑
    bool success = m_pePatcher->ExecutePatch(whitePath, payloadPath, m_lastOutputPath, patchMode);
    if (!success) {
        m_logView->Append(LogLevel::Error, "Patch", "Patch 失败，请检查上方日志详情");
        return;
    }

    auto logger = [this](LogLevel lvl, const std::string& tag, const std::string& msg) {
        if (m_logView) {
            m_logView->Append(lvl, tag, msg);
        }
    };

    // 1. 剥离数字签名
    if (bRemoveSig) {
        Core::PeSecurity::RemoveSignature(m_lastOutputPath, logger);
    }

    // 2. 子系统转换
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

    m_logView->Append(LogLevel::Info, "Complete", "Patch 处理完成。");
    m_logView->Append(LogLevel::Info, "Complete", "输出文件: " + outStr);

    // Patch 完成后自动弹出资源管理器定位选中该文件
    std::wstring selectCmd = L"/select,\"" + m_lastOutputPath + L"\"";
    ShellExecuteW(nullptr, L"open", L"explorer.exe", selectCmd.c_str(), nullptr, SW_SHOW);
}

} // namespace Patcher::View
