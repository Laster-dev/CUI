#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/window/Window.h"
#include "framework/window/WindowBackdrop.h"
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

/**
 * @brief 创建一个指定材质与特性的独立子窗口
 */
std::shared_ptr<Window> SpawnMaterialWindow(
    const std::string& title,
    BackdropType backdrop,
    bool transparentMode,
    const std::string& desc) {

    auto win = std::make_shared<Window>();
    const int width = transparentMode ? 380 : 560;
    const int height = transparentMode ? 240 : 380;

    if (!win->Create(title, width, height, transparentMode)) {
        return nullptr;
    }

    win->SetBackdropType(backdrop);

    // 构建子窗口的独立内容树
    auto root = Column(14).Padding(20).Build();
    root->SetFlexGrow(1.0f);
    root->SetAlign(Alignment::Stretch);

    if (transparentMode) {
        root->SetBackground(D2D1::ColorF(0, 0, 0, 0));
        root->SetBackgroundToken(ThemeTokenId::Unset);
    } else {
        root->SetBackgroundToken(ThemeTokenId::WindowBackground);
    }

    // 顶部标题与材质 Badge
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
        .Width(300.0f);

    auto progress = ProgressBarWidget(70.0f);
    progress->SetHeight(6.0f);
    progress->SetWidth(300.0f);

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

    root->AddChild(card);
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
    // 1. 创建不同系统材质的独立窗口 (Create Backdrop Windows)
    // ==========================================
    auto btnSpawnMica = Button("✨ 创建 Mica (云母) 窗口")
        .Height(32.0f)
        .OnClick([statusLabel](UIElement*) {
            SpawnMaterialWindow(
                "Mica (云母材质) 独立窗口",
                BackdropType::Mica,
                false,
                "使用 BackdropType::Mica 创建。Windows 11 标准主窗口沉浸式材质，与桌面壁纸协调相融。");
            statusLabel->Text = "已创建并弹出【Mica 云母材质独立窗口】。";
        });

    auto btnSpawnMicaAlt = Button("🎨 创建 MicaAlt (深层云母) 窗口")
        .Height(32.0f)
        .OnClick([statusLabel](UIElement*) {
            SpawnMaterialWindow(
                "MicaAlt (深层云母) 独立窗口",
                BackdropType::MicaAlt,
                false,
                "使用 BackdropType::MicaAlt 创建。色彩层次更鲜明，适合多 Tab 标签页与深层窗口。");
            statusLabel->Text = "已创建并弹出【MicaAlt 深层云母独立窗口】。";
        });

    auto btnSpawnAcrylic = Button("💎 创建 Acrylic (亚克力毛玻璃) 窗口")
        .Height(32.0f)
        .OnClick([statusLabel](UIElement*) {
            SpawnMaterialWindow(
                "Acrylic (亚克力毛玻璃) 独立窗口",
                BackdropType::Acrylic,
                false,
                "使用 BackdropType::Acrylic 创建。具备桌面透视与高斯模糊效果，极富现代玻璃质感。");
            statusLabel->Text = "已创建并弹出【Acrylic 亚克力毛玻璃独立窗口】。";
        });

    auto btnSpawnSolid = Button("⬛ 创建 Solid (纯色实底) 窗口")
        .Height(32.0f)
        .OnClick([statusLabel](UIElement*) {
            SpawnMaterialWindow(
                "Solid (实底纯色) 独立窗口",
                BackdropType::Solid,
                false,
                "使用 BackdropType::Solid 创建。经典主题纯色填充，无毛玻璃透视。");
            statusLabel->Text = "已创建并弹出【Solid 实底纯色独立窗口】。";
        });

    auto btnSpawnNone = Button("🚫 创建 None (无后置背景) 窗口")
        .Height(32.0f)
        .OnClick([statusLabel](UIElement*) {
            SpawnMaterialWindow(
                "None (无后置背景) 独立窗口",
                BackdropType::None,
                false,
                "使用 BackdropType::None 创建。纯净默认渲染，完全由 Direct2D 控件树自行绘制。");
            statusLabel->Text = "已创建并弹出【None 无材质独立窗口】。";
        });

    // ==========================================
    // 2. 创建特殊样式窗口与全透明悬浮挂件 (Special Window Styles)
    // ==========================================
    auto btnSpawnTransparent = Button("🪟 创建全透明镂空挂件窗口")
        .Height(32.0f)
        .OnClick([statusLabel](UIElement*) {
            SpawnMaterialWindow(
                "透明悬浮挂件窗口",
                BackdropType::None,
                true,
                "启用 transparentMode = true。窗口背景逐像素全透明镂空，仅呈现 Direct2D 硬件加速绘制的圆角发光卡片。");
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
    spec.title = "Window(创建多材质与多样式窗口)";
    spec.subtitle = "基于 Win32 原生句柄与 Direct2D/DirectComposition 硬件加速合成的现代化多窗口架构，支持快速创建 Mica/Acrylic/Solid 等各种材质的独立窗口与全透明通道混合窗口。";
    spec.sections = {
        {
            "创建各种系统材质的独立窗口 (Create Windows of Different Backdrops)",
            "点击下方按钮，将调用 win->Create(...) 与 win->SetBackdropType(...) 实时创建并弹出对应 DWM 背景材质的独立窗口。",
            Column(12, {
                Row(8, { btnSpawnMica, btnSpawnMicaAlt, btnSpawnAcrylic }).AlignVertical(Alignment::Center),
                Row(8, { btnSpawnSolid, btnSpawnNone }).AlignVertical(Alignment::Center),
            }),
        },
        {
            "创建特殊样式与全透明悬浮窗口 (Special Window Styles)",
            "演示通过 transparentMode = true 创建不规则镂空、全透明通道混合的独立悬浮挂件窗口，以及批量管理所有活动子窗口。",
            Column(12, {
                Row(8, { btnSpawnTransparent, btnCloseAll }).AlignVertical(Alignment::Center),
                statusLabel,
            }),
        },
    };

    spec.source = R"cpp(// 1. 创建 Mica (云母材质) 独立窗口
auto win = std::make_shared<Window>();
win->Create("Mica 窗口", 600, 400, false);
win->SetBackdropType(BackdropType::Mica); // 设置云母背景材质

auto root = Column(16).Padding(24).Build();
root->AddChild(MakeLabel("这是 Mica 独立窗口", 16.0f, ThemeTokenId::TextPrimary, true));
win->SetRootElement(root);
win->Show();

// 2. 创建 Acrylic (亚克力毛玻璃) 独立窗口
auto acrylicWin = std::make_shared<Window>();
acrylicWin->Create("Acrylic 窗口", 600, 400, false);
acrylicWin->SetBackdropType(BackdropType::Acrylic);
acrylicWin->Show();

// 3. 创建全透明通道镂空悬浮窗口 (transparentMode = true)
auto floatWin = std::make_shared<Window>();
floatWin->Create("悬浮挂件", 360, 220, true); // 开启逐像素 Alpha 透明混合
floatWin->Show();
)cpp";

    return BuildSamplePage(spec);
}

} // namespace Gallery
