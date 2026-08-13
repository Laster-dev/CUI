#pragma once
#include "../Control.h"
#include "ChartData.h"

namespace CUI {

class ChartBase : public Control {
public:
    ChartBase();
    virtual ~ChartBase() = default;

    virtual const char* GetClassName() const override { return "ChartBase"; }
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void SetProperty(PropertyId id, const Value& val) override;

    void SetCategories(std::vector<std::string> categories, bool replayReveal = true);
    const std::vector<std::string>& GetCategories() const { return m_categories; }

    void SetSeries(std::vector<ChartSeries> series, bool replayReveal = true);
    void AddSeries(ChartSeries series);
    void ClearSeries();
    const std::vector<ChartSeries>& GetSeries() const { return m_series; }
    // Rolling/live update: no intro replay, hover clamped, local dirty only.
    void SetLiveData(std::vector<std::string> categories, std::vector<ChartSeries> series, bool replayReveal = false);

    void SetShowGrid(bool show);
    bool GetShowGrid() const { return m_showGrid; }
    void SetShowLegend(bool show);
    bool GetShowLegend() const { return m_showLegend; }
    void SetShowTooltip(bool show);
    bool GetShowTooltip() const { return m_showTooltip; }

    int GetHoverIndex() const { return m_hoverIndex; }
    int GetHoverSeries() const { return m_hoverSeries; }

    Event<ChartBase*, int, int>& OnHoverChanged() { return m_onHoverChanged; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnRenderOverlay(GraphicsContext& ctx) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual HCURSOR GetCursor() const override;
    virtual bool AcceptsTabFocus() const override { return true; }
    virtual bool ShouldClipToBounds() const override { return true; }
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual void CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override;

    void PlayReveal();

protected:
    virtual void DrawPlot(GraphicsContext& ctx, const Rect& plot) = 0;
    virtual void HitTestHover(Point pt, int& index, int& series) const = 0;
    virtual void BuildTooltipLines(std::vector<std::string>& lines) const;
    virtual Point TooltipAnchor() const;
    virtual int HoverIndexCount() const;
    virtual void BindHoverMotion(const Rect& plot);

    Rect ContentRect() const;
    Rect PlotRect() const;
    Rect LegendRect() const;
    D2D1_COLOR_F SeriesColor(int index) const;
    std::string CategoryLabel(int index) const;
    int SeriesValueCount() const;
    bool DataRange(float& minValue, float& maxValue) const;
    void BuildYScale(const Rect& plot, float& yMin, float& yMax, std::vector<ChartTick>& ticks) const;
    float MapX(const Rect& plot, int index, int count) const;
    float MapY(const Rect& plot, float value, float yMin, float yMax) const;
    void DrawCartesianFrame(GraphicsContext& ctx, const Rect& plot,
                            float yMin, float yMax, const std::vector<ChartTick>& ticks);
    virtual void DrawLegend(GraphicsContext& ctx);
    void DrawTooltipCard(GraphicsContext& ctx, const std::vector<std::string>& lines, Point anchor);
    void SetHover(int index, int series);
    void NotifyDataChanged();
    void SyncVisibilityAnim();
    float Reveal() const { return m_reveal.Current(); }
    float HoverAmount() const { return m_hoverAmount.Current(); }
    float RevealAt(int index, int count, float spread = 0.42f) const;
    float CrossX() const { return m_crossX.Current(); }
    float CrossY() const { return m_crossY.Current(); }
    bool ChartAnimating() const;

    std::vector<std::string> m_categories;
    std::vector<ChartSeries> m_series;
    int m_hoverIndex = -1;
    int m_hoverSeries = -1;
    bool m_showGrid = true;
    bool m_showLegend = true;
    bool m_showTooltip = true;
    bool m_wasVisible = false;
    AnimatedScalar m_reveal{ 0.0f };
    AnimatedScalar m_hoverAmount{ 0.0f };
    AnimatedScalar m_crossX{ 0.0f };
    AnimatedScalar m_crossY{ 0.0f };
    AnimatedScalar m_tipX{ 0.0f };
    AnimatedScalar m_tipY{ 0.0f };
    std::vector<std::string> m_tipLines;
    Event<ChartBase*, int, int> m_onHoverChanged;
};

} // namespace CUI
