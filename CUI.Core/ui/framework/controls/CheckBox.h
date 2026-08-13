#pragma once
#include "Control.h"

namespace CUI {

enum class CheckState {
    Unchecked,
    Checked,
    Indeterminate
};

class CheckBox : public Control {
public:
    CheckBox();
    explicit CheckBox(const std::string& text);
    virtual ~CheckBox() = default;

    virtual const char* GetClassName() const override { return "CheckBox"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
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

    CheckState GetState() const { return m_state; }
    void SetState(CheckState state);

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
