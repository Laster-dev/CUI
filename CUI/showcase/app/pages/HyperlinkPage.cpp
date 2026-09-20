#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "PageRegistry.h"
#include "framework/core/Widgets.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;

using namespace CUI::DSL;

ShowcasePage BuildHyperlinkPage(const ShowcaseContext& ctx) {
    auto target = Widgets::HyperlinkButton("访问 GitHub 官方主页", "https://github.com").Shared();
    return { "Hyperlink 超链接", CreatePage(
        "HyperlinkButton 超链接按钮全属性控制台",
        "HyperlinkButton 打开链接。",
        CreateDemoSurface({ target }, 0.0f)) };
}
