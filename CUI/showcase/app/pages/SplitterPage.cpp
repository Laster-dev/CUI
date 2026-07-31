#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI::DSL;

ShowcasePage BuildSplitterPage(const ShowcaseContext& ctx) {
    auto splitterH = SplitterWidget(CUI::Orientation::Horizontal).Width(6).Build();
    auto splitterV = SplitterWidget(CUI::Orientation::Vertical).Height(6).Build();
    auto demo = CreateDemoSurface({
        CreateShowcaseText("1. 水平（左右）面板可拖拽拆分:", 12.0f, "#AAAAAA"),
        Row().Height(180).Background("#1E1E1E").CornerRadius(4).Border("#333333", 1).Children({
            Column(8).Width(180).Background("#252526").Padding(12).Children({
                CreateShowcaseText("[左侧文件树面板]", 12.0f, "#007ACC", true),
                CreateShowcaseText("• main.cpp", 11.0f, "#CCCCCC"),
                CreateShowcaseText("• gallery_layout.xml", 11.0f, "#CCCCCC")
            }).Build(),
            splitterH,
            Column(8).FlexGrow(1.0f).Background("#1E1E1E").Padding(12).Children({
                CreateShowcaseText("[右侧主编辑区]", 12.0f, "#4EC9B0", true),
                CreateShowcaseText("拖拽中间 Splitter，可实时改变左右面板尺寸。", 11.0f, "#888888")
            }).Build()
        }).Build(),
        CreateShowcaseText("2. 垂直（上下）面板可拖拽拆分:", 12.0f, "#AAAAAA"),
        Column(0).Height(220).Background("#1E1E1E").CornerRadius(4).Border("#333333", 1).Children({
            Column(4).Height(110).Background("#252526").Padding(12).Children({
                CreateShowcaseText("[上方代码编辑视口]", 12.0f, "#D16969", true),
                CreateShowcaseText("1  #include <iostream>", 11.0f, "#6A9955"),
                CreateShowcaseText("2  int main() { return 0; }", 11.0f, "#DCDCAA")
            }).Build(),
            splitterV,
            Column(4).FlexGrow(1.0f).Background("#1E1E1E").Padding(12).Children({
                CreateShowcaseText("[下方集成终端/控制台]", 12.0f, "#CE9178", true),
                CreateShowcaseText("Build succeeded. 0 Errors, 0 Warnings.", 11.0f, "#4EC9B0")
            }).Build()
        }).Build()
    });

    return { "Splitter 拆分条", CreatePage(
        "Splitter / GridSplitter 可拖拽拆分条控件",
        "支持水平/垂直方向动态比例拆分。按住中轴线拖拽，左右/上下面板尺寸动态响应联动！",
        demo,
        CreatePropertyGrid(ctx, splitterH)) };
}
