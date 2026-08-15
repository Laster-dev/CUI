#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/chart/Chart.h"
#include <format>
#include <memory>
#include <random>
#include <string>
#include <vector>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

std::string FormatHover(ChartBase* chart, int index, int series) {
    if (!chart || index < 0) {
        return "悬停柱体或按 ← → 方向键读值";
    }
    const auto& cats = chart->GetCategories();
    const auto& seriesList = chart->GetSeries();
    const std::string cat = (index < static_cast<int>(cats.size()) && !cats[index].empty())
        ? cats[index] : std::format("{}", index + 1);
    std::string s = std::format("{}", cat);
    for (int i = 0; i < static_cast<int>(seriesList.size()); ++i) {
        if (series >= 0 && i != series) continue;
        if (index >= static_cast<int>(seriesList[i].values.size())) continue;
        const std::string name = seriesList[i].name.empty()
            ? std::format("系列{}", i + 1) : seriesList[i].name;
        s += std::format("  ·  {} {}", name, FormatChartNumber(seriesList[i].values[index]));
    }
    return s;
}

void ApplySales(ChartBase& chart, bool quarterly) {
    if (quarterly) {
        chart.SetCategories({ "Q1", "Q2", "Q3", "Q4" });
        ChartSeries a;
        a.name = "华北";
        a.values = { 82.0f, 91.0f, 76.0f, 104.0f };
        ChartSeries b;
        b.name = "华东";
        b.values = { 64.0f, 70.0f, 88.0f, 95.0f };
        chart.SetSeries({ std::move(a), std::move(b) });
        return;
    }
    chart.SetCategories({ "1月", "2月", "3月", "4月", "5月", "6月", "7月" });
    ChartSeries a;
    a.name = "华北";
    a.values = { 12.0f, 18.0f, 15.0f, 22.0f, 28.0f, 24.0f, 31.0f };
    ChartSeries b;
    b.name = "华东";
    b.values = { 9.0f, 14.0f, 19.0f, 16.0f, 21.0f, 27.0f, 25.0f };
    ChartSeries c;
    c.name = "华南";
    c.values = { 6.0f, 9.0f, 13.0f, 11.0f, 17.0f, 15.0f, 19.0f };
    chart.SetSeries({ std::move(a), std::move(b), std::move(c) });
}

void ApplyRandom(ChartBase& chart) {
    chart.SetCategories({ "A组", "B组", "C组", "D组", "E组", "F组" });
    std::mt19937 rng(20260815);
    ChartSeries a;
    a.name = "CPU";
    ChartSeries b;
    b.name = "内存";
    ChartSeries c;
    c.name = "磁盘";
    for (int i = 0; i < 6; ++i) {
        a.values.push_back(std::uniform_real_distribution<float>(8.0f, 42.0f)(rng));
        b.values.push_back(std::uniform_real_distribution<float>(12.0f, 60.0f)(rng));
        c.values.push_back(std::uniform_real_distribution<float>(5.0f, 30.0f)(rng));
    }
    chart.SetSeries({ std::move(a), std::move(b), std::move(c) });
}

} // anonymous namespace

