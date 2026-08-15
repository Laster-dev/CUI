#pragma once
#include "../controls/UIElement.h"
#include "../render/GraphicsContext.h"
#include "../render/CompositionContext.h"
#include "../render/DirtyRegion.h"
#include "../render/RenderLayer.h"
#include "../render/LayerRasterizer.h"
#include "../animation/AnimationManager.h"
#include "../animation/FrameScheduler.h"
#include "../input/RoutedEvent.h"
#include "../input/Command.h"
#include "WindowBackdrop.h"
#include "PopupHost.h"
#include "../dnd/DragDropService.h"
#include <windows.h>
#include <chrono>
#include <memory>
#include <string>

namespace CUI {

/**
 * @brief 窗口生命周期与原生 Win32 HWND 宿主承载类。
 * Window 类是 CUI 引擎与 Windows 操作系统的终极接口。它统一管理：
 * 1. **Win32 HWND 窗口管理**：创建窗口、注册 WindowProc 消息循环分发。
 * 2. **Direct2D 渲染系统**：桥接硬件渲染上下文（GraphicsContext）和合成层交换链，驱动重绘。
 * 3. **交互与命中分发**：接收底层鼠标/键盘 Win32 原始输入，穿透命中 UIElement 控件并派发 Routed 事件。
 * 4. **动画与定时器**：关联 AnimationManager 主时钟和帧步进频率计时（FrameScheduler）。
 * 5. **高级主题与特效**：支持 DWM 云母/亚克力背景（BackdropType）和主题快速涟漪扩散动效。
 */
class Window {
public:
    /**
     * @brief 获取当前活动窗口实例指针的静态方法。
     * @return 返回指向当前 Window 实例的指针，如果尚未实例化则返回 nullptr。
     */
    static Window* Current();

    Window();
    virtual ~Window();

    /**
     * @brief 创建原生 Win32 宿主窗口。
     * @param title 窗口标题栏文本。
     * @param width 窗口初始物理像素宽度。
     * @param height 窗口初始物理像素高度。
     * @param transparentMode 是否启用全透明 Alpha 混合模式。
     * @return 如果创建并初始化 DX11/D2D 设备成功则返回 true，否则返回 false。
     */
    bool Create(const std::string& title, int width = 1280, int height = 800, bool transparentMode = false);

    /**
     * @brief 显示当前窗口，并向操作系统发送首次重绘指令。
     */
    void Show();

    /**
     * @brief 阻塞并运行 Win32 消息循环（GetMessage / DispatchMessage）。
     */
    void RunMessageLoop();

    /**
     * @brief 设置窗口根级元素。
     * @param root 指向 UIElement 根级控件的共享指针。
     */
        /**
     * @brief 窗口主题模式 (Light / Dark) 属性代理。
     */
    struct WindowThemeModeProperty {
        Window* owner = nullptr;
        WindowThemeModeProperty() = default;
        explicit WindowThemeModeProperty(Window* o) : owner(o) {}
        WindowThemeModeProperty& operator=(CUI::ThemeMode m) { if (owner) owner->SetThemeMode(m); return *this; }
        operator CUI::ThemeMode() const { return owner ? owner->GetThemeMode() : CUI::ThemeMode::Dark; }
        CUI::ThemeMode Get() const { return owner ? owner->GetThemeMode() : CUI::ThemeMode::Dark; }
    } ThemeMode;

    /**
     * @brief 窗口背景材质 (Mica / Acrylic / None) 属性代理。
     */
    struct WindowBackdropTypeProperty {
        Window* owner = nullptr;
        WindowBackdropTypeProperty() = default;
        explicit WindowBackdropTypeProperty(Window* o) : owner(o) {}
        WindowBackdropTypeProperty& operator=(CUI::BackdropType b) { if (owner) owner->SetBackdropType(b); return *this; }
        operator CUI::BackdropType() const { return owner ? owner->GetBackdropType() : CUI::BackdropType::None; }
        CUI::BackdropType Get() const { return owner ? owner->GetBackdropType() : CUI::BackdropType::None; }
    } BackdropType;

