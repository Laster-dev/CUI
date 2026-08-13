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
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void SetProperty(PropertyId id, const Value& val) override;

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

    float GetLineSpacing() const { return m_lineSpacing; }
    void SetLineSpacing(float spacing) {
        m_lineSpacing = spacing;
        InvalidateMeasure();
        MarkRenderContentDirty();
    }
    float GetLineHeight() const { return m_lineHeight; }
    void SetLineHeight(float height) {
        m_lineHeight = height;
        InvalidateMeasure();
        MarkRenderContentDirty();
    }

private:
    std::string m_textAlign{ "Left" };
    std::string m_verticalAlign{ "Center" };
    float m_lineSpacing = 1.0f;
    float m_lineHeight = 0.0f;
};

} // namespace CUI
