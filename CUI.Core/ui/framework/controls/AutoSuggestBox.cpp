#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "AutoSuggestBox.h"
#include "../core/Value.h"
#include "../style/ThemeManager.h"
#include "../window/PopupPlacement.h"
#include "../window/Dpi.h"
#include <algorithm>
#include <cmath>

namespace CUI {

namespace {
std::string ToLowerAscii(std::string s) {
    for (char& c : s) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return s;
}

bool ContainsInsensitive(const std::string& hay, const std::string& needle) {
    if (needle.empty()) {
        return true;
    }
    const std::string h = ToLowerAscii(hay);
    const std::string n = ToLowerAscii(needle);
    return h.find(n) != std::string::npos;
}

void DrawSearchGlyph(GraphicsContext& ctx, const Rect& slot, D2D1_COLOR_F color) {
    const float cx = slot.x + slot.width * 0.42f;
    const float cy = slot.y + slot.height * 0.48f;
    const float r = (std::min)(slot.width, slot.height) * 0.18f;
    constexpr int kSeg = 20;
    Point prev(cx + r, cy);
    for (int i = 1; i <= kSeg; ++i) {
        const float a = (static_cast<float>(i) / static_cast<float>(kSeg)) * 6.28318530718f;
        Point next(cx + std::cos(a) * r, cy + std::sin(a) * r);
        ctx.DrawSmoothLine(prev, next, color, 1.5f);
        prev = next;
    }
    const float handle = r * 0.95f;
    ctx.DrawSmoothLine(
        Point(cx + r * 0.65f, cy + r * 0.65f),
        Point(cx + r * 0.65f + handle, cy + r * 0.65f + handle),
        color,
        1.7f);
}
} // namespace

AutoSuggestBox::AutoSuggestBox() {
    SetPlaceholder("搜索…");
    SetBackgroundToken(ThemeTokenId::InputBackground);
    SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
    SetBorderToken(ThemeTokenId::InputBorder);
    SetFocusedBorderToken(ThemeTokenId::FocusedBorder);
    SetColorToken(ThemeTokenId::TextPrimary);
    SetPlaceholderColorToken(ThemeTokenId::TextMuted);
    SetBackground(ThemeManager::Instance().GetColor(ThemeTokenId::InputBackground));
    SetBorderBrush(ThemeManager::Instance().GetColor(ThemeTokenId::InputBorder));
    SetBorderThickness(1.0f);
    SetColor(ThemeManager::Instance().GetColor(ThemeTokenId::TextPrimary));
    SetFontFamily("Segoe UI");
    SetFontSize(13.0f);
    SetPadding(Thickness(4, 6, 4, 6));
    SetCornerRadius(4.0f);
    SetWidth(280.0f);
    SetHeight(34.0f);
    UIElement::SetText("");
}

std::vector<PropertyMeta> AutoSuggestBox::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "placeholder", "占位文本 (Placeholder)", "内容", "string" });
    metas.push_back({ "fontFamily", "字体名称 (FontFamily)", "字体文本", "enum", { "Segoe UI", "微软雅黑", "Consolas" } });
    metas.push_back({ "fontSize", "字体大小 (FontSize)", "字体文本", "number" });
    return metas;
}

HCURSOR AutoSuggestBox::GetCursor() const {
    if (!IsEnabled()) {
        return nullptr;
    }
    return LoadCursor(nullptr, IDC_IBEAM);
}

void AutoSuggestBox::SetPlaceholder(const std::string& text) {
    UIElement::SetPlaceholder(text);
    MarkRenderContentDirty();
}

void AutoSuggestBox::SetText(const std::string& text) {
    SetTextInternal(text, true, true);
}

void AutoSuggestBox::SetTextInternal(const std::string& text, bool fireChanged, bool scheduleSuggest) {
    if (UIElement::GetText() == text) {
        if (scheduleSuggest) {
            ScheduleSuggestRefresh();
        }
        return;
    }
    UIElement::SetText(text);
    const int len = static_cast<int>(Utf8ToUtf16(text).length());
    m_caret = std::clamp(m_caret, 0, len);
    m_selAnchor = -1;
    EnsureCaretVisible();
    ResetCaretBlink();
    MarkRenderContentDirty();
    if (fireChanged) {
        m_onTextChanged.Invoke(this, GetText());
    }
    if (scheduleSuggest) {
        ScheduleSuggestRefresh();
    }
}

