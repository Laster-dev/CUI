#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI::DSL;

ShowcasePage BuildToggleSwitchPage(const ShowcaseContext& ctx) {
    auto target = CUI::Widgets::ToggleSwitch().Header("启用系统 GPU 硬件加速").IsOn(true).Shared();
    auto target2 = CUI::Widgets::ToggleSwitch().Header("开启暗黑极客主题").IsOn(false).Shared();
    return { "ToggleSwitch 开关", CreatePage(
        "ToggleSwitch 现代化开关控件",
        "Win11/iOS 风格滑动开关，带 On/Off 平滑动画与色彩自定义。",
        CreateDemoSurface({ target, target2 })) };
}
