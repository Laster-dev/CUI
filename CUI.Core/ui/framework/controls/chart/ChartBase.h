#pragma once
#include "../Control.h"
#include "ChartData.h"

namespace CUI {

/**
 * @brief 所有统计图表（折线图、柱状图、饼图等）的抽象统一底盘基类（ChartBase）。
 * 接管了大部分通用的图表逻辑：
 * 1. **坐标系与网格划分**：处理笛卡尔直角坐标系建立，自动进行 Y 轴范围划分及刻度（Ticks）测算。
 * 2. **浮空高亮提示（Tooltip Card）**：当指针悬停在图表数据点上时，在锚定点弹出一个跟随的富文本提示小窗口。
 * 3. **多系列图例（Legend）**：绘制并摆放各数据系列的色块指示图例。
 * 4. **入场动效（Reveal Animation）**：数据载入时，提供图线自左向右绘制、柱状图自底向上生长的特效。
 */
class ChartBase : public Control {
public:
    ChartBase();
    virtual ~ChartBase() = default;

    virtual const char* GetClassName() const override { return "ChartBase"; } // 获取类名
    virtual Value GetProperty(PropertyId id) const override; // 反射获取属性值
    virtual bool HasProperty(PropertyId id) const override; // 检查是否存在对应属性
    void SetProperty(PropertyId id, const Value& val) override; // 反射设定属性值

    void SetCategories(std::vector<std::string> categories, bool replayReveal = true); // 设定 X 轴所有分类标签名称
    const std::vector<std::string>& GetCategories() const { return m_categories; } // 获取所有分类标签

    void SetSeries(std::vector<ChartSeries> series, bool replayReveal = true); // 设定并批量覆盖数据系列内容，并默认重新播放入场动效
    void AddSeries(ChartSeries series); // 新增添加一个数据系列
    void ClearSeries(); // 清空图表中的全部数据系列
    const std::vector<ChartSeries>& GetSeries() const { return m_series; } // 获取全部系列明细
    void SetLiveData(std::vector<std::string> categories, std::vector<ChartSeries> series, bool replayReveal = false); // 动态实时增量更新数据接口（屏蔽入场动画，防止界面高频刷新发生抖动）

    void SetShowGrid(bool show); // 设定是否在直角坐标系绘制水平/垂直背景网格线
    bool GetShowGrid() const { return m_showGrid; } // 检查是否显示网格线
    void SetShowLegend(bool show); // 设定是否在图表底部或顶部渲染系列色块图例
    bool GetShowLegend() const { return m_showLegend; } // 检查是否显示图例
    void SetShowTooltip(bool show); // 设定在鼠标滑过数据项时是否弹窗富文本提示卡片
    bool GetShowTooltip() const { return m_showTooltip; } // 检查是否显示提示卡片

    int GetHoverIndex() const { return m_hoverIndex; } // 获取鼠标当前悬停的分类项 X 轴索引位置
    int GetHoverSeries() const { return m_hoverSeries; } // 获取鼠标当前悬停的数据系列索引位置

    Event<ChartBase*, int, int>& OnHoverChanged() { return m_onHoverChanged; } // 悬停状态改变时的事件发布中心

    virtual Size Measure(Size availableSize) override; // 测算图表理想尺寸高宽
    virtual void OnRender(GraphicsContext& ctx) override; // 绘制图表背景、坐标网格、图例、以及调用 DrawPlot 重写方法绘制核心图线
    virtual void OnRenderOverlay(GraphicsContext& ctx) override; // 在覆盖层自绘悬浮跟随的提示小窗口卡片
    virtual void OnMouseMove(Point pt) override; // 鼠标滑动，测试命中并切换高亮数据项，淡入提示卡片
    virtual void OnMouseLeave() override; // 鼠标离去，清空高亮状态并淡出提示卡片
    virtual bool OnKeyDown(int vkCode) override; // 响应键盘左右方向键以流动切换高亮索引
    virtual HCURSOR GetCursor() const override; // 获取鼠标悬浮时的样式
    virtual bool AcceptsTabFocus() const override { return true; } // 支持键盘 Tab 导航聚焦
    virtual bool ShouldClipToBounds() const override { return true; } // 开启边界截断以防图线绘制出界
    virtual bool OnAnimationTick() override; // 驱动入场生长绘制以及提示卡片位移追随等动画
    virtual bool HasSelfAnimation() const override; // 检查是否仍有动画正在播放中
    virtual void CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override; // 收集图表动画造成的重绘脏矩形区域

