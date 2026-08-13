#pragma once
#include "Control.h"
#include "ScrollbarAutoHide.h"
#include "../window/PopupHost.h"
#include <functional>
#include <string>
#include <vector>
#include <algorithm>

namespace CUI {

// WinUI-style search field with self-drawn suggestion popup (not composed of TextBox/ListBox).
class AutoSuggestBox : public Control, public IPopup {
public:
    using SuggestionProvider = std::function<std::vector<std::string>(const std::string& query)>;

    AutoSuggestBox();
    virtual ~AutoSuggestBox() = default;

    virtual const char* GetClassName() const override { return "AutoSuggestBox"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual HCURSOR GetCursor() const override;
    bool AcceptsTabFocus() const override { return true; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnRenderOverlay(GraphicsContext& ctx) override;
    virtual UIElement* HitTest(float x, float y) override;
    virtual UIElement* HitTestOverlay(float x, float y) override;

    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual void OnMouseWheel(float delta) override;
    virtual void OnKeyDown(int vkCode) override;
    virtual void OnCharInput(wchar_t ch) override;
    virtual void OnFocus() override;
    virtual void OnBlur() override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual void CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override;
    virtual bool ShouldClipToBounds() const override { return !m_suggestionsOpen; }

    // IPopup
    virtual bool IsPopupOpen() const override { return m_suggestionsOpen; }
    virtual Rect GetPopupBounds() const override;
    virtual bool HitDismissExempt(float x, float y) const override;
    virtual UIElement* HitTestPopup(float x, float y) override { return HitTestOverlay(x, y); }
    virtual void RenderPopup(GraphicsContext& ctx) override;
    virtual void OnLightDismiss() override { CloseSuggestions(); }

    const std::string& GetText() const { return UIElement::GetText(); }
    void SetText(const std::string& text);
    void SetPlaceholder(const std::string& text);

    // Full catalog; filtered with case-insensitive substring match unless a provider is set.
    void SetSuggestionItems(const std::vector<std::string>& items);
    void ClearSuggestionItems();
    const std::vector<std::string>& GetSuggestionItems() const { return m_catalog; }

    // Optional custom provider; when set, catalog filter is skipped.
    void SetSuggestionProvider(SuggestionProvider provider);

    float GetSuggestionItemHeight() const { return m_suggestionItemHeight; }
    void SetSuggestionItemHeight(float h) { m_suggestionItemHeight = h; }

    int GetMaxVisibleSuggestions() const { return m_maxVisibleSuggestions; }
    void SetMaxVisibleSuggestions(int n) { m_maxVisibleSuggestions = (std::max)(1, n); }

    Event<AutoSuggestBox*, const std::string&>& OnTextChanged() { return m_onTextChanged; }
    Event<AutoSuggestBox*, const std::string&>& OnSuggestionChosen() { return m_onSuggestionChosen; }
    Event<AutoSuggestBox*, const std::string&>& OnQuerySubmitted() { return m_onQuerySubmitted; }

private:
    enum class HitPart : uint8_t { None, Text, Clear };

    static constexpr float kIconSlot = 28.0f;
    static constexpr float kClearSlot = 28.0f;
    static constexpr float kDebounceSec = 0.12f;

    Rect IconRect() const;
    Rect TextRect() const;
    Rect ClearRect() const;
    HitPart HitTestPart(Point pt) const;

    void SetTextInternal(const std::string& text, bool fireChanged, bool scheduleSuggest);
    void InsertUtf16(const std::wstring& chunk);
    void DeleteSelectionOrBackward();
    void DeleteForward();
    void SetCaret(int utf16Pos);
    void EnsureCaretVisible();
    void ResetCaretBlink();

    void ScheduleSuggestRefresh();
    void RefreshSuggestionsNow();
    void OpenSuggestions();
    void CloseSuggestions();
    void ChooseSuggestion(int index);
    void SubmitQuery();
    void EnsureSuggestionVisible(int index);

    float PopupProgress() const;
    int HitTestSuggestionIndex(Point pt) const;
    float SuggestionContentHeight() const;
    float SuggestionMaxScroll() const;

    std::vector<std::string> m_catalog;
    std::vector<std::string> m_filtered;
    SuggestionProvider m_provider;

    int m_caret = 0;          // UTF-16 index
    int m_selAnchor = -1;     // -1 = no selection; otherwise selection is [min,max) with caret
    float m_textScrollX = 0.0f;
    bool m_caretVisible = true;
    float m_caretBlink = 0.0f;

    bool m_suggestionsOpen = false;
    int m_highlightedIndex = -1;
    float m_suggestScroll = 0.0f;
    float m_debounceLeft = -1.0f;
    float m_suggestionItemHeight = 28.0f;
    int m_maxVisibleSuggestions = 8;

    bool m_clearHovered = false;
    HitPart m_pressed = HitPart::None;

    AnimatedScalar m_popupAnim{};
    ScrollbarAutoHide m_scrollbarAutoHide;

    Event<AutoSuggestBox*, const std::string&> m_onTextChanged;
    Event<AutoSuggestBox*, const std::string&> m_onSuggestionChosen;
    Event<AutoSuggestBox*, const std::string&> m_onQuerySubmitted;
};

} // namespace CUI
