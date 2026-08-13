#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI::DSL;

ShowcasePage BuildColorPickerPage(const ShowcaseContext& ctx) {
    auto target = ColorPickerWidget().Build();
    return { "ColorPicker 调色板", CreatePage(
        "ColorPicker 调色板与色彩拾取控件",
        "内置调色板面板，支持已选颜色实时预览与变动事件。",
        CreateDemoSurface({ target }, 0.0f)) };
}
