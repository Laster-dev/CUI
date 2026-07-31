#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI::DSL;

ShowcasePage BuildToggleSwitchPage(const ShowcaseContext& ctx) {
    auto target = ToggleSwitchTile("启用系统 GPU 硬件加速", true).Build();
    auto target2 = ToggleSwitchTile("开启暗黑极客主题", false).Build();
    return { "ToggleSwitch 开关", CreatePage(
        "ToggleSwitch 现代化开关控件",
        "Win11/iOS 风格滑动开关，带 On/Off 平滑动画与色彩自定义。",
        CreateDemoSurface({ target, target2 }),
        CreatePropertyGrid(ctx, target)) };
}
