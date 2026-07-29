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
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;

    CheckState GetState() const;
    void SetState(CheckState state);

    bool IsThreeState() const { return GetProperty("isThreeState").AsBool(false); }
    void SetIsThreeState(bool threeState) { SetProperty("isThreeState", Value(threeState)); }

    Event<CheckBox*, CheckState>& OnCheckStateChanged() { return m_onCheckStateChangedEvent; }

private:
    Event<CheckBox*, CheckState> m_onCheckStateChangedEvent;
};

} // namespace CUI