void AutoSuggestBox::SetSuggestionItems(const std::vector<std::string>& items) {
    m_catalog = items;
    ScheduleSuggestRefresh();
}

void AutoSuggestBox::ClearSuggestionItems() {
    m_catalog.clear();
    m_filtered.clear();
    CloseSuggestions();
    MarkRenderContentDirty();
}

void AutoSuggestBox::SetSuggestionProvider(SuggestionProvider provider) {
    m_provider = std::move(provider);
    ScheduleSuggestRefresh();
}

Size AutoSuggestBox::Measure(Size availableSize) {
    (void)availableSize;
    const float w = GetWidth() >= 0.0f ? GetWidth() : 280.0f;
    const float h = GetHeight() >= 0.0f ? GetHeight() : 34.0f;
    m_desiredSize = Size(w, h);
    return m_desiredSize;
}

Rect AutoSuggestBox::IconRect() const {
    return Rect(m_bounds.x, m_bounds.y, kIconSlot, m_bounds.height);
}

Rect AutoSuggestBox::ClearRect() const {
    return Rect(m_bounds.x + m_bounds.width - kClearSlot, m_bounds.y, kClearSlot, m_bounds.height);
}

Rect AutoSuggestBox::TextRect() const {
    const float rightPad = GetText().empty() ? 4.0f : kClearSlot;
    return Rect(
        m_bounds.x + kIconSlot,
        m_bounds.y,
        (std::max)(8.0f, m_bounds.width - kIconSlot - rightPad),
        m_bounds.height);
}

AutoSuggestBox::HitPart AutoSuggestBox::HitTestPart(Point pt) const {
    if (!m_bounds.Contains(pt.x, pt.y)) {
        return HitPart::None;
    }
    if (!GetText().empty() && ClearRect().Contains(pt.x, pt.y)) {
        return HitPart::Clear;
    }
    return HitPart::Text;
}

void AutoSuggestBox::SetCaret(int utf16Pos) {
    const int len = static_cast<int>(Utf8ToUtf16(GetText()).length());
    m_caret = std::clamp(utf16Pos, 0, len);
    m_selAnchor = -1;
    EnsureCaretVisible();
    ResetCaretBlink();
    MarkRenderContentDirty();
}

void AutoSuggestBox::EnsureCaretVisible() {
    GraphicsContext ctx;
    const Rect textRect = TextRect();
    const std::wstring w = Utf8ToUtf16(GetText());
    const float caretX = ctx.MeasureText(
        Utf16ToUtf8(w.substr(0, static_cast<size_t>(m_caret))),
        GetFontFamily(),
        GetFontSize(),
        DWRITE_FONT_WEIGHT_NORMAL).width;
    const float viewW = (std::max)(8.0f, textRect.width - 4.0f);
    if (caretX - m_textScrollX > viewW) {
        m_textScrollX = caretX - viewW;
    }
    if (caretX - m_textScrollX < 0.0f) {
        m_textScrollX = caretX;
    }
    m_textScrollX = (std::max)(0.0f, m_textScrollX);
}

void AutoSuggestBox::ResetCaretBlink() {
    m_caretBlink = 0.0f;
    m_caretVisible = true;
    RequestAnimationTicks();
}

void AutoSuggestBox::InsertUtf16(const std::wstring& chunk) {
    if (chunk.empty()) {
        return;
    }
    std::wstring w = Utf8ToUtf16(GetText());
    int start = m_caret;
    int end = m_caret;
    if (m_selAnchor >= 0) {
        start = (std::min)(m_caret, m_selAnchor);
        end = (std::max)(m_caret, m_selAnchor);
    }
    w.replace(static_cast<size_t>(start), static_cast<size_t>(end - start), chunk);
    m_caret = start + static_cast<int>(chunk.length());
    m_selAnchor = -1;
    SetTextInternal(Utf16ToUtf8(w), true, true);
    EnsureCaretVisible();
}

void AutoSuggestBox::DeleteSelectionOrBackward() {
    std::wstring w = Utf8ToUtf16(GetText());
    if (m_selAnchor >= 0 && m_selAnchor != m_caret) {
        const int start = (std::min)(m_caret, m_selAnchor);
        const int end = (std::max)(m_caret, m_selAnchor);
        w.erase(static_cast<size_t>(start), static_cast<size_t>(end - start));
        m_caret = start;
        m_selAnchor = -1;
        SetTextInternal(Utf16ToUtf8(w), true, true);
        return;
    }
    if (m_caret <= 0) {
        return;
    }
    w.erase(static_cast<size_t>(m_caret - 1), 1);
    m_caret -= 1;
    SetTextInternal(Utf16ToUtf8(w), true, true);
}

