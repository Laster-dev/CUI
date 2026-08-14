#pragma once
#include "UIElement.h"

namespace CUI {

enum class TextAlignment : uint8_t {
    Left,
    Center,
    Right
};

enum class TextVerticalAlignment : uint8_t {
    Top,
    Center,
    Bottom
};

class TextBlock : public UIElement {
public:
    TextBlock();
    explicit TextBlock(const std::string& text);
    virtual ~TextBlock() = default;

    virtual const char* GetClassName() const override { return "TextBlock"; }
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void SetProperty(PropertyId id, const Value& val) override;

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;

    TextAlignment GetTextAlign() const { return m_textAlign; }
    void SetTextAlign(TextAlignment align) {
        if (m_textAlign == align) return;
        m_textAlign = align;
        MarkRenderContentDirty();
    }

    TextVerticalAlignment GetVerticalAlign() const { return m_verticalAlign; }
    void SetVerticalAlign(TextVerticalAlignment align) {
        if (m_verticalAlign == align) return;
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
    TextAlignment m_textAlign = TextAlignment::Left;
    TextVerticalAlignment m_verticalAlign = TextVerticalAlignment::Center;
    float m_lineSpacing = 1.0f;
    float m_lineHeight = 0.0f;
};

} // namespace CUI