    /**
     * @brief 渲染性能统计浮层是否显示属性代理。
     */
    struct WindowRenderStatsProperty {
        Window* owner = nullptr;
        WindowRenderStatsProperty() = default;
        explicit WindowRenderStatsProperty(Window* o) : owner(o) {}
        WindowRenderStatsProperty& operator=(bool v) { if (owner) owner->SetRenderStatsOverlayVisible(v); return *this; }
        operator bool() const { return owner ? owner->IsRenderStatsOverlayVisible() : false; }
        bool Get() const { return owner ? owner->IsRenderStatsOverlayVisible() : false; }
    } RenderStatsOverlayVisible;

    /**
     * @brief 窗口承载的顶层根 UI 元素属性代理。
     */
    struct WindowRootElementProperty {
        Window* owner = nullptr;
        WindowRootElementProperty() = default;
        explicit WindowRootElementProperty(Window* o) : owner(o) {}
        WindowRootElementProperty& operator=(std::shared_ptr<UIElement> r) { if (owner) owner->SetRootElement(std::move(r)); return *this; }
        operator std::shared_ptr<UIElement>() const { return owner ? owner->GetRootElement() : nullptr; }
        std::shared_ptr<UIElement> Get() const { return owner ? owner->GetRootElement() : nullptr; }
        std::shared_ptr<UIElement> operator->() const { return owner ? owner->GetRootElement() : nullptr; }
    } RootElement;

    /**
     * @brief Win32 宿主窗口原生句柄只读属性代理。
     */
    struct WindowHWNDProperty {
        Window* owner = nullptr;
        WindowHWNDProperty() = default;
        explicit WindowHWNDProperty(Window* o) : owner(o) {}
        operator ::HWND() const { return owner ? owner->GetHWND() : nullptr; }
        ::HWND Get() const { return owner ? owner->GetHWND() : nullptr; }
    } HWND;

    void SetRootElement(std::shared_ptr<UIElement> root);

    /**
     * @brief 标记整棵树重新测算布局并执行立即重排。
     */
    void Relayout();

    /**
     * @brief 获取窗口的根级元素。
     */
    std::shared_ptr<UIElement> GetRootElement() const { return m_rootElement; }

    /**
     * @brief 获取当前窗口的原生 Win32 句柄。
     */
    ::HWND GetHWND() const { return m_hwnd; }

    /**
     * @brief 获取 Direct2D 设备图形上下文。
     */
    GraphicsContext& GetGraphicsContext() { return m_gfxContext; }

    /**
     * @brief 获取合成树渲染上下文。
     */
    CompositionContext& GetCompositionContext() { return m_compositionContext; }
    const CompositionContext& GetCompositionContext() const { return m_compositionContext; }

    /**
     * @brief 获取下拉菜单或弹出气泡专用的 PopupHost 宿主。
     */
    PopupHost& GetPopupHost() { return m_popupHost; }

    /**
     * @brief 检查窗口是否处于全透明通道混合模式。
     */
    bool IsTransparentMode() const { return m_transparentMode; }

    /**
     * @brief 设置是否启用全透明通道混合模式。
     */
    void SetTransparentMode(bool enabled);

    /**
     * @brief 设定低性能优化运行模式（此模式下会限制部分粒子或复杂过渡动画）。
     */
    void SetLowPerformanceMode(bool enabled);

    /**
     * @brief 判定是否处于低性能运行模式。
     */
    bool IsLowPerformanceMode() const { return m_lowPerformanceMode; }

    /**
     * @brief 设置窗口的毛玻璃/云母/亚克力等系统级后置背景样式。
     */
    void SetBackdropType(CUI::BackdropType type);

    /**
     * @brief 获取窗口的毛玻璃后置背景样式。
     */
    CUI::BackdropType GetBackdropType() const { return m_backdropType; }

    /**
     * @brief 手动修改程序的主题颜色模式（Light/Dark）。
     */
    void SetThemeMode(CUI::ThemeMode theme);

    /**
     * @brief 触发带水波纹涟漪渐变的主题颜色平滑过渡。
     * @param theme 目标的主题模式。
     * @param originPoint 水波纹扩散的源起坐标中心点。
     */
    void SetThemeModeWithRipple(CUI::ThemeMode theme, Point originPoint);

    /**
     * @brief 获取当前的主题颜色模式。
     */
    CUI::ThemeMode GetThemeMode() const { return m_themeMode; }

