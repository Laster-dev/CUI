#pragma once
#include "ChartBase.h"

namespace CUI {

/**
 * @brief 饼状图/环形图统计图表（PieChart）。
 * 继承自 ChartBase。用于绘制按百分比划分的圆弧扇区，并在 Hover 时平滑向外凸出（Pop-out）以作高亮强调。
 */
class PieChart : public ChartBase {
public:
    PieChart();
    virtual ~PieChart() = default;

    virtual const char* GetClassName() const override { return "PieChart"; } // 获取类名

protected:
    virtual void DrawPlot(GraphicsContext& ctx, const Rect& plot) override; // 重写：绘制各百分比扇形区以及 Hover 时的平滑弹出偏移动画
    virtual void HitTestHover(Point pt, int& index, int& series) const override; // 重写：命中检测，根据鼠标在圆形区域内的夹角与半径判定滑过的扇区
    virtual void BuildTooltipLines(std::vector<std::string>& lines) const override; // 重写：在提示信息中额外计算并组装百分比数值行
    virtual Point TooltipAnchor() const override; // 重写：计算悬浮提示窗口指向的目标扇形弧边中心点坐标位置
    virtual int HoverIndexCount() const override; // 重写：饼图无 X 分类概念，返回数据项系列个数
    virtual void DrawLegend(GraphicsContext& ctx) override; // 重写：自绘基于各个百分比扇区色块的说明图例
    virtual void BindHoverMotion(const Rect& plot) override; // 重写：计算饼图提示卡片定位跟随的动画参数

private:
    struct Slice {
        int index = 0;        // 扇区所属的数值索引位置
        float value = 0.0f;   // 扇区代表的真实数值大小
        float start = 0.0f;   // 扇区圆弧起始角度值（弧度制）
        float sweep = 0.0f;   // 扇区圆弧跨越跨度角度值（弧度制）
        D2D1_COLOR_F color{}; // 扇区的色彩分配值
    };

    void CollectSlices(std::vector<Slice>& slices, float& total) const; // 辅助方法：汇总所有系列的值并折算生成各个 Slice 扇区几何角度与占比
    Point SliceAnchor(const Rect& plot, const Slice& slice) const; // 辅助方法：计算对应扇形弧线几何中心物理坐标点

    // 无私有成员变量
};

} // namespace CUI
