#pragma once
#include "Control.h"

namespace CUI {

/**
 * @brief 折叠/展示的动画朝向方向。
 * Down: 向下展开折叠。
 * Up: 向上展开折叠。
 */
enum class ExpandDirection {
    Down = 0, // 向下折展方向
    Up = 1    // 向上折展方向
};

/**
 * @brief 折叠面板控件（Expander / CollapsePanel）。
 * 包含头部大文本、副标题文本（Subtitle）、以及右侧旋转的指示小箭头（Chevron）。
 * 点击头部会平滑地以高度渐变拉开或收拢下方的子内容（Content）。
 */
class Expander : public Control {
public:
    Expander();
    explicit Expander(const std::string& headerText);
    virtual ~Expander() = default;

    virtual const char* GetClassName() const override { return "Expander"; } // 获取类名
    virtual Value GetProperty(PropertyId id) const override; // 反射获取属性值
    virtual bool HasProperty(PropertyId id) const override; // 检查是否存在对应属性
    void SetProperty(PropertyId id, const Value& val) override; // 反射设定属性值
    virtual HCURSOR GetCursor() const override; // 获取悬浮交互鼠标样式

    virtual Size Measure(Size availableSize) override; // 测算头部空间及下方折拢状态下的子内容排版占高
    virtual void Arrange(Rect finalRect) override; // 排布头部栏及下方面板裁剪内容
    virtual void Render(GraphicsContext& ctx) override; // 托管带剪切矩形（ClipRect）的折叠内容绘制以防溢出
    virtual void OnRender(GraphicsContext& ctx) override; // 绘制头部底色、主副文字、分割线及小箭头图标
    virtual UIElement* HitTest(float x, float y) override; // 命中测试定位

    virtual void OnMouseMove(Point pt) override; // 鼠标滑动，高亮头部可交互背景
    virtual void OnMouseLeave() override; // 鼠标移出，还原头部高亮状态
    virtual void OnMouseDown(Point pt) override; // 鼠标按下，捕获点击位置
    virtual void OnMouseUp(Point pt) override; // 鼠标松开，若在头部释放则触发展开/折拢切换
    virtual bool OnKeyDown(int vkCode) override; // 响应键盘方向键及空格键切换折叠状态
    virtual bool OnAnimationTick() override; // 驱动下方面板折缩高度平滑变动动画
    virtual bool HasSelfAnimation() const override; // 检查是否依然处于展开/折拢高度渐变动画中
    virtual void OnThemeChanged() override; // 响应主题更改，重新重绘

    const std::string& GetHeader() const { return m_header; } // 获取头部主标题文本内容
    void SetHeader(const std::string& header); // 设置头部主标题文本内容

    const std::string& GetSubtitle() const { return m_subtitle; } // 获取头部副标题说明文本内容
    void SetSubtitle(const std::string& subtitle); // 设置头部副标题说明文本内容

    bool GetIsExpanded() const { return m_isExpanded; } // 检查当前是否处于展开状态
    void SetIsExpanded(bool expanded); // 设定是否展开面板并启动高度缩放过渡动画
    void SetExpanded(bool expanded) { SetIsExpanded(expanded); } // 兼容别名：设定是否展开

    ExpandDirection GetExpandDirection() const { return m_expandDirection; } // 获取折展的方向朝向
    void SetExpandDirection(ExpandDirection direction); // 设置折展的方向朝向

    /**
     * @brief 折叠面板折叠区承载的 UIElement 内容元素属性代理。
     */
    struct ExpanderContentProperty {
        Expander* owner;
        ExpanderContentProperty& operator=(std::shared_ptr<UIElement> c) { owner->SetContent(std::move(c)); return *this; }
        operator std::shared_ptr<UIElement>() const { return owner->GetContent(); }
        std::shared_ptr<UIElement> Get() const { return owner->GetContent(); }
        std::shared_ptr<UIElement> operator->() const { return owner->GetContent(); }
    } Content{this};

    /**
     * @brief 折叠展开方向属性代理 (Down/Up 等)。
     */
    struct ExpanderExpandDirectionProperty {
        Expander* owner;
        ExpanderExpandDirectionProperty& operator=(ExpandDirection d) { owner->SetExpandDirection(d); return *this; }
        operator ExpandDirection() const { return owner->GetExpandDirection(); }
        ExpandDirection Get() const { return owner->GetExpandDirection(); }
    } ExpandDirection{this};

