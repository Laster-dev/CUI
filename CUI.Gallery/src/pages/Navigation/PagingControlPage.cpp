#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/PagingControl.h"
#include <format>
#include <memory>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildPagingControlPage() {
    // ---------- 1. 常规用法 ----------
    auto paging1 = std::make_shared<PagingControl>();
    paging1->SetTotalPages(10);
    paging1->SetCurrentPage(1);

    auto status1 = MakeStatus("当前第 1 / 10 页。支持点击页码、左右 Chevron、滚轮与 ← → 键。");
    paging1->OnPageChanged().Connect([status1](PagingControl* sender, int page) {
        status1->Text = std::format("当前第 {} / {} 页。", page, sender->GetTotalPages());
    });

    auto btnPrev = ElevatedButton("上一页", [paging1](UIElement*) {
        paging1->SetCurrentPage(paging1->GetCurrentPage() - 1);
    }).Build();
    auto btnNext = ElevatedButton("下一页", [paging1](UIElement*) {
        paging1->SetCurrentPage(paging1->GetCurrentPage() + 1);
    }).Build();
    auto btnJump3 = ElevatedButton("跳转到第 3 页", [paging1, status1](UIElement*) {
        paging1->SetCurrentPage(3);
        status1->Text = "已程序化跳转到第 3 页。";
    }).Build();
    auto btnLast = ElevatedButton("最后一页", [paging1](UIElement*) {
        paging1->SetCurrentPage(paging1->GetTotalPages());
    }).Build();

    // ---------- 2. 大总数分页（省略号窗口） ----------
    auto paging2 = std::make_shared<PagingControl>();
    paging2->SetTotalPages(100);
    paging2->SetCurrentPage(42);

    auto status2 = MakeStatus("当前第 42 / 100 页。页码过多时中间折叠为 …，点击首尾页码快速跳转。");
    paging2->OnPageChanged().Connect([status2](PagingControl* sender, int page) {
        status2->Text = std::format("当前第 {} / {} 页（省略号窗口自适应）。", page, sender->GetTotalPages());
    });

    auto btnMid = ElevatedButton("跳到第 50 页", [paging2](UIElement*) { paging2->SetCurrentPage(50); }).Build();
    auto btnBegin = ElevatedButton("回到第 1 页", [paging2](UIElement*) { paging2->SetCurrentPage(1); }).Build();
    auto btnEnd = ElevatedButton("跳到第 100 页", [paging2](UIElement*) { paging2->SetCurrentPage(100); }).Build();

    // ---------- 3. 动态配置 ----------
    auto paging3 = std::make_shared<PagingControl>();
    paging3->SetTotalPages(20);
    paging3->SetCurrentPage(5);

    auto status3 = MakeStatus("当前第 5 / 20 页。通过按钮或数值框动态调整总页数与当前页。");
    paging3->OnPageChanged().Connect([status3](PagingControl* sender, int page) {
        status3->Text = std::format("当前第 {} / {} 页。", page, sender->GetTotalPages());
    });

    auto totalBox = NumberBoxWidget(20.0).Build();
    totalBox->SetWidth(140.0f);
    totalBox->SetMinimum(1.0f);
    totalBox->SetMaximum(200.0f);
    totalBox->SetStep(1.0f);
    totalBox->OnValueChanged().Connect([paging3, status3](NumberBox* box, float value) {
        const int total = static_cast<int>(value + 0.5f);
        paging3->SetTotalPages(total);
        status3->Text = std::format("总页数已更新为 {}（当前第 {} 页）。", total, paging3->GetCurrentPage());
    });

    auto pageBox = NumberBoxWidget(5.0).Build();
    pageBox->SetWidth(140.0f);
    pageBox->SetMinimum(1.0f);
    pageBox->SetMaximum(200.0f);
    pageBox->SetStep(1.0f);
    pageBox->OnValueChanged().Connect([paging3](NumberBox*, float value) {
        paging3->SetCurrentPage(static_cast<int>(value + 0.5f));
    });

    auto btnTotal10 = ElevatedButton("总页数 10", [paging3](UIElement*) { paging3->SetTotalPages(10); }).Build();
    auto btnTotal50 = ElevatedButton("总页数 50", [paging3](UIElement*) { paging3->SetTotalPages(50); }).Build();
    auto btnTotal200 = ElevatedButton("总页数 200", [paging3](UIElement*) { paging3->SetTotalPages(200); }).Build();

    SamplePageSpec spec;
    spec.title = "PagingControl (分页控件)";
    spec.subtitle = "Fluent 风格自绘分页条：Chevron 导航、滑动选中胶囊、省略号窗口、滚轮 / 键盘切换与动态总页数。";
    spec.sections = {
        {
            "常规用法 — 页码导航",
            "1. 点击页码 / 左右 Chevron / 滚轮 / ← → 方向键均可切换；\n"
            "2. 选中胶囊在页码间平滑滑动；OnPageChanged 回调当前页；\n"
            "3. 首尾页自动禁用对应方向的 Chevron。",
            Column(12, {
                Row(8, { btnPrev, btnNext, btnJump3, btnLast }),
                paging1,
                status1,
            }),
        },
        {
            "大总数分页 — 省略号窗口",
            "总页数很多时，中间页码折叠为 … 省略号，首尾与当前页附近保留可点击页码，点击省略号附近的首尾页快速跳转。",
            Column(12, {
                Row(8, { btnBegin, btnMid, btnEnd }),
                paging2,
                status2,
            }),
        },
        {
            "动态配置 — 总页数 / 当前页",
            "1. SetTotalPages / SetCurrentPage 运行时更新；\n"
            "2. NumberBox 直接输入总页数或目标页码，越界自动钳制。",
            Column(12, {
                Row(8, { btnTotal10, btnTotal50, btnTotal200 }),
                Row(8, { Text("总页数:"), totalBox, Text("跳转页码:"), pageBox }),
                paging3,
                status3,
            }),
        },
    };

    spec.source = R"(
// 1) 创建分页条
auto paging = std::make_shared<PagingControl>();
paging->SetTotalPages(10);
paging->SetCurrentPage(1);

// 2) 页码变更事件
paging->OnPageChanged().Connect([](PagingControl* sender, int page) {
    // page 为新的当前页码
});

// 3) 程序化控制
paging->SetCurrentPage(3);
paging->SetTotalPages(100);
)";

    return BuildSamplePage(spec);
}

} // namespace Gallery
