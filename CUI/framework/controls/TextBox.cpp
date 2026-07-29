#include "TextBox.h"

namespace CUI {

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
    SetProperty("padding", Value(Thickness(8, 6, 8, 6)));
    SetProperty("width", Value(260.0f));
    SetProperty("height", Value(32.0f));
}

TextBox::TextBox(const std::string& placeholder) : TextBox() {
    SetProperty("placeholder", Value(placeholder));
}

Size TextBox::Measure(Size availableSize) {
    float expW = GetProperty("width").AsFloat(260.0f);
    float expH = GetProperty("height").AsFloat(32.0f);
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

int TextBox::GetCaretIndexFromX(GraphicsContext& ctx, float mouseX) {
    std::string text = GetProperty("text").AsString("");
    std::wstring wtext = Utf8ToUtf16(text);
    if (wtext.empty()) return 0;

    std::string font = GetProperty("fontFamily").AsString("Segoe UI");
    float fontSize = GetProperty("fontSize").AsFloat(13.0f);
    Thickness padding = GetProperty("padding").AsThickness(Thickness(8, 6, 8, 6));

    float relX = mouseX - (m_bounds.x + padding.left);
    if (relX <= 0.0f) return 0;

    int bestIdx = static_cast<int>(wtext.length());
    float minDiff = 100000.0f;

    for (size_t i = 0; i <= wtext.length(); ++i) {
        std::string sub = Utf16ToUtf8(wtext.substr(0, i));
        Size sz = ctx.MeasureText(sub, font, fontSize);
        float diff = std::abs(sz.width - relX);
        if (diff < minDiff) {
            minDiff = diff;
            bestIdx = static_cast<int>(i);
        }
    }
    return bestIdx;
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
    std::string font = GetProperty("fontFamily").AsString("Segoe UI");
    float fontSize = GetProperty("fontSize").AsFloat(13.0f);
    Thickness padding = GetProperty("padding").AsThickness(Thickness(8, 6, 8, 6));

    Rect textRect(
        m_bounds.x + padding.left,
        m_bounds.y + padding.top,
        m_bounds.width - padding.left - padding.right,
        m_bounds.height - padding.top - padding.bottom
    );

    if (text.empty() && m_compString.empty() && !m_isFocused) {
        D2D1_COLOR_F phColor = GetProperty("placeholderColor").AsColor(D2D1::ColorF(0x85 / 255.0f, 0x85 / 255.0f, 0x85 / 255.0f, 1.0f));
        ctx.DrawText(placeholder, textRect, phColor, font, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    } else {
        std::wstring wtext = Utf8ToUtf16(text);

        // Draw VS Code Selection Blue Rectangle (#264F78) if selected
        if (m_isFocused && HasSelection()) {
            int selMin = (std::min)(m_selectionStart, m_selectionEnd);
            int selMax = (std::max)(m_selectionStart, m_selectionEnd);

            std::string subMin = Utf16ToUtf8(wtext.substr(0, selMin));
            std::string subMax = Utf16ToUtf8(wtext.substr(0, selMax));

            Size sizeMin = ctx.MeasureText(subMin, font, fontSize);
            Size sizeMax = ctx.MeasureText(subMax, font, fontSize);

            float selX = textRect.x + sizeMin.width;
            float selW = sizeMax.width - sizeMin.width;
            if (selX + selW > textRect.x + textRect.width) selW = (textRect.x + textRect.width) - selX;

            Rect selRect(selX, textRect.y, selW, textRect.height);
            ctx.FillRect(selRect, D2D1::ColorF(0x26 / 255.0f, 0x4F / 255.0f, 0x78 / 255.0f, 0.8f));
        }

        // Handle Chinese IME Composition String (e.g. jiao'ao 预编辑拼音)
        std::string displayText = text;
        if (!m_compString.empty()) {
            if (m_cursorPos > static_cast<int>(wtext.length())) m_cursorPos = static_cast<int>(wtext.length());
            std::wstring wsub = wtext.substr(0, m_cursorPos);
            std::wstring wrest = wtext.substr(m_cursorPos);
            std::wstring wdisp = wsub + m_compString + wrest;
            displayText = Utf16ToUtf8(wdisp);
        }

        D2D1_COLOR_F textColor = GetProperty("color").AsColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f));
        ctx.DrawText(displayText, textRect, textColor, font, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        // Draw IME composition string dashed underline
        if (!m_compString.empty()) {
            std::wstring wsub = wtext.substr(0, m_cursorPos);
            Size sizeBefore = ctx.MeasureText(Utf16ToUtf8(wsub), font, fontSize);
            Size sizeComp = ctx.MeasureText(Utf16ToUtf8(m_compString), font, fontSize);

            float compX = textRect.x + sizeBefore.width;
            float compY = textRect.y + textRect.height - 2.0f;
            ctx.DrawLine(Point(compX, compY), Point(compX + sizeComp.width, compY), D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f), 1.5f);
        }

        // Draw blinking cursor line (500ms cycle)
        bool cursorBlinkState = ((GetTickCount64() / 500) % 2 == 0);
        if (m_isFocused && cursorBlinkState) {
            if (m_cursorPos > static_cast<int>(wtext.length())) {
                m_cursorPos = static_cast<int>(wtext.length());
            }
            std::wstring wsub = wtext.substr(0, m_cursorPos);
            if (!m_compString.empty()) wsub += m_compString;
            std::string subText = Utf16ToUtf8(wsub);

            Size textMeasured = ctx.MeasureText(subText, font, fontSize);
            float cursorX = textRect.x + textMeasured.width;
            if (cursorX > textRect.x + textRect.width) cursorX = textRect.x + textRect.width;

            ctx.FillRect(Rect(cursorX, textRect.y + 2, 1.5f, textRect.height - 4), D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f));
        }
    }
}

