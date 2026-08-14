#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/Button.h"
#include "framework/style/ThemeManager.h"
#include <memory>
#include <string>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

constexpr const char* kSvgHeart =
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M512 896a42.666667 42.666667 0 0 1-30.293333-12.373333l-330.24-330.24A245.333333 245.333333 0 0 1 498.346667 206.506667L512 220.16l13.653333-13.653333a245.333333 245.333333 0 0 1 346.88 346.88l-330.24 330.24A42.666667 42.666667 0 0 1 512 896z\"/>"
    "</svg>";

constexpr const char* kSvgSearch =
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M448 768A320 320 0 1 0 448 128a320 320 0 0 0 0 640z m0 85.333333C213.12 853.333333 21.333333 661.546667 21.333333 426.666667S213.12 0 448 0s426.666667 191.786667 426.666667 426.666667c0 96-31.573333 184.746667-85.333333 256.426666l194.56 194.56a42.666667 42.666667 0 0 1-60.373334 60.373334l-194.56-194.56A423.893333 423.893333 0 0 1 448 853.333333z\"/>"
    "</svg>";

constexpr const char* kSvgCopy =
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M661.333333 234.666667A64 64 0 0 1 725.333333 298.666667v597.333333a64 64 0 0 1-64 64h-469.333333A64 64 0 0 1 128 896V298.666667a64 64 0 0 1 64-64z m-21.333333 85.333333H213.333333v554.666667h426.666667v-554.666667z m191.829333-256a64 64 0 0 1 63.744 57.856l0.256 6.144v575.701333a42.666667 42.666667 0 0 1-85.034666 4.992l-0.298667-4.992V149.333333H384a42.666667 42.666667 0 0 1-42.368-37.674666L341.333333 106.666667a42.666667 42.666667 0 0 1 37.674667-42.368L384 64h447.829333z\"/>"
    "</svg>";

constexpr const char* kSvgStar =
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M512 64l138.88 281.387 310.4 45.12-224.64 218.987 53.035 309.173L512 772.693 234.325 918.667l53.035-309.173L62.72 390.507l310.4-45.12z\"/>"
    "</svg>";

} // namespace

