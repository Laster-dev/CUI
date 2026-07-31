#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI::DSL;

ShowcasePage BuildHyperlinkPage(const ShowcaseContext&) {
    auto target = HyperlinkButtonWidget("访问 GitHub 官方主页", "https://github.com").Build();
    return { "Hyperlink 超链接", CreatePage(
        "HyperlinkButton 超链接按钮全属性控制台",
        "带有 Hover 下划线特效与网页 URL 导航交互。",
        CreateDemoSurface({ target }, 0.0f),
        CreateRightScrollPanel({
            CreateShowcaseText("超链接属性控制表 (Hyperlink)", 12.0f, "#569CD6", true),
            CreateShowcaseText("跳转网址 (NavigateUri):", 11.0f, "#AAAAAA"),
            TextField("https://github.com").Width(280).Height(26).Build(),
            CreateShowcaseText("显示文本 (Text):", 11.0f, "#AAAAAA"),
            TextField("访问 GitHub 官方主页").Width(280).Height(26).Build(),
            CheckboxTile("是否启用 (IsEnabled)").Build()
        })) };
}
