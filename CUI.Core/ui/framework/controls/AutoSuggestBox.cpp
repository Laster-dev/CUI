#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "AutoSuggestBox.h"
#include "TextBox.h"
#include "../core/Value.h"
#include "../style/ThemeManager.h"
#include "../window/PopupPlacement.h"
#include "../window/Dpi.h"
#include <algorithm>
#include <cmath>

namespace CUI {

class AutoSuggestField : public TextBox {
public:
    AutoSuggestBox* host = nullptr;

    void OnRoutedEvent(RoutedEventArgs& args) override {
        if (args.handled) {
            return;
        }
        if (args.type == RoutedEventType::KeyDown && host
            && host->HandleSuggestionKey(args.keyCode)) {
            args.handled = true;
            return;
        }
        TextBox::OnRoutedEvent(args);
    }

    void OnFocus() override {
        TextBox::OnFocus();
        if (host) {
            host->NotifyFieldFocus();
        }
    }

    void OnBlur() override {
        TextBox::OnBlur();
        if (host) {
            host->NotifyFieldBlur();
        }
    }
};

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

} // namespace

AutoSuggestBox::AutoSuggestBox() {
    SetPlaceholder("搜索…");
    SetBackgroundToken(ThemeTokenId::Unset);
    SetHoverBackgroundToken(ThemeTokenId::Unset);
    SetBorderToken(ThemeTokenId::Unset);
    SetFocusedBorderToken(ThemeTokenId::Unset);
    SetColorToken(ThemeTokenId::TextPrimary);
    SetPlaceholderColorToken(ThemeTokenId::TextMuted);
    SetBackground(D2D1::ColorF(0, 0, 0, 0));
    SetHoverBackground(D2D1::ColorF(0, 0, 0, 0));
    SetBorderBrush(D2D1::ColorF(0, 0, 0, 0));
    SetBorderThickness(0.0f);
    SetColor(ThemeManager::Instance().GetColor(ThemeTokenId::TextPrimary));
    SetFontFamily("微软雅黑");
    SetFontSize(12.0f);
    SetPadding(Thickness(0, 0, 0, 0));
    SetCornerRadius(0.0f);
    SetWidth(280.0f);
    SetHeight(48.0f);
    UIElement::SetText("");

    auto field = std::make_shared<AutoSuggestField>();
    field->host = this;
    m_field = field;
    StyleField();
    m_field->OnTextChanged().Connect([this](TextBox*, const std::string& text) {
        if (m_syncingField) {
            return;
        }
        SetTextInternal(text, true, true);
    });
    AddChild(m_field);
}

void AutoSuggestBox::StyleField() {
    if (!m_field) {
        return;
    }
    m_field->SetPlaceholder(GetPlaceholder());
    m_field->SetFontFamily(GetFontFamily());
    m_field->SetFontSize(GetFontSize());
    m_field->SetColorToken(GetColorToken());
    m_field->SetPlaceholderColorToken(GetPlaceholderColorToken());
    m_field->SetWidth(GetWidth() >= 0.0f ? GetWidth() : 280.0f);
    m_field->SetHeight(GetHeight() >= 0.0f ? GetHeight() : 48.0f);
    m_field->SetPadding(Thickness(8.0f, 18.0f, 8.0f, 8.0f));
}

HCURSOR AutoSuggestBox::GetCursor() const {
    return nullptr;
}