std::shared_ptr<UIElement> BuildButtonPage() {
    auto status = MakeStatus("准备就绪。尝试点击任意按钮...");

    // 1. 标准样式
    auto btnAccent = Make<Button>("主要按钮 (Accent)");
    btnAccent->OnClick().Connect([status](UIElement*) { status->SetText("点击了：主要按钮 (Accent)"); });

    auto btnStandard = Make<Button>("标准按钮 (Standard)");
    btnStandard->SetBackgroundToken(ThemeTokenId::CardBackground);
    btnStandard->SetColorToken(ThemeTokenId::TextPrimary);
    btnStandard->SetBorderToken(ThemeTokenId::CardBorder);
    btnStandard->SetBorderThickness(1.0f);
    btnStandard->OnClick().Connect([status](UIElement*) { status->SetText("点击了：标准按钮 (Standard)"); });

    auto btnOutline = Make<Button>("轮廓按钮 (Outline)");
    btnOutline->SetBackgroundToken(ThemeTokenId::Unset);
    btnOutline->SetColorToken(ThemeTokenId::AccentColor);
    btnOutline->SetBorderToken(ThemeTokenId::AccentColor);
    btnOutline->SetBorderThickness(1.0f);
    btnOutline->OnClick().Connect([status](UIElement*) { status->SetText("点击了：轮廓按钮 (Outline)"); });

    auto btnSubtle = Make<Button>("幽灵/文本按钮 (Subtle)");
    btnSubtle->SetBackgroundToken(ThemeTokenId::Unset);
    btnSubtle->SetColorToken(ThemeTokenId::TextPrimary);
    btnSubtle->SetBorderThickness(0.0f);
    btnSubtle->OnClick().Connect([status](UIElement*) { status->SetText("点击了：文本按钮 (Subtle)"); });

    auto btnDisabled = Make<Button>("禁用按钮 (Disabled)");
    btnDisabled->SetIsEnabled(false);

    // 2. 颜色与预设
    auto btnDanger = Make<Button>("危险操作 (Danger)");
    btnDanger->SetBackground(Color::Hex("#E53935"));
    btnDanger->SetHoverBackground(Color::Hex("#D32F2F"));
    btnDanger->SetPressedBackground(Color::Hex("#B71C1C"));
    btnDanger->SetColor(Color::White);
    btnDanger->OnClick().Connect([status](UIElement*) { status->SetText("点击了：危险操作按钮 (Crimson)"); });

    auto btnSuccess = Make<Button>("成功状态 (Success)");
    btnSuccess->SetBackground(Color::Hex("#2E7D32"));
    btnSuccess->SetHoverBackground(Color::Hex("#1B5E20"));
    btnSuccess->SetPressedBackground(Color::Hex("#0D3C10"));
    btnSuccess->SetColor(Color::White);
    btnSuccess->OnClick().Connect([status](UIElement*) { status->SetText("点击了：成功状态按钮 (Emerald)"); });

    auto btnWarning = Make<Button>("警告提示 (Warning)");
    btnWarning->SetBackground(Color::Hex("#F57C00"));
    btnWarning->SetHoverBackground(Color::Hex("#E65100"));
    btnWarning->SetPressedBackground(Color::Hex("#BF360C"));
    btnWarning->SetColor(Color::White);
    btnWarning->OnClick().Connect([status](UIElement*) { status->SetText("点击了：警告提示按钮 (Amber)"); });

    auto btnPurple = Make<Button>("紫色梦幻 (Purple)");
    btnPurple->SetBackground(Color::Hex("#7B1FA2"));
    btnPurple->SetHoverBackground(Color::Hex("#6A1B9A"));
    btnPurple->SetPressedBackground(Color::Hex("#4A148C"));
    btnPurple->SetColor(Color::White);
    btnPurple->OnClick().Connect([status](UIElement*) { status->SetText("点击了：紫色梦幻按钮 (Purple)"); });

    // 3. 各种圆角
    auto btnSquare = Make<Button>("直角方形 (r=0px)");
    btnSquare->SetCornerRadius(0.0f);
    btnSquare->OnClick().Connect([status](UIElement*) { status->SetText("点击了：直角方形按钮 (r=0px)"); });

    auto btnRadius4 = Make<Button>("小圆角 (r=4px)");
    btnRadius4->SetCornerRadius(4.0f);
    btnRadius4->OnClick().Connect([status](UIElement*) { status->SetText("点击了：小圆角按钮 (r=4px)"); });

    auto btnRadius8 = Make<Button>("中圆角 (r=8px)");
    btnRadius8->SetCornerRadius(8.0f);
    btnRadius8->OnClick().Connect([status](UIElement*) { status->SetText("点击了：中圆角按钮 (r=8px)"); });

    auto btnCapsule = Make<Button>("胶囊圆角 (r=20px)");
    btnCapsule->SetCornerRadius(20.0f);
    btnCapsule->SetPadding(Thickness(16, 6, 16, 6));
    btnCapsule->OnClick().Connect([status](UIElement*) { status->SetText("点击了：胶囊圆角按钮 (r=20px)"); });

    // 4. 各种尺寸
    auto btnSmall = Make<Button>("小尺寸 (Small)");
    btnSmall->SetFontSize(11.0f);
    btnSmall->SetPadding(Thickness(6, 2, 6, 2));
    btnSmall->OnClick().Connect([status](UIElement*) { status->SetText("点击了：小尺寸按钮"); });

    auto btnNormal = Make<Button>("标准尺寸 (Normal)");
    btnNormal->SetFontSize(13.0f);
    btnNormal->SetPadding(Thickness(12, 5, 12, 5));
    btnNormal->OnClick().Connect([status](UIElement*) { status->SetText("点击了：标准尺寸按钮"); });

    auto btnLarge = Make<Button>("大尺寸 (Large)");
    btnLarge->SetFontSize(16.0f);
    btnLarge->SetPadding(Thickness(18, 9, 18, 9));
    btnLarge->OnClick().Connect([status](UIElement*) { status->SetText("点击了：大尺寸按钮"); });

    auto btnFixed = Make<Button>("固定尺寸 (200 x 44)");
    btnFixed->SetWidth(200.0f);
    btnFixed->SetHeight(44.0f);
    btnFixed->OnClick().Connect([status](UIElement*) { status->SetText("点击了：固定尺寸按钮 (200x44)"); });

    // 5. 图标与 SVG 矢量按钮
    auto btnUnicode = Make<Button>("保存文件");
    btnUnicode->SetIcon("💾");
    btnUnicode->OnClick().Connect([status](UIElement*) { status->SetText("点击了：Unicode 图标按钮 (💾 保存)"); });

    auto btnAdd = Make<Button>("新建项目");
    btnAdd->SetIcon("➕");
    btnAdd->OnClick().Connect([status](UIElement*) { status->SetText("点击了：Unicode 图标按钮 (➕ 新建)"); });

    auto btnSvgHeart = Make<Button>("点赞");
    btnSvgHeart->SetIcon(kSvgHeart);
    btnSvgHeart->SetBackground(Color::Hex("#E91E63"));
    btnSvgHeart->SetHoverBackground(Color::Hex("#D81B60"));
    btnSvgHeart->SetPressedBackground(Color::Hex("#C2185B"));
    btnSvgHeart->SetColor(Color::White);
    btnSvgHeart->OnClick().Connect([status](UIElement*) { status->SetText("点击了：SVG 心形矢量按钮"); });

    auto btnSvgSearch = Make<Button>("搜索文档");
    btnSvgSearch->SetIcon(kSvgSearch);
    btnSvgSearch->OnClick().Connect([status](UIElement*) { status->SetText("点击了：SVG 搜索矢量按钮"); });

    auto btnSvgCopy = Make<Button>("复制文本");
    btnSvgCopy->SetIcon(kSvgCopy);
    btnSvgCopy->OnClick().Connect([status](UIElement*) { status->SetText("点击了：SVG 复制矢量按钮"); });

    // 纯图标按钮 (Icon Only)
    auto iconBtn1 = Make<Button>("");
    iconBtn1->SetIcon("🔍");
    iconBtn1->SetToolTip("搜索");
    iconBtn1->SetPadding(Thickness(8, 6, 8, 6));
    iconBtn1->OnClick().Connect([status](UIElement*) { status->SetText("点击了：纯图标按钮 (🔍)"); });

    auto iconBtn2 = Make<Button>("");
    iconBtn2->SetIcon("⚙️");
    iconBtn2->SetToolTip("设置");
    iconBtn2->SetPadding(Thickness(8, 6, 8, 6));
    iconBtn2->OnClick().Connect([status](UIElement*) { status->SetText("点击了：纯图标按钮 (⚙️)"); });

    auto iconBtnSvg = Make<Button>("");
    iconBtnSvg->SetIcon(kSvgStar);
    iconBtnSvg->SetToolTip("收藏 (SVG)");
    iconBtnSvg->SetPadding(Thickness(8, 6, 8, 6));
    iconBtnSvg->OnClick().Connect([status](UIElement*) { status->SetText("点击了：纯 SVG 矢量图标按钮 (⭐)"); });

    SamplePageSpec spec;
    spec.title = "Button(按钮)";
    spec.subtitle = "按钮用于触发操作。支持多种主题颜色、外形轮廓、圆角、尺寸及 Unicode/SVG 矢量图标。";
    spec.sections = {
        {
            "基本变体 (Variants)",
            "主要 Accent 按钮、标准卡片按钮、边框轮廓按钮、纯文本按钮与禁用状态。",
            Row(12).Children({ btnAccent, btnStandard, btnOutline, btnSubtle, btnDisabled }).Build(),
        },
        {
            "预设与自定义颜色 (Colors & Custom Themes)",
            "通过 SetBackground / SetHoverBackground 设置自定义品牌色彩。",
            Row(12).Children({ btnDanger, btnSuccess, btnWarning, btnPurple }).Build(),
        },
        {
            "圆角形态 (Corner Radius)",
            "通过 SetCornerRadius 改变外形：直角、小圆角、卡片圆角与完全胶囊型。",
            Row(12).Children({ btnSquare, btnRadius4, btnRadius8, btnCapsule }).Build(),
        },
        {
            "尺寸规格 (Sizes)",
            "提供小、中、大不同尺寸以及显式 Width/Height 固定的按钮规格。",
            Row(12).Children({ btnSmall, btnNormal, btnLarge, btnFixed }).Build(),
        },
        {
            "Unicode 与 SVG 矢量图标按钮 (Icons & SVG Vector Buttons)",
            "通过 SetIcon 可直接嵌入 Unicode 字符或标准 SVG 矢量图形。也支持无文字的纯图标按钮。",
            Column(12).Children({
                Row(12).Children({ btnUnicode, btnAdd, btnSvgHeart, btnSvgSearch, btnSvgCopy }).Build(),
                Row(12).Children({ MakeLabel("纯图标按钮:", 13.0f, ThemeTokenId::TextSecondary), iconBtn1, iconBtn2, iconBtnSvg }).Build(),
            }).Build(),
        },
        {
            "交互状态反馈",
            "点击任意上方按钮可触发事件响应并在下方更新提示。",
            Column(6).Children({ status }).Build(),
        },
    };
    spec.source =
        "// 1. Accent & Standard Buttons\n"
        "auto btn = Make<Button>(\"Accent Button\");\n"
        "btn->OnClick().Connect([](UIElement*) { /* action */ });\n\n"
        "// 2. Custom Color Button (Crimson)\n"
        "auto danger = Make<Button>(\"Danger\");\n"
        "danger->SetBackground(D2D1::ColorF(0xE53935));\n\n"
        "// 3. Capsule Corner Radius\n"
        "auto pill = Make<Button>(\"Pill\");\n"
        "pill->SetCornerRadius(20.0f);\n\n"
        "// 4. SVG Vector Icon Button\n"
        "auto svgBtn = Make<Button>(\"Like\");\n"
        "svgBtn->SetIcon(\"<svg>...</svg>\");\n";

    return BuildSamplePage(spec);
}

} // namespace Gallery
