#pragma once
#include "ChartBase.h"

namespace CUI {

class PieChart : public ChartBase {
public:
    PieChart();
    virtual ~PieChart() = default;

    virtual const char* GetClassName() const override { return "PieChart"; }

protected:
    virtual void DrawPlot(GraphicsContext& ctx, const Rect& plot) override;
    virtual void HitTestHover(Point pt, int& index, int& series) const override;
    virtual void BuildTooltipLines(std::vector<std::string>& lines) const override;
    virtual Point TooltipAnchor() const override;
    virtual int HoverIndexCount() const override;
    virtual void DrawLegend(GraphicsContext& ctx) override;
    virtual void BindHoverMotion(const Rect& plot) override;

private:
    struct Slice {
        int index = 0;
        float value = 0.0f;
        float start = 0.0f;
        float sweep = 0.0f;
        D2D1_COLOR_F color{};
    };

    void CollectSlices(std::vector<Slice>& slices, float& total) const;
    Point SliceAnchor(const Rect& plot, const Slice& slice) const;
};

} // namespace CUI