void AutoSuggestBox::SetPlaceholder(const std::string& text) {
    UIElement::SetPlaceholder(text);
    if (m_field) {
        m_field->SetPlaceholder(text);
    }
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
    if (m_field && m_field->GetText() != text) {
        m_syncingField = true;
        m_field->SetText(text);
        m_syncingField = false;
    }
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

void AutoSuggestBox::SetSuggestionProvider(SuggestionProviderFn provider) {
    m_provider = std::move(provider);
    ScheduleSuggestRefresh();
}

Size AutoSuggestBox::Measure(Size availableSize) {
    (void)availableSize;
    const float w = GetWidth() >= 0.0f ? GetWidth() : 280.0f;
    const float h = GetHeight() >= 0.0f ? GetHeight() : 48.0f;
    m_desiredSize = Size(w, h);
    return m_desiredSize;
}

void AutoSuggestBox::Arrange(Rect finalRect) {
    if (GetVisibility() == Visibility::Collapsed) {
        SetBounds(Rect());
        m_arrangeDirty = false;
        return;
    }
    const Thickness margin = GetMargin();
    Rect arranged(
        finalRect.x + margin.left,
        finalRect.y + margin.top,
        (std::max)(0.0f, finalRect.width - margin.left - margin.right),
        (std::max)(0.0f, finalRect.height - margin.top - margin.bottom));
    SetBounds(arranged);
    LayoutField();
    m_arrangeDirty = false;
}

void AutoSuggestBox::LayoutField() {
    if (!m_field) {
        return;
    }
    if (m_field->GetFontSize() != GetFontSize()) {
        m_field->SetFontSize(GetFontSize());
    }
    if (m_field->GetFontFamily() != GetFontFamily()) {
        m_field->SetFontFamily(GetFontFamily());
    }
    const Rect r = m_bounds;
    m_field->Measure(Size(r.width, r.height));
    m_field->Arrange(r);
}

bool AutoSuggestBox::InputActive() const {
    return IsFocused() || (m_field && m_field->IsFocused());
}

void AutoSuggestBox::NotifyFieldFocus() {
    m_pendingClose = false;
    MarkRenderContentDirty();
}

void AutoSuggestBox::NotifyFieldBlur() {
    m_pendingClose = true;
    RequestAnimationTicks();
}

void AutoSuggestBox::FlushPendingClose() {
    if (!m_pendingClose) {
        return;
    }
    m_pendingClose = false;
    if (!InputActive()) {
        CloseSuggestions();
    }
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

    if (m_filtered.empty() || !InputActive()) {
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
    if (PopupHost* host = PopupHost::Current()) {
        if (::HWND hwnd = host->GetOwnerHwnd()) {
            ::POINT sp{};
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
    if (progress <= 0.2f || m_filtered.empty()) {
        return -1;
    }
    const Rect menu = GetPopupBounds();
    if (!menu.Contains(pt.x, pt.y)) {
        return -1;
    }
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
    if (m_field) {
        if (UIElement* hit = m_field->HitTest(x, y)) {
            return hit;
        }
    }
    if (m_bounds.Contains(x, y)) {
        if (m_field) {
            return m_field.get();
        }
        return this;
    }
    return nullptr;
}

UIElement* AutoSuggestBox::HitTestOverlay(float x, float y) {
    const float progress = PopupProgress();
    if (progress <= 0.2f || m_filtered.empty()) {
        return nullptr;
    }
    const Rect menu = GetPopupBounds();
    if (menu.Contains(x, y)) {
        return this;
    }
    return nullptr;
}

void AutoSuggestBox::OnRender(GraphicsContext& ctx) {
    (void)ctx;
}

void AutoSuggestBox::RenderPopup(GraphicsContext& ctx) {
    const float progress = PopupProgress();
    if (progress <= 0.001f || m_filtered.empty()) {
        return;
    }

    const Rect menu = GetPopupBounds();
    ctx.PushPopupReveal(menu, progress, Point(menu.x + menu.width * 0.5f, menu.y));

    const auto& tokens = ThemeManager::Instance().GetTokens();
    constexpr float radius = 4.0f;
    ctx.FillRoundedRect(menu, radius, tokens.cardBackground);
    ctx.DrawRoundedRect(menu, radius, tokens.cardBorder, 1.25f);

    const float itemH = m_suggestionItemHeight;
    for (int i = 0; i < static_cast<int>(m_filtered.size()); ++i) {
        const float y = menu.y + 2.0f + static_cast<float>(i) * itemH - m_suggestScroll;
        if (y + itemH < menu.y || y > menu.y + menu.height) {
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
        Rect track(menu.x + menu.width - kScrollW, menu.y, kScrollW, menu.height);
        const float vis = m_scrollbarAutoHide.Opacity();
        D2D1_COLOR_F trackColor = tokens.cardBorder;
        trackColor.a = 0.35f * vis;
        ctx.DrawRoundedRect(track, 4.0f, trackColor, 1.0f);
        const float contentH = SuggestionContentHeight();
        const float thumbH = (std::max)(16.0f, (menu.height * menu.height) / contentH);
        const float travel = (std::max)(0.0f, menu.height - thumbH);
        const float thumbY = track.y + (m_suggestScroll / maxScroll) * travel;
        D2D1_COLOR_F thumb = tokens.accentColor;
        thumb.a = 0.45f * vis;
        ctx.FillRoundedRect(Rect(track.x + 1.5f, thumbY, kScrollW - 3.0f, thumbH), 4.0f, thumb);
    }

    ctx.PopPopupReveal();
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
        }
    }
}

void AutoSuggestBox::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);

    if (m_keyboardNavActive) {
        const float dx = pt.x - m_keyboardNavMousePt.x;
        const float dy = pt.y - m_keyboardNavMousePt.y;
        if ((dx * dx + dy * dy) < 4.0f) {
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

bool AutoSuggestBox::HandleSuggestionKey(int vkCode) {
    if (vkCode == VK_ESCAPE) {
        if (m_suggestionsOpen) {
            CloseSuggestions();
            return true;
        }
        return false;
    }

    if (vkCode == VK_RETURN) {
        if (m_suggestionsOpen && m_highlightedIndex >= 0) {
            ChooseSuggestion(m_highlightedIndex);
        } else {
            SubmitQuery();
        }
        return true;
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
        return true;
    }

    if (vkCode == VK_UP) {
        if (m_suggestionsOpen) {
            MoveHighlightBy(-1);
            return true;
        }
        return false;
    }

    return false;
}

bool AutoSuggestBox::OnKeyDown(int vkCode) {
    if (m_field && m_field->IsFocused()) {
        return false;
    }
    if (HandleSuggestionKey(vkCode)) {
        return true;
    }
    if (m_field) {
        return m_field->OnKeyDown(vkCode);
    }
    return false;
}

void AutoSuggestBox::OnCharInput(wchar_t ch) {
    if (m_field) {
        m_field->OnCharInput(ch);
    }
}

void AutoSuggestBox::OnFocus() {
    Control::OnFocus();
    m_pendingClose = false;
    MarkRenderContentDirty();
}

void AutoSuggestBox::OnBlur() {
    Control::OnBlur();
    if (!m_field || !m_field->IsFocused()) {
        CloseSuggestions();
    }
    MarkRenderContentDirty();
}

bool AutoSuggestBox::OnAnimationTick() {
    const float dt = UIElement::GetAnimationDeltaSeconds();
    bool animating = Control::OnAnimationTick();

    FlushPendingClose();

    if (m_debounceLeft >= 0.0f) {
        m_debounceLeft -= dt;
        if (m_debounceLeft <= 0.0f) {
            RefreshSuggestionsNow();
        }
        animating = true;
    }

    m_popupAnim.SetTarget(m_suggestionsOpen ? 1.0f : 0.0f);
    const AnimationSpec popupSpec = PopupReveal::kSpec;
    if (m_popupAnim.Tick(dt, popupSpec)) {
        animating = true;
        MarkRenderRectDirty(GetPopupBounds().Inflate(6.0f));
    }

    if (m_scrollbarAutoHide.Tick(dt)) {
        animating = true;
        MarkRenderRectDirty(GetPopupBounds().Inflate(2.0f));
    }

    if (animating || m_pendingClose) {
        RequestAnimationTicks();
    }
    return animating;
}

bool AutoSuggestBox::HasSelfAnimation() const {
    return Control::HasSelfAnimation()
        || m_pendingClose
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
