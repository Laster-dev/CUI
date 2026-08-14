#pragma once
#include "Control.h"
#include "../core/Observable.h"

namespace CUI {

/**
 * @brief 复选框状态枚举。
 * Unchecked: 未选中。
 * Checked: 已选中。
 * Indeterminate: 不确定状态（半选，通常用于父节点指示子节点部分选中）。
 */
enum class CheckState {
    Unchecked,
    Checked,
    Indeterminate
};

template<>
struct PropertyValueTraits<CheckState> {
    static CheckState FromValue(const Value& value) {
        const std::string state = value.AsString("Unchecked");
        return state == "Checked" ? CheckState::Checked
            : (state == "Indeterminate" ? CheckState::Indeterminate : CheckState::Unchecked);
    }
    static Value ToValue(CheckState value) {
        return Value(value == CheckState::Checked ? "Checked"
            : (value == CheckState::Indeterminate ? "Indeterminate" : "Unchecked"));
    }
};

/**
 * @brief 复选框控件。
 * 支持两态（选中/未选）和三态（选中/未选/半选）。内置平滑状态切换及复选勾选符号绘制动画。
 */
class CheckBox : public Control {
public:
    CheckBox();
    explicit CheckBox(const std::string& text);
    virtual ~CheckBox() override;

    virtual const char* GetClassName() const override { return "CheckBox"; }
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void SetProperty(PropertyId id, const Value& val) override;
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual bool AcceptsTabFocus() const override { return true; }
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    PropertyRef<bool, PropertyId::ControlValue> Checked;     ///< 经典二态选中绑定属性 (支持 bool)
    PropertyRef<CheckState, PropertyId::CheckState> State;  ///< 三态选中绑定属性 (支持 CheckState)

    CheckState GetState() const { return m_state; }
    void SetState(CheckState state);

    // 绑定至布尔型数据源 (自动转换 Checked <-> bool)
    void Bind(const std::shared_ptr<Observable<bool>>& value);
    // 绑定至三态型数据源
    void Bind(const std::shared_ptr<Observable<CheckState>>& value, bool twoWay = true);
    void Unbind();
    bool IsUpdatingFromBinding() const { return Checked.IsUpdating() || State.IsUpdating(); }

    bool IsThreeState() const { return m_isThreeState; }
    void SetIsThreeState(bool threeState) {
        m_isThreeState = threeState;
        NotifyFieldChanged(PropertyId::IsThreeState, Value(threeState));
    }

    Event<CheckBox*, CheckState>& OnCheckStateChanged() { return m_onCheckStateChangedEvent; }

protected:
    void CycleState();

private:
    CheckState m_state = CheckState::Unchecked;
    bool m_isThreeState = false;
    AnimatedScalar m_fillAnim{};
    AnimatedScalar m_checkAnim{};
    AnimatedScalar m_indeterminateAnim{};

    Event<CheckBox*, CheckState> m_onCheckStateChangedEvent;
};

} // namespace CUI
