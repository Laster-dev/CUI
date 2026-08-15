#pragma once
#include "UIElement.h"
#include <functional>

namespace CUI {

/**
 * @brief 即时绘制模式画布控件 (CanvasControl / Immediate Mode Canvas)。
 * 
 * 与作为绝对坐标布局容器的 Canvas (Panel) 不同，CanvasControl 是一个专门用于**直接绘图**的控件。
 *
 * 核心架构区别与优势：
 * 1. **高性能 Direct2D 即时绘制**：
 *    - 开发者通过 `SetOnDraw` 提供一个绘制回调闭包，在闭包中使用 `GraphicsContext& ctx`
 *      自由调用 `DrawRect`, `FillRoundedRect`, `DrawSvg`, `DrawSmoothLine` 等底层 Direct2D 绘图命令。
 *    - **零 DOM 节点开销**：在画布内部绘制成百上千个点、线或矢量图形时，不会产生任何 `UIElement` 对象，
 *      内存占用极低，极适合大批量数据渲染、自定义图表、全屏自由画板与动态粒子效果。
 * 
 * 2. **手动坐标碰撞检测 (Hit Testing)**：
 *    - 画布作为一个整体捕获鼠标与触控事件。
 *    - 在 `SetOnCanvasMouseDown`、`SetOnCanvasMouseMove` 等交互回调中，开发者可以获取局部点坐标 `Point pt`，
 *      根据业务数据自行进行数学计算与碰撞检测（如判定点击落在哪条线段或哪个逻辑节点上）。
 */
class CanvasControl : public UIElement {
public:
    /** @brief 绘图回调闭包定义：传入 GraphicsContext 引用与当前画布物理尺寸 (Size)。 */
    using DrawCallback = std::function<void(GraphicsContext& ctx, Size size)>;

    /** @brief 鼠标交互回调闭包定义：传入相对于画布左上角的坐标 (Point pt)。 */
    using MouseCallback = std::function<void(Point pt)>;

    /** @brief 鼠标按键结合坐标的扩展回调闭包定义。 */
    using MouseButtonCallback = std::function<void(Point pt, int button)>;

    /**
     * @brief 每帧更新回调闭包定义：传入帧间隔秒数，返回 true 表示继续下一帧。
     * 适用于物理模拟、粒子系统等需要高频刷新的场景；全部静止后返回 false 自动停止 Tick，避免空转。
     */
    using TickCallback = std::function<bool(float deltaSeconds)>;

    /**
     * @brief 默认构造函数。
     */
    CanvasControl() = default;

    /**
     * @brief 虚析构函数。
     */
    virtual ~CanvasControl() = default;

    /**
     * @brief 获取控件类名。
     * @return 字符串 "CanvasControl"。
     */
    virtual const char* GetClassName() const override { return "CanvasControl"; }

    /**
     * @brief 使画布的渲染失效，强制在下一帧排队重新触发 OnDraw 绘制。
     */
    void Invalidate() { InvalidateMeasure(); InvalidateArrange(); }

    /**
     * @brief 设置即时绘制模式的 OnDraw 回调函数。
     * @param callback 绘制闭包函数 `[](GraphicsContext& ctx, Size canvasSize){ ... }`。
     */
    void SetOnDraw(DrawCallback callback) { m_onDraw = std::move(callback); Invalidate(); }

    /**
     * @brief 设置画布的鼠标按下事件回调。
     * @param cb 交互闭包 `[](Point pt){ ... }`，开发者可以在闭包中拿着 pt 进行手写坐标碰撞测试。
     */
    void SetOnCanvasMouseDown(MouseCallback cb) { m_onMouseDown = std::move(cb); }

    /**
     * @brief 设置画布的鼠标抬起释放事件回调。
     * @param cb 交互闭包 `[](Point pt){ ... }`。
     */
    void SetOnCanvasMouseUp(MouseCallback cb) { m_onMouseUp = std::move(cb); }

    /**
     * @brief 设置画布的鼠标移动事件回调（适合处理画笔拖拽绘制或 Hover 提示）。
     * @param cb 交互闭包 `[](Point pt){ ... }`。
     */
    void SetOnCanvasMouseMove(MouseCallback cb) { m_onMouseMove = std::move(cb); }

    /**
     * @brief 设置每帧更新回调（物理/粒子高频模拟）。
     * @param cb 回调闭包 `[](float dt){ return keepRunning; }`，返回 false 后自动停止 Tick。
     */
    void SetOnTick(TickCallback cb) { m_onTick = std::move(cb); }

    /**
     * @brief 核心渲染入口：将 GraphicsContext 和当前画布大小透传给 m_onDraw 回调。
     * 渲染采用世界坐标模型，这里会推送画布左上角平移，使回调内部在画布局部坐标系 (0,0)-(w,h) 中绘制，
     * 与鼠标回调的局部坐标保持一致。
     * @param ctx GraphicsContext 绘图上下文引用。
     */
    virtual void OnRender(GraphicsContext& ctx) override;

    /**
     * @brief 响应鼠标按下动作，派发画布局部坐标 Point pt 至注册的 m_onMouseDown 回调。
     * @param pt 窗口坐标，框架会转换为画布局部坐标。
     */
    virtual void OnMouseDown(Point pt) override;

    /**
     * @brief 响应鼠标松开动作，派发画布局部坐标 Point pt 至注册的 m_onMouseUp 回调。
     * @param pt 窗口坐标，框架会转换为画布局部坐标。
     */
    virtual void OnMouseUp(Point pt) override;

    /**
     * @brief 响应鼠标在画布上方移动动作，派发画布局部坐标 Point pt 至注册的 m_onMouseMove 回调。
     * @param pt 窗口坐标，框架会转换为画布局部坐标。
     */
    virtual void OnMouseMove(Point pt) override;

    /**
     * @brief 全局动画 Tick 驱动入口：将帧间隔透传给 m_onTick 回调并依据其返回值决定是否继续。
     * @return true 表示需要继续下一帧更新。
     */
    bool OnAnimationTick() override;

private:
    DrawCallback m_onDraw;          ///< 画布的 Direct2D 自绘制回调闭包
    MouseCallback m_onMouseDown;    ///< 画布鼠标按下回调
    MouseCallback m_onMouseUp;      ///< 画布鼠标松开回调
    MouseCallback m_onMouseMove;    ///< 画布鼠标移动回调
    TickCallback m_onTick;          ///< 每帧物理/粒子更新回调
};

} // namespace CUI