Element BuildBarChartPage() {
    // ---------- 1. 常规用法（多系列分组柱状图） ----------
    auto bar = std::make_shared<BarChart>();
    bar->SetText("柱状图 · 月度销量对比");
    bar->SetHeight(320.0f);
    ApplySales(*bar, false);

    auto status1 = MakeStatus("悬停柱体或按 ← → 方向键读值");
    bar->OnHoverChanged().Connect([status1](ChartBase* sender, int index, int series) {
        status1->Text = FormatHover(sender, index, series);
    });

    auto btnMonth = ElevatedButton("月度数据", [bar](UIElement*) { ApplySales(*bar, false); }).Build();
    auto btnQuarter = ElevatedButton("季度数据", [bar](UIElement*) { ApplySales(*bar, true); }).Build();
    auto btnReveal = ElevatedButton("重放入场动画", [bar](UIElement*) { bar->PlayReveal(); }).Build();

    // ---------- 2. 自定义颜色系列 ----------
    auto colorChart = std::make_shared<BarChart>();
    colorChart->SetText("自定义系列颜色 · 三个渠道");
    colorChart->SetHeight(280.0f);
    colorChart->SetCategories({ "1月", "2月", "3月", "4月" });

    ChartSeries web;
    web.name = "线上商城";
    web.color = D2D1::ColorF(D2D1::ColorF::DodgerBlue, 0.95f);
    web.hasColor = true;
    web.values = { 26.0f, 32.0f, 29.0f, 41.0f };
    ChartSeries store;
    store.name = "实体门店";
    store.color = D2D1::ColorF(D2D1::ColorF::OrangeRed, 0.95f);
    store.hasColor = true;
    store.values = { 18.0f, 22.0f, 27.0f, 24.0f };
    ChartSeries agent;
    agent.name = "渠道代理";
    agent.color = D2D1::ColorF(D2D1::ColorF::MediumSeaGreen, 0.95f);
    agent.hasColor = true;
    agent.values = { 12.0f, 15.0f, 19.0f, 22.0f };
    colorChart->SetSeries({ std::move(web), std::move(store), std::move(agent) });

    auto status2 = MakeStatus("每个系列通过 ChartSeries.color + hasColor 指定专属颜色。");

    // ---------- 3. 动态更新 + 显示选项 ----------
    auto dyn = std::make_shared<BarChart>();
    dyn->SetText("动态数据 · 三资源负载（随机）");
    dyn->SetHeight(280.0f);
    ApplyRandom(*dyn);

    auto status3 = MakeStatus("点击“随机重掷”生成新数据并重放入场生长动画。");
    auto btnShuffle = ElevatedButton("随机重掷", [dyn, status3](UIElement*) {
        ApplyRandom(*dyn);
        status3->Text = "已生成新的随机数据（自底向上生长动画）。";
    }).Build();
    auto btnReveal3 = ElevatedButton("重放入场动画", [dyn](UIElement*) { dyn->PlayReveal(); }).Build();

    auto chkGrid = CheckboxTile("显示网格").Build();
    chkGrid->SetState(CheckState::Checked);
    chkGrid->OnCheckStateChanged().Connect([dyn](CheckBox*, CheckState st) {
        dyn->SetShowGrid(st == CheckState::Checked);
    });
    auto chkLegend = CheckboxTile("显示图例").Build();
    chkLegend->SetState(CheckState::Checked);
    chkLegend->OnCheckStateChanged().Connect([dyn](CheckBox*, CheckState st) {
        dyn->SetShowLegend(st == CheckState::Checked);
    });
    auto chkTip = CheckboxTile("悬停提示卡片").Build();
    chkTip->SetState(CheckState::Checked);
    chkTip->OnCheckStateChanged().Connect([dyn](CheckBox*, CheckState st) {
        dyn->SetShowTooltip(st == CheckState::Checked);
    });

    SamplePageSpec spec;
    spec.title = "BarChart (柱状图)";
    spec.subtitle = "多系列分组圆角柱体、自底向上生长动效、逐柱命中高亮、自定义系列颜色与随机数据重掷。";
    spec.sections = {
        {
            "常规用法 — 多系列分组柱状图",
            "1. 每个分类下多系列柱体并排绘制，图例标注颜色；\n"
            "2. 悬停柱体高亮并弹出提示卡片，← → 方向键逐柱切换；\n"
            "3. 数据切换时柱体自底向上生长。",
            Column(12, {
                Row(8, { btnMonth, btnQuarter, btnReveal }),
                bar,
                status1,
            }),
        },
        {
            "自定义系列颜色",
            "ChartSeries.color 可指定任意颜色（hasColor = true）；未指定时自动从内置色盘递进取色。",
            Column(12, {
                colorChart,
                status2,
            }),
        },
        {
            "动态更新与显示选项",
            "1. 随机重掷生成新数据，配合入场生长动画观察柱体变化；\n"
            "2. 网格 / 图例 / 悬停提示可独立开关。",
            Column(12, {
                Row(12, { btnShuffle, btnReveal3, chkGrid, chkLegend, chkTip }),
                dyn,
                status3,
            }),
        },
    };

    spec.source = R"(
// 1) 构建柱状图
auto chart = std::make_shared<BarChart>();
chart->SetText("销量对比");
chart->SetHeight(320.0f);
chart->SetCategories({ "1月", "2月", "3月" });

// 2) 指定系列颜色（可选，默认自动取色）
ChartSeries s;
s.name = "线上商城";
s.values = { 26.0f, 32.0f, 29.0f };
s.color = D2D1::ColorF(D2D1::ColorF::DodgerBlue, 0.95f);
s.hasColor = true;
chart->SetSeries({ std::move(s) });

// 3) 更新数据 / 动画 / 显示
chart->SetSeries(newSeries);   // 自动重放入场生长动画
chart->PlayReveal();
chart->SetShowGrid(false);
)";

    return BuildSamplePage(spec);
}

} // namespace Gallery
