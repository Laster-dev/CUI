#pragma once
#include "Control.h"

namespace CUI {

/**
 * @brief 星级评分控件（RatingControl）。
 * 允许用户点击或悬停来给某项内容打分（例如 0 到 5 星），支持只读模式与半星（0.5 步长）精度。
 */
class RatingControl : public Control {
public:
    RatingControl();
    virtual ~RatingControl() = default;

    virtual const char* GetClassName() const override { return "RatingControl"; } // 获取类名
    virtual HCURSOR GetCursor() const override; // 获取悬浮交互鼠标样式
    virtual Value GetProperty(PropertyId id) const override; // 反射获取属性值
    virtual bool HasProperty(PropertyId id) const override; // 检查是否存在对应属性
    void SetProperty(PropertyId id, const CUI::Value& val) override; // 反射设定属性值

    virtual Size Measure(Size availableSize) override; // 测算根据最大星级数与间距排布出的最适尺寸
    virtual void OnRender(GraphicsContext& ctx) override; // 绘制多颗星星底色、高亮填充及描边
    virtual void OnMouseDown(Point pt) override; // 鼠标按下，捕获打分输入
    virtual void OnMouseMove(Point pt) override; // 鼠标移动，高频跟踪打分Hover预览分值
    virtual void OnMouseUp(Point pt) override; // 鼠标抬起，确定最终打分
    virtual void OnMouseLeave() override; // 鼠标移开，还原至实际打分分值
    virtual bool OnKeyDown(int vkCode) override; // 响应键盘方向键增减星级
    virtual bool OnAnimationTick() override; // 驱动打分变动时的弹性缓冲渲染过渡动画
    virtual bool HasSelfAnimation() const override; // 检查是否依然处于打分过渡动画中

    PropertyRef<float, PropertyId::ControlValue> ValueProperty; // 当前评分分数的双向绑定属性代理

    float GetValue() const { return m_value; } // 获取实际打分分值
    void SetValue(float val); // 设置实际打分分值

    /**
     * @brief 评分控件最大星级总数 (1~10) 属性代理。
     */
    struct RatingControlMaxRatingProperty {
        RatingControl* owner = nullptr;
        RatingControlMaxRatingProperty() = default;
        explicit RatingControlMaxRatingProperty(RatingControl* o) : owner(o) {}
        RatingControlMaxRatingProperty& operator=(int m) { if (owner) owner->SetMaxRating(m); return *this; }
        operator int() const { return owner ? owner->GetMaxRating() : 5; }
        int Get() const { return owner ? owner->GetMaxRating() : 5; }
    } MaxRating;

    /**
     * @brief 评分调节精度步长 (如 0.5 或 1.0) 属性代理。
     */
    struct RatingControlStepProperty {
        RatingControl* owner = nullptr;
        RatingControlStepProperty() = default;
        explicit RatingControlStepProperty(RatingControl* o) : owner(o) {}
        RatingControlStepProperty& operator=(float s) { if (owner) owner->SetStep(s); return *this; }
        operator float() const { return owner ? owner->GetStep() : 1.0f; }
        float Get() const { return owner ? owner->GetStep() : 1.0f; }
    } Step;

    /**
     * @brief 评分控件当前分值属性代理。
     */
    struct RatingControlValueProperty {
        RatingControl* owner = nullptr;
        RatingControlValueProperty() = default;
        explicit RatingControlValueProperty(RatingControl* o) : owner(o) {}
        RatingControlValueProperty& operator=(float v) { if (owner) owner->SetValue(v); return *this; }
        operator float() const { return owner ? owner->GetValue() : 0.0f; }
        float Get() const { return owner ? owner->GetValue() : 0.0f; }
    } Value;

    int GetMaxRating() const { return m_maxRating; } // 获取最大星级数（例如5星或10星）
    void SetMaxRating(int maxRating); // 设置最大星级数

    float GetStep() const { return m_step; } // 获取打分步长（支持0.5星或1.0星）
    void SetStep(float step); // 设置打分步长

    bool GetIsReadOnly() const { return m_isReadOnly; } // 检查是否为只读展示模式
    void SetIsReadOnly(bool readOnly);
    /**
     * @brief 评分控件是否处于只读展示模式属性代理。
     */
    struct RatingControlIsReadOnlyProperty {
        RatingControl* owner;
        RatingControlIsReadOnlyProperty& operator=(bool r) { owner->SetIsReadOnly(r); return *this; }
        operator bool() const { return owner->GetIsReadOnly(); }
        bool Get() const { return owner->GetIsReadOnly(); }
    } IsReadOnly{this};

 // 设置是否为只读展示模式

    bool IsClearEnabled() const { return m_isClearEnabled; } // 是否允许清空分数
    void SetIsClearEnabled(bool enabled); // 是否允许清空分数

    float GetStarSize() const { return m_starSize; } // 获取单颗星星渲染像素大小
    void SetStarSize(float size); // 设置单颗星星渲染像素大小

    Event<RatingControl*, float>& OnValueChanged() { return m_onValueChangedEvent; } // 评分改变时的事件发布中心

private:
    float SnapValue(float val) const; // 根据精度步长对打分进行对齐约束
    float ShownValue() const; // 获取当前应当绘制预览的星级分数
    bool CanInteract() const; // 判定是否能够进行用户点击打分交互
    Rect ContentRect() const; // 计算星星排布区的整体边界盒
    Rect ClearRect() const; // 计算清空按钮的控制区域（若存在）
    Rect StarRect(int index) const; // 计算第 index 个星星的局部绘制矩形包络盒
    float ValueFromPoint(Point pt) const; // 根据鼠标坐标反算实际的星级分数
    void DrawStar(GraphicsContext& ctx, const Rect& slot, float fill01,
                  D2D1_COLOR_F fill, D2D1_COLOR_F empty, D2D1_COLOR_F stroke) const; // 绘制一颗具体的带进度填充的矢量星星
    void CommitFromPoint(Point pt); // 从点击位置确立并提交最新分数

    float m_value = 0.0f;               // 内部缓存的实际评分分值
    int m_maxRating = 5;                // 最大总星级数
    float m_step = 0.5f;                // 评分颗粒步长
    float m_starSize = 22.0f;           // 星星像素宽高尺寸
    bool m_isReadOnly = false;          // 是否为只读显示模式
    bool m_isClearEnabled = true;       // 是否允许通过再次点击相同分值以归零清空
    bool m_isDragging = false;          // 标记用户是否正拖动鼠标划过打分
    float m_hoverValue = -1.0f;         // 鼠标悬停时的预览分值
    AnimatedScalar m_displayValueAnim{}; // 打分变动时的弹性缓冲渲染过渡动画
    Event<RatingControl*, float> m_onValueChangedEvent; // 评分改变事件对象
};

} // namespace CUI
