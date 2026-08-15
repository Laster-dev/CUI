#pragma once
#include "Control.h"

namespace CUI {

/**
 * @brief 布局比例拆分滑动条控件（Splitter）。
 * 支持水平（Horizontal）或垂直（Vertical）方向。
 * 夹在两个同级控件中间，用户拖拽该条时，将引发并派发位置位移改变量以重新切割父容器的尺寸分配。
 */
class Splitter : public Control {
public:
    Splitter();
    virtual ~Splitter() = default;

    virtual const char* GetClassName() const override { return "Splitter"; } // 获取类名
    virtual HCURSOR GetCursor() const override; // 获取悬浮交互时的 Windows 拖拽鼠标指针样式（根据朝向返回左右/上下箭头）

    virtual Size Measure(Size availableSize) override; // 计算拆分条本身的静态厚度与尺寸
    virtual UIElement* HitTest(float x, float y) override; // 碰撞命中测试
    virtual void OnRender(GraphicsContext& ctx) override; // 绘制拆分滑动条底板以及中间的装饰抓取小斑点
    virtual void OnMouseDown(Point pt) override; // 鼠标按下，开始捕获拖拽手势
    virtual void OnMouseMove(Point pt) override; // 鼠标拖动，计算当前位移差值量并派发位移事件
    virtual void OnMouseUp(Point pt) override; // 鼠标抬起释放拖拽

    bool IsVerticalSplitter() const { return GetOrientation() == CUI::Orientation::Vertical; } // 判定是否为垂直切割条
    void SetOrientation(CUI::Orientation orientation); // 设置分割条的排布方向

    Event<Splitter*, float>& OnSplitterMoved() { return m_onSplitterMovedEvent; } // 拖拽移动触发的事件发布中心

private:
    bool m_isDragging = false;                          // 标记当前是否处于拖拽调节模式中
    Point m_dragStartPt;                                // 拖动开始时鼠标的高精度点击局部坐标点
    Event<Splitter*, float> m_onSplitterMovedEvent;       // 分割条位置变动事件对象
};

} // namespace CUI
