#include "CanvasControl.h"
#include "../animation/AnimationManager.h"
#include <d2d1helper.h>

namespace CUI {

/**
 * @brief 将窗口坐标点转换为画布局部坐标（减去画布左上角偏移）。
 */
static Point ToLocal(const CanvasControl* canvas, Point pt) {
    const Rect& b = canvas->GetBounds();
    return Point{ pt.x - b.x, pt.y - b.y };
}

/**
 * @brief 核心自绘制接口实现。
 * 推送画布左上角平移，使 m_onDraw 回调在画布局部坐标系 (0,0)-(w,h) 中绘制，
 * 与鼠标回调的局部坐标保持一致。
 */
void CanvasControl::OnRender(GraphicsContext& ctx) {
    const Rect& b = GetBounds();
    ctx.PushTransform(D2D1::Matrix3x2F::Translation(b.x, b.y));
    Size size{ b.width, b.height };
    if (OnDraw) {
        OnDraw(ctx, size);
    } else if (m_onDraw) {
        m_onDraw(ctx, size);
    }
    ctx.PopTransform();
}

/**
 * @brief 鼠标按下事件响应。
 * 调用 UIElement 基类实现并向 m_onMouseDown 回调派发画布局部的 Point pt 坐标。
 */
void CanvasControl::OnMouseDown(Point pt) {
    UIElement::OnMouseDown(pt);
    Point localPt = ToLocal(this, pt);
    if (OnCanvasMouseDown) {
        OnCanvasMouseDown(localPt);
    } else if (m_onMouseDown) {
        m_onMouseDown(localPt);
    }
}

/**
 * @brief 鼠标松开释放事件响应。
 * 向 m_onMouseUp 回调派发画布局部的 Point pt 坐标。
 */
void CanvasControl::OnMouseUp(Point pt) {
    UIElement::OnMouseUp(pt);
    Point localPt = ToLocal(this, pt);
    if (OnCanvasMouseUp) {
        OnCanvasMouseUp(localPt);
    } else if (m_onMouseUp) {
        m_onMouseUp(localPt);
    }
}

/**
 * @brief 鼠标移动事件响应。
 * 向 m_onMouseMove 回调派发画布局部的 Point pt 坐标。
 */
void CanvasControl::OnMouseMove(Point pt) {
    UIElement::OnMouseMove(pt);
    Point localPt = ToLocal(this, pt);
    if (OnCanvasMouseMove) {
        OnCanvasMouseMove(localPt);
    } else if (m_onMouseMove) {
        m_onMouseMove(localPt);
    }
}

/**
 * @brief 全局动画 Tick 驱动入口。
 * 将帧间隔秒数透传给 m_onTick 回调，并依据其返回值决定是否继续注册动画 Tick。
 */
bool CanvasControl::OnAnimationTick() {
    float dt = 1.0f / 60.0f;
    if (AnimationManager* mgr = AnimationManager::Current()) {
        dt = mgr->GetDeltaSeconds();
    }
    bool keep = false;
    if (OnTick) {
        keep = OnTick(dt);
        if (keep) RequestAnimationTicks();
    } else if (m_onTick) {
        keep = m_onTick(dt);
    }
    return UIElement::OnAnimationTick() || keep;
}

} // namespace CUI
