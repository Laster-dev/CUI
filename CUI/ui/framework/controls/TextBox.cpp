#define NOMINMAX
#include "TextBox.h"
#include "ContextMenu.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cmath>
#include <cwctype>

namespace CUI {

std::vector<PropertyMeta> TextBox::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "fontFamily", "字体名称 (FontFamily)", "字体文本", "enum", { "Segoe UI", "Consolas", "微软雅黑", "Times New Roman" } });
    metas.push_back({ "fontSize", "字体大小 (FontSize)", "字体文本", "number" });
    metas.push_back({ "fontWeight", "字体粗细 (FontWeight)", "字体文本", "enum", { "Normal", "Bold", "Light" } });
    metas.push_back({ "lineSpacing", "行间距 (LineSpacing)", "高级排版", "number" });
    metas.push_back({ "lineHeight", "固定行高 (LineHeight)", "高级排版", "number" });
    metas.push_back({ "placeholder", "占位提示词 (Placeholder)", "输入控制", "string" });
    metas.push_back({ "caretWidth", "光标宽度 (CaretWidth)", "光标排版", "number" });
    metas.push_back({ "caretBlinkRate", "光标闪烁频率 (BlinkMs)", "光标排版", "number" });
    metas.push_back({ "TextWrapping", "自动换行 (TextWrapping)", "输入控制", "enum", { "NoWrap", "Wrap" } });
    metas.push_back({ "AcceptsReturn", "允许回车 (AcceptsReturn)", "输入控制", "bool" });
    metas.push_back({ "isReadOnly", "只读 (IsReadOnly)", "输入控制", "bool" });
    return metas;
}

namespace {

bool GetBoolProperty(const Control* control, const char* primary, const char* alternate, bool def) {
    std::string primaryVal = control->GetProperty(primary).AsString("");
    if (!primaryVal.empty()) return control->GetProperty(primary).AsBool(def);
    return control->GetProperty(alternate).AsBool(def);
}

std::string GetStringProperty(const Control* control, const char* primary, const char* alternate, const std::string& def) {
    std::string primaryVal = control->GetProperty(primary).AsString("");
    if (!primaryVal.empty()) return primaryVal;
    std::string alternateVal = control->GetProperty(alternate).AsString("");
    if (!alternateVal.empty()) return alternateVal;
    return def;
}

float GetFloatProperty(const Control* control, const char* primary, const char* alternate, float def) {
    if (control->HasProperty(primary)) return control->GetProperty(primary).AsFloat(def);
    if (control->HasProperty(alternate)) return control->GetProperty(alternate).AsFloat(def);
    return def;
}

std::wstring BuildDisplayText(const std::wstring& wtext, int cursorPos, const std::wstring& compString) {
    if (compString.empty()) return wtext;
    int safePos = std::clamp(cursorPos, 0, static_cast<int>(wtext.length()));
    return wtext.substr(0, safePos) + compString + wtext.substr(safePos);
}

} // namespace

TextBox::TextBox() {
    SetProperty("text", Value(""));
    SetProperty("placeholder", Value(""));
    SetProperty("background", Value(D2D1::ColorF(0, 0, 0, 0)));
    SetProperty("hoverBackground", Value(D2D1::ColorF(0, 0, 0, 0)));
    SetProperty("borderBrush", Value(D2D1::ColorF(0, 0, 0, 0)));
    SetProperty("focusedBorderBrush", Value(D2D1::ColorF(0, 0, 0, 0)));
    SetProperty("borderThickness", Value(0.0f));
    SetProperty("theme.underlineColorToken", Value("inputBorder"));
    SetProperty("theme.activeUnderlineColorToken", Value("accentColor"));
    SetProperty("theme.caretColorToken", Value("accentColor"));
    SetProperty("theme.colorToken", Value("textPrimary"));
    SetProperty("theme.placeholderColorToken", Value("textMuted"));
    SetProperty("underlineColor", Value(ThemeManager::Instance().GetColor("inputBorder")));
    SetProperty("activeUnderlineColor", Value(ThemeManager::Instance().GetColor("accentColor")));
    SetProperty("color", Value(ThemeManager::Instance().GetColor("textPrimary")));
    SetProperty("placeholderColor", Value(ThemeManager::Instance().GetColor("textMuted")));
    SetProperty("fontFamily", Value("Segoe UI"));
    SetProperty("fontSize", Value(13.0f));
    SetProperty("lineSpacing", Value(1.0f));
    SetProperty("lineHeight", Value(0.0f));
    SetProperty("padding", Value(Thickness(0, 18, 0, 8)));
    SetProperty("width", Value(260.0f));
    SetProperty("height", Value(48.0f));
    SetProperty("AcceptsReturn", Value(false));
    SetProperty("TextWrapping", Value("NoWrap"));
    SetProperty("isReadOnly", Value(false));
}

TextBox::TextBox(const std::string& placeholder) : TextBox() {
    SetProperty("placeholder", Value(placeholder));
}

void TextBox::SetCompositionString(const std::wstring& compStr) {
    if (m_compString == compStr) {
        return;
    }
    m_compString = compStr;
    m_textLayoutCache.Clear();
    MarkRenderContentDirty();
}

void TextBox::SetText(const std::string& text) {
    if (GetText() != text) {
        SetProperty("text", Value(text));
        m_textLayoutCache.Clear();
        m_onTextChangedEvent.Invoke(this, text);
    }
}

bool TextBox::GetAcceptsReturn() const {
    return GetBoolProperty(this, "AcceptsReturn", "acceptsReturn", false);
}

