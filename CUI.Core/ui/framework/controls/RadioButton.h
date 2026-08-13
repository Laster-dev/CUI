#pragma once
#include "CheckBox.h"

namespace CUI {

class RadioButton : public CheckBox {
public:
    RadioButton();
    explicit RadioButton(const std::string& text);
    virtual ~RadioButton() = default;

    virtual const char* GetClassName() const override { return "RadioButton"; }
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void SetProperty(PropertyId id, const Value& val) override;

    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    const std::string& GetGroupName() const { return m_groupName; }
    void SetGroupName(const std::string& group) {
        if (m_groupName == group) {
            return;
        }
        m_groupName = group;
        NotifyFieldChanged(PropertyId::GroupName, Value(group));
        MarkRenderContentDirty();
    }

private:
    void SetChecked(bool checked);
    void UncheckSiblingsInGroup();
    std::string m_groupName{ "DefaultGroup" };
    AnimatedScalar m_selectionAnim{};
};

} // namespace CUI
