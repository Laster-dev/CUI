#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "../PerfMetrics.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/chart/Chart.h"
#include "framework/controls/Button.h"
#include "framework/controls/CheckBox.h"
#include "framework/controls/TextBlock.h"
#include "framework/animation/AnimationManager.h"
#include <cstdio>
#include <deque>
#include <sstream>

using namespace CUI;
using namespace CUI::DSL;

namespace {
std::shared_ptr<Button> MakeBtn(const std::string& text) {
    auto btn = std::make_shared<Button>(text);
    btn->SetWidth(88.0f);
    btn->SetHeight(32.0f);
    btn->SetCornerRadius(4.0f);
    return btn;
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

std::string FormatHover(ChartBase* chart, int index, int series) {
    if (!chart || index < 0) {
        return "悬停或 ← → 读值";
    }
    std::ostringstream oss;
    const auto& cats = chart->GetCategories();
    const auto& seriesList = chart->GetSeries();
    const std::string cat = (index < static_cast<int>(cats.size())) ? cats[index] : std::to_string(index + 1);
    oss << cat;
    for (int s = 0; s < static_cast<int>(seriesList.size()); ++s) {
        if (series >= 0 && s != series) {
            continue;
        }
        if (index >= static_cast<int>(seriesList[s].values.size())) {
            continue;
        }
        const std::string name = seriesList[s].name.empty() ? ("系列" + std::to_string(s + 1)) : seriesList[s].name;
        oss << "  ·  " << name << " " << FormatChartNumber(seriesList[s].values[index]);
    }
    return oss.str();
}

constexpr int kPerfWindow = 60;

std::string FormatPerfReadout(const PerfSnapshot& snap) {
    char buf[128];
    if (snap.gpuPct >= 0.0f) {
        std::snprintf(buf, sizeof(buf),
            "内存:私有%.0fMB 完整%.0fMB    CPU %.0f%%    GPU %.0f%%    %.0f FPS",
            snap.privateMb, snap.workingSetMb, snap.cpuPct, snap.gpuPct, snap.fps);
    } else {
        std::snprintf(buf, sizeof(buf),
            "内存:私有%.0fMB 完整%.0fMB    CPU %.0f%%    GPU —    %.0f FPS",
            snap.privateMb, snap.workingSetMb, snap.cpuPct, snap.fps);
    }
    return buf;
}

std::vector<std::string> PerfCategories(int count) {
    std::vector<std::string> cats(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        const int age = count - 1 - i;
        if (i == count - 1) {
            cats[static_cast<size_t>(i)] = "now";
        } else if (age % 10 == 0) {
            cats[static_cast<size_t>(i)] = "-" + std::to_string(age / 2) + "s";
        }
    }
    return cats;
}

class PerfChartPump : public UIElement {
public:
    PerfChartPump(
        std::shared_ptr<LineChart> memChart,
        std::shared_ptr<LineChart> loadChart,
        std::shared_ptr<TextBlock> readout)
        : m_mem(std::move(memChart))
        , m_load(std::move(loadChart))
        , m_readout(std::move(readout)) {}

    virtual const char* GetClassName() const override { return "PerfChartPump"; }
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
            RequestAnimationTicks();
        }
    }
    virtual bool OnAnimationTick() override {
        PushSample();
        if (AnimationManager* mgr = AnimationManager::Current()) {
            mgr->RequestWake(this, AnimationManager::clock::now() + std::chrono::milliseconds(500));
        }
        return false;
    }
    virtual bool HasSelfAnimation() const override { return false; }

private:
    void PushSample() {
        const PerfSnapshot snap = PerfSampler::Instance().Sample();
        m_hist.push_back(snap);
        while (static_cast<int>(m_hist.size()) > kPerfWindow) {
            m_hist.pop_front();
        }
        if (m_readout) {
            m_readout->SetText(FormatPerfReadout(snap));
        }

        const int n = static_cast<int>(m_hist.size());
        auto cats = PerfCategories(n);
        ChartSeries priv;
        priv.name = "私有";
        ChartSeries full;
        full.name = "完整";
        ChartSeries cpu;
        cpu.name = "CPU";
        ChartSeries gpu;
        gpu.name = "GPU";
        ChartSeries fps;
        fps.name = "FPS";
        priv.values.reserve(static_cast<size_t>(n));
        full.values.reserve(static_cast<size_t>(n));
        cpu.values.reserve(static_cast<size_t>(n));
        gpu.values.reserve(static_cast<size_t>(n));
        fps.values.reserve(static_cast<size_t>(n));
        for (const auto& s : m_hist) {
            priv.values.push_back(s.privateMb);
            full.values.push_back(s.workingSetMb);
            cpu.values.push_back(s.cpuPct);
            gpu.values.push_back(s.gpuPct >= 0.0f ? s.gpuPct : 0.0f);
            fps.values.push_back(s.fps);
        }

        const bool first = m_first;
        m_first = false;
        if (m_mem) {
            m_mem->SetLiveData(cats, { priv, full }, first);
        }
        if (m_load) {
            m_load->SetLiveData(std::move(cats), { std::move(cpu), std::move(gpu), std::move(fps) }, first);
        }
    }

