#pragma once
#include "ChartBase.h"

namespace CUI {

class BarChart : public ChartBase {
public:
    BarChart();
    virtual ~BarChart() = default;

    virtual const char* GetClassName() const override { return "BarChart"; }

protected:
    virtual void DrawPlot(GraphicsContext& ctx, const Rect& plot) override;
    virtual void HitTestHover(Point pt, int& index, int& series) const override;
    virtual Point TooltipAnchor() const override;
    virtual void BindHoverMotion(const Rect& plot) override;

private:
    bool BarRect(const Rect& plot, int category, int series, Rect& out) const;
};

} // namespace CUI