    void SetContent(std::shared_ptr<UIElement> content); // 设定要展开或隐藏的子级内容控件
    std::shared_ptr<UIElement> GetContent() const { return m_content; } // 获取子级内容控件

    Event<Expander*>& OnExpanding() { return m_onExpanding; } // 展开动作发起时的事件派发器
    Event<Expander*>& OnCollapsed() { return m_onCollapsed; } // 折叠动作结束时的事件派发器
    Event<Expander*, bool>& OnExpandedChanged() { return m_onExpandedChanged; } // 展开状态更改时的事件委托连接点

private:
    static constexpr float kHeaderMinHeight = 48.0f;       // 头部标题栏最小像素高度约束限制 (px)
    static constexpr float kHeaderHorizontalPadding = 16.0f; // 头部标题栏的水平内边距空白 (px)
    static constexpr float kHeaderVerticalPadding = 12.0f;   // 头部标题栏的垂直内边距空白 (px)
    static constexpr float kBodyPadding = 16.0f;             // 下方内容卡片区默认的内边距 (px)
    static constexpr float kChevronHitSize = 28.0f;          // 右侧旋转箭头可命中的虚拟框大小 (px)
    static constexpr float kChevronGlyphSize = 14.0f;        // 旋转箭头本身的矢量大小 (px)
    static constexpr float kCornerRadius = 8.0f;             // Expander 卡片四周圆角半径 (px)

    float MeasureHeaderHeight(float width) const; // 测算头部文字在给定折行宽度下的总高
    float MeasureBodyHeight(float width); // 测算展开部分子控件在给定宽度下的理想总高
    float GetHeaderTextRight() const; // 计算标题文本允许拉伸排版的右边界极值
    float GetExpandProgress() const; // 读取当前展开状态折算出的动画浮点系数 (0.0f - 1.0f)
    float GetVisibleBodyHeight() const; // 计算动画当前帧应当裁剪展现出来的实际高度像素值
    Rect GetHeaderRect() const; // 获得头部栏的局部包络矩形
    Rect GetBodyRect() const; // 获得下方内容区的局部包络矩形
    Rect GetBodyClipRect() const; // 获得用于限制内容超出渲染的局部裁剪矩形边界
    Rect GetChevronRect() const; // 计算右侧小箭头的局部坐标绘制区域
    bool IsPointInHeader(Point pt) const; // 判定点坐标是否落入头部热区范围内
    void UpdateContentVisibility(); // 根据展开进度，将子内容控件的排版可见性进行静默同步
    void InvalidateExpanderLayout(); // 申请 Expander 本级及子树重新排版
    void InvalidateExpanderVisual(); // 申请 Expander 局部重绘

    std::string m_header{ "Expander" };                       // 头部主标题文本
    std::string m_subtitle;                                   // 头部副标题文本
    bool m_isExpanded = false;                                // 展开状态标记（true为展开，false为折拢）
    bool m_headerHovered = false;                             // 标记鼠标指针是否正 Hover 在标题栏上方
    bool m_headerPressed = false;                             // 标记左键是否在标题栏上方按下未抬起
    CUI::ExpandDirection m_expandDirection = CUI::ExpandDirection::Down; // 展开动画的滑动方向

    std::shared_ptr<UIElement> m_content; // 包含的具体下属展开内容元素
    float m_headerHeight = kHeaderMinHeight; // 实际测算出的头部栏高度像素值
    float m_measuredBodyHeight = 0.0f;     // 实际测算出的完全展开时内容高度值

    AnimatedScalar m_expandAnim{ 0.0f };   // 折展过程平滑高度拉伸过渡动画

    Event<Expander*> m_onExpanding;         // 展开中事件对象
    Event<Expander*> m_onCollapsed;         // 折叠中事件对象
    Event<Expander*, bool> m_onExpandedChanged; // 展开状态更改事件对象
};

using CollapsePanel = Expander; // 兼容别名：折叠面板组件

} // namespace CUI
