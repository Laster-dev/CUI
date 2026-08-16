#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/window/Window.h"
#include "framework/window/WindowBackdrop.h"
#include "framework/controls/WindowTitleBar.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBox.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/ProgressBar.h"
#include "framework/controls/Panel.h"
#include "framework/style/ThemeManager.h"
#include <format>
#include <memory>
#include <string>
#include <vector>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

// 保持动态创建的子窗口生命周期，避免局部变量离开作用域被析构销毁
static std::vector<std::shared_ptr<Window>> s_spawnedWindows;

struct TitleBarOptions {
    bool hasTitleBar = true;
    bool minVisible = true;
    bool maxVisible = true;
    bool closeVisible = true;
    bool minEnabled = true;
    bool maxEnabled = true;
    bool closeEnabled = true;
};

/**
 * @brief 创建一个指定材质与标题栏属性的独立子窗口
 */
std::shared_ptr<Window> SpawnMaterialWindow(
    const std::string& title,
    BackdropType backdrop,
    bool transparentMode,
    const std::string& desc,
    TitleBarOptions options = {}) {

    auto win = std::make_shared<Window>();
    const int width = transparentMode ? 380 : 580;
    const int height = transparentMode ? 240 : 400;

    if (!win->Create(title, width, height, transparentMode)) {
        return nullptr;
    }

    win->SetBackdropType(backdrop);

    // 构建子窗口的根容器（垂直排列）
    auto root = Column(0).Build();
    root->SetFlexGrow(1.0f);
    root->SetAlign(Alignment::Stretch);

    if (transparentMode) {
        root->SetBackground(D2D1::ColorF(0, 0, 0, 0));
        root->SetBackgroundToken(ThemeTokenId::Unset);
    } else {
        root->SetBackgroundToken(ThemeTokenId::WindowBackground);
    }

    // 1. 挂载自定义窗口标题栏（若启用）
    if (options.hasTitleBar && !transparentMode) {
        auto titleBar = TitleBarWidget(title).Build();
        titleBar->SetIsMinimizeButtonVisible(options.minVisible);
        titleBar->SetIsMaximizeButtonVisible(options.maxVisible);
        titleBar->SetIsCloseButtonVisible(options.closeVisible);
        titleBar->SetIsMinimizeButtonEnabled(options.minEnabled);
        titleBar->SetIsMaximizeButtonEnabled(options.maxEnabled);
        titleBar->SetIsCloseButtonEnabled(options.closeEnabled);
        root->AddChild(titleBar);
    }

    // 2. 窗口内部主体内容区
    auto contentArea = Column(14).Padding(20).Build();
    contentArea->SetFlexGrow(1.0f);
    contentArea->SetAlign(Alignment::Stretch);

    const std::string materialName = MaterialHost::DisplayNameZh(backdrop);
    auto titleLabel = MakeLabel(title, 18.0f, ThemeTokenId::TextPrimary, true);
    auto badgeLabel = MakeLabel(
        transparentMode ? "🪟 全透明镂空 (Transparent Mode)" : std::format("🎨 背景材质：{}", materialName),
        12.0f,
        ThemeTokenId::AccentColor,
        true);

    auto descLabel = MakeLabel(desc, 11.0f, ThemeTokenId::TextSecondary, false);

    // 交互控件区
    static int s_counter = 0;
    auto counterText = MakeLabel("计数：0", 13.0f, ThemeTokenId::TextPrimary, true);

    auto btnInc = Button("➕ 计数测试")
        .Height(30.0f)
        .Width(90.0f)
        .OnClick([counterText](UIElement*) {
            s_counter++;
            auto textBlock = std::dynamic_pointer_cast<TextBlock>(counterText);
            if (textBlock) {
                textBlock->SetText(std::format("计数：{}", s_counter));
            }
        });

    auto btnClose = Button("❌ 关闭此窗口")
        .Height(30.0f)
        .Width(100.0f)
        .OnClick([win](UIElement*) {
            if (win && win->GetHWND()) {
                DestroyWindow(win->GetHWND());
            }
        });

    auto input = TextField("在此新窗口中输入文本...")
        .Height(32.0f)
        .Width(320.0f);

    auto progress = ProgressBarWidget(70.0f);
    progress->SetHeight(6.0f);
    progress->SetWidth(320.0f);

    auto card = Column(12, {
        titleLabel,
        badgeLabel,
        descLabel,
        Row(8, { btnInc, counterText, btnClose }).AlignVertical(Alignment::Center),
        input,
        progress,
    }).Padding(16).CornerRadius(8).Build();

    if (transparentMode) {
        card->SetBackground(D2D1::ColorF(0.12f, 0.14f, 0.18f, 0.88f));
        card->SetBorderBrush(D2D1::ColorF(0.38f, 0.65f, 0.98f, 0.65f));
        card->SetBorderThickness(1.5f);
    } else {
        card->SetBackgroundToken(ThemeTokenId::CardBackground);
        card->SetBorderToken(ThemeTokenId::CardBorder);
        card->SetBorderThickness(1.0f);
    }

    contentArea->AddChild(card);
    root->AddChild(contentArea);

    win->SetRootElement(root);
    win->Show();

    s_spawnedWindows.push_back(win);
    return win;
}

} // namespace