void AutoSuggestBox::DeleteForward() {
    std::wstring w = Utf8ToUtf16(GetText());
    if (m_selAnchor >= 0 && m_selAnchor != m_caret) {
        DeleteSelectionOrBackward();
        return;
    }
    if (m_caret >= static_cast<int>(w.length())) {
        return;
    }
    w.erase(static_cast<size_t>(m_caret), 1);
    SetTextInternal(Utf16ToUtf8(w), true, true);
}

void AutoSuggestBox::ScheduleSuggestRefresh() {
    m_debounceLeft = kDebounceSec;
    RequestAnimationTicks();
}

void AutoSuggestBox::RefreshSuggestionsNow() {
    m_debounceLeft = -1.0f;

    std::string keepHighlight;
    if (m_highlightedIndex >= 0 && m_highlightedIndex < static_cast<int>(m_filtered.size())) {
        keepHighlight = m_filtered[static_cast<size_t>(m_highlightedIndex)];
    }

    const std::string query = GetText();
    if (m_provider) {
        m_filtered = m_provider(query);
    } else {
        m_filtered.clear();
        if (!query.empty()) {
            for (const auto& item : m_catalog) {
                if (ContainsInsensitive(item, query)) {
                    m_filtered.push_back(item);
                }
            }
        }
    }

    if (m_filtered.empty() || !m_isFocused) {
        CloseSuggestions();
        return;
    }

    m_highlightedIndex = 0;
    if (!keepHighlight.empty()) {
        for (int i = 0; i < static_cast<int>(m_filtered.size()); ++i) {
            if (m_filtered[static_cast<size_t>(i)] == keepHighlight) {
                m_highlightedIndex = i;
                break;
            }
        }
    }
    m_suggestScroll = 0.0f;
    EnsureSuggestionVisible(m_highlightedIndex);
    OpenSuggestions();
}

void AutoSuggestBox::BeginKeyboardNavigation() {
    m_keyboardNavActive = true;
    // Snapshot pointer in client DIPs so a no-op MOVE after scroll does not
    // steal the highlight back to whatever row sits under the cursor.
    if (PopupHost* host = PopupHost::Current()) {
        if (HWND hwnd = host->GetOwnerHwnd()) {
            POINT sp{};
            if (::GetCursorPos(&sp) && ::ScreenToClient(hwnd, &sp)) {
                const float scale = GetDpiScaleForWindow(hwnd);
                if (scale > 0.001f) {
                    m_keyboardNavMousePt = Point(
                        static_cast<float>(sp.x) / scale,
                        static_cast<float>(sp.y) / scale);
                }
            }
        }
    }
}

void AutoSuggestBox::MoveHighlightBy(int delta) {
    if (m_filtered.empty()) {
        return;
    }
    BeginKeyboardNavigation();
    if (m_highlightedIndex < 0) {
        m_highlightedIndex = (delta >= 0) ? 0 : static_cast<int>(m_filtered.size()) - 1;
    } else {
        m_highlightedIndex = std::clamp(
            m_highlightedIndex + delta,
            0,
            static_cast<int>(m_filtered.size()) - 1);
    }
    EnsureSuggestionVisible(m_highlightedIndex);
    MarkRenderRectDirty(GetPopupBounds().Inflate(2.0f));
}

void AutoSuggestBox::OpenSuggestions() {
    if (m_filtered.empty()) {
        return;
    }
    if (!m_suggestionsOpen) {
        m_suggestionsOpen = true;
        if (PopupHost* host = PopupHost::Current()) {
            host->Open(this);
        }
        RequestAnimationTicks();
    }
    MarkRenderContentDirty();
    MarkRenderRectDirty(GetPopupBounds().Inflate(4.0f));
}

void AutoSuggestBox::CloseSuggestions() {
    if (!m_suggestionsOpen && m_popupAnim.Current() <= 0.001f) {
        m_highlightedIndex = -1;
        return;
    }
    m_suggestionsOpen = false;
    m_highlightedIndex = -1;
    m_keyboardNavActive = false;
    if (PopupHost* host = PopupHost::Current()) {
        host->Close(this);
    }
    RequestAnimationTicks();
    MarkRenderContentDirty();
}

