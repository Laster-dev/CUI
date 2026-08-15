#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/TextBlock.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

void StyleBox(std::shared_ptr<TextBlock> t) {
    t->BackgroundToken = ThemeTokenId::CardBackground;
    t->BorderToken = ThemeTokenId::CardBorder;
    t->BorderThickness = 1.0f;
    t->Padding = Thickness(8, 4, 8, 4);
    t->Margin = Thickness(0, 0, 0, 4);
}

} // namespace

std::shared_ptr<UIElement> BuildTextBlockPage() {
    auto title = Text("TextBlock 文本块").FontSize(22).FontWeight(FontWeight::Bold)
        .ColorToken(ThemeTokenId::TextPrimary).Build();
    auto subtitle = Text("用于显示少量或多行文本，支持字号、字重、颜色、对齐与行距控制。")
        .FontSize(13).ColorToken(ThemeTokenId::TextSecondary).Build();
    auto body = Text("正文使用默认 13px 微软雅黑并跟随主题前景色。TextBlock 是纯自绘的只读文本控件，"
                     "不参与输入焦点；文本改变后会自动重新测量并局部重绘。")
        .FontSize(13).Width(520).Build();

    auto accent = Text("强调文本 · 主题强调色").FontSize(15).FontWeight(FontWeight::SemiBold)
        .ColorToken(ThemeTokenId::AccentColor).Build();
    auto danger = Text("危险 / 警告 · DangerColor").FontSize(13)
        .ColorToken(ThemeTokenId::DangerColor).Build();

    auto underline = Text("带下划线的文本").FontSize(14).Underline().Build();
    auto strikethrough = Text("带删除线的文本").FontSize(14).Strikethrough().Build();
    auto italic = Text("斜体字形 Italic").FontSize(14).FontStyle(FontStyle::Italic).Build();
    auto code = Text("Consolas 等宽字体").FontSize(13).FontFamily("Consolas").Build();
    auto colored = Text("自定义颜色 #E68A00").FontSize(14).Color(Color::Hex("#E68A00")).Build();

    auto left = Text("左对齐 Left").FontSize(13).Width(240).Build();
    left->SetTextAlign(TextAlignment::Left);
    auto center = Text("居中对齐 Center").FontSize(13).Width(240).Build();
    center->SetTextAlign(TextAlignment::Center);
    auto right = Text("右对齐 Right").FontSize(13).Width(240).Build();
    right->SetTextAlign(TextAlignment::Right);
    StyleBox(left);
    StyleBox(center);
    StyleBox(right);

    auto multiline = Text();
    multiline->SetText("第一行：DirectWrite 自绘文本\n"
                       "第二行：显式 \\n 换行\n"
                       "第三行：行距 LineSpacing = 1.6");
    multiline->Width = 360.0f;
    multiline->SetLineSpacing(1.6f);
    multiline->FontSize = 13.0f;

    SamplePageSpec spec;
    spec.title = "TextBlock(文本块)";
    spec.subtitle = "显示少量或多行文本的只读控件，纯 DirectWrite 自绘排版。";
    spec.sections = {
        {
            "基础用法",
            "不同字号与字重，颜色通过 ThemeTokenId 跟随主题，浅色/深色自动适配。",
            Column(10, {
                title,
                subtitle,
                body,
                accent,
                danger,
            }),
        },
        {
            "修饰与配色",
            "下划线、删除线、斜体、等宽字体以及硬编码自定义颜色。",
            Column(10, {
                underline,
                strikethrough,
                italic,
                code,
                colored,
            }),
        },
        {
            "对齐方式",
            "SetTextAlign 支持 Left / Center / Right；行内垂直对齐由 SetVerticalAlign 控制。",
            Column(8, { left, center, right }),
        },
        {
            "多行与行距",
            "文本中的 \\n 显式换行；SetLineSpacing 调整行距，SetLineHeight 可强制固定行高。",
            multiline,
        },
    };
    spec.source =
        "auto t = Text(\"你好，世界\").FontSize(16).Build();\n"
        "t->ColorToken = ThemeTokenId::TextPrimary;\n"
        "t->SetTextAlign(TextAlignment::Center);\n"
        "t->SetLineSpacing(1.6f);\n"
        "t->Underline = true;\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