    void PlayReveal(); // 手动重新启动一次入场生长动画

protected:
    virtual void DrawPlot(GraphicsContext& ctx, const Rect& plot) = 0; // 纯虚函数：由派生图表重写以绘制具体的图表图线（如折线或柱体）
    virtual void HitTestHover(Point pt, int& index, int& series) const = 0; // 纯虚函数：由派生图表重写以完成高精的图表点命中碰撞测试
    virtual void BuildTooltipLines(std::vector<std::string>& lines) const; // 收集并组装要放入悬停卡片上展示的各行文字内容
    virtual Point TooltipAnchor() const; // 计算本帧提示卡片应当锚定跟随的物理二维坐标位置
    virtual int HoverIndexCount() const; // 获取参与悬浮的高亮项目上限个数
    virtual void BindHoverMotion(const Rect& plot); // 绑定并计算悬浮指示十字辅助线的过渡动画目标值

    Rect ContentRect() const; // 计算考虑内边距后的图表内容区大小
    Rect PlotRect() const; // 计算笛卡尔绘图区占用的实际物理坐标矩形
    Rect LegendRect() const; // 计算底部图例占用的物理坐标矩形
    D2D1_COLOR_F SeriesColor(int index) const; // 提取指定索引的系列标志底色（如果未定制则从资源色盘依次递进读取）
    std::string CategoryLabel(int index) const; // 提取指定索引的 X 轴分类名称文本
    int SeriesValueCount() const; // 获取数据系列中的分值条数
    bool DataRange(float& minValue, float& maxValue) const; // 计算所有数据中极小和极大的分值边界
    void BuildYScale(const Rect& plot, float& yMin, float& yMax, std::vector<ChartTick>& ticks) const; // 构建 Y 轴的缩放比例尺与刻度行段
    float MapX(const Rect& plot, int index, int count) const; // 将 X 轴分类索引号映射换算为绘图区上的水平绝对 X 像素坐标
    float MapY(const Rect& plot, float value, float yMin, float yMax) const; // 将具体的数值根据 Y 轴极值缩放映射换算为垂直高度 Y 像素坐标
    void DrawCartesianFrame(GraphicsContext& ctx, const Rect& plot,
                            float yMin, float yMax, const std::vector<ChartTick>& ticks); // 绘制笛卡尔坐标系的背景网格线与 Y 轴刻度文本
    virtual void DrawLegend(GraphicsContext& ctx); // 绘制图表图例色块
    void DrawTooltipCard(GraphicsContext& ctx, const std::vector<std::string>& lines, Point anchor); // 在指定坐标自绘出精细的带阴影浮空信息提示框
    void SetHover(int index, int series); // 登记当前的 Hover 数据项索引
    void NotifyDataChanged(); // 刷新布局和图表数值
    void SyncVisibilityAnim(); // 同步卡片可见性动画
    float Reveal() const { return m_reveal.Current(); } // 获取当前的入场生长动画播放进度 (0.0f - 1.0f)
    float HoverAmount() const { return m_hoverAmount.Current(); } // 获取提示卡片的淡入淡出透明度动画进度
    float RevealAt(int index, int count, float spread = 0.42f) const; // 获取指定 X 索引处的局步入场生长动画延迟系数
    float CrossX() const { return m_crossX.Current(); } // 获取十字辅助垂直线当前的水平位置
    float CrossY() const { return m_crossY.Current(); } // 获取十字辅助水平线当前的垂直位置
    bool ChartAnimating() const; // 检查内部所有的图表时钟是否有一个仍在运行中

    std::vector<std::string> m_categories;                              // 分类轴标签明细
    std::vector<ChartSeries> m_series;                                  // 数据系列明细队列
    int m_hoverIndex = -1;                                              // 指针高亮悬停的 X 分类索引
    int m_hoverSeries = -1;                                             // 指针高亮悬停的数据系列索引
    bool m_showGrid = true;                                             // 背景网格线显示闸
    bool m_showLegend = true;                                           // 图例显示闸
    bool m_showTooltip = true;                                          // 悬停提示框显示闸
    bool m_wasVisible = false;                                          // 登记上一次是否处于视口可见中以触发首次入场动效
    AnimatedScalar m_reveal{ 0.0f };                                    // 入场生长动效时钟
    AnimatedScalar m_hoverAmount{ 0.0f };                               // 提示卡片淡入淡出动画时钟
    AnimatedScalar m_crossX{ 0.0f };                                    // 指示线 X 平移动画时钟
    AnimatedScalar m_crossY{ 0.0f };                                    // 指示线 Y 平移动画时钟
    AnimatedScalar m_tipX{ 0.0f };                                      // 提示卡片水平位移跟随动画
    AnimatedScalar m_tipY{ 0.0f };                                      // 提示卡片垂直位移跟随动画
    std::vector<std::string> m_tipLines;                                // 暂存的当前提示文本各行内容
    Event<ChartBase*, int, int> m_onHoverChanged;                       // 悬停状态改变事件对象
};

} // namespace CUI