void AutoSuggestBox::ChooseSuggestion(int index) {
    if (index < 0 || index >= static_cast<int>(m_filtered.size())) {
        return;
    }
    const std::string chosen = m_filtered[static_cast<size_t>(index)];
    SetTextInternal(chosen, true, false);
    m_caret = static_cast<int>(Utf8ToUtf16(chosen).length());
    m_selAnchor = -1;
    EnsureCaretVisible();
    CloseSuggestions();
    m_onSuggestionChosen.Invoke(this, chosen);
    MarkRenderContentDirty();
}

void AutoSuggestBox::SubmitQuery() {
    CloseSuggestions();
    m_onQuerySubmitted.Invoke(this, GetText());
}

void AutoSuggestBox::EnsureSuggestionVisible(int index) {
    if (index < 0) {
        return;
    }
    const float itemH = m_suggestionItemHeight;
    const float visibleH = (std::min)(
        SuggestionContentHeight(),
        itemH * static_cast<float>(m_maxVisibleSuggestions));
    const float top = static_cast<float>(index) * itemH;
    const float bottom = top + itemH;
    if (top < m_suggestScroll) {
        m_suggestScroll = top;
    } else if (bottom > m_suggestScroll + visibleH) {
        m_suggestScroll = bottom - visibleH;
    }
    m_suggestScroll = std::clamp(m_suggestScroll, 0.0f, SuggestionMaxScroll());
}

float AutoSuggestBox::SuggestionContentHeight() const {
    return m_suggestionItemHeight * static_cast<float>((std::max)(m_filtered.size(), size_t{ 1 }));
}

float AutoSuggestBox::SuggestionMaxScroll() const {
    const float visibleH = (std::min)(
        SuggestionContentHeight(),
        m_suggestionItemHeight * static_cast<float>(m_maxVisibleSuggestions));
    return (std::max)(0.0f, SuggestionContentHeight() - visibleH);
}

float AutoSuggestBox::PopupProgress() const {
    if (!UIElement::AreAnimationsEnabled()) {
        return m_suggestionsOpen ? 1.0f : 0.0f;
    }
    return m_popupAnim.Current();
}

Rect AutoSuggestBox::GetPopupBounds() const {
    const float itemH = m_suggestionItemHeight;
    const size_t count = (std::max)(m_filtered.size(), size_t{ 1 });
    const float desiredH = itemH * static_cast<float>((std::min)(count, static_cast<size_t>(m_maxVisibleSuggestions)));
    return PlacePopupNearAnchor(m_bounds, m_bounds.width, desiredH, GetPopupViewportOrDefault(), 2.0f);
}

bool AutoSuggestBox::HitDismissExempt(float x, float y) const {
    if (m_bounds.Contains(x, y)) {
        return true;
    }
    return GetPopupBounds().Contains(x, y);
}

int AutoSuggestBox::HitTestSuggestionIndex(Point pt) const {
    const float progress = PopupProgress();
    if (progress <= 0.5f || m_filtered.empty()) {
        return -1;
    }
    const Rect menu = GetPopupBounds();
    const float currentH = (m_suggestionsOpen && progress >= 0.98f) ? menu.height : (menu.height * progress);
    if (!Rect(menu.x, menu.y, menu.width, currentH).Contains(pt.x, pt.y)) {
        return -1;
    }
    // Match RenderPopup: rows start at menu.y + 2 - scroll.
    const float localY = pt.y - (menu.y + 2.0f) + m_suggestScroll;
    if (localY < 0.0f) {
        return -1;
    }
    const int index = static_cast<int>(localY / m_suggestionItemHeight);
    if (index < 0 || index >= static_cast<int>(m_filtered.size())) {
        return -1;
    }
    return index;
}

UIElement* AutoSuggestBox::HitTest(float x, float y) {
    if (GetVisibility() != Visibility::Visible) {
        return nullptr;
    }
    if (m_bounds.Contains(x, y)) {
        return this;
    }
    return nullptr;
}

UIElement* AutoSuggestBox::HitTestOverlay(float x, float y) {
    const float progress = PopupProgress();
    if (progress <= 0.5f || m_filtered.empty()) {
        return nullptr;
    }
    const Rect menu = GetPopupBounds();
    const float currentH = (m_suggestionsOpen && progress >= 0.98f) ? menu.height : (menu.height * progress);
    if (Rect(menu.x, menu.y, menu.width, currentH).Contains(x, y)) {
        return this;
    }
    return nullptr;
}

