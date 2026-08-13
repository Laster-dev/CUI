#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/AutoSuggestBox.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/Toast.h"
#include "framework/style/ThemeManager.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildAutoSuggestPage(const ShowcaseContext& ctx) {
    auto box = std::make_shared<AutoSuggestBox>();
    box->SetWidth(320.0f);
    box->SetPlaceholder("搜索水果…");
    box->SetSuggestionItems({
        "苹果 Apple",
        "香蕉 Banana",
        "樱桃 Cherry",
        "葡萄 Grape",
        "芒果 Mango",
        "橙子 Orange",
        "梨 Pear",
        "桃子 Peach",
        "草莓 Strawberry",
        "西瓜 Watermelon",
        "蓝莓 Blueberry",
        "菠萝 Pineapple",
        "柠檬 Lemon",
        "椰子 Coconut",
        "猕猴桃 Kiwi",
    });

    auto status = std::static_pointer_cast<TextBlock>(
        CreateShowcaseText("输入关键字过滤建议；↑↓ 选择，Enter 确认，Esc 关闭。", 12.0f, "textSecondary", false));

    box->OnTextChanged().Connect([status](AutoSuggestBox*, const std::string& text) {
        status->SetText(text.empty() ? "输入关键字过滤建议；↑↓ 选择，Enter 确认，Esc 关闭。"
                                     : ("正在输入: " + text));
    });
    box->OnSuggestionChosen().Connect([window = ctx.windowRef, status](AutoSuggestBox*, const std::string& item) {
        status->SetText("已选择: " + item);
        Toast::Show(window->GetRootElement().get(), "AutoSuggestBox", "选择了 " + item, ToastCorner::BottomRight, 1600);
    });
    box->OnQuerySubmitted().Connect([window = ctx.windowRef, status](AutoSuggestBox*, const std::string& q) {
        status->SetText("提交查询: " + q);
        Toast::Show(window->GetRootElement().get(), "AutoSuggestBox", "提交: " + (q.empty() ? "(空)" : q), ToastCorner::BottomRight, 1600);
    });

    auto demo = Column(12).Children({
        CreateDemoSurface({
            CreateShowcaseText("TextBox 输入 + 自绘建议层", 13.0f, "textPrimary", true),
            box,
            status
        }, 10.0f),
        CreateShowcaseText("空查询时按 ↓ 可展开完整目录。", 11.0f, "textMuted", false)
    }).Build();

    return { "AutoSuggestBox 搜索建议", CreatePage(
        "AutoSuggestBox / SearchBox",
        "输入复用 TextBox；建议弹出层自绘，不嵌套 ListBox。",
        demo,
        CreatePropertyGrid(ctx, box), box) };
}