void TextBox::OnMouseDblClick(Point pt) {
    Control::OnMouseDown(pt);
    m_isFocused = true;

    std::wstring wtext = Utf8ToUtf16(GetProperty("text").AsString());
    if (wtext.empty()) return;

    GraphicsContext ctx;
    int idx = GetCaretIndexFromX(ctx, pt.x);

    // Double-click word selection algorithm
    int start = idx;
    while (start > 0 && iswalnum(wtext[start - 1])) {
        start--;
    }
    int end = idx;
    while (end < static_cast<int>(wtext.length()) && iswalnum(wtext[end])) {
        end++;
    }

    if (start == end && idx < static_cast<int>(wtext.length())) {
        // If punctuation or Chinese character, select single character
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
}

void TextBox::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);
    OnFocus();
    GraphicsContext ctx;
    int idx = GetCaretIndexFromX(ctx, pt.x);
    m_cursorPos = idx;
    m_selectionStart = idx;
    m_selectionEnd = idx;
    m_isDraggingSelection = true;
}

void TextBox::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);
    if (m_isDraggingSelection && m_isPressed) {
        GraphicsContext ctx;
        int idx = GetCaretIndexFromX(ctx, pt.x);
        m_selectionEnd = idx;
        m_cursorPos = idx;
    }
}

void TextBox::OnMouseUp(Point pt) {
    Control::OnMouseUp(pt);
    m_isDraggingSelection = false;
}

void TextBox::OnKeyDown(int vkCode) {
    std::wstring wtext = Utf8ToUtf16(GetProperty("text").AsString());

    bool isCtrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    bool isShiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

    if (isCtrlDown && (vkCode == 'A' || vkCode == 'a')) {
        SelectAll();
        return;
    }

    if (vkCode == VK_BACK) {
        if (HasSelection()) {
            DeleteSelection();
        } else if (!wtext.empty() && m_cursorPos > 0) {
            wtext.erase(m_cursorPos - 1, 1);
            m_cursorPos--;
            m_selectionStart = m_cursorPos;
            m_selectionEnd = m_cursorPos;
            SetText(Utf16ToUtf8(wtext));
        }
    } else if (vkCode == VK_DELETE) {
        if (HasSelection()) {
            DeleteSelection();
        } else if (m_cursorPos < static_cast<int>(wtext.length())) {
            wtext.erase(m_cursorPos, 1);
            m_selectionStart = m_cursorPos;
            m_selectionEnd = m_cursorPos;
            SetText(Utf16ToUtf8(wtext));
        }
    } else if (vkCode == VK_LEFT) {
        if (m_cursorPos > 0) {
            m_cursorPos--;
            if (!isShiftDown) {
                m_selectionStart = m_cursorPos;
            }
            m_selectionEnd = m_cursorPos;
        }
    } else if (vkCode == VK_RIGHT) {
        if (m_cursorPos < static_cast<int>(wtext.length())) {
            m_cursorPos++;
            if (!isShiftDown) {
                m_selectionStart = m_cursorPos;
            }
            m_selectionEnd = m_cursorPos;
        }
    }
}

void TextBox::OnCharInput(wchar_t ch) {
    if (ch >= 32) { // Printable characters including Chinese Unicode
        if (HasSelection()) {
            DeleteSelection();
        }
        std::wstring wtext = Utf8ToUtf16(GetProperty("text").AsString());
        if (m_cursorPos > static_cast<int>(wtext.length())) {
            m_cursorPos = static_cast<int>(wtext.length());
        }
        wtext.insert(m_cursorPos, 1, ch);
        m_cursorPos++;
        m_selectionStart = m_cursorPos;
        m_selectionEnd = m_cursorPos;
        SetText(Utf16ToUtf8(wtext));
    }
}

} // namespace CUI
