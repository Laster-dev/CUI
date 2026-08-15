#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/chart/Chart.h"
#include "framework/animation/AnimationManager.h"
#include <cmath>
#include <chrono>
#include <deque>
#include <format>
#include <memory>
#include <random>
#include <string>
#include <vector>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

// ---------- 悬停读值 ----------
std::string FormatHover(ChartBase* chart, int index, int series) {
    if (!chart || index < 0) {
        return "悬停图线或按 ← → 方向键读值";
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
    chart.SetSeries({ std::move(a), std::move(b) });
}

// ---------- 实时数据泵：正弦波 + 噪声，250ms 一帧 ----------
class SineStreamPump : public UIElement {
public:
    SineStreamPump(std::shared_ptr<LineChart> chart, std::shared_ptr<TextBlock> status)
        : m_chart(std::move(chart)), m_status(std::move(status)) {}

    virtual const char* GetClassName() const override { return "SineStreamPump"; }
    virtual Size Measure(Size availableSize) override {
        (void)availableSize;
        m_desiredSize = Size(0.0f, 0.0f);
        return m_desiredSize;
    }
    virtual void Arrange(Rect finalRect) override {
        SetBounds(Rect(finalRect.x, finalRect.y, 0.0f, 0.0f));
    }
    virtual void OnRender(GraphicsContext& ctx) override {
        (void)ctx;
        if (!m_started) {
            m_started = true;
            m_running = true;
            RequestAnimationTicks();
        }
    }
    virtual bool OnAnimationTick() override {
        if (m_running) {
            PushSample();
        }
        if (AnimationManager* mgr = AnimationManager::Current()) {
            mgr->RequestWake(this, AnimationManager::clock::now() + std::chrono::milliseconds(250));
        }
        return false;
    }
    virtual bool HasSelfAnimation() const override { return false; }

    bool IsRunning() const { return m_running; }
    void SetRunning(bool on) { m_running = on; }

private:
    void PushSample() {
        using namespace std::chrono;
        const auto now = steady_clock::now();
        const double t = duration_cast<milliseconds>(now.time_since_epoch()).count() / 1000.0;
        static std::mt19937 rng(20260815);
        const float n1 = std::uniform_real_distribution<float>(-2.5f, 2.5f)(rng);
        const float n2 = std::uniform_real_distribution<float>(-3.0f, 3.0f)(rng);
        m_sin.push_back(50.0f + 28.0f * static_cast<float>(std::sin(t * 1.6)) + n1);
        m_cos.push_back(50.0f + 20.0f * static_cast<float>(std::cos(t * 2.3)) + n2);
        const int kWindow = 60;
        while (static_cast<int>(m_sin.size()) > kWindow) m_sin.pop_front();
        while (static_cast<int>(m_cos.size()) > kWindow) m_cos.pop_front();

        std::vector<std::string> cats(static_cast<size_t>(m_sin.size()));
        ChartSeries s1;
        s1.name = "正弦 sin";
        s1.values.assign(m_sin.begin(), m_sin.end());
        ChartSeries s2;
        s2.name = "余弦 cos";
        s2.values.assign(m_cos.begin(), m_cos.end());
        m_chart->SetLiveData(std::move(cats), { std::move(s1), std::move(s2) }, false);

        if (m_status) {
            m_status->SetText(std::format("实时流运行中 · 窗口 {} 个采样点 · 最新: sin {:.1f} / cos {:.1f}",
                                          m_sin.size(), m_sin.back(), m_cos.back()));
        }
    }

    std::shared_ptr<LineChart> m_chart;
    std::shared_ptr<TextBlock> m_status;
    std::deque<float> m_sin;
    std::deque<float> m_cos;
    bool m_started = false;
    bool m_running = false;
};

} // anonymous namespace