bool TextBox::IsTextWrapping() const {
    std::string wrap = GetStringProperty(this, "TextWrapping", "textWrapping", "NoWrap");
    std::transform(wrap.begin(), wrap.end(), wrap.begin(), ::tolower);
    return wrap == "wrap";
}

bool TextBox::IsMultiline() const {
    return GetAcceptsReturn() || IsTextWrapping();
}

std::wstring TextBox::GetDisplayedText() const {
    std::string text = GetText();
    std::wstring wtext = Utf8ToUtf16(text);
    if (IsPasswordMode() && !m_isPasswordRevealed) {
        return std::wstring(wtext.length(), GetPasswordChar());
    }
    return wtext;
}

Rect TextBox::GetRevealButtonRect() const {
    float btnSize = 24.0f;
    float btnX = m_bounds.x + m_bounds.width - btnSize - 6.0f;
    float btnY = m_bounds.y + (m_bounds.height - btnSize) * 0.5f;
    return Rect(btnX, btnY, btnSize, btnSize);
}

Rect TextBox::GetTextRect() const {
    std::string placeholder = GetProperty("placeholder").AsString("");
    bool hasFloatingLabel = !placeholder.empty();
    Thickness padding = hasFloatingLabel
        ? GetProperty("padding").AsThickness(Thickness(0, 18, 0, 8))
        : Thickness(0, 6, 0, 6);
    float extraTop = hasFloatingLabel ? ((1.0f - m_labelAnim.Current()) * 8.0f) : 0.0f;
    float rightMargin = (IsPasswordMode() && GetShowRevealButton()) ? 32.0f : 0.0f;
    return Rect(
        m_bounds.x + padding.left,
        m_bounds.y + padding.top + extraTop,
        (std::max)(0.0f, m_bounds.width - padding.left - padding.right - rightMargin),
        m_bounds.height - padding.top - padding.bottom - extraTop - 2.0f
    );
}

Point TextBox::GetLayoutOrigin(const Rect& textRect) const {
    return Point(textRect.x - m_scrollOffsetX, textRect.y - m_scrollOffsetY);
}

float TextBox::GetContentWidth(GraphicsContext& ctx, const std::wstring& wtext, const Rect& textRect) const {
    auto layout = BuildTextLayout(ctx, wtext, textRect);
    if (!layout) return 0.0f;

    DWRITE_TEXT_METRICS metrics = {};
    layout->GetMetrics(&metrics);
    return metrics.width;
}

void TextBox::ClampScrollOffsets(GraphicsContext& ctx, const std::wstring& wtext, const Rect& textRect) {
    if (IsMultiline()) {
        m_scrollOffsetX = 0.0f;
        Rect layoutRect = textRect;
        layoutRect.height = 100000.0f;
        auto layout = BuildTextLayout(ctx, wtext, layoutRect);
        if (layout) {
            DWRITE_TEXT_METRICS metrics = {};
            layout->GetMetrics(&metrics);
            float maxScrollY = std::max(0.0f, metrics.height - textRect.height);
            m_scrollOffsetY = std::clamp(m_scrollOffsetY, 0.0f, maxScrollY);
        }
    } else {
        m_scrollOffsetY = 0.0f;
        float contentWidth = GetContentWidth(ctx, wtext, textRect);
        float maxScrollX = std::max(0.0f, contentWidth - textRect.width);
        m_scrollOffsetX = std::clamp(m_scrollOffsetX, 0.0f, maxScrollX);
    }
}

Microsoft::WRL::ComPtr<IDWriteTextLayout> TextBox::BuildTextLayout(GraphicsContext& ctx, const std::wstring& wtext,
                                                                     const Rect& textRect) const {
    (void)ctx;
    GraphicsContext::TextLayoutOptions options;
    options.maxWidth = IsTextWrapping() ? textRect.width : 100000.0f;
    options.maxHeight = IsMultiline() ? 100000.0f : textRect.height;
    options.wrapping = IsTextWrapping() ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP;
    options.paragraphAlignment = IsMultiline()
        ? DWRITE_PARAGRAPH_ALIGNMENT_NEAR
        : DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
    options.lineSpacing = GetFloatProperty(this, "lineSpacing", "LineSpacing", 1.0f);
    options.lineHeight = GetFloatProperty(this, "lineHeight", "LineHeight", 0.0f);

    float fontSize = GetFloatProperty(this, "fontSize", "FontSize", 13.0f);
    std::string fontFamily = GetStringProperty(this, "fontFamily", "FontFamily", "Segoe UI");
    TextLayoutCache::Key key;
    key.text = wtext;
    key.fontName = fontFamily;
    key.fontSize = fontSize;
    key.maxWidth = options.maxWidth;
    key.maxHeight = options.maxHeight;
    key.wrapping = options.wrapping;
    key.paragraphAlignment = options.paragraphAlignment;
    key.lineSpacing = options.lineSpacing;
    key.lineHeight = options.lineHeight;
    return m_textLayoutCache.GetOrCreate(key);
}

int TextBox::GetCaretIndexFromPoint(GraphicsContext& ctx, float x, float y) {
    std::wstring wtext = BuildDisplayText(
        GetDisplayedText(),
        m_cursorPos,
        m_compString
    );

    Rect textRect = GetTextRect();
    auto layout = BuildTextLayout(ctx, wtext, textRect);
    if (!layout) return 0;

    Point origin = GetLayoutOrigin(textRect);
    return static_cast<int>(ctx.HitTestTextLayout(layout.Get(), x, y, origin));
}

