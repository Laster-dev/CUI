#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "BarChart.h"
#include <algorithm>
#include <cmath>

namespace CUI {

BarChart::BarChart() {
    SetText("柱状图");
}

bool BarChart::BarRect(const Rect& plot, int category, int series, Rect& out) const {
    const int catCount = SeriesValueCount();
    const int seriesCount = (std::max)(1, static_cast<int>(m_series.size()));
    if (category < 0 || category >= catCount || series < 0 || series >= seriesCount) {
        return false;
    }
    if (series >= static_cast<int>(m_series.size())
        || category >= static_cast<int>(m_series[series].values.size())) {
        return false;
    }

    float yMin = 0.0f;
    float yMax = 1.0f;
    std::vector<ChartTick> ticks;
    BuildYScale(plot, yMin, yMax, ticks);

    const float groupW = plot.width / static_cast<float>(catCount);
    const float inner = groupW * 0.72f;
    const float barW = inner / static_cast<float>(seriesCount);
    const float groupPad = (groupW - inner) * 0.5f;
    const float x = plot.x + static_cast<float>(category) * groupW + groupPad + static_cast<float>(series) * barW;
    const float value = m_series[series].values[category];
    const float y0 = MapY(plot, 0.0f, yMin, yMax);
    const float y1 = MapY(plot, value, yMin, yMax);
    const int order = category * seriesCount + series;
    const float grow = RevealAt(order, catCount * seriesCount, 0.52f);
    const bool hot = (category == m_hoverIndex)
        && (m_hoverSeries < 0 || series == m_hoverSeries);
    const float lift = hot ? (1.0f + 0.05f * HoverAmount()) : 1.0f;
    const float fullH = std::abs(y1 - y0) * grow * lift;
    const float top = (y1 < y0) ? (y0 - fullH) : y0;
    out = Rect(x + 1.0f, top, (std::max)(1.0f, barW - 2.0f), (std::max)(1.0f, fullH));
    return grow > 0.02f;
}

void BarChart::BindHoverMotion(const Rect& plot) {
    ChartBase::BindHoverMotion(plot);
    Rect bar;
    if (m_hoverIndex >= 0 && m_hoverSeries >= 0 && BarRect(plot, m_hoverIndex, m_hoverSeries, bar)) {
        m_crossX.SetTarget(bar.x + bar.width * 0.5f);
        m_crossY.SetTarget(bar.y);
        return;
    }
    if (m_hoverIndex >= 0 && !m_series.empty() && BarRect(plot, m_hoverIndex, 0, bar)) {
        m_crossX.SetTarget(bar.x + bar.width * 0.5f);
        m_crossY.SetTarget(bar.y);
    }
}

void BarChart::DrawPlot(GraphicsContext& ctx, const Rect& plot) {
    float yMin = 0.0f;
    float yMax = 1.0f;
    std::vector<ChartTick> ticks;
    BuildYScale(plot, yMin, yMax, ticks);
    DrawCartesianFrame(ctx, plot, yMin, yMax, ticks);

    const int catCount = SeriesValueCount();
    const int seriesCount = static_cast<int>(m_series.size());
    const float hover = HoverAmount();
    for (int s = 0; s < seriesCount; ++s) {
        D2D1_COLOR_F color = SeriesColor(s);
        for (int c = 0; c < catCount; ++c) {
            Rect bar;
            if (!BarRect(plot, c, s, bar)) {
                continue;
            }
            D2D1_COLOR_F fill = color;
            const bool hot = (c == m_hoverIndex) && (m_hoverSeries < 0 || s == m_hoverSeries);
            if (hover > 0.01f && !hot) {
                fill.a *= 1.0f - 0.58f * hover;
            }
            ctx.FillRoundedRect(bar, 3.0f, fill);
        }
    }
}

void BarChart::HitTestHover(Point pt, int& index, int& series) const {
    index = -1;
    series = -1;
    const Rect plot = PlotRect();
    if (!plot.Contains(pt.x, pt.y) && pt.y > plot.y + plot.height + 4.0f) {
        return;
    }
    const int catCount = SeriesValueCount();
    const int seriesCount = static_cast<int>(m_series.size());
    for (int c = 0; c < catCount; ++c) {
        for (int s = 0; s < seriesCount; ++s) {
            Rect bar;
            if (BarRect(plot, c, s, bar) && bar.Contains(pt.x, pt.y)) {
                index = c;
                series = s;
                return;
            }
        }
    }
    if (!plot.Contains(pt.x, pt.y) || catCount <= 0) {
        return;
    }
    const float groupW = plot.width / static_cast<float>(catCount);
    int cat = static_cast<int>((pt.x - plot.x) / groupW);
    cat = std::clamp(cat, 0, catCount - 1);
    index = cat;
    series = -1;
}

Point BarChart::TooltipAnchor() const {
    const Rect plot = PlotRect();
    Rect bar;
    if (m_hoverIndex >= 0 && m_hoverSeries >= 0 && BarRect(plot, m_hoverIndex, m_hoverSeries, bar)) {
        return Point(bar.x + bar.width * 0.5f, bar.y);
    }
    return ChartBase::TooltipAnchor();
}

} // namespace CUI
