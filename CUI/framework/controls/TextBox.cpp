#define NOMINMAX
#include "TextBox.h"
#include <algorithm>
#include <cwctype>

namespace CUI {

std::vector<PropertyMeta> TextBox::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "fontFamily", "字体名称 (FontFamily)", "字体文本", "enum", { "Segoe UI", "Consolas", "微软雅黑", "Times New Roman" } });
    metas.push_back({ "fontSize", "字体大小 (FontSize)", "字体文本", "number" });
    metas.push_back({ "fontWeight", "字体粗细 (FontWeight)", "字体文本", "enum", { "Normal", "Bold", "Light" } });
    metas.push_back({ "color", "文字颜色 (Color)", "字体文本", "color" });
    metas.push_back({ "lineSpacing", "行间距 (LineSpacing)", "高级排版", "number" });
    metas.push_back({ "lineHeight", "固定行高 (LineHeight)", "高级排版", "number" });
    metas.push_back({ "placeholder", "占位提示词 (Placeholder)", "输入控制", "string" });
    metas.push_back({ "placeholderColor", "提示词颜色 (PhColor)", "输入控制", "color" });
    metas.push_back({ "caretColor", "光标颜色 (CaretColor)", "光标排版", "color" });
    metas.push_back({ "caretWidth", "光标宽度 (CaretWidth)", "光标排版", "number" });
    metas.push_back({ "caretBlinkRate", "光标闪烁频率 (BlinkMs)", "光标排版", "number" });
    metas.push_back({ "TextWrapping", "自动换行 (TextWrapping)", "输入控制", "enum", { "NoWrap", "Wrap" } });
    metas.push_back({ "AcceptsReturn", "允许回车 (AcceptsReturn)", "输入控制", "bool" });
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
    SetProperty("placeholder", Value("Enter text..."));
    SetProperty("background", Value(D2D1::ColorF(0x3C / 255.0f, 0x3C / 255.0f, 0x3C / 255.0f, 1.0f)));
    SetProperty("hoverBackground", Value(D2D1::ColorF(0x44 / 255.0f, 0x44 / 255.0f, 0x44 / 255.0f, 1.0f)));
    SetProperty("borderBrush", Value(D2D1::ColorF(0x33 / 255.0f, 0x33 / 255.0f, 0x33 / 255.0f, 1.0f)));
    SetProperty("focusedBorderBrush", Value(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f)));
    SetProperty("borderThickness", Value(1.0f));
    SetProperty("color", Value(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f)));
    SetProperty("placeholderColor", Value(D2D1::ColorF(0x85 / 255.0f, 0x85 / 255.0f, 0x85 / 255.0f, 1.0f)));
    SetProperty("fontFamily", Value("Segoe UI"));
    SetProperty("fontSize", Value(13.0f));
    SetProperty("lineSpacing", Value(1.0f));
    SetProperty("lineHeight", Value(0.0f));
    SetProperty("padding", Value(Thickness(8, 6, 8, 6)));
    SetProperty("width", Value(260.0f));
    SetProperty("height", Value(32.0f));
    SetProperty("AcceptsReturn", Value(false));
    SetProperty("TextWrapping", Value("NoWrap"));
}

TextBox::TextBox(const std::string& placeholder) : TextBox() {
    SetProperty("placeholder", Value(placeholder));
}

