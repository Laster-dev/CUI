#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI::DSL;

ShowcasePage BuildSplitterPage(const ShowcaseContext& ctx) {
    // Vertical orientation = vertical bar = drag left/right (水平拆分面板).
    // Horizontal orientation = horizontal bar = drag up/down (垂直拆分面板).
    auto splitterLR = SplitterWidget(CUI::Orientation::Vertical).Build();
    auto splitterTB = SplitterWidget(CUI::Orientation::Horizontal).Build();

    auto demo = CreateDemoSurface({
        CreateShowcaseText("1. 水平拆分（左右面板，竖向分割条）:", 12.0f, "#AAAAAA"),
        Row().Height(220).Background("#1E1E1E").CornerRadius(4).Border("#333333", 1).Children({
            Column(8).Width(220).MinWidth(80).Background("#252526").Padding(12).Children({
                CreateShowcaseText("[左侧文件树面板]", 12.0f, "#007ACC", true),
                CreateShowcaseText("• main.cpp", 11.0f, "#CCCCCC"),
                CreateShowcaseText("• CUIDsl.h / typed setters", 11.0f, "#CCCCCC")
            }).Build(),
            splitterLR,
            Column(8).FlexGrow(1.0f).MinWidth(100).Background("#1E1E1E").Padding(12).Children({
                CreateShowcaseText("[右侧主编辑区]", 12.0f, "#4EC9B0", true),
                CreateShowcaseText("拖拽中间竖向 Splitter，左右面板宽度联动。", 11.0f, "#888888")
            }).Build()
        }).Build(),
        CreateShowcaseText("2. 垂直拆分（上下面板，横向分割条）:", 12.0f, "#AAAAAA"),
        Column(0).Height(280).Background("#1E1E1E").CornerRadius(4).Border("#333333", 1).Children({
            Column(4).Height(130).MinHeight(60).Background("#252526").Padding(12).Children({
                CreateShowcaseText("[上方代码编辑视口]", 12.0f, "#D16969", true),
                CreateShowcaseText("1  #include <iostream>", 11.0f, "#6A9955"),
                CreateShowcaseText("2  int main() { return 0; }", 11.0f, "#DCDCAA")
            }).Build(),
            splitterTB,
            Column(4).FlexGrow(1.0f).MinHeight(60).Background("#1E1E1E").Padding(12).Children({
                CreateShowcaseText("[下方集成终端/控制台]", 12.0f, "#CE9178", true),
                CreateShowcaseText("Build succeeded. 0 Errors, 0 Warnings.", 11.0f, "#4EC9B0")
            }).Build()
        }).Build()
    });

    return { "Splitter 拆分条", CreatePage(
        "Splitter / GridSplitter 可拖拽拆分条控件",
        "Vertical=左右拖；Horizontal=上下拖。按住分割条拖拽即可联动面板尺寸。",
        demo,
        CreatePropertyGrid(ctx, splitterLR),
        splitterLR) };
}
