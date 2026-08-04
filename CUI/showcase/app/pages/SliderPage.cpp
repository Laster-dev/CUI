#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI::DSL;

ShowcasePage BuildSliderPage(const ShowcaseContext& ctx) {
    auto target = SliderWidget(45.0f, 0.0f, 100.0f).Width(280).Height(24).Build();
    return { "Slider 滑块", CreatePage(
        "Slider 滑块控件",
        "支持水平与垂直方向、Step 步长约束、键盘上下左右微调与数值变动事件。",
        CreateDemoSurface({ target }, 0.0f),
        CreatePropertyGrid(ctx, target), target) };
}
