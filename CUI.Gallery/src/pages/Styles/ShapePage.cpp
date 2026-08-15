#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/style/ThemeManager.h"
#include <format>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildShapePage() {
    ThemeManager& tm = ThemeManager::Instance();
    const D2D1_COLOR_F accent = tm.GetFlatColor(ThemeTokenId::AccentColor);
    const D2D1_COLOR_F cardBg = tm.GetFlatColor(ThemeTokenId::CardBackground);
    const D2D1_COLOR_F cardBorder = tm.GetFlatColor(ThemeTokenId::CardBorder);

    // 圆角矩形阶梯
    auto radiusRow = Row(16, {});
    for (float r : { 0.0f, 2.0f, 4.0f, 8.0f, 12.0f, 16.0f }) {
        auto rect = RectangleWidget(88.0f, 56.0f);
        rect->SetCornerRadius(r);
        rect->Fill = cardBg;
        rect->Stroke = cardBorder;
        rect->StrokeThickness = 1.0f;
        radiusRow->AddChild(Column(6, {
            rect,
            MakeLabel(std::format("半径 {:.0f}", r), 11.0f, ThemeTokenId::TextMuted, false),
        }));
    }

    // 圆形与椭圆
    auto circle = EllipseWidget(56.0f, 56.0f);
    circle->Fill = accent;
    auto ellipse = EllipseWidget(88.0f, 48.0f);
    ellipse->Fill = cardBg;
    ellipse->Stroke = accent;
    ellipse->StrokeThickness = 1.5f;
    auto hollow = EllipseWidget(56.0f, 56.0f);
    hollow->Fill = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f);
    hollow->Stroke = cardBorder;
    hollow->StrokeThickness = 1.0f;

    // 直线与描边粗细
    auto line1 = LineWidget(0.0f, 0.0f, 140.0f, 0.0f);
    line1->Stroke = accent;
    line1->StrokeThickness = 1.0f;
    line1->Width = 140.0f;
    line1->Height = 8.0f;
    auto line2 = LineWidget(0.0f, 0.0f, 140.0f, 0.0f);
    line2->Stroke = accent;
    line2->StrokeThickness = 2.5f;
    line2->Width = 140.0f;
    line2->Height = 8.0f;
    auto line3 = LineWidget(0.0f, 0.0f, 140.0f, 0.0f);
    line3->Stroke = accent;
    line3->StrokeThickness = 4.0f;
    line3->Width = 140.0f;
    line3->Height = 8.0f;

    // 矢量路径
    auto triangle = PathWidget("M 6 30 L 30 6 L 54 30 Z");
    triangle->SetWidth(60.0f);
    triangle->SetHeight(36.0f);
    triangle->Fill = accent;
    auto heart = PathWidget("M 10 28 A 18 18 0 0 1 46 28 A 18 18 0 0 1 82 28 Q 82 54 46 82 Q 10 54 10 28 Z");
    heart->SetWidth(92.0f);
    heart->SetHeight(88.0f);
    heart->Fill = accent;

    auto starSvg = SvgIconWidget(
        "<svg viewBox=\"0 0 24 24\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<path d=\"M12 2 L15 9 L22 9.3 L16.7 14 L18.3 21 L12 17.3 L5.7 21 L7.3 14 L2 9.3 L9 9 Z\"/>"
        "</svg>");
    starSvg->SetWidth(48.0f);
    starSvg->SetHeight(48.0f);
    starSvg->SetTintColor(accent);

    SamplePageSpec spec;
    spec.title = "Shape(形状与圆角)";
    spec.subtitle = "圆角矩形、椭圆、直线与矢量路径，以及填充和描边规范。";
    spec.sections = {
        {
            "圆角矩形",
            "Rectangle 通过 SetCornerRadius 控制四角圆角半径。",
            radiusRow,
        },
        {
            "圆形与椭圆",
            "Ellipse 在包络边界内内切绘制，支持填充与描边。",
            Row(24, {
                Column(6, { circle, MakeLabel("实心圆", 11.0f, ThemeTokenId::TextMuted, false) }),
                Column(6, { ellipse, MakeLabel("填充 + 描边", 11.0f, ThemeTokenId::TextMuted, false) }),
                Column(6, { hollow, MakeLabel("仅描边", 11.0f, ThemeTokenId::TextMuted, false) }),
            }),
        },
        {
            "直线",
            "Line 支持自定义 Stroke 颜色与 StrokeThickness 粗细。",
            Column(10, {
                line1,
                line2,
                line3,
            }),
        },
        {
            "矢量路径与 SVG",
            "Path 使用 SVG Path 语法；SvgIcon 支持 TintColor 主题着色。",
            Row(24, {
                Column(6, { triangle, MakeLabel("Path 三角形", 11.0f, ThemeTokenId::TextMuted, false) }),
                Column(6, { heart, MakeLabel("Path 心形", 11.0f, ThemeTokenId::TextMuted, false) }),
                Column(6, { starSvg, MakeLabel("SvgIcon 着色", 11.0f, ThemeTokenId::TextMuted, false) }),
            }),
        },
    };
    spec.source =
        "auto rect = RectangleWidget(88.0f, 56.0f);\n"
        "rect->SetCornerRadius(8.0f);\n"
        "rect->Fill = accent;\n"
        "rect->Stroke = cardBorder;\n"
        "rect->StrokeThickness = 1.0f;\n"
        "auto circle = EllipseWidget(56.0f, 56.0f);\n"
        "circle->Fill = accent;\n"
        "auto line = LineWidget(0.0f, 0.0f, 140.0f, 0.0f);\n"
        "line->Stroke = accent;\n"
        "line->StrokeThickness = 2.0f;\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
