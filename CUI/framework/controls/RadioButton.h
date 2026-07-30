#pragma once
#include "CheckBox.h"

namespace CUI {

class RadioButton : public CheckBox {
public:
    RadioButton();
    explicit RadioButton(const std::string& text);
    virtual ~RadioButton() = default;

    virtual const char* GetClassName() const override { return "RadioButton"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;

    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual bool OnAnimationTick() override;

    std::string GetGroupName() const { return GetProperty("groupName").AsString(""); }
    void SetGroupName(const std::string& group) { SetProperty("groupName", Value(group)); }

private:
    void SetChecked(bool checked);
    void UncheckSiblingsInGroup();
    float m_selectionProgress = 0.0f;
};

} // namespace CUI
