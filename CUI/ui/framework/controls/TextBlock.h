#pragma once
#include "UIElement.h"

namespace CUI {

class TextBlock : public UIElement {
public:
    TextBlock();
    explicit TextBlock(const std::string& text);
    virtual ~TextBlock() = default;

    virtual const char* GetClassName() const override { return "TextBlock"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
};

} // namespace CUI
