#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "PieChart.h"
#include "../../style/ThemeManager.h"
#include <algorithm>
#include <cmath>

namespace CUI {

namespace {
constexpr float kPi = 3.14159265f;
constexpr float kTwoPi = 6.2831853f;

Point Polar(Point center, float radius, float angle) {
    return Point(center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius);
}

void FillSlice(GraphicsContext& ctx, Point center, float radius, float start, float sweep, D2D1_COLOR_F color) {
    if (sweep <= 0.001f || radius < 2.0f) {
        return;
    }
    const int steps = std::clamp(static_cast<int>(std::ceil(std::abs(sweep) / 0.12f)), 4, 48);
    Point pts[50];
    pts[0] = center;
    for (int i = 0; i <= steps; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(steps);
        pts[i + 1] = Polar(center, radius, start + sweep * t);
    }
    ctx.FillPolygon(pts, steps + 2, color);
}
} // namespace

PieChart::PieChart() {
    SetText("饼图");
}

void PieChart::CollectSlices(std::vector<Slice>& slices, float& total) const {
    slices.clear();
    total = 0.0f;
    const int count = SeriesValueCount();
    if (count <= 0 || m_series.empty()) {
        return;
    }
    const auto& values = m_series[0].values;
    for (int i = 0; i < count; ++i) {
        const float v = (i < static_cast<int>(values.size())) ? values[i] : 0.0f;
        if (!(v > 0.0f) || !std::isfinite(v)) {
            continue;
        }
        Slice s;
        s.index = i;
        s.value = v;
        s.color = ChartPaletteColor(static_cast<int>(slices.size()));
        if (i == 0) {
            s.color = SeriesColor(0);
        }
        total += v;
        slices.push_back(s);
    }
    float angle = -kPi * 0.5f;
    for (auto& s : slices) {
        s.start = angle;
        s.sweep = (total > 0.0f) ? (s.value / total) * kTwoPi : 0.0f;
        angle += s.sweep;
    }
}

Point PieChart::SliceAnchor(const Rect& plot, const Slice& slice) const {
    const float radius = (std::min)(plot.width, plot.height) * 0.42f;
    const Point center(plot.x + plot.width * 0.5f, plot.y + plot.height * 0.5f);
    const float mid = slice.start + slice.sweep * 0.5f;
    const float explode = 8.0f * HoverAmount();
    return Polar(center, radius * 0.62f + explode, mid);
}

void PieChart::BindHoverMotion(const Rect& plot) {
    ChartBase::BindHoverMotion(plot);
    std::vector<Slice> slices;
    float total = 0.0f;
    CollectSlices(slices, total);
    for (const auto& slice : slices) {
        if (slice.index == m_hoverIndex) {
            const Point p = SliceAnchor(plot, slice);
            m_crossX.SetTarget(p.x);
            m_crossY.SetTarget(p.y);
            return;
        }
    }
}

void PieChart::DrawPlot(GraphicsContext& ctx, const Rect& plot) {
    std::vector<Slice> slices;
    float total = 0.0f;
    CollectSlices(slices, total);
    if (slices.empty() || total <= 0.0f) {
        D2D1_COLOR_F muted = ThemeManager::Instance().GetFlatColor(ThemeTokenId::TextMuted);
        ctx.DrawText("暂无正值", plot, muted, "微软雅黑", 13.0f,
                     DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        return;
    }

    const float reveal = std::clamp(Reveal(), 0.0f, 1.0f);
    const float ease = 1.0f - (1.0f - reveal) * (1.0f - reveal) * (1.0f - reveal);
    const float visibleEnd = -kPi * 0.5f + kTwoPi * ease;
    const float radius = (std::min)(plot.width, plot.height) * 0.42f * (0.86f + 0.14f * ease);
    const Point center(plot.x + plot.width * 0.5f, plot.y + plot.height * 0.5f);
    const float hover = HoverAmount();

    for (const auto& slice : slices) {
        const float start = slice.start;
        const float end = slice.start + slice.sweep;
        if (start >= visibleEnd) {
            continue;
        }
        const float sweep = (std::min)(end, visibleEnd) - start;
        if (sweep <= 0.004f) {
            continue;
        }
        const bool hot = slice.index == m_hoverIndex;
        D2D1_COLOR_F color = slice.color;
        if (hover > 0.01f && !hot) {
            color.a *= 1.0f - 0.55f * hover;
        }
        const float mid = slice.start + slice.sweep * 0.5f;
        const float explode = hot ? (10.0f * hover) : 0.0f;
        const Point c = Polar(center, explode, mid);
        const float r = radius + (hot ? 4.0f * hover : 0.0f);
        FillSlice(ctx, c, r, start, sweep, color);
    }
}

void PieChart::DrawLegend(GraphicsContext& ctx) {
    if (!GetShowLegend()) {
        return;
    }
    const Rect box = LegendRect();
    if (box.IsEmpty()) {
        return;
    }
    const float fade = std::clamp((Reveal() - 0.28f) / 0.5f, 0.0f, 1.0f);
    if (fade <= 0.01f) {
        return;
    }
    ctx.PushOpacity(fade);
    ctx.PushTransform(D2D1::Matrix3x2F::Translation((1.0f - fade) * 10.0f, 0.0f));
    std::vector<Slice> slices;
    float total = 0.0f;
    CollectSlices(slices, total);
    D2D1_COLOR_F text = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextSecondary);
    float y = box.y + 2.0f;
    for (const auto& slice : slices) {
        if (y + 18.0f > box.y + box.height) {
            break;
        }
        ctx.FillRoundedRect(Rect(box.x + 4.0f, y + 4.0f, 10.0f, 10.0f), 2.0f, slice.color);
        const float pct = (total > 0.0f) ? (slice.value / total * 100.0f) : 0.0f;
        const std::string label = CategoryLabel(slice.index) + "  " + FormatChartNumber(pct) + "%";
        ctx.DrawText(
            label,
            Rect(box.x + 20.0f, y, box.width - 24.0f, 18.0f),
            text, "微软雅黑", 11.0f,
            DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_NORMAL, true);
        y += 18.0f;
    }
    ctx.PopTransform();
    ctx.PopOpacity();
}

void PieChart::HitTestHover(Point pt, int& index, int& series) const {
    index = -1;
    series = -1;
    const Rect plot = PlotRect();
    const float radius = (std::min)(plot.width, plot.height) * 0.42f;
    const Point center(plot.x + plot.width * 0.5f, plot.y + plot.height * 0.5f);
    const float dx = pt.x - center.x;
    const float dy = pt.y - center.y;
    const float dist = std::sqrt(dx * dx + dy * dy);
    if (dist > radius + 8.0f) {
        return;
    }
    float angle = std::atan2(dy, dx);
    std::vector<Slice> slices;
    float total = 0.0f;
    CollectSlices(slices, total);
    for (const auto& slice : slices) {
        float a = angle;
        float start = slice.start;
        float end = slice.start + slice.sweep;
        while (a < start) {
            a += kTwoPi;
        }
        while (a > end + kTwoPi) {
            a -= kTwoPi;
        }
        if (a >= start && a <= end + 0.001f) {
            index = slice.index;
            series = 0;
            return;
        }
    }
}

void PieChart::BuildTooltipLines(std::vector<std::string>& lines) const {
    lines.clear();
    if (m_hoverIndex < 0) {
        return;
    }
    std::vector<Slice> slices;
    float total = 0.0f;
    CollectSlices(slices, total);
    for (const auto& slice : slices) {
        if (slice.index != m_hoverIndex) {
            continue;
        }
        const float pct = (total > 0.0f) ? (slice.value / total * 100.0f) : 0.0f;
        lines.push_back(CategoryLabel(slice.index));
        lines.push_back(FormatChartNumber(slice.value) + "  (" + FormatChartNumber(pct) + "%)");
        return;
    }
}

Point PieChart::TooltipAnchor() const {
    const Rect plot = PlotRect();
    std::vector<Slice> slices;
    float total = 0.0f;
    CollectSlices(slices, total);
    for (const auto& slice : slices) {
        if (slice.index == m_hoverIndex) {
            return SliceAnchor(plot, slice);
        }
    }
    return Point(plot.x + plot.width * 0.5f, plot.y + plot.height * 0.5f);
}

int PieChart::HoverIndexCount() const {
    return SeriesValueCount();
}

} // namespace CUI