    /**
     * @brief 监听主题全局修改的事件委托连接点。
     */
    Event<Window*, CUI::ThemeMode>& OnThemeChanged() { return m_onThemeChanged; }

    /**
     * @brief 设置是否显示右上角的帧率与脏渲染统计信息 Overlay。
     */
    void SetRenderStatsOverlayVisible(bool visible) { m_showRenderStatsOverlay = visible; }

    /**
     * @brief 判定是否显示帧率与脏渲染统计信息 Overlay。
     */
    bool IsRenderStatsOverlayVisible() const { return m_showRenderStatsOverlay; }

    /**
     * @brief 获取当前窗口对应显示器的 DPI 缩放比例因数。
     */
    float GetDpiScale() const { return m_dpiScale; }

    /**
     * @brief 获取滚动计算后的平均显示帧率。
     */
    float GetDisplayFps() const;

    /**
     * @brief 获取鼠标指针正悬停在其上方的那个控件指针。
     */
    UIElement* GetHoveredElement() const { return m_hoveredRaw; }

    /**
     * @brief 获取命令快捷键管理器。
     */
    CommandManager& GetCommands() { return m_commands; }
    const CommandManager& GetCommands() const { return m_commands; }

    /**
     * @brief 获取目前正持有输入焦点的控件。
     */
    UIElement* GetFocusedElement() const;

    /**
     * @brief 强行将焦点应用并转移到指定的控件。
     * @param target 目标聚焦元素。
     * @param state 聚焦方式类型（Pointer 或 Keyboard）。
     */
    void ApplyFocus(UIElement* target, FocusState state);

    /**
     * @brief 将 Win32 原生客户区坐标转化为 DPI 缩放后的 CUI 逻辑二维点坐标。
     */
    Point ClientPointToLogical(int x, int y) const;

    /**
     * @brief 使指定的逻辑二维矩形范围失效变脏，申请安排在下一帧刷新重绘该区域。
     */
    void InvalidateLogicalRect(const Rect& rect);

    /**
     * @brief 刷新 OLE 拖放操作的视觉状态。
     */
    void InvalidateDragFeedback();

    /**
     * @brief 获取 OLE 拖放控制器服务。
     */
    DragDropService& GetDragDrop() { return m_dragDrop; }

    /**
     * @brief 指定窗口当前激活工作的 ContextMenu 实例。
     */
    void SetActiveContextMenu(std::shared_ptr<ContextMenu> menu) { m_activeContextMenu = menu; }

private:
    static Window* s_current; // 全局窗口单例静态缓存指针

    static LRESULT CALLBACK WindowProc(::HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam); // 静态 Win32 回调入口
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);                         // 实例消息路由处理函数

