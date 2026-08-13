#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/Toast.h"
#include "framework/controls/TextBlock.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildSegmentedPage(const ShowcaseContext& ctx) {
    auto target = SegmentedWidget({ "规则", "全局", "直连" }).Width(280).Height(32).Build();

    auto status = std::static_pointer_cast<TextBlock>(
        CreateShowcaseText("当前：规则", 12.0f, "textSecondary", false));
    target->OnSelectionChanged().Connect([window = ctx.windowRef, status](SegmentedControl*, int, const std::string& item) {
        status->SetText("当前：" + item);
        Toast::Show(window->GetRootElement().get(), "SegmentedControl", item, ToastCorner::BottomRight, 1400);
    });

    auto compact = SegmentedWidget({ "日", "周", "月", "年" }).Width(240).Height(28).Build();
    compact->SetSelectedIndex(1);

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
        demo,
        CreatePropertyGrid(ctx, target), target) };
}
