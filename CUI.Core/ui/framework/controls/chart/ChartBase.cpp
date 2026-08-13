#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ChartBase.h"
#include "../../style/ThemeManager.h"
#include <algorithm>
#include <cmath>

#ifndef VK_LEFT
#define VK_LEFT 0x25
#define VK_RIGHT 0x27
#define VK_HOME 0x24
#define VK_END 0x23
#endif

namespace CUI {

namespace {
constexpr float kPad = 12.0f;
constexpr float kTitleH = 22.0f;
constexpr float kAxisLeft = 44.0f;
constexpr float kAxisBottom = 26.0f;
constexpr float kLegendW = 118.0f;
constexpr float kLegendRow = 18.0f;
constexpr AnimationSpec kRevealSpec{ 0.22f, 0.008f, 0.58f };
constexpr AnimationSpec kHoverSpec{ 0.20f, 0.01f, 0.18f };
constexpr AnimationSpec kFollowSpec{ 0.16f, 0.4f, 0.0f };

D2D1_COLOR_F WithAlpha(D2D1_COLOR_F c, float a) {
    c.a = std::clamp(a, 0.0f, 1.0f);
    return c;
}

float EaseOutCubic(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    const float u = 1.0f - t;
    return 1.0f - u * u * u;
}
} // namespace

ChartBase::ChartBase() {
    SetBackgroundToken(ThemeTokenId::CardBackground);
    SetBorderToken(ThemeTokenId::CardBorder);
    SetBorderThickness(1.0f);
    SetCornerRadius(6.0f);
    SetColorToken(ThemeTokenId::TextSecondary);
    SetTitleColorToken(ThemeTokenId::TextPrimary);
    SetHoverBackground(D2D1::ColorF(0, 0, 0, 0));
    SetPressedBackground(D2D1::ColorF(0, 0, 0, 0));
    SetWidth(-1.0f);
    SetHeight(260.0f);
    SetClipToBounds(true);
    m_reveal.Reset(0.0f);
    m_reveal.SetTarget(1.0f);
}

void ChartBase::SetCategories(std::vector<std::string> categories, bool replayReveal) {
    if (m_categories == categories) {
        return;
    }
    m_categories = std::move(categories);
    if (replayReveal) {
        NotifyDataChanged();
    } else {
        if (m_hoverIndex >= SeriesValueCount()) {
            SetHover(-1, -1);
        }
        MarkRenderRectDirty(m_bounds);
    }
}

void ChartBase::SetSeries(std::vector<ChartSeries> series, bool replayReveal) {
    m_series = std::move(series);
    if (replayReveal) {
        NotifyDataChanged();
    } else {
        if (m_hoverIndex >= SeriesValueCount()) {
            SetHover(-1, -1);
        }
        MarkRenderRectDirty(m_bounds);
    }
}

void ChartBase::SetLiveData(std::vector<std::string> categories, std::vector<ChartSeries> series, bool replayReveal) {
    m_categories = std::move(categories);
    m_series = std::move(series);
    if (replayReveal) {
        NotifyDataChanged();
        return;
    }
    if (m_hoverIndex >= SeriesValueCount()) {
        m_hoverIndex = -1;
        m_hoverSeries = -1;
        m_hoverAmount.SetTarget(0.0f);
    }
    MarkRenderRectDirty(m_bounds);
}

void ChartBase::AddSeries(ChartSeries series) {
    m_series.push_back(std::move(series));
    NotifyDataChanged();
}

void ChartBase::ClearSeries() {
    if (m_series.empty()) {
        return;
    }
    m_series.clear();
    NotifyDataChanged();
}

void ChartBase::SetShowGrid(bool show) {
    if (m_showGrid == show) {
        return;
    }
    m_showGrid = show;
    MarkRenderRectDirty(m_bounds);
}

void ChartBase::SetShowLegend(bool show) {
    if (m_showLegend == show) {
        return;
    }
    m_showLegend = show;
    MarkRenderRectDirty(m_bounds);
}

void ChartBase::SetShowTooltip(bool show) {
    if (m_showTooltip == show) {
        return;
    }
    m_showTooltip = show;
    MarkRenderRectDirty(m_bounds);
}

void ChartBase::NotifyDataChanged() {
    m_hoverIndex = -1;
    m_hoverSeries = -1;
    m_hoverAmount.SetTarget(0.0f);
    PlayReveal();
}

void ChartBase::PlayReveal() {
    if (!UIElement::AreAnimationsEnabled()) {
        m_reveal.Reset(1.0f);
        MarkRenderRectDirty(m_bounds);
        return;
    }
    m_reveal.Reset(0.0f);
    m_reveal.SetTarget(1.0f);
    RequestAnimationTicks();
    MarkRenderRectDirty(m_bounds);
}

void ChartBase::SyncVisibilityAnim() {
    const bool vis = GetVisibility() == Visibility::Visible;
    if (vis == m_wasVisible) {
        return;
    }
    m_wasVisible = vis;
    if (vis) {
        PlayReveal();
    }
}

float ChartBase::RevealAt(int index, int count, float spread) const {
    const float t = std::clamp(m_reveal.Current(), 0.0f, 1.0f);
    if (count <= 1 || spread <= 0.001f) {
        return EaseOutCubic(t);
    }
    spread = std::clamp(spread, 0.0f, 0.85f);
    const float start = (static_cast<float>(std::clamp(index, 0, count - 1))
        / static_cast<float>(count - 1)) * spread;
    const float local = (t - start) / (1.0f - spread);
    return EaseOutCubic(local);
}

bool ChartBase::ChartAnimating() const {
    return m_reveal.IsAnimating(kRevealSpec.epsilon)
        || m_hoverAmount.IsAnimating(kHoverSpec.epsilon)
        || m_crossX.IsAnimating(0.4f)
        || m_crossY.IsAnimating(0.4f)
        || m_tipX.IsAnimating(0.4f)
        || m_tipY.IsAnimating(0.4f);
}

Size ChartBase::Measure(Size availableSize) {
    float w = GetWidth();
    float h = GetHeight();
    if (w < 0.0f) {
        w = (availableSize.width > 0.0f) ? availableSize.width : 480.0f;
    }
    if (h < 0.0f) {
        h = 260.0f;
    }
    m_desiredSize = Size(w, h);
    return m_desiredSize;
}

Rect ChartBase::ContentRect() const {
    return Rect(
        m_bounds.x + kPad,
        m_bounds.y + kPad,
        (std::max)(0.0f, m_bounds.width - kPad * 2.0f),
        (std::max)(0.0f, m_bounds.height - kPad * 2.0f));
}

Rect ChartBase::LegendRect() const {
    if (!m_showLegend) {
        return Rect();
    }
    const Rect content = ContentRect();
    const float top = content.y + (GetText().empty() ? 0.0f : kTitleH);
    return Rect(content.x + content.width - kLegendW, top, kLegendW,
                (std::max)(0.0f, content.y + content.height - top));
}

Rect ChartBase::PlotRect() const {
    const Rect content = ContentRect();
    float top = content.y;
    if (!GetText().empty()) {
        top += kTitleH;
    }
    float right = content.x + content.width;
    if (m_showLegend) {
        right -= kLegendW + 8.0f;
    }
    return Rect(
        content.x + kAxisLeft,
        top,
        (std::max)(0.0f, right - (content.x + kAxisLeft)),
        (std::max)(0.0f, content.y + content.height - kAxisBottom - top));
}

D2D1_COLOR_F ChartBase::SeriesColor(int index) const {
    if (index >= 0 && index < static_cast<int>(m_series.size()) && m_series[index].hasColor) {
        return m_series[index].color;
    }
    if (index == 0) {
        return ThemeManager::Instance().GetFlatColor(ThemeTokenId::AccentColor);
    }
    return ChartPaletteColor(index);
}

std::string ChartBase::CategoryLabel(int index) const {
    if (index >= 0 && index < static_cast<int>(m_categories.size())) {
        return m_categories[index];
    }
    return std::to_string(index + 1);
}

int ChartBase::SeriesValueCount() const {
    return CategoryCount(m_categories, m_series);
}

bool ChartBase::DataRange(float& minValue, float& maxValue) const {
    bool any = false;
    minValue = 0.0f;
    maxValue = 1.0f;
    for (const auto& series : m_series) {
        for (float v : series.values) {
            if (!std::isfinite(v)) {
                continue;
            }
            if (!any) {
                minValue = v;
                maxValue = v;
                any = true;
            } else {
                minValue = (std::min)(minValue, v);
                maxValue = (std::max)(maxValue, v);
            }
        }
    }
    return any;
}

void ChartBase::BuildYScale(const Rect& plot, float& yMin, float& yMax, std::vector<ChartTick>& ticks) const {
    float dataMin = 0.0f;
    float dataMax = 1.0f;
    if (!DataRange(dataMin, dataMax)) {
        dataMin = 0.0f;
        dataMax = 1.0f;
    }
    if (dataMin >= 0.0f) {
        dataMin = 0.0f;
    }
    if (dataMax <= 0.0f) {
        dataMax = 0.0f;
    }
    const int maxTicks = std::clamp(static_cast<int>(plot.height / 28.0f), 3, 8);
    ComputeNiceScale(dataMin, dataMax, maxTicks, yMin, yMax, ticks);
}

float ChartBase::MapX(const Rect& plot, int index, int count) const {
    if (count <= 1) {
        return plot.x + plot.width * 0.5f;
    }
    const float t = static_cast<float>(index) / static_cast<float>(count - 1);
    return plot.x + t * plot.width;
}

float ChartBase::MapY(const Rect& plot, float value, float yMin, float yMax) const {
    const float span = (yMax - yMin);
    if (span <= 1.0e-6f) {
        return plot.y + plot.height * 0.5f;
    }
    const float t = (value - yMin) / span;
    return plot.y + (1.0f - t) * plot.height;
}

void ChartBase::DrawCartesianFrame(GraphicsContext& ctx, const Rect& plot,
                                   float yMin, float yMax, const std::vector<ChartTick>& ticks) {
    const float fade = std::clamp(Reveal() * 1.35f, 0.0f, 1.0f);
    ctx.PushOpacity(fade);
    D2D1_COLOR_F axis = ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBorder);
    D2D1_COLOR_F label = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextSecondary);
    D2D1_COLOR_F grid = WithAlpha(axis, axis.a * 0.45f);

    ctx.DrawLine(Point(plot.x, plot.y), Point(plot.x, plot.y + plot.height), axis, 1.0f);
    ctx.DrawLine(Point(plot.x, plot.y + plot.height), Point(plot.x + plot.width, plot.y + plot.height), axis, 1.0f);

    if (m_showGrid) {
        for (const auto& tick : ticks) {
            const float y = MapY(plot, tick.value, yMin, yMax);
            ctx.DrawLine(Point(plot.x, y), Point(plot.x + plot.width, y), grid, 1.0f);
        }
    }

    for (const auto& tick : ticks) {
        const float y = MapY(plot, tick.value, yMin, yMax);
        ctx.DrawText(
            tick.label,
            Rect(plot.x - kAxisLeft, y - 8.0f, kAxisLeft - 6.0f, 16.0f),
            label, "微软雅黑", 10.0f,
            DWRITE_TEXT_ALIGNMENT_TRAILING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    const int count = SeriesValueCount();
    if (count <= 0) {
        return;
    }
    int step = 1;
    const float minLabel = 36.0f;
    if (count > 1) {
        const float slot = plot.width / static_cast<float>(count - 1);
        if (slot < minLabel) {
            step = (std::max)(1, static_cast<int>(std::ceil(minLabel / slot)));
        }
    }
    for (int i = 0; i < count; i += step) {
        const float x = MapX(plot, i, count);
        ctx.DrawText(
            CategoryLabel(i),
            Rect(x - 28.0f, plot.y + plot.height + 4.0f, 56.0f, 18.0f),
            label, "微软雅黑", 10.0f,
            DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    }
    ctx.PopOpacity();
}

void ChartBase::DrawLegend(GraphicsContext& ctx) {
    if (!m_showLegend) {
        return;
    }
    const Rect box = LegendRect();
    if (box.IsEmpty()) {
        return;
    }
    const float fade = std::clamp((Reveal() - 0.25f) / 0.55f, 0.0f, 1.0f);
    if (fade <= 0.01f) {
        return;
    }
    ctx.PushOpacity(fade);
    ctx.PushTransform(D2D1::Matrix3x2F::Translation((1.0f - fade) * 10.0f, 0.0f));
    D2D1_COLOR_F text = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextSecondary);
    float y = box.y + 2.0f;
    const int rows = m_series.empty() ? 0 : static_cast<int>(m_series.size());
    for (int i = 0; i < rows; ++i) {
        if (y + kLegendRow > box.y + box.height) {
            break;
        }
        const Rect swatch(box.x + 4.0f, y + 4.0f, 10.0f, 10.0f);
        ctx.FillRoundedRect(swatch, 2.0f, SeriesColor(i));
        const std::string& name = m_series[i].name.empty() ? ("系列 " + std::to_string(i + 1)) : m_series[i].name;
        ctx.DrawText(
            name,
            Rect(box.x + 20.0f, y, box.width - 24.0f, kLegendRow),
            text, "微软雅黑", 11.0f,
            DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER,             DWRITE_FONT_WEIGHT_NORMAL, true);
        y += kLegendRow;
    }
    ctx.PopTransform();
    ctx.PopOpacity();
}

void ChartBase::DrawTooltipCard(GraphicsContext& ctx, const std::vector<std::string>& lines, Point anchor) {
    if (!m_showTooltip || lines.empty()) {
        return;
    }
    float maxW = 0.0f;
    float h = 8.0f;
    for (const auto& line : lines) {
        Size sz = ctx.MeasureText(line, "微软雅黑", 11.0f);
        maxW = (std::max)(maxW, sz.width);
        h += 16.0f;
    }
    const float cardW = maxW + 16.0f;
    const float cardH = h;
    float x = anchor.x + 12.0f;
    float y = anchor.y - cardH - 8.0f;
    if (x + cardW > m_bounds.x + m_bounds.width - 4.0f) {
        x = anchor.x - cardW - 12.0f;
    }
    if (y < m_bounds.y + 4.0f) {
        y = anchor.y + 12.0f;
    }
    const Rect card(x, y, cardW, cardH);
    D2D1_COLOR_F bg = ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBackground);
    D2D1_COLOR_F border = ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBorder);
    D2D1_COLOR_F text = ThemeManager::Instance().GetFlatColor(ThemeTokenId::TextPrimary);
    ctx.FillRoundedRect(card, 5.0f, bg);
    ctx.DrawRoundedRect(card, 5.0f, border, 1.0f);
    float ty = card.y + 4.0f;
    for (const auto& line : lines) {
        ctx.DrawText(line, Rect(card.x + 8.0f, ty, card.width - 16.0f, 16.0f),
                     text, "微软雅黑", 11.0f);
        ty += 16.0f;
    }
}

