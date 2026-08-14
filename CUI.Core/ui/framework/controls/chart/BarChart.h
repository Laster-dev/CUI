#pragma once
#include "ChartBase.h"

namespace CUI {

/**
 * @brief 柱状统计图表（BarChart）。
 * 继承自 ChartBase。用于绘制垂直分布、带圆角的单系列或多系列并排柱体，支持自底向上的生长动效。
 */
class BarChart : public ChartBase {
public:
    BarChart();
    virtual ~BarChart() = default;

    virtual const char* GetClassName() const override { return "BarChart"; } // 获取类名

protected:
    virtual void DrawPlot(GraphicsContext& ctx, const Rect& plot) override; // 重写：绘制带圆角的彩色实体柱体及 Hover 高亮效果
    virtual void HitTestHover(Point pt, int& index, int& series) const override; // 重写：命中检测，精准定位光标落入的特定柱体位置
    virtual Point TooltipAnchor() const override; // 重写：计算悬浮提示窗口指向的目标柱体顶部中心点坐标位置
    virtual void BindHoverMotion(const Rect& plot) override; // 重写：计算十字线和提示浮窗在柱体间平滑移动的动画参数

private:
    bool BarRect(const Rect& plot, int category, int series, Rect& out) const; // 辅助方法：计算第 series 系列、第 category 分类下柱子的逻辑像素包络大小

    // 无私有成员变量
};

} // namespace CUI