GraphicsContext::TextCaretInfo TextBox::GetCaretScreenPos(GraphicsContext& ctx, int caretPos) {
    std::wstring wtext = BuildDisplayText(
        GetDisplayedText(),
        m_cursorPos,
        m_compString
    );

    Rect textRect = GetTextRect();
    auto layout = BuildTextLayout(ctx, wtext, textRect);
    if (!layout) return {};

    int displayPos = caretPos;
    if (!m_compString.empty() && caretPos >= m_cursorPos) {
        displayPos = caretPos + static_cast<int>(m_compString.length());
    }

    displayPos = std::clamp(displayPos, 0, static_cast<int>(wtext.length()));
    return ctx.GetTextCaretInfo(layout.Get(), static_cast<UINT32>(displayPos), GetLayoutOrigin(textRect));
}

void TextBox::EnsureCaretVisible(GraphicsContext& ctx) {
    Rect textRect = GetTextRect();
    std::wstring wtext = BuildDisplayText(
        GetDisplayedText(),
        m_cursorPos,
        m_compString
    );

    int visiblePos = m_cursorPos;
    if (!m_compString.empty()) {
        visiblePos += static_cast<int>(m_compString.length());
    }

    auto layout = BuildTextLayout(ctx, wtext, textRect);
    if (!layout) return;

    Point origin = GetLayoutOrigin(textRect);
    auto caret = ctx.GetTextCaretInfo(layout.Get(), static_cast<UINT32>(visiblePos), origin);

    if (IsMultiline()) {
        if (caret.y < textRect.y) {
            m_scrollOffsetY -= (textRect.y - caret.y);
        } else if (caret.y + caret.height > textRect.y + textRect.height) {
            m_scrollOffsetY += (caret.y + caret.height) - (textRect.y + textRect.height);
        }
    } else {
        const float padding = 2.0f;
        if (caret.x < textRect.x + padding) {
            m_scrollOffsetX -= (textRect.x + padding - caret.x);
        } else if (caret.x > textRect.x + textRect.width - padding) {
            m_scrollOffsetX += caret.x - (textRect.x + textRect.width - padding);
        }
    }

    ClampScrollOffsets(ctx, wtext, textRect);
}

void TextBox::InsertText(const std::wstring& text) {
    if (IsReadOnly() || !IsEnabled()) {
        return;
    }
    if (text.empty()) return;

    PushUndoState();

    std::wstring wtext = Utf8ToUtf16(GetProperty("text").AsString());
    if (HasSelection()) {
        int selMin = (std::min)(m_selectionStart, m_selectionEnd);
        int selMax = (std::max)(m_selectionStart, m_selectionEnd);
        if (selMin >= 0 && selMax <= static_cast<int>(wtext.length()) && selMax > selMin) {
            wtext.erase(selMin, selMax - selMin);
            m_cursorPos = selMin;
        }
    }

    if (m_cursorPos > static_cast<int>(wtext.length())) {
        m_cursorPos = static_cast<int>(wtext.length());
    }

    wtext.insert(m_cursorPos, text);
    m_cursorPos += static_cast<int>(text.size());
    m_selectionStart = m_cursorPos;
    m_selectionEnd = m_cursorPos;
    SetText(Utf16ToUtf8(wtext));

    GraphicsContext ctx;
    EnsureCaretVisible(ctx);
}

void TextBox::PushUndoState() {
    if (m_undoing) return;

    TextBoxUndoState state;
    state.text = GetProperty("text").AsString();
    state.cursorPos = m_cursorPos;
    state.selectionStart = m_selectionStart;
    state.selectionEnd = m_selectionEnd;
    m_undoStack.push_back(state);
    if (m_undoStack.size() > 200) {
        m_undoStack.erase(m_undoStack.begin());
    }
    m_redoStack.clear();
}

HCURSOR TextBox::GetCursor() const {
    if (!IsEnabled()) return nullptr;
    if (IsPasswordMode() && GetShowRevealButton()) {
        Rect btnRect = GetRevealButtonRect();
        if (btnRect.Contains(m_lastMousePos.x, m_lastMousePos.y)) {
            return LoadCursor(nullptr, IDC_HAND);
        }
    }
    return LoadCursor(nullptr, IDC_IBEAM);
}

void TextBox::Undo() {
    if (IsReadOnly() || !IsEnabled()) return;
    if (m_undoStack.empty()) return;

    TextBoxUndoState current;
    current.text = GetProperty("text").AsString();
    current.cursorPos = m_cursorPos;
    current.selectionStart = m_selectionStart;
    current.selectionEnd = m_selectionEnd;
    m_redoStack.push_back(current);

    TextBoxUndoState prev = m_undoStack.back();
    m_undoStack.pop_back();

    m_undoing = true;
    SetProperty("text", Value(prev.text));
    m_undoing = false;
    m_cursorPos = prev.cursorPos;
    m_selectionStart = prev.selectionStart;
    m_selectionEnd = prev.selectionEnd;
}

void TextBox::Redo() {
    if (IsReadOnly() || !IsEnabled()) return;
    if (m_redoStack.empty()) return;

    TextBoxUndoState current;
    current.text = GetProperty("text").AsString();
    current.cursorPos = m_cursorPos;
    current.selectionStart = m_selectionStart;
    current.selectionEnd = m_selectionEnd;
    m_undoStack.push_back(current);

    TextBoxUndoState next = m_redoStack.back();
    m_redoStack.pop_back();

    m_undoing = true;
    SetProperty("text", Value(next.text));
    m_undoing = false;
    m_cursorPos = next.cursorPos;
    m_selectionStart = next.selectionStart;
    m_selectionEnd = next.selectionEnd;
}

