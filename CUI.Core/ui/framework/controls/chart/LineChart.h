#pragma once
#include "ChartBase.h"

namespace CUI {

class LineChart : public ChartBase {
public:
    LineChart();
    virtual ~LineChart() = default;

    virtual const char* GetClassName() const override { return "LineChart"; }

protected:
    virtual void DrawPlot(GraphicsContext& ctx, const Rect& plot) override;
    virtual void HitTestHover(Point pt, int& index, int& series) const override;
    virtual Point TooltipAnchor() const override;
    virtual void BindHoverMotion(const Rect& plot) override;
};

} // namespace CUI