    std::shared_ptr<LineChart> m_mem;
    std::shared_ptr<LineChart> m_load;
    std::shared_ptr<TextBlock> m_readout;
    std::deque<PerfSnapshot> m_hist;
    bool m_started = false;
    bool m_first = true;
};
} // namespace

ShowcasePage BuildChartPage(const ShowcaseContext& ctx) {
    auto line = std::make_shared<LineChart>();
    auto bar = std::make_shared<BarChart>();
    auto pie = std::make_shared<PieChart>();
    ApplySales(*line, false);
    ApplySales(*bar, false);
    ApplySales(*pie, false);
    line->SetText("折线 · 月度销量");
    bar->SetText("柱状 · 月度销量");
    pie->SetText("饼图 · 华北月度占比");
    bar->SetVisibility(Visibility::Collapsed);
    pie->SetVisibility(Visibility::Collapsed);

    auto hover = std::static_pointer_cast<TextBlock>(
        CreateShowcaseText("悬停或 ← → 读值", 12.0f, "textSecondary", false));

    auto bindHover = [hover](ChartBase* chart) {
        chart->OnHoverChanged().Connect([hover](ChartBase* sender, int index, int series) {
            hover->SetText(FormatHover(sender, index, series));
        });
    };
    bindHover(line.get());
    bindHover(bar.get());
    bindHover(pie.get());

    auto show = [line, bar, pie](int which) {
        line->SetVisibility(which == 0 ? Visibility::Visible : Visibility::Collapsed);
        bar->SetVisibility(which == 1 ? Visibility::Visible : Visibility::Collapsed);
        pie->SetVisibility(which == 2 ? Visibility::Visible : Visibility::Collapsed);
    };

    auto btnLine = MakeBtn("折线");
    btnLine->OnClick().Connect([show](UIElement*) { show(0); });
    auto btnBar = MakeBtn("柱状");
    btnBar->OnClick().Connect([show](UIElement*) { show(1); });
    auto btnPie = MakeBtn("饼图");
    btnPie->OnClick().Connect([show](UIElement*) { show(2); });

    auto btnMonth = MakeBtn("月度");
    btnMonth->OnClick().Connect([line, bar, pie](UIElement*) {
        ApplySales(*line, false);
        ApplySales(*bar, false);
        ApplySales(*pie, false);
        line->SetText("折线 · 月度销量");
        bar->SetText("柱状 · 月度销量");
        pie->SetText("饼图 · 华北月度占比");
    });
    auto btnQuarter = MakeBtn("季度");
    btnQuarter->OnClick().Connect([line, bar, pie](UIElement*) {
        ApplySales(*line, true);
        ApplySales(*bar, true);
        ApplySales(*pie, true);
        line->SetText("折线 · 季度销量");
        bar->SetText("柱状 · 季度销量");
        pie->SetText("饼图 · 华北季度占比");
    });

    auto chkGrid = std::make_shared<CheckBox>("网格");
    chkGrid->SetState(CheckState::Checked);
    chkGrid->OnCheckStateChanged().Connect([line, bar](CheckBox*, CheckState st) {
        const bool on = st == CheckState::Checked;
        line->SetShowGrid(on);
        bar->SetShowGrid(on);
    });
    auto chkLegend = std::make_shared<CheckBox>("图例");
    chkLegend->SetState(CheckState::Checked);
    chkLegend->OnCheckStateChanged().Connect([line, bar, pie](CheckBox*, CheckState st) {
        const bool on = st == CheckState::Checked;
        line->SetShowLegend(on);
        bar->SetShowLegend(on);
        pie->SetShowLegend(on);
    });

    auto memChart = std::make_shared<LineChart>();
    memChart->SetText("进程内存 (MB)");
    memChart->SetHeight(200.0f);
    auto loadChart = std::make_shared<LineChart>();
    loadChart->SetText("CPU / GPU / FPS");
    loadChart->SetHeight(200.0f);
    auto perfReadout = std::static_pointer_cast<TextBlock>(
        CreateShowcaseText("对接底栏同一采样…", 12.0f, "textPrimary", true));
    auto pump = std::make_shared<PerfChartPump>(memChart, loadChart, perfReadout);

    auto demo = Column(12).Children({
        CreateDemoSurface({
            CreateShowcaseText("1. 三图切换 · 悬停十字线/扇区读值 · 缩放重算刻度", 12.0f, "textSecondary", false),
            Row(8).Children({ btnLine, btnBar, btnPie, btnMonth, btnQuarter, chkGrid, chkLegend }).Build(),
            hover,
            line,
            bar,
            pie,
        }, 10.0f),
        CreateDemoSurface({
            CreateShowcaseText("2. 实时对接 · 与底栏同一套 PerfSampler（私有/完整内存、CPU、GPU、FPS）", 12.0f, "textSecondary", false),
            perfReadout,
            memChart,
            loadChart,
            pump,
        }, 10.0f),
    }).Build();

    return { "Chart 图表", CreatePage(
        "Chart 数据可视化",
        "上块为固定演示数据；下块对接进程私有/完整工作集、CPU、GPU 显存占用与显示 FPS，约 30 秒滚动窗口。",
        demo,
        CreatePropertyGrid(ctx, line), line) };
}