void ChartBase::OnRender(GraphicsContext& ctx) {
    SyncVisibilityAnim();
    Control::OnRender(ctx);
    const Rect content = ContentRect();
    if (content.IsEmpty()) {
        return;
    }
    if (!GetText().empty()) {
        D2D1_COLOR_F title = ResolveThemeColor(GetTitleColorToken(), ThemeTokenId::TextPrimary);
        ctx.DrawText(
            GetText(),
            Rect(content.x, content.y, content.width, kTitleH - 4.0f),
            title, "微软雅黑", 13.0f,
            DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_SEMI_BOLD);
    }

    const Rect plot = PlotRect();
    if (plot.IsEmpty() || m_series.empty() || SeriesValueCount() <= 0) {
        D2D1_COLOR_F muted = ThemeManager::Instance().GetFlatColor(ThemeTokenId::TextMuted);
        ctx.DrawText("暂无数据", content, muted, "微软雅黑", 13.0f,
                     DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        return;
    }

    DrawPlot(ctx, plot);
    DrawLegend(ctx);
}

void ChartBase::OnRenderOverlay(GraphicsContext& ctx) {
    const float amount = HoverAmount();
    if (!m_showTooltip || amount < 0.01f) {
        return;
    }
    if (m_hoverIndex >= 0) {
        BuildTooltipLines(m_tipLines);
    }
    if (m_tipLines.empty()) {
        return;
    }
    ctx.PushOpacity(amount);
    DrawTooltipCard(ctx, m_tipLines, Point(m_tipX.Current(), m_tipY.Current()));
    ctx.PopOpacity();
}

void ChartBase::BuildTooltipLines(std::vector<std::string>& lines) const {
    lines.clear();
    if (m_hoverIndex < 0) {
        return;
    }
    lines.push_back(CategoryLabel(m_hoverIndex));
    for (int s = 0; s < static_cast<int>(m_series.size()); ++s) {
        if (m_hoverSeries >= 0 && s != m_hoverSeries) {
            continue;
        }
        float value = 0.0f;
        if (m_hoverIndex < static_cast<int>(m_series[s].values.size())) {
            value = m_series[s].values[m_hoverIndex];
        }
        const std::string name = m_series[s].name.empty() ? ("系列 " + std::to_string(s + 1)) : m_series[s].name;
        lines.push_back(name + "  " + FormatChartNumber(value));
    }
}

Point ChartBase::TooltipAnchor() const {
    const Rect plot = PlotRect();
    const int count = SeriesValueCount();
    return Point(MapX(plot, m_hoverIndex, count), plot.y + plot.height * 0.35f);
}

int ChartBase::HoverIndexCount() const {
    return SeriesValueCount();
}

void ChartBase::BindHoverMotion(const Rect& plot) {
    const int count = SeriesValueCount();
    if (m_hoverIndex < 0 || count <= 0 || plot.IsEmpty()) {
        return;
    }
    m_crossX.SetTarget(MapX(plot, m_hoverIndex, count));
    m_crossY.SetTarget(plot.y + plot.height * 0.35f);
    const Point tip = TooltipAnchor();
    m_tipX.SetTarget(tip.x);
    m_tipY.SetTarget(tip.y);
}

void ChartBase::SetHover(int index, int series) {
    if (m_hoverIndex == index && m_hoverSeries == series) {
        return;
    }
    m_hoverIndex = index;
    m_hoverSeries = series;
    m_hoverAmount.SetTarget(index >= 0 ? 1.0f : 0.0f);
    if (index >= 0 && m_hoverAmount.Current() < 0.05f) {
        const Rect plot = PlotRect();
        BindHoverMotion(plot);
        m_crossX.Reset(m_crossX.Target());
        m_crossY.Reset(m_crossY.Target());
        m_tipX.Reset(m_tipX.Target());
        m_tipY.Reset(m_tipY.Target());
    }
    RequestAnimationTicks();
    MarkRenderRectDirty(m_bounds);
    m_onHoverChanged.Invoke(this, m_hoverIndex, m_hoverSeries);
}

void ChartBase::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);
    int index = -1;
    int series = -1;
    HitTestHover(pt, index, series);
    SetHover(index, series);
}

