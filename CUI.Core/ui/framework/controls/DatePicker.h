#pragma once
#include "Control.h"
#include "ScrollbarAutoHide.h"
#include "../window/PopupHost.h"
#include <ctime>

namespace CUI {

/**
 * @brief 日期选择器的视图渲染模式。
 * DayGrid: 日网格（选择具体某天）。
 * MonthGrid: 月网格（选择具体某月）。
 * YearGrid: 年网格（选择具体某年）。
 */
enum class DatePickerViewMode {
    DayGrid,   // 天视图选择模式
    MonthGrid, // 月视图选择模式
    YearGrid   // 年视图选择模式
};

/**
 * @brief 日期选择器（DatePicker）控件。
 * 允许用户点击按钮展现一个日历大面板，并从中选择具体的年、月、日日期值。实现了 IPopup 接口。
 */
class DatePicker : public Control, public IPopup {
public:
    DatePicker();
    virtual ~DatePicker() = default;

    virtual const char* GetClassName() const override { return "DatePicker"; } // 获取类名
    virtual Value GetProperty(PropertyId id) const override; // 反射获取属性值
    virtual bool HasProperty(PropertyId id) const override; // 检查是否存在对应属性
    void SetProperty(PropertyId id, const Value& val) override; // 反射设定属性值
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; } // 获取悬浮交互鼠标样式

    virtual Size Measure(Size availableSize) override; // 计算日期展示框的理想自适应尺寸
    virtual void OnRender(GraphicsContext& ctx) override; // 绘制正常状态下的日期标签与下拉箭头
    virtual void OnRenderOverlay(GraphicsContext& ctx) override; // 绘制弹出的日历大面板
    virtual UIElement* OnHitTestOverlay(float x, float y) override; // 弹出层非客户区命中穿透定位
    virtual bool NeedsOverlayHitTest() const override { return true; } // 声明需要高频弹窗命中响应
    virtual void OnMouseDown(Point pt) override; // 鼠标按下，分析选中的是具体年、月、日还是左右翻页键
    virtual void OnMouseWheel(float delta) override; // 响应鼠标滚轮，快速翻动月度/年度列表
    virtual bool OnAnimationTick() override; // 驱动弹出面板的缩放展开折叠动画
    virtual bool HasSelfAnimation() const override; // 检查是否依然处于过渡动画中

    // IPopup 弹出层接口实现
    virtual bool IsPopupOpen() const override { return m_isPopupOpen; } // 获取弹出层是否已展开
    virtual Rect GetPopupBounds() const override; // 获取弹出层在全局视口坐标系下的边界包络盒
    virtual bool HitDismissExempt(float x, float y) const override; // 判定该点击位置是否免于强制消退收起
    virtual UIElement* HitTestPopup(float x, float y) override { return OnHitTestOverlay(x, y); } // 弹窗穿透命中定位
    virtual void RenderPopup(GraphicsContext& ctx) override; // 绘制具体的日历网格及月份/年份选择盘
    virtual void OnLightDismiss() override { SetPopupOpen(false); } // 轻点空白背景消退收拢关闭

    void SetPopupOpen(bool open); // 打开或折叠日期面板

    int GetYear() const { return m_year; } // 获取当前设置的年份
    int GetMonth() const { return m_month; } // 获取当前设置的月份（1-12）
    int GetDay() const { return m_day; } // 获取当前设置的日期（1-31）
    void SetDate(int year, int month, int day); // 设定具体日期并刷新日历状态
    std::string GetFormattedDate() const; // 读取以 "YYYY-MM-DD" 格式化后的日期字符串

    PropertyRef<std::string, PropertyId::DateStr> SelectedDate;

    Event<DatePicker*, int, int, int>& OnDateChanged() { return m_onDateChangedEvent; } // 日期改变时的事件发布中心

private:
    int m_year = 2026;                                  // 当前年份值
    int m_month = 7;                                    // 当前月份值
    int m_day = 30;                                     // 当前具体日期天数
    int m_viewStartYear = 2020;                         // 年选择视图中排版显示的起始年份基准值
    DatePickerViewMode m_viewMode = DatePickerViewMode::DayGrid; // 当前日历面板的视图模式（天/月/年）
    bool m_isPopupOpen = false;                         // 日期面板是否正处于弹开状态
    float m_scrollOffset = 0.0f;                        // 面板项长内容时的内部滚动平移量
    ScrollbarAutoHide m_scrollbarAutoHide;              // 自带的自动隐藏滚动条托管器
    AnimatedScalar m_popupAnim{};                       // 弹窗层折展动画进度
    Event<DatePicker*, int, int, int> m_onDateChangedEvent; // 日期更改事件对象
};

} // namespace CUI