    void OnPaint();                                                     // 处理 WM_PAINT 并刷新 Direct2D 画面
    void OnResize(UINT width, UINT height);                             // 处理 WM_SIZE 改变后台缓冲区尺寸
    void UpdateDwmChrome();                                             // 将后置背景 Backdrop 样式下发给 DWM 合成器
    CUI::WindowMaterialState MakeMaterialState() const;                 // 根据当前字段计算统一材质状态
    RenderCacheStamp BuildRenderCacheStamp() const;                     // 组装当前帧的缓存版本戳（代次/尺寸/DPI/透明）
    bool OnMouseMove(int x, int y);                                     // 处理 WM_MOUSEMOVE 鼠标移入命中
    bool OnLButtonDown(int x, int y);                                   // 左键按下派发
    void OnLButtonDblClick(int x, int y);                               // 左键双击派发
    bool OnLButtonUp(int x, int y);                                     // 左键释放派发
    void OnRButtonDown(int x, int y);                                   // 右键按下派发
    void OnRButtonUp(int x, int y);                                     // 右键释放派发
    void OnMButtonDown(int x, int y);                                   // 中键按下派发
    void OnMButtonUp(int x, int y);                                     // 中键释放派发
    void ClearMenuBarInteractionState();                                // 清除菜单栏交互激活状态
    void StopMiddleClickAutoscroll();                                   // 停用浏览器式的中键自动滚屏交互
    bool IsMiddleClickAutoscrollActive() const;                         // 检测中键自动滚屏是否处于运作状态
    UIElement* HitTestChrome(float x, float y) const;                   // 对非客户区标题栏进行命中测试
    static std::shared_ptr<UIElement> CaptureElementRef(UIElement* element); // 为普通指针提升为强共享引用
    std::shared_ptr<UIElement> LockElement(const std::weak_ptr<UIElement>& element) const; // 锁住弱指针提取强指针
    static bool NeedsContinuousMouseRedraw(UIElement* element);         // 辨识此控件是否需要高频鼠标移位强制重绘
    void SetHoveredElement(UIElement* element);                         // 登记当前悬停的节点
    void SetPressedElement(UIElement* element);                         // 登记当前按下的节点
    void SetFocusedElement(UIElement* element);                         // 登记当前获焦的节点
    void InvalidateAnimatedRegions(bool animationStillActive);          // 污损重绘包含有活跃动画组件的画面图层区域
    void CommitFrame(bool animationStillActive);                        // 渲染完毕后，向 DWM 交换链或 DirectComposition 提交新帧
    void FlushLayoutIfNeeded();                                         // 刷新 Measure/Arrange 两阶段排版管道
    void DispatchRoutedPointer(RoutedEventType type, Point pt, UIElement* target); // 执行隧穿/冒泡二级路由指针事件
    bool TryMoveFocus(bool forward);                                    // 通过 Tab/Shift+Tab 按键导航流动焦点
    bool TryMoveDirectionalFocus(UIElement* focused, int vkCode);       // 响应键盘方向键流动几何对齐焦点
    bool DispatchKey(int vkCode, bool sysKey);                          // 向焦点控件派发按键按下事件
    bool ActivateMenuBar();                                             // 将焦点流激活至顶层 MenuBar
    void DrawKeyboardFocusRing();                                       // 绘制键盘焦点虚线环
    void CollectTabFocusable(UIElement* el, std::vector<UIElement*>& out) const; // 递归搜集整个树种允许 Tab 获焦的元素
    void RequestFullRepaint();                                          // 使整个窗口全客户区失效变脏，强制完全重绘
    void InvalidatePendingRenderRegions(bool fallbackToFullWindow);     // 刷新等待提交的所有失效重绘脏图区
    bool HasPendingNativePaint() const;                                 // 检测 Win32 消息泵中是否还有未完成的重绘请求
    void SampleDisplayFps();                                            // 采样实际 OnPaint 触发渲染的显示 FPS 数值
    void DrawRenderStatsOverlay();                                      // 渲染右上角的帧率监视层
    void ApplyVisualState();                                            // 向系统更新并设定窗口的状态
    void RegisterShellDropTarget();                                     // 挂载 OLE DropTarget 以接收系统资源管理器文件拖入
    void RevokeShellDropTarget();                                       // 注销 OLE 拖放源监听

    void* m_oleDropTarget = nullptr;                                    // 指向原生 OLE DropTarget 结构体的指针
    bool m_oleDropRegistered = false;                                   // 指示是否已向操作系统成功注册了拖放服务
    bool m_needOleUninit = false;                                       // 标识是否需要在窗口析构时调用 OleUninitialize()

    ::HWND m_hwnd = nullptr;                                              // 窗口的 OS 原生句柄
    float m_dpiScale = 1.0f;                                            // 屏幕 DPI 逻辑缩放因子
    Size m_logicalClientSize{ 0.0f, 0.0f };                             // DPI 缩放后的逻辑窗口客户区尺寸 (宽度, 高度)
    GraphicsContext m_gfxContext;                                       // Direct3D 与 Direct2D 底层设备硬件图形渲染上下文
    std::shared_ptr<UIElement> m_rootElement;                           // 指向当前窗口排版树根级控件的强指针

