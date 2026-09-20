#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "PageRegistry.h"
#include "framework/core/Widgets.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/Toast.h"
#include "framework/controls/TextBlock.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildSegmentedPage(const ShowcaseContext& ctx) {
    CUI::Widgets::Ref target = CUI::Widgets::SegmentedControl().Width(280).Height(32).Shared();
    target->AddItem("规则");
    target->AddItem("全局");
    target->AddItem("直连");

    CUI::Widgets::Ref status =std::static_pointer_cast<TextBlock>(
        CreateShowcaseText("当前：规则", 12.0f, "textSecondary", false));
    target->OnSelectionChanged().Connect([window = ctx.windowRef, status](SegmentedControl*, int, const std::string& item) {
                status.Text("当前：" + item);
        Toast::Show(window->GetRootElement().get(), "SegmentedControl", item, ToastCorner::BottomRight, 1400);
    });

    CUI::Widgets::Ref compact = CUI::Widgets::SegmentedControl().Width(240).Height(28).Shared();
    compact->AddItem("日");
    compact->AddItem("周");
    compact->AddItem("月");
    compact->AddItem("年");
        compact.SelectedIndex(1);

    auto demo = Column(12).Children({
        CreateDemoSurface({
            CreateShowcaseText("代理模式（多选一）", 12.0f, "textSecondary", true),
            target,
            status
        }, 8.0f),
        CreateDemoSurface({
            CreateShowcaseText("时间粒度", 12.0f, "textSecondary", true),
            compact
        }, 8.0f)
    }).Build();

    return { "SegmentedControl 分段", CreatePage(
        "SegmentedControl 分段选择",
        "ComboBox 的平铺变体：同一条上的互斥分段，选中项填强调色。左右方向键切换。",
        demo) };
}
