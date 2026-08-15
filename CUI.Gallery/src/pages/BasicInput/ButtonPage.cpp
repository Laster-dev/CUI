#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/Button.h"
#include "framework/style/ThemeManager.h"
#include <memory>
#include <string>

using CUI::Color;
using CUI::Element;
using CUI::ThemeTokenId;
using CUI::UIElement;
using namespace CUI::DSL;
using CUI::DSL::Fluent::Button;

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

Element BuildButtonPage() {
    auto status = MakeStatus("准备就绪。尝试点击任意按钮...");

    // 1. 标准样式
    auto btnAccent = Button("主要按钮 (Accent)")
        .OnClick([status](UIElement*) { status->Text = "点击了：主要按钮 (Accent)"; });

    auto btnStandard = Button("标准按钮 (Standard)")
        .BackgroundToken(ThemeTokenId::CardBackground)
        .HoverBackgroundToken(ThemeTokenId::HoverBackground)
        .PressedBackgroundToken(ThemeTokenId::PressedBackground)
        .ColorToken(ThemeTokenId::TextPrimary)
        .BorderToken(ThemeTokenId::CardBorder)
        .OnClick([status](UIElement*) { status->Text = "点击了：标准按钮 (Standard)"; });

    auto btnOutline = Button("轮廓按钮 (Outline)")
        .BackgroundToken(ThemeTokenId::Unset)
        .HoverBackgroundToken(ThemeTokenId::HoverBackground)
        .PressedBackgroundToken(ThemeTokenId::PressedBackground)
        .ColorToken(ThemeTokenId::AccentColor)
        .BorderToken(ThemeTokenId::AccentColor, 1.0f)
        .OnClick([status](UIElement*) { status->Text = "点击了：轮廓按钮 (Outline)"; });

    auto btnSubtle = Button("幽灵/文本按钮 (Subtle)")
        .BackgroundToken(ThemeTokenId::Unset)
        .HoverBackgroundToken(ThemeTokenId::HoverBackground)
        .PressedBackgroundToken(ThemeTokenId::PressedBackground)
        .ColorToken(ThemeTokenId::AccentColor)
        .BorderToken(ThemeTokenId::Unset, 0.0f)
        .OnClick([status](UIElement*) { status->Text = "点击了：文本按钮 (Subtle)"; });

    auto btnDisabled = Button("禁用按钮 (Disabled)")
        .BackgroundToken(ThemeTokenId::CardBackground)
        .ColorToken(ThemeTokenId::TextMuted)
        .BorderToken(ThemeTokenId::CardBorder)
        .Enabled(false);
    // 2. 颜色与预设
    auto btnDanger = Button("危险操作 (Danger)")
        .Background("#E53935")
        .Hover("#D32F2F")
        .Pressed("#B71C1C")
        .Foreground(Color::White)
        .OnClick([status](UIElement*) { status->Text = "点击了：危险操作按钮 (Crimson)"; });

    auto btnSuccess = Button("成功状态 (Success)")
        .Background("#2E7D32")
        .Hover("#1B5E20")
        .Pressed("#0D3C10")
        .Foreground(Color::White)
        .OnClick([status](UIElement*) {
            status->Text = "点击了：成功状态按钮 (Emerald)";
        });

    auto btnWarning = Button("警告提示 (Warning)")
        .Background("#F57C00")
        .Hover("#E65100")
        .Pressed("#BF360C")
        .Foreground(Color::White)
        .OnClick([status](UIElement*) { status->Text = "点击了：警告提示按钮 (Amber)"; });

    auto btnPurple = Button("紫色梦幻 (Purple)")
        .Background("#7B1FA2")
        .Hover("#6A1B9A")
        .Pressed("#4A148C")
        .Foreground(Color::White)
        .OnClick([status](UIElement*) { status->Text = "点击了：紫色梦幻按钮 (Purple)"; });

    // 3. 各种圆角
    auto btnSquare = Button("直角方形 (r=0px)")
        .CornerRadius(0.0f)
        .OnClick([status](UIElement*) { status->Text = "点击了：直角方形按钮 (r=0px)"; });
    auto btnRadius4 = Button("小圆角 (r=4px)")
        .CornerRadius(4.0f)
        .OnClick([status](UIElement*) { status->Text = "点击了：小圆角按钮 (r=4px)"; });
    auto btnRadius8 = Button("中圆角 (r=8px)")
        .CornerRadius(8.0f)
        .OnClick([status](UIElement*) { status->Text = "点击了：中圆角按钮 (r=8px)"; });
    auto btnCapsule = Button("胶囊圆角 (r=20px)")
        .CornerRadius(20.0f)
        .Padding(16, 6, 16, 6)
        .OnClick([status](UIElement*) { status->Text = "点击了：胶囊圆角按钮 (r=20px)"; });

    // 4. 各种尺寸
    auto btnSmall = Button("小尺寸 (Small)")
        .FontSize(11.0f).Padding(6, 2, 6, 2)
        .OnClick([status](UIElement*) { status->Text = "点击了：小尺寸按钮"; });
    auto btnNormal = Button("标准尺寸 (Normal)")
        .FontSize(13.0f).Padding(12, 5, 12, 5)
        .OnClick([status](UIElement*) { status->Text = "点击了：标准尺寸按钮"; });
    auto btnLarge = Button("大尺寸 (Large)")
        .FontSize(16.0f).Padding(18, 9, 18, 9)
        .OnClick([status](UIElement*) { status->Text = "点击了：大尺寸按钮"; });
    auto btnFixed = Button("固定尺寸 (200 x 44)")
        .Size(200.0f, 44.0f)
        .OnClick([status](UIElement*) { status->Text = "点击了：固定尺寸按钮 (200x44)"; });

    // 5. 图标与 SVG 矢量按钮
    auto btnUnicode = Button("保存文件").Icon("💾")
        .OnClick([status](UIElement*) { status->Text = "点击了：Unicode 图标按钮 (💾 保存)"; });
    auto btnAdd = Button("新建项目").Icon("➕")
        .OnClick([status](UIElement*) { status->Text = "点击了：Unicode 图标按钮 (➕ 新建)"; });
    auto btnSvgHeart = Button("点赞").Icon(kSvgHeart)
        .Background("#E91E63").Hover("#D81B60").Pressed("#C2185B").Foreground(Color::White)
        .OnClick([status](UIElement*) { status->Text = "点击了：SVG 心形矢量按钮"; });
    auto btnSvgSearch = Button("搜索文档").Icon(kSvgSearch)
        .OnClick([status](UIElement*) { status->Text = "点击了：SVG 搜索矢量按钮"; });
    auto btnSvgCopy = Button("复制文本").Icon(kSvgCopy)
        .OnClick([status](UIElement*) { status->Text = "点击了：SVG 复制矢量按钮"; });

    // 纯图标按钮 (Icon Only)
    auto iconBtn1 = Button().Icon("🔍").ToolTip("搜索").Padding(8, 6, 8, 6)
        .OnClick([status](UIElement*) { status->Text = "点击了：纯图标按钮 (🔍)"; });
    auto iconBtn2 = Button().Icon("⚙️").ToolTip("设置").Padding(8, 6, 8, 6)
        .OnClick([status](UIElement*) { status->Text = "点击了：纯图标按钮 (⚙️)"; });
    auto iconBtnSvg = Button().Icon(kSvgStar).ToolTip("收藏 (SVG)").Padding(8, 6, 8, 6)
        .OnClick([status](UIElement*) { status->Text = "点击了：纯 SVG 矢量图标按钮 (⭐)"; });

    SamplePageSpec spec;
    spec.title = "Button(按钮)";
    spec.subtitle = "按钮用于触发操作。支持多种主题颜色、外形轮廓、圆角、尺寸及 Unicode/SVG 矢量图标。";
    spec.sections = {
        {
            "基本变体 (Variants)",
            "主要 Accent 按钮、标准卡片按钮、边框轮廓按钮、纯文本按钮与禁用状态。",
            Row(12, { btnAccent, btnStandard, btnOutline, btnSubtle, btnDisabled }),
        },
        {
            "预设与自定义颜色 (Colors & Custom Themes)",
            "通过 Background / Hover / Pressed 设置自定义品牌色彩。",
            Row(12, { btnDanger, btnSuccess, btnWarning, btnPurple }),
        },
        {
            "圆角形态 (Corner Radius)",
            "通过 CornerRadius 改变外形：直角、小圆角、卡片圆角与完全胶囊型。",
            Row(12, { btnSquare, btnRadius4, btnRadius8, btnCapsule }),
        },
        {
            "尺寸规格 (Sizes)",
            "提供小、中、大不同尺寸以及显式 Width/Height 固定的按钮规格。",
            Row(12, { btnSmall, btnNormal, btnLarge, btnFixed }),
        },
        {
            "Unicode 与 SVG 矢量图标按钮 (Icons & SVG Vector Buttons)",
            "通过 Icon 可直接嵌入 Unicode 字符或标准 SVG 矢量图形。也支持无文字的纯图标按钮。",
            Column(12, {
                Row(12, { btnUnicode, btnAdd, btnSvgHeart, btnSvgSearch, btnSvgCopy }),
                Row(12, { MakeLabel("纯图标按钮:", 13.0f, ThemeTokenId::TextSecondary), iconBtn1, iconBtn2, iconBtnSvg }),
            }),
        },
        {
            "交互状态反馈",
            "点击任意上方按钮可触发事件响应并在下方更新提示。",
            Column(6, { status }),
        },
    };
    spec.source =
        "// 1. Accent & Standard Buttons\n"
        "auto btn = Button(\"Accent Button\")\n"
        "    .OnClick([](UIElement*) { /* action */ });\n\n"
        "// 2. Custom Color Button (Crimson)\n"
        "auto danger = Button(\"Danger\")\n"
        "    .Background(\"#E53935\");\n\n"
        "// 3. Capsule Corner Radius\n"
        "auto pill = Button(\"Pill\")\n"
        "    .CornerRadius(20.0f);\n\n"
        "// 4. SVG Vector Icon Button\n"
        "auto svgBtn = Button(\"Like\")\n"
        "    .Icon(\"<svg>...</svg>\");\n";

    return BuildSamplePage(spec);
}

} // namespace Gallery