void TextBox::SetText(const std::string& text) {
    if (GetText() != text) {
        SetProperty("text", Value(text));
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

Rect TextBox::GetTextRect() const {
    Thickness padding = GetProperty("padding").AsThickness(Thickness(8, 6, 8, 6));
    return Rect(
        m_bounds.x + padding.left,
        m_bounds.y + padding.top,
        m_bounds.width - padding.left - padding.right,
        m_bounds.height - padding.top - padding.bottom
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

    return ctx.CreateTextLayout(wtext, fontFamily, fontSize, options);
}

int TextBox::GetCaretIndexFromPoint(GraphicsContext& ctx, float x, float y) {
    std::wstring wtext = BuildDisplayText(
        Utf8ToUtf16(GetProperty("text").AsString("")),
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
        Utf8ToUtf16(GetProperty("text").AsString("")),
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
        Utf8ToUtf16(GetProperty("text").AsString("")),
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

void TextBox::Undo() {
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
    float expH = GetProperty("height").AsFloat(32.0f);
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

void TextBox::SelectAll() {
    std::wstring wtext = Utf8ToUtf16(GetProperty("text").AsString());
    m_selectionStart = 0;
    m_selectionEnd = static_cast<int>(wtext.length());
    m_cursorPos = m_selectionEnd;
}

void TextBox::DeleteSelection() {
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

    D2D1_COLOR_F bg = GetProperty("background").AsColor(D2D1::ColorF(0x3C / 255.0f, 0x3C / 255.0f, 0x3C / 255.0f, 1.0f));
    if (m_isHovered) {
        bg = GetProperty("hoverBackground").AsColor(bg);
    }
    if (radius > 0.0f) {
        ctx.FillRoundedRect(m_bounds, radius, bg);
    } else {
        ctx.FillRect(m_bounds, bg);
    }

    D2D1_COLOR_F borderBrush = m_isFocused
        ? GetProperty("focusedBorderBrush").AsColor(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f))
        : GetProperty("borderBrush").AsColor(D2D1::ColorF(0x33 / 255.0f, 0x33 / 255.0f, 0x33 / 255.0f, 1.0f));
    float borderThickness = GetProperty("borderThickness").AsFloat(1.0f);
    if (borderThickness > 0.0f) {
        if (radius > 0.0f) {
            ctx.DrawRoundedRect(m_bounds, radius, borderBrush, m_isFocused ? 1.5f : borderThickness);
        } else {
            ctx.DrawRect(m_bounds, borderBrush, m_isFocused ? 1.5f : borderThickness);
        }
    }

    std::string text = GetProperty("text").AsString("");
    std::string placeholder = GetProperty("placeholder").AsString("Enter text...");
    std::string font = GetStringProperty(this, "fontFamily", "FontFamily", "Segoe UI");
    float fontSize = GetFloatProperty(this, "fontSize", "FontSize", 13.0f);
    Rect textRect = GetTextRect();

    if (text.empty() && m_compString.empty() && !m_isFocused) {
        D2D1_COLOR_F phColor = GetProperty("placeholderColor").AsColor(D2D1::ColorF(0x85 / 255.0f, 0x85 / 255.0f, 0x85 / 255.0f, 1.0f));
        DWRITE_PARAGRAPH_ALIGNMENT vAlign = IsMultiline()
            ? DWRITE_PARAGRAPH_ALIGNMENT_NEAR
            : DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
        ctx.DrawText(placeholder, textRect, phColor, font, fontSize,
                     DWRITE_TEXT_ALIGNMENT_LEADING, vAlign);
        return;
    }

    std::wstring wtext = Utf8ToUtf16(text);
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
                             D2D1::ColorF(0x26 / 255.0f, 0x4F / 255.0f, 0x78 / 255.0f, 0.8f));
            }
        }
    }

    D2D1_COLOR_F textColor = GetProperty("color").AsColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f));
    ctx.DrawTextLayout(layout.Get(), layoutRect, textColor);

    if (!m_compString.empty()) {
        auto compCaret = ctx.GetTextCaretInfo(layout.Get(), static_cast<UINT32>(m_cursorPos), origin);
        float compY = compCaret.y + compCaret.height - 2.0f;
        auto compEnd = ctx.GetTextCaretInfo(layout.Get(), static_cast<UINT32>(m_cursorPos + static_cast<int>(m_compString.length())), origin);
        ctx.DrawLine(Point(compCaret.x, compY), Point(compEnd.x, compY),
                     D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f), 1.5f);
    }

    int blinkRate = GetProperty("caretBlinkRate").AsInt(500);
    if (blinkRate <= 0) blinkRate = 500;
    bool cursorBlinkState = ((GetTickCount64() / blinkRate) % 2 == 0);

    if (m_isFocused && cursorBlinkState && IsEnabled()) {
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
        D2D1_COLOR_F cursorColor = GetProperty("caretColor").AsColor(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f));

        ctx.FillRect(Rect(cursorX, cursorY, cursorWidth, cursorH), cursorColor);
    }

    ctx.PopClip();
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
}

void TextBox::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);
    OnFocus();
    GraphicsContext ctx;
    int idx = GetCaretIndexFromPoint(ctx, pt.x, pt.y);
    m_cursorPos = idx;
    m_selectionStart = idx;
    m_selectionEnd = idx;
    m_isDraggingSelection = true;
    EnsureCaretVisible(ctx);
}

void TextBox::OnMouseMove(Point pt) {
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
    m_suppressCharCount = 0;

    std::wstring wtext = Utf8ToUtf16(GetProperty("text").AsString());

    bool isCtrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    bool isShiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

    if (isCtrlDown && (vkCode == 'Z' || vkCode == 'z')) {
        Undo();
        return;
    }

    if (isCtrlDown && (vkCode == 'Y' || vkCode == 'y')) {
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
    if (result.empty()) return;

    InsertText(result);
    // IME commit is also delivered as WM_CHAR; suppress duplicates (e.g. Shift during composition).
    m_suppressCharCount = static_cast<int>(result.length());

    GraphicsContext ctx;
    EnsureCaretVisible(ctx);
}

void TextBox::OnCharInput(wchar_t ch) {
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
