#pragma once
#include "Control.h"
#include "../text/TextLayoutCache.h"
#include "../dnd/DragDropService.h"
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

class TextBox : public Control, public IDropTarget {
public:
    TextBox();
    explicit TextBox(const std::string& placeholder);
    virtual ~TextBox() override;

    virtual const char* GetClassName() const override { return "TextBox"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual HCURSOR GetCursor() const override;
    bool AcceptsTabFocus() const override { return true; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseDblClick(Point pt) override;
    virtual void OnMouseRightClick(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseEnter() override;
    virtual void OnMouseLeave() override;
    virtual void OnKeyDown(int vkCode) override;
    virtual void OnFocus() override;
    virtual void OnBlur() override;
    void OnCharInput(wchar_t ch);
    void CommitImeResult(const std::wstring& result);

    void SetCompositionString(const std::wstring& compStr);
    std::wstring GetCompositionString() const { return m_compString; }

    void SelectAll();
    bool HasSelection() const { return m_selectionStart != m_selectionEnd; }
    void DeleteSelection();

    const std::string& GetText() const { return UIElement::GetText(); }
    void SetText(const std::string& text);

    const std::string& GetPlaceholder() const { return UIElement::GetPlaceholder(); }
    void SetPlaceholder(const std::string& ph) { UIElement::SetPlaceholder(ph); }

    virtual std::wstring GetDisplayedText() const;
    bool IsPasswordMode() const { return m_isPasswordMode; }
    void SetIsPasswordMode(bool isPass) {
        m_isPasswordMode = isPass;
        MarkRenderContentDirty();
    }

    wchar_t GetPasswordChar() const {
        return L'•';
    }

    bool IsPasswordRevealed() const { return m_isPasswordRevealed; }
    void SetIsPasswordRevealed(bool revealed) { m_isPasswordRevealed = revealed; MarkRenderContentDirty(); }

    bool GetShowRevealButton() const { return m_showRevealButton; }
    void SetShowRevealButton(bool show) {
        m_showRevealButton = show;
        MarkRenderContentDirty();
    }

    bool IsReadOnly() const { return m_isReadOnly; }
    void SetIsReadOnly(bool readOnly) {
        m_isReadOnly = readOnly;
        MarkRenderContentDirty();
    }

    void SetAcceptsReturn(bool accepts) {
        m_acceptsReturn = accepts;
        MarkRenderContentDirty();
    }

    void SetTextWrapping(bool wrap) {
        m_textWrapping = wrap;
        MarkRenderContentDirty();
    }

    float GetLineSpacing() const { return m_lineSpacing; }
    void SetLineSpacing(float spacing) {
        m_lineSpacing = spacing;
        MarkRenderContentDirty();
    }

    float GetLineHeight() const { return m_lineHeight; }
    void SetLineHeight(float height) {
        m_lineHeight = height;
        MarkRenderContentDirty();
    }

    int GetCaretBlinkRate() const { return m_caretBlinkRate; }
    void SetCaretBlinkRate(int ms) {
        m_caretBlinkRate = ms;
        MarkRenderContentDirty();
    }

    float GetCaretWidth() const { return m_caretWidth; }
    void SetCaretWidth(float width) {
        m_caretWidth = width;
        MarkRenderContentDirty();
    }

    Event<TextBox*, const std::string&>& OnTextChanged() { return m_onTextChangedEvent; }

    void SetAllowDrop(bool allow) { m_allowDrop = allow; }
    bool GetAllowDrop() const { return m_allowDrop; }
    DragDropEffects OnDragOver(Point pt, const DataPackage& data, DragDropEffects allowed) override;
    void OnDragLeave() override;
    bool OnDrop(Point pt, DataPackage& data, DragDropEffects effect) override;
    Rect DropHighlightRect() const override { return m_bounds; }

protected:
    Rect GetRevealButtonRect() const;

private:
    bool GetAcceptsReturn() const { return m_acceptsReturn; }
    bool IsTextWrapping() const { return m_textWrapping; }
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
    void NotifyHostOverlayDirty();

    int m_cursorPos = 0;
    int m_selectionStart = 0;
    int m_selectionEnd = 0;
    bool m_isDraggingSelection = false;
    float m_scrollOffsetX = 0.0f;
    float m_scrollOffsetY = 0.0f;
    std::wstring m_compString;
    int m_suppressCharCount = 0;
    bool m_isPasswordRevealed = false;
    bool m_isPasswordMode = false;
    bool m_showRevealButton = true;
    bool m_isReadOnly = false;
    bool m_acceptsReturn = false;
    bool m_textWrapping = false;
    float m_lineSpacing = 1.0f;
    float m_lineHeight = 0.0f;
    int m_caretBlinkRate = 500;
    float m_caretWidth = 1.5f;
    AnimatedScalar m_labelAnim{};
    AnimatedScalar m_focusLineAnim{};
    bool m_lastCaretBlinkPhase = true;
    bool m_caretBlinkDirty = false;
    Rect m_lastCaretDirtyRect{};

    std::vector<TextBoxUndoState> m_undoStack;
    std::vector<TextBoxUndoState> m_redoStack;
    bool m_undoing = false;
    mutable TextLayoutCache m_textLayoutCache;

    Event<TextBox*, const std::string&> m_onTextChangedEvent;
    bool m_allowDrop = false;
    bool m_dropHover = false;
};

} // namespace CUI
