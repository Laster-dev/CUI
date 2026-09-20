#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "PageRegistry.h"
#include "framework/core/Widgets.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;

using namespace CUI::DSL;

ShowcasePage BuildColorPickerPage(const ShowcaseContext& ctx) {
    auto target = Widgets::ColorPicker().Shared();
    return { "ColorPicker 调色板", CreatePage(
        "ColorPicker 调色板与色彩拾取控件",
        "内置调色板面板，支持已选颜色实时预览与变动事件。",
        CreateDemoSurface({ target }, 0.0f)) };
}