void ChartBase::OnMouseLeave() {
    Control::OnMouseLeave();
    SetHover(-1, -1);
}

bool ChartBase::OnKeyDown(int vkCode) {
    const int count = HoverIndexCount();
    if (count <= 0) {
        return false;
    }
    int next = m_hoverIndex;
    if (vkCode == VK_LEFT) {
        next = (m_hoverIndex < 0) ? (count - 1) : (m_hoverIndex - 1);
        if (next < 0) {
            next = count - 1;
        }
    } else if (vkCode == VK_RIGHT) {
        next = (m_hoverIndex < 0) ? 0 : (m_hoverIndex + 1);
        if (next >= count) {
            next = 0;
        }
    } else if (vkCode == VK_HOME) {
        next = 0;
    } else if (vkCode == VK_END) {
        next = count - 1;
    } else {
        return false;
    }
    SetHover(next, -1);
    return true;
}

HCURSOR ChartBase::GetCursor() const {
    return LoadCursor(nullptr, IDC_CROSS);
}

bool ChartBase::OnAnimationTick() {
    bool any = Control::OnAnimationTick();
    SyncVisibilityAnim();
    if (GetVisibility() != Visibility::Visible) {
        return any;
    }
    if (!UIElement::AreAnimationsEnabled()) {
        m_reveal.Reset(1.0f);
        m_hoverAmount.Reset(m_hoverIndex >= 0 ? 1.0f : 0.0f);
        return any;
    }

    const Rect plot = PlotRect();
    if (m_hoverIndex >= 0) {
        BindHoverMotion(plot);
    }

    const float dt = UIElement::GetAnimationDeltaSeconds();
    any = m_reveal.Tick(dt, kRevealSpec) || any;
    any = m_hoverAmount.Tick(dt, kHoverSpec) || any;
    any = m_crossX.Tick(dt, kFollowSpec) || any;
    any = m_crossY.Tick(dt, kFollowSpec) || any;
    any = m_tipX.Tick(dt, kFollowSpec) || any;
    any = m_tipY.Tick(dt, kFollowSpec) || any;
    if (any) {
        MarkRenderRectDirty(m_bounds);
        RequestAnimationTicks();
    }
    return any;
}

bool ChartBase::HasSelfAnimation() const {
    if (GetVisibility() != Visibility::Visible) {
        return Control::HasSelfAnimation();
    }
    return Control::HasSelfAnimation() || ChartAnimating();
}

void ChartBase::CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const {
    UIElement::CollectSelfAnimationBounds(dirtyRect, hasDirty);
    if (GetVisibility() != Visibility::Visible || m_bounds.IsEmpty()) {
        return;
    }
    if (ChartAnimating() || HoverAmount() > 0.01f || Reveal() < 0.999f) {
        dirtyRect = hasDirty ? dirtyRect.Union(m_bounds) : m_bounds;
        hasDirty = true;
    }
}

} // namespace CUI