void AutoSuggestBox::OnRender(GraphicsContext& ctx) {
    const float radius = GetCornerRadius() >= 0.0f ? GetCornerRadius() : 4.0f;
    D2D1_COLOR_F bg = GetAnimatedBackground(ThemeManager::Instance().GetFlatColor(ThemeTokenId::InputBackground));
    ctx.FillRoundedRect(m_bounds, radius, bg);

    D2D1_COLOR_F border = (m_isFocused || m_suggestionsOpen)
        ? ResolveThemeColor(GetFocusedBorderToken(), ThemeTokenId::FocusedBorder)
        : ResolveThemeColor(GetBorderToken(), ThemeTokenId::InputBorder);
    const float borderW = (m_isFocused || m_suggestionsOpen) ? 1.5f : GetBorderThickness();
    if (borderW > 0.0f) {
        ctx.DrawRoundedRect(m_bounds, radius, border, borderW);
    }

    const auto& tokens = ThemeManager::Instance().GetTokens();
    DrawSearchGlyph(ctx, IconRect(), tokens.textSecondary);

    const Rect textRect = TextRect();
    ctx.PushClip(textRect);

    const std::string& text = GetText();
    if (text.empty() && !m_isFocused) {
        ctx.DrawText(
            GetPlaceholder(),
            Rect(textRect.x + 2.0f, textRect.y, textRect.width - 4.0f, textRect.height),
            ResolveThemeColor(GetPlaceholderColorToken(), ThemeTokenId::TextMuted),
            GetFontFamily(),
            GetFontSize(),
            DWRITE_TEXT_ALIGNMENT_LEADING,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
            DWRITE_FONT_WEIGHT_NORMAL,
            true);
    } else {
        const std::wstring w = Utf8ToUtf16(text);
        D2D1_COLOR_F textColor = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextPrimary);

        if (m_selAnchor >= 0 && m_selAnchor != m_caret) {
            const int a = (std::min)(m_caret, m_selAnchor);
            const int b = (std::max)(m_caret, m_selAnchor);
            const float x0 = ctx.MeasureText(
                Utf16ToUtf8(w.substr(0, static_cast<size_t>(a))),
                GetFontFamily(), GetFontSize(), DWRITE_FONT_WEIGHT_NORMAL).width;
            const float x1 = ctx.MeasureText(
                Utf16ToUtf8(w.substr(0, static_cast<size_t>(b))),
                GetFontFamily(), GetFontSize(), DWRITE_FONT_WEIGHT_NORMAL).width;
            D2D1_COLOR_F sel = tokens.accentColor;
            sel.a = 0.28f;
            ctx.FillRect(
                Rect(textRect.x + 2.0f + x0 - m_textScrollX, textRect.y + 6.0f, (std::max)(1.0f, x1 - x0), textRect.height - 12.0f),
                sel);
        }

        ctx.DrawText(
            text,
            Rect(textRect.x + 2.0f - m_textScrollX, textRect.y, textRect.width + m_textScrollX + 40.0f, textRect.height),
            textColor,
            GetFontFamily(),
            GetFontSize(),
            DWRITE_TEXT_ALIGNMENT_LEADING,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        if (m_isFocused && m_caretVisible) {
            const float caretX = ctx.MeasureText(
                Utf16ToUtf8(w.substr(0, static_cast<size_t>(m_caret))),
                GetFontFamily(), GetFontSize(), DWRITE_FONT_WEIGHT_NORMAL).width;
            const float x = textRect.x + 2.0f + caretX - m_textScrollX;
            ctx.FillRect(Rect(x, textRect.y + 7.0f, 1.5f, textRect.height - 14.0f), textColor);
        }
    }
    ctx.PopClip();

    if (!text.empty()) {
        const Rect clear = ClearRect();
        D2D1_COLOR_F clearBg = m_clearHovered ? tokens.hoverBackground : D2D1::ColorF(0, 0, 0, 0);
        if (m_clearHovered) {
            ctx.FillRoundedRect(
                Rect(clear.x + 4.0f, clear.y + 5.0f, clear.width - 8.0f, clear.height - 10.0f),
                3.0f,
                clearBg);
        }
        ctx.DrawText(
            "×",
            clear,
            tokens.textSecondary,
            "Segoe UI",
            16.0f,
            DWRITE_TEXT_ALIGNMENT_CENTER,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
}

void AutoSuggestBox::RenderPopup(GraphicsContext& ctx) {
    const float progress = PopupProgress();
    if (progress <= 0.001f || m_filtered.empty()) {
        return;
    }

    const Rect menu = GetPopupBounds();
    const float currentH = (m_suggestionsOpen && progress >= 0.98f) ? menu.height : (menu.height * progress);
    ctx.PushClip(Rect(menu.x, menu.y, menu.width, currentH));

    const auto& tokens = ThemeManager::Instance().GetTokens();
    const float radius = GetCornerRadius() >= 0.0f ? GetCornerRadius() : 4.0f;
    ctx.FillRoundedRect(menu, radius, tokens.cardBackground);
    ctx.DrawRoundedRect(menu, radius, tokens.cardBorder, 1.25f);

    const float itemH = m_suggestionItemHeight;
    for (int i = 0; i < static_cast<int>(m_filtered.size()); ++i) {
        const float y = menu.y + 2.0f + static_cast<float>(i) * itemH - m_suggestScroll;
        if (y + itemH < menu.y || y > menu.y + currentH) {
            continue;
        }
        Rect row(menu.x + 2.0f, y, menu.width - 4.0f, itemH - 2.0f);
        const bool hi = (i == m_highlightedIndex);
        if (hi) {
            D2D1_COLOR_F sel = tokens.accentColor;
            sel.a = 0.18f;
            ctx.FillRoundedRect(row, 3.0f, sel);
        }
        ctx.DrawText(
            m_filtered[static_cast<size_t>(i)],
            Rect(row.x + 10.0f, row.y, row.width - 16.0f, row.height),
            hi ? tokens.accentColor : tokens.textPrimary,
            GetFontFamily(),
            GetFontSize(),
            DWRITE_TEXT_ALIGNMENT_LEADING,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
            hi ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
            true);
    }

    const float maxScroll = SuggestionMaxScroll();
    if (maxScroll > 0.001f && m_scrollbarAutoHide.IsDrawn()) {
        constexpr float kScrollW = 8.0f;
        Rect track(menu.x + menu.width - kScrollW, menu.y, kScrollW, currentH);
        const float vis = m_scrollbarAutoHide.Opacity();
        D2D1_COLOR_F trackColor = tokens.cardBorder;
        trackColor.a = 0.35f * vis;
        ctx.DrawRoundedRect(track, 4.0f, trackColor, 1.0f);
        const float contentH = SuggestionContentHeight();
        const float thumbH = (std::max)(16.0f, (currentH * currentH) / contentH);
        const float travel = (std::max)(0.0f, currentH - thumbH);
        const float thumbY = track.y + (m_suggestScroll / maxScroll) * travel;
        D2D1_COLOR_F thumb = tokens.accentColor;
        thumb.a = 0.45f * vis;
        ctx.FillRoundedRect(Rect(track.x + 1.5f, thumbY, kScrollW - 3.0f, thumbH), 4.0f, thumb);
    }

    ctx.PopClip();
}

void AutoSuggestBox::OnRenderOverlay(GraphicsContext& ctx) {
    if (PopupHost::Current() && m_suggestionsOpen) {
        return;
    }
    RenderPopup(ctx);
}

void AutoSuggestBox::OnMouseDown(Point pt) {
    if (!IsEnabled()) {
        return;
    }
    Control::OnMouseDown(pt);

    if (m_suggestionsOpen || PopupProgress() > 0.5f) {
        const int sug = HitTestSuggestionIndex(pt);
        if (sug >= 0) {
            ChooseSuggestion(sug);
            return;
        }
    }

    m_pressed = HitTestPart(pt);
    if (m_pressed == HitPart::Clear) {
        SetTextInternal("", true, true);
        CloseSuggestions();
        m_pressed = HitPart::None;
        return;
    }

    if (m_pressed == HitPart::Text) {
        // Click-to-caret: approximate by measuring prefixes.
        GraphicsContext ctx;
        const Rect textRect = TextRect();
        const std::wstring w = Utf8ToUtf16(GetText());
        const float localX = pt.x - (textRect.x + 2.0f) + m_textScrollX;
        int best = 0;
        float bestDist = 1.0e9f;
        for (int i = 0; i <= static_cast<int>(w.length()); ++i) {
            const float x = ctx.MeasureText(
                Utf16ToUtf8(w.substr(0, static_cast<size_t>(i))),
                GetFontFamily(), GetFontSize(), DWRITE_FONT_WEIGHT_NORMAL).width;
            const float d = std::abs(x - localX);
            if (d < bestDist) {
                bestDist = d;
                best = i;
            }
        }
        SetCaret(best);
        if (!m_filtered.empty() && !GetText().empty()) {
            OpenSuggestions();
        }
    }
}

void AutoSuggestBox::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);
    const bool clearHot = !GetText().empty() && ClearRect().Contains(pt.x, pt.y);
    if (clearHot != m_clearHovered) {
        m_clearHovered = clearHot;
        MarkRenderContentDirty();
    }

    if (m_keyboardNavActive) {
        const float dx = pt.x - m_keyboardNavMousePt.x;
        const float dy = pt.y - m_keyboardNavMousePt.y;
        if ((dx * dx + dy * dy) < 4.0f) {
            // Spurious MOVE after scroll / key — keep keyboard highlight.
            return;
        }
        m_keyboardNavActive = false;
    }
    m_keyboardNavMousePt = pt;

    if (m_suggestionsOpen || PopupProgress() > 0.5f) {
        const int sug = HitTestSuggestionIndex(pt);
        if (sug >= 0 && sug != m_highlightedIndex) {
            m_highlightedIndex = sug;
            MarkRenderRectDirty(GetPopupBounds().Inflate(2.0f));
        }
    }
}