Size TextBox::Measure(Size availableSize) {
    float expW = GetProperty("width").AsFloat(260.0f);
    float expH = GetProperty("height").AsFloat(48.0f);
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

bool TextBox::OnAnimationTick() {
    bool base = Control::OnAnimationTick();
    bool hasFloatingLabel = !GetProperty("placeholder").AsString("").empty();
    bool shouldFloat = hasFloatingLabel && (m_isFocused || !GetProperty("text").AsString("").empty() || !m_compString.empty());
    float target = shouldFloat ? 1.0f : 0.0f;
    float focusTarget = m_isFocused ? 1.0f : 0.0f;
    m_labelAnim.SetTarget(target);
    m_focusLineAnim.SetTarget(focusTarget);

    bool animating = m_labelAnim.Tick(UIElement::GetAnimationDeltaSeconds(), AnimationSpec{ 0.22f, 0.01f });
    animating = m_focusLineAnim.Tick(UIElement::GetAnimationDeltaSeconds(), AnimationSpec{ 0.18f, 0.01f }) || animating;
    // Keep the animation pump alive while focused so caret blink uses dirty regions
    // instead of a full-window timer repaint.
    const bool caretBlink = m_isFocused && IsEnabled();
    return base || animating || caretBlink;
}

bool TextBox::HasSelfAnimation() const {
    bool hasFloatingLabel = !GetProperty("placeholder").AsString("").empty();
    bool shouldFloat = hasFloatingLabel && (m_isFocused || !GetProperty("text").AsString("").empty() || !m_compString.empty());
    float labelTarget = shouldFloat ? 1.0f : 0.0f;
    float focusTarget = m_isFocused ? 1.0f : 0.0f;
    return Control::HasSelfAnimation()
        || std::abs(labelTarget - m_labelAnim.Current()) > 0.01f
        || std::abs(focusTarget - m_focusLineAnim.Current()) > 0.01f
        || (m_isFocused && IsEnabled());
}

void TextBox::SelectAll() {
    std::wstring wtext = Utf8ToUtf16(GetProperty("text").AsString());
    m_selectionStart = 0;
    m_selectionEnd = static_cast<int>(wtext.length());
    m_cursorPos = m_selectionEnd;
}

void TextBox::DeleteSelection() {
    if (IsReadOnly() || !IsEnabled()) {
        return;
    }
    if (!HasSelection()) return;

    std::wstring wtext = Utf8ToUtf16(GetProperty("text").AsString());
    int selMin = (std::min)(m_selectionStart, m_selectionEnd);
    int selMax = (std::max)(m_selectionStart, m_selectionEnd);

    if (selMin >= 0 && selMax <= static_cast<int>(wtext.length()) && selMax > selMin) {
        PushUndoState();
        wtext.erase(selMin, selMax - selMin);
        m_cursorPos = selMin;
        m_selectionStart = selMin;
        m_selectionEnd = selMin;
        SetText(Utf16ToUtf8(wtext));
    }
}

void TextBox::OnRender(GraphicsContext& ctx) {
    float radius = GetProperty("cornerRadius").AsFloat(0.0f);
    D2D1_COLOR_F bg = HasProperty("theme.backgroundToken")
        ? ResolveThemeColor("theme.backgroundToken", "inputBackground")
        : GetProperty("background").AsColor(D2D1::ColorF(0, 0, 0, 0));
    const bool enabled = IsEnabled();
    if (!enabled) {
        D2D1_COLOR_F disabledBg = ThemeManager::Instance().GetColor("hoverBackground");
        disabledBg.a = 0.45f;
        if (radius > 0.0f) {
            ctx.FillRoundedRect(m_bounds, radius, disabledBg);
        } else {
            ctx.FillRect(m_bounds, disabledBg);
        }
    } else if (bg.a > 0.0f) {
        if (radius > 0.0f) {
            ctx.FillRoundedRect(m_bounds, radius, bg);
        } else {
            ctx.FillRect(m_bounds, bg);
        }
    }

    std::string text = GetProperty("text").AsString("");
    std::string placeholder = GetProperty("placeholder").AsString("Enter text...");
    bool hasFloatingLabel = !placeholder.empty();
    std::string font = GetStringProperty(this, "fontFamily", "FontFamily", "Segoe UI");
    float fontSize = GetFloatProperty(this, "fontSize", "FontSize", 13.0f);
    D2D1_COLOR_F phBase = ResolveThemeColor("theme.placeholderColorToken", "textMuted");
    D2D1_COLOR_F phActive = ResolveThemeColor("theme.activeUnderlineColorToken", "accentColor");
    if (!enabled) {
        phBase.a *= 0.55f;
        phActive = phBase;
    }
    Rect textRect = GetTextRect();
    float labelProgress = m_labelAnim.Current();
    float focusLineProgress = enabled ? m_focusLineAnim.Current() : 0.0f;
    float labelFontSize = fontSize + (11.0f - fontSize) * labelProgress;
    float labelY = m_bounds.y + 16.0f + (4.0f - 16.0f) * labelProgress;
    Rect labelRect(m_bounds.x, labelY, m_bounds.width, 16.0f);
    D2D1_COLOR_F labelColor = BlendColor(phBase, phActive, m_isFocused ? labelProgress : labelProgress * 0.35f);

    if (hasFloatingLabel) {
        ctx.DrawText(
            placeholder,
            labelRect,
            labelColor,
            font,
            labelFontSize,
            DWRITE_TEXT_ALIGNMENT_LEADING,
            DWRITE_PARAGRAPH_ALIGNMENT_NEAR,
            labelProgress > 0.6f ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL
        );
    }

    if (hasFloatingLabel && text.empty() && m_compString.empty() && labelProgress < 0.98f) {
        float alpha = 1.0f - labelProgress;
        D2D1_COLOR_F phColor = D2D1::ColorF(phBase.r, phBase.g, phBase.b, phBase.a * alpha);
        Rect inlinePlaceholderRect(m_bounds.x, m_bounds.y + 15.0f, m_bounds.width, m_bounds.height - 18.0f);
        ctx.DrawText(placeholder, inlinePlaceholderRect, phColor, font, fontSize,
                     DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    }

    std::wstring wtext = GetDisplayedText();
    std::wstring displayWText = BuildDisplayText(wtext, m_cursorPos, m_compString);

    if (m_isFocused) {
        EnsureCaretVisible(ctx);
    }

    auto layout = BuildTextLayout(ctx, displayWText, textRect);
    if (!layout) return;

    Point origin = GetLayoutOrigin(textRect);
    Rect layoutRect(origin.x, origin.y, textRect.width, textRect.height + m_scrollOffsetY);

    ctx.PushClip(textRect);

    if (m_isFocused && HasSelection()) {
        int selMin = (std::min)(m_selectionStart, m_selectionEnd);
        int selMax = (std::max)(m_selectionStart, m_selectionEnd);

        std::vector<D2D1_RECT_F> selRects;
        if (ctx.GetTextSelectionBounds(layout.Get(), static_cast<UINT32>(selMin), static_cast<UINT32>(selMax), origin, selRects)) {
            for (const auto& r : selRects) {
                ctx.FillRect(Rect(r.left, r.top, r.right - r.left, r.bottom - r.top),
                             D2D1::ColorF(phActive.r, phActive.g, phActive.b, 0.28f));
            }
        }
    }

    D2D1_COLOR_F textColor = ResolveThemeColor("theme.colorToken", "textPrimary");
    if (!enabled) {
        textColor = ResolveThemeColor("theme.placeholderColorToken", "textMuted");
        textColor.a *= 0.65f;
    }
    ctx.DrawTextLayout(layout.Get(), layoutRect, textColor);

    if (!m_compString.empty()) {
        auto compCaret = ctx.GetTextCaretInfo(layout.Get(), static_cast<UINT32>(m_cursorPos), origin);
        float compY = compCaret.y + compCaret.height - 2.0f;
        auto compEnd = ctx.GetTextCaretInfo(layout.Get(), static_cast<UINT32>(m_cursorPos + static_cast<int>(m_compString.length())), origin);
        ctx.DrawLine(Point(compCaret.x, compY), Point(compEnd.x, compY),
                     ThemeManager::Instance().GetColor("accentColor"), 1.5f);
    }

    int blinkRate = GetProperty("caretBlinkRate").AsInt(500);
    if (blinkRate <= 0) blinkRate = 500;
    bool cursorBlinkState = ((GetTickCount64() / blinkRate) % 2 == 0);

    if (m_isFocused && cursorBlinkState && enabled) {
        int displayCaretPos = m_cursorPos;
        if (!m_compString.empty()) {
            displayCaretPos += static_cast<int>(m_compString.length());
        }
        displayCaretPos = std::clamp(displayCaretPos, 0, static_cast<int>(displayWText.length()));

        auto caret = ctx.GetTextCaretInfo(layout.Get(), static_cast<UINT32>(displayCaretPos), origin);
        float cursorX = caret.x;
        float cursorY = caret.y + 2.0f;
        float cursorH = std::max(12.0f, caret.height - 4.0f);
        float cursorWidth = GetProperty("caretWidth").AsFloat(1.5f);
        D2D1_COLOR_F cursorColor = ResolveThemeColor("theme.caretColorToken", "accentColor");

        ctx.FillRect(Rect(cursorX, cursorY, cursorWidth, cursorH), cursorColor);
    }

    ctx.PopClip();

    D2D1_COLOR_F underlineColor = ResolveThemeColor("theme.underlineColorToken", "inputBorder");
    D2D1_COLOR_F activeUnderlineColor = ResolveThemeColor("theme.activeUnderlineColorToken", "accentColor");
    if (!enabled) {
        underlineColor.a *= 0.4f;
        activeUnderlineColor = underlineColor;
    }
    float lineY = m_bounds.y + m_bounds.height - 2.0f;
    ctx.DrawLine(Point(m_bounds.x, lineY), Point(m_bounds.x + m_bounds.width, lineY), underlineColor, 1.0f);

    float focusFactor = enabled ? std::clamp(focusLineProgress, 0.0f, 1.0f) : 0.0f;
    if (focusFactor > 0.01f) {
        float eased = 1.0f - std::pow(1.0f - focusFactor, 2.4f);
        float activeWidth = m_bounds.width * eased;
        float activeX = m_bounds.x + (m_bounds.width - activeWidth) * 0.5f;
        ctx.DrawLine(Point(activeX, lineY), Point(activeX + activeWidth, lineY), activeUnderlineColor, 1.0f + eased);
    }

    if (IsPasswordMode() && GetShowRevealButton()) {
        Rect btnRect = GetRevealButtonRect();
        D2D1_COLOR_F eyeColor = ThemeManager::Instance().GetColor("textMuted");
        eyeColor.a = 0.85f;
        float cx = btnRect.x + btnRect.width * 0.5f;
        float cy = btnRect.y + btnRect.height * 0.5f;

        Rect eyeBounds(cx - 7.0f, cy - 4.5f, 14.0f, 9.0f);
        ctx.DrawRoundedRect(eyeBounds, 4.5f, eyeColor, 1.2f);
        ctx.FillRoundedRect(Rect(cx - 2.0f, cy - 2.0f, 4.0f, 4.0f), 2.0f, eyeColor);

        if (!m_isPasswordRevealed) {
            ctx.DrawLine(Point(cx - 7.0f, cy + 5.0f), Point(cx + 7.0f, cy - 5.0f), eyeColor, 1.4f);
        }
    }
}

void TextBox::OnMouseDblClick(Point pt) {
    Control::OnMouseDown(pt);
    m_isFocused = true;

    std::wstring wtext = Utf8ToUtf16(GetProperty("text").AsString());
    if (wtext.empty()) return;

    GraphicsContext ctx;
    int idx = GetCaretIndexFromPoint(ctx, pt.x, pt.y);

    int start = idx;
    while (start > 0 && iswalnum(wtext[start - 1])) {
        start--;
    }
    int end = idx;
    while (end < static_cast<int>(wtext.length()) && iswalnum(wtext[end])) {
        end++;
    }

    if (start == end && idx < static_cast<int>(wtext.length())) {
        start = idx;
        end = idx + 1;
    }

    m_selectionStart = start;
    m_selectionEnd = end;
    m_cursorPos = end;
}

void TextBox::OnFocus() {
    UIElement::OnFocus();
    m_isFocused = true;
    MarkRenderContentDirty();
}

void TextBox::OnBlur() {
    UIElement::OnBlur();
    m_isFocused = false;
    m_selectionStart = m_cursorPos;
    m_selectionEnd = m_cursorPos;
    m_compString.clear();
    m_suppressCharCount = 0;
    m_scrollOffsetX = 0.0f;
    m_scrollOffsetY = 0.0f;
    MarkRenderContentDirty();
}

void TextBox::OnMouseDown(Point pt) {
    if (!IsEnabled()) {
        return;
    }
    Control::OnMouseDown(pt);
    if (IsPasswordMode() && !IsReadOnly() && GetShowRevealButton() && GetRevealButtonRect().Contains(pt.x, pt.y)) {
        SetIsPasswordRevealed(!IsPasswordRevealed());
        return;
    }
    OnFocus();
    GraphicsContext ctx;
    int idx = GetCaretIndexFromPoint(ctx, pt.x, pt.y);
    m_cursorPos = idx;
    m_selectionStart = idx;
    m_selectionEnd = idx;
    m_isDraggingSelection = true;
    EnsureCaretVisible(ctx);
}

void TextBox::OnMouseRightClick(Point pt) {
    if (!IsEnabled()) {
        return;
    }
    Control::OnMouseRightClick(pt);

    if (!m_contextMenu) {
        auto menu = std::make_shared<ContextMenu>();
        if (!IsReadOnly()) {
            menu->AddItem("撤销 (Undo)", "Ctrl+Z", [this]() { Undo(); });
            menu->AddItem("重做 (Redo)", "Ctrl+Y", [this]() { Redo(); });
            menu->AddSeparator();
            menu->AddItem("剪切 (Cut)", "Ctrl+X", [this]() {
                if (HasSelection()) {
                    std::wstring wtext = Utf8ToUtf16(GetProperty("text").AsString());
                    int selMin = (std::min)(m_selectionStart, m_selectionEnd);
                    int selMax = (std::max)(m_selectionStart, m_selectionEnd);
                    std::wstring selected = wtext.substr(selMin, selMax - selMin);
                    if (OpenClipboard(nullptr)) {
                        EmptyClipboard();
                        size_t bytes = (selected.size() + 1) * sizeof(wchar_t);
                        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
                        if (hMem) {
                            wchar_t* pMem = static_cast<wchar_t*>(GlobalLock(hMem));
                            memcpy(pMem, selected.c_str(), bytes);
                            GlobalUnlock(hMem);
                            SetClipboardData(CF_UNICODETEXT, hMem);
                        }
                        CloseClipboard();
                    }
                    DeleteSelection();
                }
            });
        }
        menu->AddItem("复制 (Copy)", "Ctrl+C", [this]() {
            if (HasSelection()) {
                std::wstring wtext = Utf8ToUtf16(GetProperty("text").AsString());
                int selMin = (std::min)(m_selectionStart, m_selectionEnd);
                int selMax = (std::max)(m_selectionStart, m_selectionEnd);
                std::wstring selected = wtext.substr(selMin, selMax - selMin);
                if (OpenClipboard(nullptr)) {
                    EmptyClipboard();
                    size_t bytes = (selected.size() + 1) * sizeof(wchar_t);
                    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
                    if (hMem) {
                        wchar_t* pMem = static_cast<wchar_t*>(GlobalLock(hMem));
                        memcpy(pMem, selected.c_str(), bytes);
                        GlobalUnlock(hMem);
                        SetClipboardData(CF_UNICODETEXT, hMem);
                    }
                    CloseClipboard();
                }
            }
        });
        if (!IsReadOnly()) {
            menu->AddItem("粘贴 (Paste)", "Ctrl+V", [this]() {
                if (IsClipboardFormatAvailable(CF_UNICODETEXT) && OpenClipboard(nullptr)) {
                    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
                    if (hData) {
                        const wchar_t* pText = static_cast<const wchar_t*>(GlobalLock(hData));
                        if (pText) {
                            std::wstring clipText(pText);
                            GlobalUnlock(hData);
                            if (!GetAcceptsReturn()) {
                                for (wchar_t& ch : clipText) {
                                    if (ch == L'\r' || ch == L'\n') ch = L' ';
                                }
                            }
                            InsertText(clipText);
                        }
                    }
                    CloseClipboard();
                }
            });
            menu->AddItem("删除 (Delete)", "", [this]() { DeleteSelection(); });
            menu->AddSeparator();
        }
        menu->AddItem("全选 (Select All)", "Ctrl+A", [this]() { SelectAll(); });
        SetContextMenu(menu);
    }
}

void TextBox::OnMouseMove(Point pt) {
    if (!IsEnabled()) {
        return;
    }
    Control::OnMouseMove(pt);
    if (m_isDraggingSelection && m_isPressed) {
        GraphicsContext ctx;
        Rect textRect = GetTextRect();

        if (!IsMultiline()) {
            const float scrollStep = 10.0f;
            if (pt.x > textRect.x + textRect.width) {
                m_scrollOffsetX += scrollStep;
            } else if (pt.x < textRect.x) {
                m_scrollOffsetX -= scrollStep;
            }
            std::wstring wtext = BuildDisplayText(
                Utf8ToUtf16(GetProperty("text").AsString("")),
                m_cursorPos,
                m_compString
            );
            ClampScrollOffsets(ctx, wtext, textRect);
        }

        int idx = GetCaretIndexFromPoint(ctx, pt.x, pt.y);
        m_selectionEnd = idx;
        m_cursorPos = idx;
        EnsureCaretVisible(ctx);
    }
}

void TextBox::OnMouseUp(Point pt) {
    Control::OnMouseUp(pt);
    m_isDraggingSelection = false;
}

void TextBox::OnKeyDown(int vkCode) {
    if (!IsEnabled()) {
        return;
    }
    m_suppressCharCount = 0;

    std::wstring wtext = Utf8ToUtf16(GetProperty("text").AsString());

    bool isCtrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    bool isShiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    const bool readOnly = IsReadOnly();

    if (!readOnly && isCtrlDown && (vkCode == 'Z' || vkCode == 'z')) {
        Undo();
        return;
    }

    if (!readOnly && isCtrlDown && (vkCode == 'Y' || vkCode == 'y')) {
        Redo();
        return;
    }

    if (isCtrlDown && (vkCode == 'A' || vkCode == 'a')) {
        SelectAll();
        return;
    }

    if (isCtrlDown && (vkCode == 'C' || vkCode == 'c')) {
        if (HasSelection()) {
            int selMin = (std::min)(m_selectionStart, m_selectionEnd);
            int selMax = (std::max)(m_selectionStart, m_selectionEnd);
            std::wstring selected = wtext.substr(selMin, selMax - selMin);

            if (OpenClipboard(nullptr)) {
                EmptyClipboard();
                size_t bytes = (selected.size() + 1) * sizeof(wchar_t);
                HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
                if (hMem) {
                    wchar_t* pMem = static_cast<wchar_t*>(GlobalLock(hMem));
                    memcpy(pMem, selected.c_str(), bytes);
                    GlobalUnlock(hMem);
                    SetClipboardData(CF_UNICODETEXT, hMem);
                }
                CloseClipboard();
            }
        }
        return;
    }

    if (readOnly) {
        // Navigation / selection still allowed below; block mutating shortcuts.
        if (isCtrlDown && (vkCode == 'X' || vkCode == 'x' || vkCode == 'V' || vkCode == 'v')) {
            return;
        }
        if (vkCode == VK_BACK || vkCode == VK_DELETE || vkCode == VK_RETURN) {
            return;
        }
    }

    if (isCtrlDown && (vkCode == 'X' || vkCode == 'x')) {
        if (HasSelection()) {
            int selMin = (std::min)(m_selectionStart, m_selectionEnd);
            int selMax = (std::max)(m_selectionStart, m_selectionEnd);
            std::wstring selected = wtext.substr(selMin, selMax - selMin);

            if (OpenClipboard(nullptr)) {
                EmptyClipboard();
                size_t bytes = (selected.size() + 1) * sizeof(wchar_t);
                HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
                if (hMem) {
                    wchar_t* pMem = static_cast<wchar_t*>(GlobalLock(hMem));
                    memcpy(pMem, selected.c_str(), bytes);
                    GlobalUnlock(hMem);
                    SetClipboardData(CF_UNICODETEXT, hMem);
                }
                CloseClipboard();
            }
            DeleteSelection();
            wtext = Utf8ToUtf16(GetProperty("text").AsString());
        }
        return;
    }

    if (isCtrlDown && (vkCode == 'V' || vkCode == 'v')) {
        if (IsClipboardFormatAvailable(CF_UNICODETEXT) && OpenClipboard(nullptr)) {
            HANDLE hData = GetClipboardData(CF_UNICODETEXT);
            if (hData) {
                const wchar_t* pText = static_cast<const wchar_t*>(GlobalLock(hData));
                if (pText) {
                    std::wstring clipText(pText);
                    GlobalUnlock(hData);

                    if (!GetAcceptsReturn()) {
                        for (wchar_t& ch : clipText) {
                            if (ch == L'\r' || ch == L'\n') ch = L' ';
                        }
                    } else {
                        std::wstring normalized;
                        normalized.reserve(clipText.size());
                        for (size_t i = 0; i < clipText.size(); ++i) {
                            wchar_t ch = clipText[i];
                            if (ch == L'\r') {
                                if (i + 1 < clipText.size() && clipText[i + 1] == L'\n') continue;
                                normalized.push_back(L'\n');
                            } else {
                                normalized.push_back(ch);
                            }
                        }
                        clipText = normalized;
                    }

                    InsertText(clipText);
                    m_suppressCharCount = 1;
                }
            }
            CloseClipboard();
        }
        return;
    }

    if (vkCode == VK_RETURN && GetAcceptsReturn()) {
        InsertText(L"\n");
        GraphicsContext ctx;
        EnsureCaretVisible(ctx);
        return;
    }

    if (vkCode == VK_BACK) {
        if (HasSelection()) {
            DeleteSelection();
        } else if (!wtext.empty() && m_cursorPos > 0) {
            PushUndoState();
            wtext.erase(m_cursorPos - 1, 1);
            m_cursorPos--;
            m_selectionStart = m_cursorPos;
            m_selectionEnd = m_cursorPos;
            SetText(Utf16ToUtf8(wtext));
        }
        GraphicsContext ctx;
        EnsureCaretVisible(ctx);
        return;
    }

    if (vkCode == VK_DELETE) {
        if (HasSelection()) {
            DeleteSelection();
        } else if (m_cursorPos < static_cast<int>(wtext.length())) {
            PushUndoState();
            wtext.erase(m_cursorPos, 1);
            m_selectionStart = m_cursorPos;
            m_selectionEnd = m_cursorPos;
            SetText(Utf16ToUtf8(wtext));
        }
        GraphicsContext ctx;
        EnsureCaretVisible(ctx);
        return;
    }

    GraphicsContext ctx;

    if (vkCode == VK_LEFT) {
        if (m_cursorPos > 0) {
            m_cursorPos--;
            if (!isShiftDown) m_selectionStart = m_cursorPos;
            m_selectionEnd = m_cursorPos;
        }
        EnsureCaretVisible(ctx);
        return;
    }

    if (vkCode == VK_RIGHT) {
        if (m_cursorPos < static_cast<int>(wtext.length())) {
            m_cursorPos++;
            if (!isShiftDown) m_selectionStart = m_cursorPos;
            m_selectionEnd = m_cursorPos;
        }
        EnsureCaretVisible(ctx);
        return;
    }

    if (vkCode == VK_UP || vkCode == VK_DOWN) {
        if (!IsMultiline()) return;

        auto caret = GetCaretScreenPos(ctx, m_cursorPos);
        float targetY = caret.y + (vkCode == VK_UP ? -caret.height * 0.5f : caret.height * 1.5f);

        Rect textRect = GetTextRect();
        std::wstring displayWText = BuildDisplayText(wtext, m_cursorPos, m_compString);
        auto layout = BuildTextLayout(ctx, displayWText, textRect);
        if (!layout) return;

        UINT32 newPos = ctx.HitTestTextLayout(layout.Get(), caret.x, targetY, GetLayoutOrigin(textRect));
        m_cursorPos = static_cast<int>(newPos);
        if (!isShiftDown) m_selectionStart = m_cursorPos;
        m_selectionEnd = m_cursorPos;
        EnsureCaretVisible(ctx);
        return;
    }

    if (vkCode == VK_HOME) {
        if (IsMultiline()) {
            auto caret = GetCaretScreenPos(ctx, m_cursorPos);
            Rect textRect = GetTextRect();
            std::wstring displayWText = BuildDisplayText(wtext, m_cursorPos, m_compString);
            auto layout = BuildTextLayout(ctx, displayWText, textRect);
            if (layout) {
                UINT32 newPos = ctx.HitTestTextLayout(layout.Get(), textRect.x, caret.y, GetLayoutOrigin(textRect));
                m_cursorPos = static_cast<int>(newPos);
            }
        } else {
            m_cursorPos = 0;
            m_scrollOffsetX = 0.0f;
        }
        if (!isShiftDown) m_selectionStart = m_cursorPos;
        m_selectionEnd = m_cursorPos;
        return;
    }

    if (vkCode == VK_END) {
        if (IsMultiline()) {
            auto caret = GetCaretScreenPos(ctx, m_cursorPos);
            Rect textRect = GetTextRect();
            std::wstring displayWText = BuildDisplayText(wtext, m_cursorPos, m_compString);
            auto layout = BuildTextLayout(ctx, displayWText, textRect);
            if (layout) {
                UINT32 newPos = ctx.HitTestTextLayout(layout.Get(), textRect.x + textRect.width, caret.y, GetLayoutOrigin(textRect));
                m_cursorPos = static_cast<int>(newPos);
            }
        } else {
            m_cursorPos = static_cast<int>(wtext.length());
            EnsureCaretVisible(ctx);
        }
        if (!isShiftDown) m_selectionStart = m_cursorPos;
        m_selectionEnd = m_cursorPos;
        return;
    }
}

void TextBox::CommitImeResult(const std::wstring& result) {
    m_compString.clear();
    if (result.empty() || !IsEnabled() || IsReadOnly()) return;

    InsertText(result);
    // IME commit is also delivered as WM_CHAR; suppress duplicates (e.g. Shift during composition).
    m_suppressCharCount = static_cast<int>(result.length());

    GraphicsContext ctx;
    EnsureCaretVisible(ctx);
}

void TextBox::OnCharInput(wchar_t ch) {
    if (!IsEnabled() || IsReadOnly()) {
        return;
    }
    if (m_suppressCharCount > 0) {
        m_suppressCharCount--;
        return;
    }

    if (ch == L'\r' || ch == L'\n') {
        if (GetAcceptsReturn()) {
            InsertText(L"\n");
            GraphicsContext ctx;
            EnsureCaretVisible(ctx);
        }
        return;
    }

    if (ch >= 32) {
        InsertText(std::wstring(1, ch));
        GraphicsContext ctx;
        EnsureCaretVisible(ctx);
    }
}

} // namespace CUI
