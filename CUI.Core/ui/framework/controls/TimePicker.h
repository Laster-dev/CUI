#pragma once
#include "Control.h"
#include "../window/PopupHost.h"
#include <chrono>

namespace CUI {

/**
 * @brief 时间选择器（TimePicker）控件。
 * 允许用户点击按钮展现一个 24 小时制的时间滚轮大面板，并滚动选择具体的时、分数值。实现了 IPopup 接口。
 */
class TimePicker : public Control, public IPopup {
public:
    TimePicker();
    virtual ~TimePicker() = default;

    virtual const char* GetClassName() const override { return "TimePicker"; } // 获取类名
    virtual Value GetProperty(PropertyId id) const override; // 反射获取属性值
    virtual bool HasProperty(PropertyId id) const override; // 检查是否存在对应属性
    void SetProperty(PropertyId id, const Value& val) override; // 反射设定属性值
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; } // 获取悬浮交互鼠标样式

    virtual Size Measure(Size availableSize) override; // 测算时间输入框的理想自适应尺寸
    virtual void OnRender(GraphicsContext& ctx) override; // 绘制静止状态下的时间文本标签与下拉指示箭头
    virtual void OnRenderOverlay(GraphicsContext& ctx) override; // 绘制弹出的时间选择滚轮大面板
    virtual UIElement* OnHitTestOverlay(float x, float y) override; // 弹出层非客户区命中穿透定位
    virtual bool NeedsOverlayHitTest() const override { return true; } // 声明需要高频弹窗命中响应
    virtual void OnMouseDown(Point pt) override; // 鼠标按下，判定点击是具体哪一列（时或分）或确定取消按钮
    virtual void OnMouseWheel(float delta) override; // 响应滚轮，可对当前聚焦列的时间数据进行滚动平移微调
    virtual bool OnAnimationTick() override; // 驱动时间滚轮平滑滚动回弹以及弹出层折展淡入动画
    virtual bool HasSelfAnimation() const override; // 检查时间滚轮或弹出层是否仍有动画正在播放

    // IPopup 弹出层接口实现
    virtual bool IsPopupOpen() const override { return m_isPopupOpen; } // 获取弹出层是否已展开
    virtual Rect GetPopupBounds() const override; // 获取弹出层在全局视口下的物理边界包络盒
    virtual bool HitDismissExempt(float x, float y) const override; // 判定该点击位置是否免于强制消退收起
    virtual UIElement* HitTestPopup(float x, float y) override { return OnHitTestOverlay(x, y); } // 弹窗穿透命中定位
    virtual void RenderPopup(GraphicsContext& ctx) override; // 绘制具体的时、分滚动轮数字列表及选中遮罩
    virtual void OnLightDismiss() override { SetPopupOpen(false); } // 点击背景空白消退时触发关闭收起弹出层

    void SetPopupOpen(bool open); // 打开或折叠关闭时间滚轮大面板

    int GetHour() const { return m_hour; } // 获取当前选中的小时数 (0-23)
    int GetMinute() const { return m_minute; } // 获取当前选中的分钟数 (0-59)
    void SetTime(int hour, int minute); // 设定具体的时、分值并平移滚轮位置
    std::string GetFormattedTime() const; // 读取以 "HH:MM" 格式化后的时间字符串

    Event<TimePicker*, int, int>& OnTimeChanged() { return m_onTimeChangedEvent; } // 时间改变时的事件发布中心

private:
    Rect GetPopupRect() const; // 获取弹出层在局部坐标下的矩形边界
    Rect GetWheelRect(int column) const; // 获取指定列（0为时，1为分）的轮廓滚动矩形
    Rect GetSelectionRect(int column) const; // 获取指定列被锁定选中行的物理视口矩形
    int HitTestColumn(float x, float y) const; // 碰撞测试定位坐标落入第几列（0为小时，1为分钟）
    void NudgeColumn(int column, int delta); // 鼠标滚轮或拖拽平移微调指定时间列的数值
    void SnapTargetsToSelection(); // 对齐并强制吸附时、分钟值至最近的正整数刻度线
    void ApplyAnimatedSelection(); // 根据当前时、分值反算出滚轮应当回弹定位到的目标浮点行数坐标

    int m_hour = 14;                                    // 选中的小时数值 (0-23)
    int m_minute = 30;                                  // 选中的分钟数值 (0-59)
    bool m_isPopupOpen = false;                         // 时间选择面板当前是否已弹开

    float m_hourPosition = 14.0f;                       // 小时列当前滚轮滚动所在的浮点坐标高度
    float m_minutePosition = 30.0f;                     // 分钟列当前滚轮滚动所在的浮点坐标高度
    float m_hourTarget = 14.0f;                         // 小时列目标要回弹对齐到的浮点坐标高度
    float m_minuteTarget = 30.0f;                       // 分钟列目标要回弹对齐到的浮点坐标高度
    AnimatedScalar m_popupAnim{};                       // 弹出层展开折叠淡入透明度动效
    std::chrono::steady_clock::time_point m_lastAnimTime{}; // 记录上一帧物理滚轮阻尼动画的时间戳，用于弹性公式计算

    Event<TimePicker*, int, int> m_onTimeChangedEvent;  // 时间更改事件对象
};

} // namespace CUI