/**
 * @brief 构建 Window (窗口创建与材质系统) 展示页面。
 */
Element BuildWindowPage() {
    auto statusLabel = MakeStatus("状态：就绪。点击下方任意按钮即可创建并弹出对应材质或样式的独立窗口。");

    // ==========================================
    // 1. 创建不同系统材质的独立窗口 (带标准完整标题栏)
    // ==========================================
    auto btnSpawnMica = Button("✨ 创建 Mica (云母) 窗口")
        .Height(32.0f)
        .OnClick([statusLabel](UIElement*) {
            SpawnMaterialWindow(
                "Mica (云母材质) 独立窗口",
                BackdropType::Mica,
                false,
                "使用 BackdropType::Mica 创建。Windows 11 标准主窗口沉浸式材质，与桌面壁纸协调相融。自带标准可拖动标题栏。");
            statusLabel->Text = "已创建并弹出【Mica 云母材质独立窗口】。";
        });

    auto btnSpawnMicaAlt = Button("🎨 创建 MicaAlt (深层云母) 窗口")
        .Height(32.0f)
        .OnClick([statusLabel](UIElement*) {
            SpawnMaterialWindow(
                "MicaAlt (深层云母) 独立窗口",
                BackdropType::MicaAlt,
                false,
                "使用 BackdropType::MicaAlt 创建。色彩层次更鲜明，适合多 Tab 标签页与深层窗口。自带标准可拖动标题栏。");
            statusLabel->Text = "已创建并弹出【MicaAlt 深层云母独立窗口】。";
        });

    auto btnSpawnAcrylic = Button("💎 创建 Acrylic (亚克力毛玻璃) 窗口")
        .Height(32.0f)
        .OnClick([statusLabel](UIElement*) {
            SpawnMaterialWindow(
                "Acrylic (亚克力毛玻璃) 独立窗口",
                BackdropType::Acrylic,
                false,
                "使用 BackdropType::Acrylic 创建。具备桌面透视与高斯模糊效果，极富现代玻璃质感。自带标准可拖动标题栏。");
            statusLabel->Text = "已创建并弹出【Acrylic 亚克力毛玻璃独立窗口】。";
        });

    auto btnSpawnSolid = Button("⬛ 创建 Solid (纯色实底) 窗口")
        .Height(32.0f)
        .OnClick([statusLabel](UIElement*) {
            SpawnMaterialWindow(
                "Solid (实底纯色) 独立窗口",
                BackdropType::Solid,
                false,
                "使用 BackdropType::Solid 创建。经典主题纯色填充，无毛玻璃透视。自带标准可拖动标题栏。");
            statusLabel->Text = "已创建并弹出【Solid 实底纯色独立窗口】。";
        });

    auto btnSpawnNone = Button("🚫 创建 None (无后置背景) 窗口")
        .Height(32.0f)
        .OnClick([statusLabel](UIElement*) {
            SpawnMaterialWindow(
                "None (无后置背景) 独立窗口",
                BackdropType::None,
                false,
                "使用 BackdropType::None 创建。纯净默认渲染，完全由 Direct2D 控件树自行绘制。自带标准可拖动标题栏。");
            statusLabel->Text = "已创建并弹出【None 无材质独立窗口】。";
        });

    // ==========================================
    // 2. 标题栏按钮显隐与禁用扩展演示 (Caption Buttons Options)
    // ==========================================
    auto btnSpawnDialogStyle = Button("🔒 对话框模式 (隐藏最小化/最大化)")
        .Height(32.0f)
        .OnClick([statusLabel](UIElement*) {
            TitleBarOptions opt;
            opt.minVisible = false;
            opt.maxVisible = false;
            opt.closeVisible = true;
            SpawnMaterialWindow(
                "对话框模式窗口 (Dialog Style)",
                BackdropType::Mica,
                false,
                "隐藏了最小化与最大化按钮（minVisible=false, maxVisible=false），仅保留关闭按钮，适合固定大小的模态与提示窗。",
                opt);
            statusLabel->Text = "已创建并弹出【对话框模式窗口】(隐藏最小化与最大化按钮)。";
        });

    auto btnSpawnDisableClose = Button("🛡️ 关键任务 (禁用关闭按钮)")
        .Height(32.0f)
        .OnClick([statusLabel](UIElement*) {
            TitleBarOptions opt;
            opt.closeEnabled = false;
            SpawnMaterialWindow(
                "关键任务窗口 (禁用关闭)",
                BackdropType::Acrylic,
                false,
                "关闭按钮已置灰禁用（closeEnabled=false），点击关闭不会触发退出，可用于强制等待保存或核心任务执行。",
                opt);
            statusLabel->Text = "已创建并弹出【关键任务窗口】(禁用关闭按钮)。";
        });

    auto btnSpawnToolPanel = Button("🛠️ 工具面板 (隐藏最大化，禁用最小化)")
        .Height(32.0f)
        .OnClick([statusLabel](UIElement*) {
            TitleBarOptions opt;
            opt.maxVisible = false;
            opt.minEnabled = false;
            SpawnMaterialWindow(
                "工具面板窗口 (Tool Panel)",
                BackdropType::MicaAlt,
                false,
                "隐藏最大化按钮并禁用最小化按钮（maxVisible=false, minEnabled=false），呈现灵活定制的辅助工具条效果。",
                opt);
            statusLabel->Text = "已创建并弹出【工具面板窗口】(隐藏最大化，禁用最小化)。";
        });

    auto btnSpawnTransparent = Button("🪟 全透明悬浮挂件窗口")
        .Height(32.0f)
        .OnClick([statusLabel](UIElement*) {
            TitleBarOptions opt;
            opt.hasTitleBar = false;
            SpawnMaterialWindow(
                "透明悬浮挂件窗口",
                BackdropType::None,
                true,
                "启用 transparentMode = true。窗口背景逐像素全透明镂空，仅呈现 Direct2D 硬件加速绘制的圆角发光卡片。",
                opt);
            statusLabel->Text = "已创建并弹出【全透明镂空挂件窗口】(transparentMode = true)。";
        });

    auto btnCloseAll = Button("❌ 一键关闭所有已创建的子窗口")
        .Height(32.0f)
        .OnClick([statusLabel](UIElement*) {
            int count = 0;
            for (auto& win : s_spawnedWindows) {
                if (win && win->GetHWND()) {
                    DestroyWindow(win->GetHWND());
                    count++;
                }
            }
            s_spawnedWindows.clear();
            statusLabel->Text = std::format("已清理并关闭 {} 个子窗口。", count);
        });

    SamplePageSpec spec;
    spec.title = "Window(创建多材质与标题栏定制窗口)";
    spec.subtitle = "基于 Win32 原生句柄与 Direct2D/DirectComposition 硬件加速合成的现代化多窗口架构，支持快速创建 Mica/Acrylic/Solid 等各种材质的独立窗口，支持标题栏最小化/最大化/关闭按钮的显隐与禁用控制。";
    spec.sections = {
        {
            "创建各种系统材质的独立窗口 (Create Windows of Different Backdrops)",
            "点击下方按钮，将调用 win->Create(...) 与 win->SetBackdropType(...) 实时创建并弹出对应 DWM 背景材质与标准标题栏的独立窗口。",
            Column(12, {
                Row(8, { btnSpawnMica, btnSpawnMicaAlt, btnSpawnAcrylic }).AlignVertical(Alignment::Center),
                Row(8, { btnSpawnSolid, btnSpawnNone }).AlignVertical(Alignment::Center),
            }),
        },
        {
            "标题栏三大按钮定制与特殊样式 (TitleBar Caption Buttons Customization)",
            "WindowTitleBar 提供了 IsMinimizeButtonVisible / IsMaximizeButtonVisible / IsCloseButtonVisible 与 IsEnabled 等精细化控制属性，支持灵活构建对话框、只读窗口与工具面板。",
            Column(12, {
                Row(8, { btnSpawnDialogStyle, btnSpawnDisableClose }).AlignVertical(Alignment::Center),
                Row(8, { btnSpawnToolPanel, btnSpawnTransparent }).AlignVertical(Alignment::Center),
                Row(8, { btnCloseAll }).AlignVertical(Alignment::Center),
                statusLabel,
            }),
        },
    };

    spec.source = R"cpp(// 1. 创建带自定义标题栏与 Acrylic 材质的独立窗口
auto win = std::make_shared<Window>();
win->Create("Acrylic 窗口", 600, 400, false);
win->SetBackdropType(BackdropType::Acrylic);

auto titleBar = TitleBarWidget("我的定制窗口").Build();
// 标题栏按钮显隐与禁用控制
titleBar->SetIsMinimizeButtonVisible(false); // 隐藏最小化
titleBar->SetIsCloseButtonEnabled(false);    // 禁用关闭按钮

auto root = Column(0).Build();
root->AddChild(titleBar);
root->AddChild(MakeLabel("窗口内容...", 14.0f, ThemeTokenId::TextPrimary));
win->SetRootElement(root);
win->Show();
)cpp";

    return BuildSamplePage(spec);
}

} // namespace Gallery
