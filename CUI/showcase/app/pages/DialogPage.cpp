#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/MessageBox.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildDialogPage(const ShowcaseContext& ctx) {
    auto target = ElevatedButton("弹出 WinUI ContentDialog 消息框").Background("#007ACC").Padding(16, 8, 16, 8).Build();
    target->OnClick().Connect([window = ctx.windowRef](UIElement*) {
        ContentDialog::ShowMessageBox(window->GetRootElement().get(), "WinUI ContentDialog", "全盘 100% 纯 C++ 声明式 UI 完整回填生成。");
    });

    return { "ContentDialog 弹窗", CreatePage(
        "WinUI ContentDialog 模态弹窗控制台",
        "WinUI 3 风格半透明遮罩与自定义消息弹窗。",
        CreateDemoSurface({ target }, 0.0f),
        CreateRightPanel({ CreateShowcaseText("弹窗属性控制表 (ContentDialog)", 12.0f, "#569CD6", true) })) };
}