void AutoSuggestBox::OnMouseLeave() {
    Control::OnMouseLeave();
    if (m_clearHovered) {
        m_clearHovered = false;
        MarkRenderContentDirty();
    }
    m_pressed = HitPart::None;
}

void AutoSuggestBox::OnMouseWheel(float delta) {
    if (!m_suggestionsOpen || m_filtered.empty()) {
        return;
    }
    const float maxScroll = SuggestionMaxScroll();
    if (maxScroll <= 0.001f) {
        return;
    }
    m_suggestScroll = std::clamp(m_suggestScroll - delta * m_suggestionItemHeight, 0.0f, maxScroll);
    m_scrollbarAutoHide.NotifyActivity(this);
    RequestAnimationTicks();
    MarkRenderRectDirty(GetPopupBounds().Inflate(2.0f));
}

void AutoSuggestBox::OnKeyDown(int vkCode) {
    const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

    if (vkCode == VK_ESCAPE) {
        if (m_suggestionsOpen) {
            CloseSuggestions();
        }
        return;
    }

    if (vkCode == VK_RETURN) {
        if (m_suggestionsOpen && m_highlightedIndex >= 0) {
            ChooseSuggestion(m_highlightedIndex);
        } else {
            SubmitQuery();
        }
        return;
    }

    if (vkCode == VK_DOWN) {
        if (!m_suggestionsOpen) {
            RefreshSuggestionsNow();
            if (m_filtered.empty() && !m_catalog.empty() && GetText().empty()) {
                m_filtered = m_catalog;
                m_highlightedIndex = 0;
                OpenSuggestions();
            }
            BeginKeyboardNavigation();
        } else {
            MoveHighlightBy(+1);
        }
        return;
    }

    if (vkCode == VK_UP) {
        if (m_suggestionsOpen) {
            MoveHighlightBy(-1);
        }
        return;
    }

    if (vkCode == VK_LEFT || vkCode == VK_RIGHT || vkCode == VK_HOME || vkCode == VK_END) {
        const int len = static_cast<int>(Utf8ToUtf16(GetText()).length());
        int next = m_caret;
        if (vkCode == VK_LEFT) {
            next = (std::max)(0, m_caret - 1);
        } else if (vkCode == VK_RIGHT) {
            next = (std::min)(len, m_caret + 1);
        } else if (vkCode == VK_HOME) {
            next = 0;
        } else {
            next = len;
        }
        if (shift) {
            if (m_selAnchor < 0) {
                m_selAnchor = m_caret;
            }
            m_caret = next;
            EnsureCaretVisible();
            ResetCaretBlink();
            MarkRenderContentDirty();
        } else {
            SetCaret(next);
        }
        return;
    }

    if (vkCode == VK_BACK) {
        DeleteSelectionOrBackward();
        return;
    }
    if (vkCode == VK_DELETE) {
        DeleteForward();
        return;
    }

    if (ctrl && (vkCode == 'A')) {
        const int len = static_cast<int>(Utf8ToUtf16(GetText()).length());
        m_selAnchor = 0;
        m_caret = len;
        MarkRenderContentDirty();
        return;
    }
    if (ctrl && (vkCode == 'C' || vkCode == 'X')) {
        if (m_selAnchor >= 0 && m_selAnchor != m_caret) {
            const std::wstring w = Utf8ToUtf16(GetText());
            const int a = (std::min)(m_caret, m_selAnchor);
            const int b = (std::max)(m_caret, m_selAnchor);
            const std::wstring slice = w.substr(static_cast<size_t>(a), static_cast<size_t>(b - a));
            if (OpenClipboard(nullptr)) {
                EmptyClipboard();
                const size_t bytes = (slice.size() + 1) * sizeof(wchar_t);
                HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes);
                if (mem) {
                    void* ptr = GlobalLock(mem);
                    if (ptr) {
                        memcpy(ptr, slice.c_str(), bytes);
                        GlobalUnlock(mem);
                        SetClipboardData(CF_UNICODETEXT, mem);
                    }
                }
                CloseClipboard();
            }
            if (vkCode == 'X') {
                DeleteSelectionOrBackward();
            }
        }
        return;
    }
    if (ctrl && vkCode == 'V') {
        if (OpenClipboard(nullptr)) {
            HANDLE data = GetClipboardData(CF_UNICODETEXT);
            if (data) {
                const wchar_t* text = static_cast<const wchar_t*>(GlobalLock(data));
                if (text) {
                    std::wstring clip(text);
                    GlobalUnlock(data);
                    // Single-line: strip newlines.
                    clip.erase(std::remove(clip.begin(), clip.end(), L'\r'), clip.end());
                    clip.erase(std::remove(clip.begin(), clip.end(), L'\n'), clip.end());
                    InsertUtf16(clip);
                }
            }
            CloseClipboard();
        }
        return;
    }
}

