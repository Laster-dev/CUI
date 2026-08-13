#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "LineChart.h"
#include "../../style/ThemeManager.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace CUI {

LineChart::LineChart() {
    SetText("折线图");
}

void LineChart::BindHoverMotion(const Rect& plot) {
    ChartBase::BindHoverMotion(plot);
    const int count = SeriesValueCount();
    if (m_hoverIndex < 0 || count <= 0 || m_series.empty()) {
        return;
    }
    float yMin = 0.0f;
    float yMax = 1.0f;
    std::vector<ChartTick> ticks;
    BuildYScale(plot, yMin, yMax, ticks);
    if (m_hoverIndex < static_cast<int>(m_series[0].values.size())) {
        m_crossY.SetTarget(MapY(plot, m_series[0].values[m_hoverIndex], yMin, yMax));
    }
}

void LineChart::DrawPlot(GraphicsContext& ctx, const Rect& plot) {
    float yMin = 0.0f;
    float yMax = 1.0f;
    std::vector<ChartTick> ticks;
    BuildYScale(plot, yMin, yMax, ticks);
    DrawCartesianFrame(ctx, plot, yMin, yMax, ticks);

    const int count = SeriesValueCount();
    if (count <= 0) {
        return;
    }
    const float baseline = MapY(plot, 0.0f, yMin, yMax);
    const float hover = HoverAmount();

    if (hover > 0.01f) {
        D2D1_COLOR_F cross = ThemeManager::Instance().GetFlatColor(ThemeTokenId::TextMuted);
        cross.a *= 0.55f * hover;
        ctx.DrawLine(Point(CrossX(), plot.y), Point(CrossX(), plot.y + plot.height), cross, 1.0f);
        ctx.DrawLine(Point(plot.x, CrossY()), Point(plot.x + plot.width, CrossY()), cross, 1.0f);
    }

    for (int s = 0; s < static_cast<int>(m_series.size()); ++s) {
        const auto& series = m_series[s];
        const D2D1_COLOR_F color = SeriesColor(s);
        const int n = (std::min)(count, static_cast<int>(series.values.size()));
        if (n <= 0) {
            continue;
        }

        std::vector<Point> pts(static_cast<size_t>(n));
        for (int i = 0; i < n; ++i) {
            const float t = RevealAt(i, n, 0.48f);
            const float targetY = MapY(plot, series.values[i], yMin, yMax);
            pts[static_cast<size_t>(i)] = Point(
                MapX(plot, i, count),
                baseline + (targetY - baseline) * t);
        }

        if (n >= 2) {
            std::vector<Point> area;
            area.reserve(static_cast<size_t>(n) + 2);
            area.insert(area.end(), pts.begin(), pts.end());
            area.push_back(Point(pts.back().x, baseline));
            area.push_back(Point(pts.front().x, baseline));
            D2D1_COLOR_F fill = color;
            fill.a *= 0.14f;
            ctx.FillPolygon(area.data(), static_cast<int>(area.size()), fill);
        }

        for (int i = 1; i < n; ++i) {
            ctx.DrawSmoothLine(pts[static_cast<size_t>(i - 1)], pts[static_cast<size_t>(i)], color, 2.2f);
        }

        for (int i = 0; i < n; ++i) {
            const float appear = RevealAt(i, n, 0.48f);
            if (appear < 0.08f) {
                continue;
            }
            const bool hot = (i == m_hoverIndex);
            const float pulse = hot ? hover : 0.0f;
            const float r = 2.6f + 3.2f * pulse;
            const Point p = pts[static_cast<size_t>(i)];
            if (pulse > 0.01f) {
                D2D1_COLOR_F glow = color;
                glow.a *= 0.22f * pulse;
                const float gr = r + 5.0f * pulse;
                ctx.FillRoundedRect(Rect(p.x - gr, p.y - gr, gr * 2.0f, gr * 2.0f), gr, glow);
            }
            ctx.FillRoundedRect(Rect(p.x - r, p.y - r, r * 2.0f, r * 2.0f), r, color);
        }
    }
}

void LineChart::HitTestHover(Point pt, int& index, int& series) const {
    index = -1;
    series = -1;
    const Rect plot = PlotRect();
    if (!plot.Contains(pt.x, pt.y)) {
        return;
    }
    const int count = SeriesValueCount();
    if (count <= 0) {
        return;
    }
    int best = 0;
    float bestDist = 1.0e9f;
    for (int i = 0; i < count; ++i) {
        const float x = MapX(plot, i, count);
        const float d = std::abs(pt.x - x);
        if (d < bestDist) {
            bestDist = d;
            best = i;
        }
    }
    index = best;
}

Point LineChart::TooltipAnchor() const {
    const Rect plot = PlotRect();
    const int count = SeriesValueCount();
    float yMin = 0.0f;
    float yMax = 1.0f;
    std::vector<ChartTick> ticks;
    BuildYScale(plot, yMin, yMax, ticks);
    float y = plot.y + plot.height * 0.25f;
    if (m_hoverIndex >= 0 && !m_series.empty()
        && m_hoverIndex < static_cast<int>(m_series[0].values.size())) {
        y = MapY(plot, m_series[0].values[m_hoverIndex], yMin, yMax);
    }
    return Point(MapX(plot, m_hoverIndex, count), y);
}

} // namespace CUI
