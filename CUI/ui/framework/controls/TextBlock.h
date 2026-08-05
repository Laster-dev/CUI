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

    const std::string& GetTextAlign() const { return m_textAlign; }
    void SetTextAlign(const std::string& align) {
        m_textAlign = align;
        MarkRenderContentDirty();
    }

    const std::string& GetVerticalAlign() const { return m_verticalAlign; }
    void SetVerticalAlign(const std::string& align) {
        m_verticalAlign = align;
        MarkRenderContentDirty();
    }

private:
    std::string m_textAlign{ "Left" };
    std::string m_verticalAlign{ "Center" };
};

} // namespace CUI