void AutoSuggestBox::OnCharInput(wchar_t ch) {
    if (ch < 32 && ch != L'\t') {
        return;
    }
    if (ch == L'\t') {
        return;
    }
    InsertUtf16(std::wstring(1, ch));
}

void AutoSuggestBox::OnFocus() {
    Control::OnFocus();
    ResetCaretBlink();
    MarkRenderContentDirty();
}

void AutoSuggestBox::OnBlur() {
    Control::OnBlur();
    CloseSuggestions();
    m_selAnchor = -1;
    MarkRenderContentDirty();
}

bool AutoSuggestBox::OnAnimationTick() {
    const float dt = UIElement::GetAnimationDeltaSeconds();
    bool animating = Control::OnAnimationTick();

    if (m_isFocused) {
        m_caretBlink += dt;
        if (m_caretBlink >= 0.53f) {
            m_caretBlink = 0.0f;
            m_caretVisible = !m_caretVisible;
            MarkRenderRectDirty(TextRect().Inflate(2.0f));
            animating = true;
        }
    }

    if (m_debounceLeft >= 0.0f) {
        m_debounceLeft -= dt;
        if (m_debounceLeft <= 0.0f) {
            RefreshSuggestionsNow();
        }
        animating = true;
    }

    m_popupAnim.SetTarget(m_suggestionsOpen ? 1.0f : 0.0f);
    AnimationSpec popupSpec{ 0.55f, 0.01f };
    if (m_popupAnim.Tick(dt, popupSpec)) {
        animating = true;
        MarkRenderRectDirty(GetPopupBounds().Inflate(6.0f));
    }

    if (m_scrollbarAutoHide.Tick(dt)) {
        animating = true;
        MarkRenderRectDirty(GetPopupBounds().Inflate(2.0f));
    }

    if (animating) {
        RequestAnimationTicks();
    }
    return animating;
}

bool AutoSuggestBox::HasSelfAnimation() const {
    return Control::HasSelfAnimation()
        || m_isFocused
        || m_debounceLeft >= 0.0f
        || std::abs(m_popupAnim.Target() - m_popupAnim.Current()) > 0.001f
        || m_scrollbarAutoHide.NeedsTicks();
}

void AutoSuggestBox::CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const {
    if (!HasSelfAnimation() || m_bounds.IsEmpty()) {
        return;
    }
    Rect area = m_bounds.Inflate(4.0f);
    if (m_suggestionsOpen || m_popupAnim.Current() > 0.001f) {
        area = area.Union(GetPopupBounds().Inflate(6.0f));
    }
    dirtyRect = hasDirty ? dirtyRect.Union(area) : area;
    hasDirty = true;
}

} // namespace CUI
