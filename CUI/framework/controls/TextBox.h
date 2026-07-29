#pragma once
#include "Control.h"
#include <vector>
#include <dwrite.h>
#include <wrl/client.h>

namespace CUI {

struct TextBoxUndoState {
    std::string text;
    int cursorPos = 0;
    int selectionStart = 0;
    int selectionEnd = 0;
};

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
    void CommitImeResult(const std::wstring& result);

    void SetCompositionString(const std::wstring& compStr) { m_compString = compStr; }
    std::wstring GetCompositionString() const { return m_compString; }

    void SelectAll();
    bool HasSelection() const { return m_selectionStart != m_selectionEnd; }
    void DeleteSelection();

    std::string GetText() const { return GetProperty("text").AsString(); }
    void SetText(const std::string& text);

    std::string GetPlaceholder() const { return GetProperty("placeholder").AsString(); }
    void SetPlaceholder(const std::string& ph) { SetProperty("placeholder", Value(ph)); }

private:
    bool GetAcceptsReturn() const;
    bool IsTextWrapping() const;
    bool IsMultiline() const;

    Rect GetTextRect() const;
    Point GetLayoutOrigin(const Rect& textRect) const;

    Microsoft::WRL::ComPtr<IDWriteTextLayout> BuildTextLayout(GraphicsContext& ctx, const std::wstring& wtext,
                                                              const Rect& textRect) const;

    int GetCaretIndexFromPoint(GraphicsContext& ctx, float x, float y);
    GraphicsContext::TextCaretInfo GetCaretScreenPos(GraphicsContext& ctx, int caretPos);
    void EnsureCaretVisible(GraphicsContext& ctx);
    void ClampScrollOffsets(GraphicsContext& ctx, const std::wstring& wtext, const Rect& textRect);
    float GetContentWidth(GraphicsContext& ctx, const std::wstring& wtext, const Rect& textRect) const;
    void InsertText(const std::wstring& text);

    void PushUndoState();
    void Undo();
    void Redo();

    int m_cursorPos = 0;
    int m_selectionStart = 0;
    int m_selectionEnd = 0;
    bool m_isDraggingSelection = false;
    float m_scrollOffsetX = 0.0f;
    float m_scrollOffsetY = 0.0f;
    std::wstring m_compString;
    int m_suppressCharCount = 0;

    std::vector<TextBoxUndoState> m_undoStack;
    std::vector<TextBoxUndoState> m_redoStack;
    bool m_undoing = false;
};

} // namespace CUI
