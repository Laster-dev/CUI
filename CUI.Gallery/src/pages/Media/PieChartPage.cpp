#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/chart/Chart.h"
#include <format>
#include <memory>
#include <string>
#include <vector>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

float SliceTotal(const ChartBase& chart) {
    float total = 0.0f;
    for (const auto& s : chart.GetSeries()) {
        for (float v : s.values) {
            if (v > 0.0f) total += v;
        }
    }
    return total;
}

std::string FormatPieHover(ChartBase* chart, int index) {
    if (!chart || index < 0) {
        return "悬停扇区或按 ← → 方向键读值";
    }
    const auto& cats = chart->GetCategories();
    const std::string cat = (index < static_cast<int>(cats.size()) && !cats[index].empty())
        ? cats[index] : std::format("{}", index + 1);
    float value = 0.0f;
    for (const auto& s : chart->GetSeries()) {
        if (index < static_cast<int>(s.values.size())) {
            value += s.values[index];
        }
    }
    const float total = SliceTotal(*chart);
    const float pct = total > 0.0f ? value / total * 100.0f : 0.0f;
    return std::format("{}  ·  {}  ({:.1f}%)", cat, FormatChartNumber(value), pct);
}

void ApplyMonthly(ChartBase& chart) {
    chart.SetCategories({ "1月", "2月", "3月", "4月", "5月", "6月", "7月" });
    ChartSeries s;
    s.name = "华北";
    s.values = { 12.0f, 18.0f, 15.0f, 22.0f, 28.0f, 24.0f, 31.0f };
    chart.SetSeries({ std::move(s) });
}

void ApplyQuarterly(ChartBase& chart) {
    chart.SetCategories({ "Q1", "Q2", "Q3", "Q4" });
    ChartSeries s;
    s.name = "华北";
    s.values = { 82.0f, 91.0f, 76.0f, 104.0f };
    chart.SetSeries({ std::move(s) });
}

void ApplyCustom(ChartBase& chart) {
    chart.SetCategories({ "操作系统", "数据库", "中间件", "缓存", "消息队列", "其他" });
    ChartSeries s;
    s.name = "资源占比";
    s.values = { 34.0f, 22.0f, 15.0f, 12.0f, 9.0f, 8.0f };
    chart.SetSeries({ std::move(s) });
}

} // anonymous namespace

Element BuildPieChartPage() {
    // ---------- 1. 常规用法 ----------
    auto pie = std::make_shared<PieChart>();
    pie->SetText("饼图 · 华北月度销量占比");
    pie->SetHeight(320.0f);
    ApplyMonthly(*pie);

    auto status1 = MakeStatus("悬停扇区读值，扇区平滑向外凸出高亮");
    pie->OnHoverChanged().Connect([status1](ChartBase* sender, int index, int) {
        status1->Text = FormatPieHover(sender, index);
    });

    auto btnMonth = ElevatedButton("月度数据", [pie](UIElement*) { ApplyMonthly(*pie); }).Build();
    auto btnQuarter = ElevatedButton("季度数据", [pie](UIElement*) { ApplyQuarterly(*pie); }).Build();
    auto btnCustom = ElevatedButton("资源占比", [pie](UIElement*) { ApplyCustom(*pie); }).Build();
    auto btnReveal = ElevatedButton("重放入场动画", [pie](UIElement*) { pie->PlayReveal(); }).Build();

    // ---------- 2. 显示选项 ----------
    auto optPie = std::make_shared<PieChart>();
    optPie->SetText("显示选项 · 图例 / 悬停提示");
    optPie->SetHeight(280.0f);
    ApplyCustom(*optPie);

    auto status2 = MakeStatus("图例与悬停提示可独立开关；饼图图例按百分比扇区自绘。");
    auto chkLegend = CheckboxTile("显示图例").Build();
    chkLegend->SetState(CheckState::Checked);
    chkLegend->OnCheckStateChanged().Connect([optPie, status2](CheckBox*, CheckState st) {
        const bool on = st == CheckState::Checked;
        optPie->SetShowLegend(on);
        status2->Text = on ? "图例已显示。" : "图例已隐藏。";
    });
    auto chkTip = CheckboxTile("悬停提示卡片").Build();
    chkTip->SetState(CheckState::Checked);
    chkTip->OnCheckStateChanged().Connect([optPie](CheckBox*, CheckState st) {
        optPie->SetShowTooltip(st == CheckState::Checked);
    });
    auto btnReveal2 = ElevatedButton("重放入场动画", [optPie](UIElement*) { optPie->PlayReveal(); }).Build();

    SamplePageSpec spec;
    spec.title = "PieChart (饼图)";
    spec.subtitle = "按占比划分的扇区圆弧，悬停平滑凸出 (Pop-out)、提示卡片显示数值与百分比、按扇区自绘图例。";
    spec.sections = {
        {
            "常规用法 — 占比扇区",
            "1. 汇总所有系列数值折算扇区角度，颜色从内置色盘自动分配；\n"
            "2. 悬停扇区平滑向外凸出并弹出提示卡片（含百分比），← → 方向键切换；\n"
            "3. 数据预设一键切换并重放入场动画。",
            Column(12, {
                Row(8, { btnMonth, btnQuarter, btnCustom, btnReveal }),
                pie,
                status1,
            }),
        },
        {
            "显示选项 — 图例 / 提示",
            "饼图图例按扇区色块 + 百分比自绘；悬停提示可关闭。",
            Column(12, {
                Row(12, { chkLegend, chkTip, btnReveal2 }),
                optPie,
                status2,
            }),
        },
    };

    spec.source = R"(
// 1) 构建饼图（单系列多分类 → 每分类一个扇区）
auto pie = std::make_shared<PieChart>();
pie->SetText("资源占比");
pie->SetHeight(320.0f);
pie->SetCategories({ "操作系统", "数据库", "中间件" });
ChartSeries s;
s.name = "占比";
s.values = { 34.0f, 22.0f, 15.0f };
pie->SetSeries({ std::move(s) });

// 2) 悬停读值（提示卡片自动含百分比）
pie->OnHoverChanged().Connect([](ChartBase* c, int idx, int) {
    float v = c->GetSeries()[0].values[idx];
});

// 3) 显示控制
pie->SetShowLegend(true);
pie->SetShowTooltip(true);
pie->PlayReveal();
)";

    return BuildSamplePage(spec);
}

} // namespace Gallery