    std::weak_ptr<UIElement> m_hoveredElement;                          // 弱引用正悬浮鼠标指针的控件节点
    std::weak_ptr<UIElement> m_pressedElement;                          // 弱引用鼠标左键正按压不放的控件节点
    UIElement* m_hoveredRaw = nullptr;                                  // 悬停控件的原始指针缓存，用于快速交互比对
    UIElement* m_pressedRaw = nullptr;                                  // 按压控件的原始指针缓存，用于快速交互比对
    std::weak_ptr<UIElement> m_rpressedElement;                         // 弱引用鼠标右键正在点击按压的控件节点
    std::weak_ptr<UIElement> m_middleScrollElement;                     // 弱引用中键自动滚屏模式锁定的目标滑动容器节点
    std::weak_ptr<UIElement> m_focusedElement;                          // 弱引用当前持有焦点的控件节点
    std::shared_ptr<ContextMenu> m_activeContextMenu = nullptr;         // 窗口目前悬浮显示中的右键上下文菜单实例
    UIElement* m_pendingContextMenuTarget = nullptr;                    // 预备弹出右键菜单的定位目标控件指针
    Point m_pendingContextMenuPt{};                                     // 预备弹出右键菜单的定位逻辑坐标
    std::shared_ptr<ContextMenu> m_pendingContextMenu;                  // 预备弹出的上下文菜单临时实例
    PopupHost m_popupHost;                                              // 下拉与悬浮提示弹出层专用宿主容器
    DragDropService m_dragDrop;                                         // OLE 拖放操作控制器
    CommandManager m_commands;                                          // 键盘快捷命令管理器
    bool m_trackingMouse = false;                                       // 标记当前是否向 Windows 开启了 TrackMouseEvent 鼠标移出边界监听
    bool m_transparentMode = false;                                     // 窗口是否在创建时启用了背景 Alpha 通透混合
    Rect m_lastAnimationDirtyRect;                                      // 记录上一帧由于过渡动画引发重绘的失效脏矩形范围
    bool m_hasLastAnimationDirtyRect = false;                           // 上一帧过渡动画脏矩形是否已就绪生效
    AnimationManager m_animationManager;                                // 窗口所拥有的动画时钟泵管理器
    FrameScheduler m_frameScheduler;                                    // 驱动窗口重绘频率与合并 Present 的帧时钟调度器
    CompositionContext m_compositionContext;                            // 用于 DirectComposition 的视觉合成交换链管理器
    LayerRasterizer m_layerRasterizer;                                  // 用于多图层光栅化的图层栅格化辅助模块
    DirtyRegion m_pendingDirtyRegion;                                   // 累加的待在下一帧进行像素渲染刷新的局部脏区域集合
    RenderLayer m_sceneLayer;                                           // 场景图普通内容图层分支
    RenderLayer m_themeOldSceneLayer;                                   // 主题过渡时，缓存旧主题静止画面的图层分支（用于水波纹渐变对比）
    bool m_themeRippleActive = false;                                   // 主题水波纹渐变动画当前是否处于运作状态
    Point m_themeRippleOrigin{ 0.0f, 0.0f };                            // 主题水波纹动画的源起逻辑中心点
    float m_themeRippleProgress = 0.0f;                                 // 主题水波纹动画的运行进度 (0.0f - 1.0f)
    std::chrono::steady_clock::time_point m_themeRippleStartTime;       // 主题水波纹动画开始时的高精时间戳
    bool m_showRenderStatsOverlay = false;                              // 是否在画面右上角贴片显示性能计数器
    bool m_lowPerformanceMode = false;                                  // 低画质性能节省运行模式
    bool m_flushInputDirty = false;                                     // 标识输入设备状态发生改变，窗口下一次 Relayout 必须刷新
    CUI::BackdropType m_backdropType = CUI::BackdropType::None;                   // 当前窗口下发的毛玻璃后置背景类型样式
    CUI::ThemeMode m_themeMode = CUI::ThemeMode::Dark;                             // 当前窗口所采纳的样式主题模式（Light / Dark）
    CUI::WindowMaterialState m_materialState;                                     // 统一材质状态（一处计算、处处消费）
    uint64_t m_materialGeneration = 0;                                            // 材质代次：每次切换自增，驱动场景缓存失效
    Event<Window*, CUI::ThemeMode> m_onThemeChanged;                         // 主题发生改变时的事件分发器
    mutable std::chrono::steady_clock::time_point m_overlayFpsSampleStart{}; // 性能面板帧率采样开始时间戳
    mutable unsigned m_overlayFrameCounter = 0;                         // 性能面板渲染累计帧数
    mutable float m_overlayFps = 0.0f;                                  // 性能面板展示的当前 FPS 缓存数值
};

} // namespace CUI
