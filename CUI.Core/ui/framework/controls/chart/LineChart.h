#pragma once
#include "ChartBase.h"

namespace CUI {

/**
 * @brief 折线统计图表（LineChart）。
 * 继承自 ChartBase。用于绘制渐变折线、数据圆点标记以及下方的半透明面积填充阴影。
 */
class LineChart : public ChartBase {
public:
    LineChart();
    virtual ~LineChart() = default;

    virtual const char* GetClassName() const override { return "LineChart"; } // 获取类名

protected:
    virtual void DrawPlot(GraphicsContext& ctx, const Rect& plot) override; // 重写：自绘折线段、填充区域和高亮小圆点
    virtual void HitTestHover(Point pt, int& index, int& series) const override; // 重写：依据水平 X 距离远近计算命中的分类索引位置
    virtual Point TooltipAnchor() const override; // 重写：计算悬浮提示窗口指向的目标折线交点坐标位置
    virtual void BindHoverMotion(const Rect& plot) override; // 重写：计算十字线和提示浮窗在折线交点间平滑滑动的动画参数
};

} // namespace CUI
