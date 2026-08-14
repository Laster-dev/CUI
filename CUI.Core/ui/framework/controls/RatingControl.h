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

    virtual const char* GetClassName() const override { return "RatingControl"; }
    virtual HCURSOR GetCursor() const override;
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void SetProperty(PropertyId id, const Value& val) override;

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    PropertyRef<float, PropertyId::ControlValue> ValueProperty; ///< 当前评分分数的双向绑定属性

    float GetValue() const { return m_value; }
    void SetValue(float val);

    int GetMaxRating() const { return m_maxRating; }
    void SetMaxRating(int maxRating);

    float GetStep() const { return m_step; }
    void SetStep(float step);

    bool IsReadOnly() const { return m_isReadOnly; }
    void SetIsReadOnly(bool readOnly);

    /**
     * @brief 是否允许清空分数。
     * 若为 true，当用户点击当前相同分数对应的星星时，打分会自动归零。
     */
    bool IsClearEnabled() const { return m_isClearEnabled; }
    void SetIsClearEnabled(bool enabled);

    float GetStarSize() const { return m_starSize; }
    void SetStarSize(float size);

    Event<RatingControl*, float>& OnValueChanged() { return m_onValueChangedEvent; }

private:
    float SnapValue(float val) const;
    float ShownValue() const;
    bool CanInteract() const;
    Rect ContentRect() const;
    Rect ClearRect() const;
    Rect StarRect(int index) const; // 0-based
    float ValueFromPoint(Point pt) const;
    void DrawStar(GraphicsContext& ctx, const Rect& slot, float fill01,
                  D2D1_COLOR_F fill, D2D1_COLOR_F empty, D2D1_COLOR_F stroke) const;
    void CommitFromPoint(Point pt);

    float m_value = 0.0f;
    int m_maxRating = 5;
    float m_step = 0.5f;
    float m_starSize = 22.0f;
    bool m_isReadOnly = false;
    bool m_isClearEnabled = true;
    bool m_isDragging = false;
    float m_hoverValue = -1.0f;
    AnimatedScalar m_displayValueAnim{}; ///< 打分变动时的弹性缓冲渲染过渡动画
    Event<RatingControl*, float> m_onValueChangedEvent;
};

} // namespace CUI
