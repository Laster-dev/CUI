#pragma once
#include "Control.h"

namespace CUI {

class TextBox : public Control {
public:
    TextBox();
    explicit TextBox(const std::string& placeholder);
    virtual ~TextBox() = default;

    virtual const char* GetClassName() const override { return "TextBox"; }
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_IBEAM) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;

    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseDblClick(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnKeyDown(int vkCode) override;
    virtual void OnFocus() override;
    virtual void OnBlur() override;
    void OnCharInput(wchar_t ch);

    void SetCompositionString(const std::wstring& compStr) { m_compString = compStr; }
    std::wstring GetCompositionString() const { return m_compString; }

    void SelectAll();
    bool HasSelection() const { return m_selectionStart != m_selectionEnd; }
    void DeleteSelection();

    std::string GetText() const { return GetProperty("text").AsString(); }
    void SetText(const std::string& text) { SetProperty("text", Value(text)); }

    std::string GetPlaceholder() const { return GetProperty("placeholder").AsString(); }
    void SetPlaceholder(const std::string& ph) { SetProperty("placeholder", Value(ph)); }

private:
    int GetCaretIndexFromX(GraphicsContext& ctx, float x);

    int m_cursorPos = 0;
    int m_selectionStart = 0;
    int m_selectionEnd = 0;
    bool m_isDraggingSelection = false;
    std::wstring m_compString;
};

} // namespace CUI