Element BuildLineChartPage() {
    // ---------- 1. 常规用法 ----------
    auto line = std::make_shared<LineChart>();
    line->SetText("折线图 · 月度销量趋势");
    line->SetHeight(320.0f);
    ApplySales(*line, false);

    auto status1 = MakeStatus("悬停图线或按 ← → 方向键读值");
    line->OnHoverChanged().Connect([status1](ChartBase* sender, int index, int series) {
        status1->Text = FormatHover(sender, index, series);
    });

    auto btnMonth = ElevatedButton("月度数据", [line](UIElement*) { ApplySales(*line, false); }).Build();
    auto btnQuarter = ElevatedButton("季度数据", [line](UIElement*) { ApplySales(*line, true); }).Build();
    auto btnReveal = ElevatedButton("重放入场动画", [line](UIElement*) { line->PlayReveal(); }).Build();

    // ---------- 2. 实时数据流 ----------
    auto live = std::make_shared<LineChart>();
    live->SetText("实时数据流 · 正弦/余弦采样 (250ms/帧)");
    live->SetHeight(260.0f);

    auto liveStatus = MakeStatus("等待数据泵启动...");
    auto pump = std::make_shared<SineStreamPump>(live, liveStatus);
    auto btnRun = ToggleButtonWidget("⏸ 暂停实时流").Build();
    btnRun->OnClick().Connect([pump, btnRun](UIElement*) {
        const bool on = !pump->IsRunning();
        pump->SetRunning(on);
        btnRun->SetText(on ? "⏸ 暂停实时流" : "▶ 启动实时流");
    });

    // ---------- 3. 显示选项 ----------
    auto optChart = std::make_shared<LineChart>();
    optChart->SetText("显示选项 · 网格 / 图例 / 悬停提示");
    optChart->SetHeight(260.0f);
    ApplySales(*optChart, true);

    auto status3 = MakeStatus("网格、图例、悬停提示均可独立开关。");

    auto chkGrid = CheckboxTile("显示网格").Build();
    chkGrid->SetState(CheckState::Checked);
    chkGrid->OnCheckStateChanged().Connect([optChart, status3](CheckBox*, CheckState st) {
        const bool on = st == CheckState::Checked;
        optChart->SetShowGrid(on);
        status3->Text = on ? "网格已显示。" : "网格已隐藏。";
    });
    auto chkLegend = CheckboxTile("显示图例").Build();
    chkLegend->SetState(CheckState::Checked);
    chkLegend->OnCheckStateChanged().Connect([optChart](CheckBox*, CheckState st) {
        optChart->SetShowLegend(st == CheckState::Checked);
    });
    auto chkTip = CheckboxTile("悬停提示卡片").Build();
    chkTip->SetState(CheckState::Checked);
    chkTip->OnCheckStateChanged().Connect([optChart](CheckBox*, CheckState st) {
        optChart->SetShowTooltip(st == CheckState::Checked);
    });
    auto btnReveal3 = ElevatedButton("重放入场动画", [optChart](UIElement*) { optChart->PlayReveal(); }).Build();

    SamplePageSpec spec;
    spec.title = "LineChart (折线图)";
    spec.subtitle = "渐变折线 + 数据圆点 + 半透明面积填充，十字线悬浮读值、← → 键盘切换、入场描线动效与实时滚动数据流。";
    spec.sections = {
        {
            "常规用法 — 多系列趋势图",
            "1. SetCategories 设定 X 轴分类，SetSeries 批量覆盖数据系列（自动重放入场动画）；\n"
            "2. 悬停数据点显示十字线 + 提示卡片，也可用 ← → 方向键流动切换；\n"
            "3. Y 轴自动以“整齐刻度”计算范围与网格，缩放窗口实时重算。",
            Column(12, {
                Row(8, { btnMonth, btnQuarter, btnReveal }),
                line,
                status1,
            }),
        },
        {
            "实时数据流 — SetLiveData 高频刷新",
            "1. 数据泵每 250ms 追加一个采样点，滚动窗口 60 帧；\n"
            "2. SetLiveData 屏蔽入场动画，避免高频刷新抖动；\n"
            "3. 点击按钮暂停 / 恢复数据流。",
            Column(12, {
                Row(8, { btnRun }),
                live,
                liveStatus,
                pump,
            }),
        },
        {
            "显示选项 — 网格 / 图例 / 提示",
            "1. SetShowGrid / SetShowLegend / SetShowTooltip 独立开关；\n"
            "2. 数据切换时自动重新播放入场描线动画。",
            Column(12, {
                Row(12, { chkGrid, chkLegend, chkTip, btnReveal3 }),
                optChart,
                status3,
            }),
        },
    };

    spec.source = R"(
// 1) 构建图表并填充数据
auto chart = std::make_shared<LineChart>();
chart->SetText("月度销量");
chart->SetHeight(320.0f);
chart->SetCategories({ "1月", "2月", "3月", "4月" });
ChartSeries s;
s.name = "华北";
s.values = { 12.0f, 18.0f, 15.0f, 22.0f };
chart->SetSeries({ std::move(s) });

// 2) 实时增量更新（屏蔽入场动画）
chart->SetLiveData(categories, series);

// 3) 悬停读值
chart->OnHoverChanged().Connect([](ChartBase* c, int idx, int ser) {
    std::string text = c->GetCategories()[idx];
});

// 4) 显示开关与动画
chart->SetShowGrid(false);
chart->SetShowTooltip(true);
chart->PlayReveal();   // 重放入场描线动画
)";

    return BuildSamplePage(spec);
}

} // namespace Gallery
