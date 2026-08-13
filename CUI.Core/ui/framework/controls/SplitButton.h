#pragma once
#include "DropDownButton.h"

namespace CUI {

class SplitButton : public DropDownButton {
public:
    SplitButton();
    explicit SplitButton(const std::string& text);
    virtual ~SplitButton() = default;

    virtual const char* GetClassName() const override { return "SplitButton"; }

    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnKeyDown(int vkCode) override;

protected:
    virtual bool OpensOnPrimaryPress() const override { return false; }

private:
    Rect PrimaryRect() const;
    bool m_pressInChevron = false;
};

} // namespace CUI
