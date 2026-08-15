#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include <format>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

Element TypeScaleItem(const std::string& style, float size, CUI::FontWeight weight, const std::string& sample) {
    return Column(6, {
        MakeLabel(style + " · " + std::format("{:.0f}px", size), 11.0f, ThemeTokenId::TextMuted, false),
        Text(sample).FontSize(size).FontWeight(weight).FontFamily("微软雅黑").Build(),
    });
}

} // namespace

Element BuildTypographyPage() {
    auto spaced = Text("行距 1.8 的段落示例\n第二行文字\n第三行文字").FontSize(14.0f).FontFamily("微软雅黑").Build();
    spaced->SetLineSpacing(1.8f);

    auto italic = Text("Italic 斜体示例").FontStyle(CUI::FontStyle::Italic).FontSize(14.0f).FontFamily("微软雅黑").Build();
    auto underline = Text("下划线示例").Underline().FontSize(14.0f).FontFamily("微软雅黑").Build();
    auto strike = Text("删除线示例").Strikethrough().FontSize(14.0f).FontFamily("微软雅黑").Build();

    SamplePageSpec spec;
    spec.title = "Typography(字体排印)";
    spec.subtitle = "字号、字重、字体族与文本样式层级展示。";
    spec.sections = {
        {
            "字号层级",
            "从展示级标题到辅助说明的完整字号阶梯。",
            Column(18, {
                TypeScaleItem("Display", 28.0f, CUI::FontWeight::Bold, "预览字体排印 Display"),
                TypeScaleItem("Title", 22.0f, CUI::FontWeight::SemiBold, "预览字体排印 Title"),
                TypeScaleItem("Heading", 18.0f, CUI::FontWeight::SemiBold, "预览字体排印 Heading"),
                TypeScaleItem("Subtitle", 15.0f, CUI::FontWeight::Medium, "预览字体排印 Subtitle"),
                TypeScaleItem("Body", 14.0f, CUI::FontWeight::Normal, "预览字体排印 Body"),
                TypeScaleItem("Caption", 12.0f, CUI::FontWeight::Normal, "预览字体排印 Caption"),
                TypeScaleItem("Micro", 11.0f, CUI::FontWeight::Normal, "预览字体排印 Micro"),
            }),
        },
        {
            "字重",
            "微软雅黑支持的常用字重范围。",
            Column(10, {
                Text("Thin / Light 轻量字重").FontSize(14.0f).FontWeight(CUI::FontWeight::Light).FontFamily("微软雅黑").Build(),
                Text("Normal 常规字重").FontSize(14.0f).FontWeight(CUI::FontWeight::Normal).FontFamily("微软雅黑").Build(),
                Text("Medium 中等字重").FontSize(14.0f).FontWeight(CUI::FontWeight::Medium).FontFamily("微软雅黑").Build(),
                Text("SemiBold 半粗字重").FontSize(14.0f).FontWeight(CUI::FontWeight::SemiBold).FontFamily("微软雅黑").Build(),
                Text("Bold 粗体字重").FontSize(14.0f).FontWeight(CUI::FontWeight::Bold).FontFamily("微软雅黑").Build(),
            }),
        },
        {
            "字体族",
            "不同字体族的排版对比。",
            Column(10, {
                Text("微软雅黑 (Microsoft YaHei)").FontSize(15.0f).FontFamily("微软雅黑").Build(),
                Text("Segoe UI").FontSize(15.0f).FontFamily("Segoe UI").Build(),
                Text("Consolas - monospace").FontSize(14.0f).FontFamily("Consolas").Build(),
            }),
        },
        {
            "行距与文本样式",
            "TextBlock 支持行距、斜体、下划线与删除线。",
            Column(10, {
                MakeLabel("SetLineSpacing(1.8f)：", 12.0f, ThemeTokenId::TextMuted, false),
                spaced,
                MakeLabel("斜体 / 下划线 / 删除线：", 12.0f, ThemeTokenId::TextMuted, false),
                Row(16, { italic, underline, strike }),
            }),
        },
    };
    spec.source =
        "Text(sample)\n"
        "    .FontSize(18.0f)\n"
        "    .FontWeight(CUI::FontWeight::SemiBold)\n"
        "    .FontFamily(\"微软雅黑\")\n"
        "    .Build();\n"
        "text->SetLineSpacing(1.6f);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
