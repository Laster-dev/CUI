#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 CUI::Widgets:: 句柄同名冲突
#endif
#include "Gallery.h"
#include "framework/core/Widgets.h"
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
        CUI::Widgets::Ref rect = CUI::Widgets::Rectangle().Width(88.0f).Height(56.0f).Shared();
                rect.CornerRadius(r);
        rect->Fill = cardBg;
        rect->Stroke = cardBorder;
        rect->StrokeThickness = 1.0f;
                radiusRow->AddChild(Column(6, {
            rect,
            MakeLabel(std::format("半径 {:.0f}", r), 11.0f, ThemeTokenId::TextMuted, false),
        }));
    }

    // 圆形与椭圆
    CUI::Widgets::Ref circle = CUI::Widgets::Ellipse().Width(56.0f).Height(56.0f).Shared();
    circle->Fill = accent;
    CUI::Widgets::Ref ellipse = CUI::Widgets::Ellipse().Width(88.0f).Height(48.0f).Shared();
    ellipse->Fill = cardBg;
    ellipse->Stroke = accent;
    ellipse->StrokeThickness = 1.5f;
    CUI::Widgets::Ref hollow = CUI::Widgets::Ellipse().Width(56.0f).Height(56.0f).Shared();
    hollow->Fill = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f);
    hollow->Stroke = cardBorder;
    hollow->StrokeThickness = 1.0f;

    // 直线与描边粗细
    CUI::Widgets::Ref line1 = CUI::Widgets::Line().X1(0.0f).Y1(0.0f).X2(140.0f).Y2(0.0f).Shared();
    line1->Stroke = accent;
    line1->StrokeThickness = 1.0f;
        line1.Width(140.0f);
        line1.Height(8.0f);
    CUI::Widgets::Ref line2 = CUI::Widgets::Line().X1(0.0f).Y1(0.0f).X2(140.0f).Y2(0.0f).Shared();
    line2->Stroke = accent;
    line2->StrokeThickness = 2.5f;
        line2.Width(140.0f);
        line2.Height(8.0f);
    CUI::Widgets::Ref line3 = CUI::Widgets::Line().X1(0.0f).Y1(0.0f).X2(140.0f).Y2(0.0f).Shared();
    line3->Stroke = accent;
    line3->StrokeThickness = 4.0f;
        line3.Width(140.0f);
        line3.Height(8.0f);

    // 矢量路径
    CUI::Widgets::Ref triangle = CUI::Widgets::Path("M 6 30 L 30 6 L 54 30 Z").Shared();
        triangle.Width(60.0f);
        triangle.Height(36.0f);
    triangle->Fill = accent;
    CUI::Widgets::Ref heart = CUI::Widgets::Path("M 10 28 A 18 18 0 0 1 46 28 A 18 18 0 0 1 82 28 Q 82 54 46 82 Q 10 54 10 28 Z").Shared();
        heart.Width(92.0f);
        heart.Height(88.0f);
    heart->Fill = accent;

    CUI::Widgets::Ref starSvg = CUI::Widgets::SvgIcon(
        "<svg viewBox=\"0 0 24 24\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<path d=\"M12 2 L15 9 L22 9.3 L16.7 14 L18.3 21 L12 17.3 L5.7 21 L7.3 14 L2 9.3 L9 9 Z\"/>"
        "</svg>").Shared();
        starSvg.Width(48.0f);
        starSvg.Height(48.0f);
    starSvg.TintColor(accent);

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
        "auto rect = CUI::Widgets::Rectangle(88.0f, 56.0f).Shared();\n"
        "rect.CornerRadius(8.0f);\n"
        "rect->Fill = accent;\n"
        "rect->Stroke = cardBorder;\n"
        "rect->StrokeThickness = 1.0f;\n"
        "auto circle = CUI::Widgets::Ellipse(56.0f, 56.0f).Shared();\n"
        "circle->Fill = accent;\n"
        "auto line = CUI::Widgets::Line(0.0f, 0.0f, 140.0f, 0.0f).Shared();\n"
        "line->Stroke = accent;\n"
        "line->StrokeThickness = 2.0f;\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery




