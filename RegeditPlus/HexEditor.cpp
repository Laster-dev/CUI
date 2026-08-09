#include "HexEditor.h"
#include "BinaryValueDialog.h"
#include "framework/style/ThemeManager.h"
#include "framework/animation/AnimationManager.h"
#include "framework/controls/MessageBox.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

using namespace CUI;

namespace RegeditPlus {

HexEditor::HexEditor() {
    SetBackgroundToken(ThemeTokenId::InputBackground);
    SetBorderToken(ThemeTokenId::InputBorder);
    SetBorderThickness(1.0f);
    SetFontFamily("Consolas");
    SetFontSize(13.0f);
    SetColorToken(ThemeTokenId::TextPrimary);
}

void HexEditor::SetBytes(std::vector<BYTE> data) {
    m_data = std::move(data);
    m_caretNibble = 0;
    m_selAnchorNibble = 0;
    m_scrollRow = 0;
    MarkRenderContentDirty();
}

int HexEditor::RowCount() const {
    if (m_data.empty()) return 1;
    return (ByteCount() + m_bytesPerRow - 1) / m_bytesPerRow;
}

int HexEditor::MaxScroll() const {
    const float viewH = ContentRect().height;
    const int visible = (std::max)(1, static_cast<int>(viewH / RowHeight()));
    return (std::max)(0, RowCount() - visible);
}

Rect HexEditor::ContentRect() const {
    return Rect(m_bounds.x + 4.0f, m_bounds.y + 4.0f,
                (std::max)(0.0f, m_bounds.width - 8.0f),
                (std::max)(0.0f, m_bounds.height - 8.0f));
}

Size HexEditor::Measure(Size availableSize) {
    (void)availableSize;
    const float w = (GetWidth() >= 0.0f) ? GetWidth() : 420.0f;
    const float h = (GetHeight() >= 0.0f) ? GetHeight() : 220.0f;
    m_desiredSize = Size(w, h);
    return m_desiredSize;
}

void HexEditor::NotifyDirty() {
    MarkRenderRectDirty(m_bounds);
    for (UIElement* walk = GetParent(); walk; walk = walk->GetParent()) {
        if (std::strcmp(walk->GetClassName(), "ContentDialog") == 0) {
            static_cast<ContentDialog*>(walk)->InvalidateCard();
            walk->MarkRenderRectDirty(walk->GetBounds());
            break;
        }
        if (std::strcmp(walk->GetClassName(), "BinaryValueDialog") == 0) {
            static_cast<BinaryValueDialog*>(walk)->InvalidateCard();
            walk->MarkRenderRectDirty(walk->GetBounds());
            break;
        }
    }
}

void HexEditor::ClampScroll() {
    m_scrollRow = std::clamp(m_scrollRow, 0, MaxScroll());
}

void HexEditor::EnsureCaretVisible() {
    const int row = (m_bytesPerRow > 0) ? (m_caretNibble / 2) / m_bytesPerRow : 0;
    const float viewH = ContentRect().height;
    const int visible = (std::max)(1, static_cast<int>(viewH / RowHeight()));
    if (row < m_scrollRow) m_scrollRow = row;
    else if (row >= m_scrollRow + visible) m_scrollRow = row - visible + 1;
    ClampScroll();
}

int HexEditor::HitTestNibble(float x, float y) const {
    const Rect content = ContentRect();
    if (!content.Contains(x, y) && y < content.y) return 0;
    const float localY = y - content.y;
    const float localX = x - content.x;
    int row = m_scrollRow + static_cast<int>(localY / RowHeight());
    if (row < 0) row = 0;
    if (row >= RowCount()) row = RowCount() - 1;

    // Layout: "00000000  " (10) + hex pairs ("XX " * n) + " " + ascii
    const float charW = 7.8f; // Consolas ~13px
    const float offsetW = 10.0f * charW;
    float hexStart = offsetW;
    float hexEnd = hexStart + m_bytesPerRow * 3.0f * charW;

    int byteInRow = 0;
    int nibbleInByte = 0;
    if (localX < hexStart) {
        byteInRow = 0;
        nibbleInByte = 0;
    } else if (localX >= hexEnd) {
        // ASCII pane → map to byte
        float asciiStart = hexEnd + charW;
        int ai = static_cast<int>((localX - asciiStart) / charW);
        byteInRow = std::clamp(ai, 0, m_bytesPerRow - 1);
        nibbleInByte = 0;
    } else {
        float hx = localX - hexStart;
        int cell = static_cast<int>(hx / (3.0f * charW));
        cell = std::clamp(cell, 0, m_bytesPerRow - 1);
        float within = hx - cell * 3.0f * charW;
        byteInRow = cell;
        nibbleInByte = (within > charW) ? 1 : 0;
    }

    int byteIndex = row * m_bytesPerRow + byteInRow;
    if (byteIndex < 0) byteIndex = 0;
    // Allow caret at end (one past last byte → nibble = size*2)
    if (byteIndex > ByteCount()) byteIndex = ByteCount();
    int nib = byteIndex * 2 + nibbleInByte;
    const int maxNib = ByteCount() * 2; // end position
    if (nib > maxNib) nib = maxNib;
    if (byteIndex == ByteCount()) nib = maxNib;
    return nib;
}

void HexEditor::OnRender(GraphicsContext& ctx) {
    D2D1_COLOR_F bg = ResolveThemeColor(GetBackgroundToken(), ThemeTokenId::InputBackground);
    D2D1_COLOR_F border = ResolveThemeColor(GetBorderToken(), ThemeTokenId::InputBorder);
    D2D1_COLOR_F text = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextPrimary);
    D2D1_COLOR_F selBg = ThemeManager::Instance().GetColor(ThemeTokenId::SelectedBackground);
    D2D1_COLOR_F muted = ThemeManager::Instance().GetColor(ThemeTokenId::TextSecondary);

    ctx.FillRect(m_bounds, bg);
    ctx.DrawRect(m_bounds, border, 1.0f);

    const Rect content = ContentRect();
    ctx.PushClip(content);

    const float charW = 7.8f;
    const float offsetW = 10.0f * charW;
    const float rowH = RowHeight();
    const int visible = (std::max)(1, static_cast<int>(content.height / rowH) + 1);
    const int selMin = (std::min)(m_caretNibble, m_selAnchorNibble);
    const int selMax = (std::max)(m_caretNibble, m_selAnchorNibble);
    const bool hasSel = selMin != selMax;

    std::string font = GetFontFamily();
    float fontH = GetFontSize();

    for (int i = 0; i < visible; ++i) {
        const int row = m_scrollRow + i;
        if (row >= RowCount() && !m_data.empty()) break;
        const float y = content.y + i * rowH;
        const int byteStart = row * m_bytesPerRow;

        char offsetBuf[16];
        snprintf(offsetBuf, sizeof(offsetBuf), "%08X  ", byteStart);
        ctx.DrawText(offsetBuf, Rect(content.x, y, offsetW, rowH), muted, font, fontH,
                     DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_NORMAL, false);

        std::string ascii;
        ascii.reserve(static_cast<size_t>(m_bytesPerRow));
        for (int b = 0; b < m_bytesPerRow; ++b) {
            const int bi = byteStart + b;
            const float hx = content.x + offsetW + b * 3.0f * charW;
            char hexBuf[4] = { ' ', ' ', ' ', '\0' };
            if (bi < ByteCount()) {
                const BYTE v = m_data[static_cast<size_t>(bi)];
                snprintf(hexBuf, sizeof(hexBuf), "%02X", v);
                const unsigned char c = v;
                ascii.push_back((c >= 32 && c < 127) ? static_cast<char>(c) : '.');
            } else {
                hexBuf[0] = ' ';
                hexBuf[1] = ' ';
                ascii.push_back(' ');
            }

            // Selection highlight over the two hex digits
            const int nib0 = bi * 2;
            const int nib1 = nib0 + 1;
            if (hasSel) {
                if (nib0 >= selMin && nib0 < selMax) {
                    ctx.FillRect(Rect(hx, y + 1.0f, charW, rowH - 2.0f), selBg);
                }
                if (nib1 >= selMin && nib1 < selMax) {
                    ctx.FillRect(Rect(hx + charW, y + 1.0f, charW, rowH - 2.0f), selBg);
                }
            }

            ctx.DrawText(hexBuf, Rect(hx, y, 2.0f * charW, rowH), text, font, fontH,
                         DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_NORMAL, false);
        }

        const float asciiX = content.x + offsetW + m_bytesPerRow * 3.0f * charW + charW;
        ctx.DrawText(ascii, Rect(asciiX, y, m_bytesPerRow * charW, rowH), text, font, fontH,
                     DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_NORMAL, false);
    }

    // Caret
    if (m_isFocused && m_caretBlinkPhase && !hasSel) {
        const int byteIndex = m_caretNibble / 2;
        const int nibbleInByte = m_caretNibble % 2;
        const int row = (m_bytesPerRow > 0) ? byteIndex / m_bytesPerRow : 0;
        const int b = byteIndex % m_bytesPerRow;
        if (row >= m_scrollRow && row < m_scrollRow + visible) {
            const float y = content.y + (row - m_scrollRow) * rowH;
            const float hx = content.x + offsetW + b * 3.0f * charW + nibbleInByte * charW;
            ctx.FillRect(Rect(hx, y + 2.0f, 1.5f, rowH - 4.0f), text);
        }
    }

    // Simple scrollbar thumb
    if (MaxScroll() > 0) {
        const float trackH = content.height;
        const float thumbH = (std::max)(24.0f, trackH * (content.height / (RowCount() * rowH)));
        const float t = (MaxScroll() > 0) ? (static_cast<float>(m_scrollRow) / MaxScroll()) : 0.0f;
        const float thumbY = content.y + t * (trackH - thumbH);
        ctx.FillRect(Rect(m_bounds.x + m_bounds.width - 6.0f, thumbY, 4.0f, thumbH),
                     D2D1::ColorF(0.6f, 0.6f, 0.6f, 0.55f));
    }

    ctx.PopClip();
}

bool HexEditor::OnAnimationTick() {
    bool base = Control::OnAnimationTick();
    m_caretBlinkDirty = false;
    if (m_isFocused && IsEnabled()) {
        const ULONGLONG nowMs = GetTickCount64();
        const bool phase = ((nowMs / 500ULL) % 2ULL) == 0ULL;
        if (phase != m_caretBlinkPhase) {
            m_caretBlinkPhase = phase;
            m_caretBlinkDirty = true;
            NotifyDirty();
        }
        if (AnimationManager* mgr = AnimationManager::Current()) {
            mgr->RequestWake(this, AnimationManager::clock::now() + std::chrono::milliseconds(500));
        }
    }
    return base || m_caretBlinkDirty;
}

bool HexEditor::HasSelfAnimation() const {
    return Control::HasSelfAnimation() || m_caretBlinkDirty;
}

void HexEditor::OnFocus() {
    Control::OnFocus();
    m_caretBlinkPhase = true;
    NotifyDirty();
}

void HexEditor::OnBlur() {
    Control::OnBlur();
    NotifyDirty();
}

void HexEditor::OnMouseDown(Point pt) {
    if (!IsEnabled()) return;
    Control::OnMouseDown(pt);
    OnFocus();
    int nib = HitTestNibble(pt.x, pt.y);
    m_caretNibble = nib;
    m_selAnchorNibble = nib;
    m_dragging = true;
    EnsureCaretVisible();
    NotifyDirty();
}

void HexEditor::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);
    if (!m_dragging || !m_isPressed) return;
    int nib = HitTestNibble(pt.x, pt.y);
    if (nib == m_caretNibble) return;
    m_caretNibble = nib;
    EnsureCaretVisible();
    NotifyDirty();
}

void HexEditor::OnMouseUp(Point pt) {
    Control::OnMouseUp(pt);
    m_dragging = false;
}

void HexEditor::OnMouseWheel(float delta) {
    const int steps = (delta > 0.0f) ? -3 : 3;
    m_scrollRow += steps;
    ClampScroll();
    NotifyDirty();
}

void HexEditor::InsertNibble(int value) {
    value &= 0xF;
    const int byteIndex = m_caretNibble / 2;
    const bool high = (m_caretNibble % 2) == 0;

    if (byteIndex >= ByteCount()) {
        m_data.push_back(0);
    }
    BYTE& b = m_data[static_cast<size_t>(byteIndex)];
    if (high) {
        b = static_cast<BYTE>((value << 4) | (b & 0x0F));
    } else {
        b = static_cast<BYTE>((b & 0xF0) | value);
    }
    m_caretNibble = (std::min)(m_caretNibble + 1, ByteCount() * 2);
    m_selAnchorNibble = m_caretNibble;
    EnsureCaretVisible();
    NotifyDirty();
}

void HexEditor::DeleteSelectionOrByte(bool forward) {
    int selMin = (std::min)(m_caretNibble, m_selAnchorNibble);
    int selMax = (std::max)(m_caretNibble, m_selAnchorNibble);
    if (selMin != selMax) {
        int b0 = selMin / 2;
        int b1 = (selMax + 1) / 2;
        if (b0 < b1 && b0 < ByteCount()) {
            b1 = (std::min)(b1, ByteCount());
            m_data.erase(m_data.begin() + b0, m_data.begin() + b1);
        }
        m_caretNibble = b0 * 2;
        m_selAnchorNibble = m_caretNibble;
    } else if (forward) {
        int bi = m_caretNibble / 2;
        if (bi < ByteCount()) {
            m_data.erase(m_data.begin() + bi);
        }
    } else {
        if (m_caretNibble > 0) {
            int bi = (m_caretNibble - 1) / 2;
            if (bi < ByteCount()) {
                m_data.erase(m_data.begin() + bi);
            }
            m_caretNibble = bi * 2;
            m_selAnchorNibble = m_caretNibble;
        }
    }
    EnsureCaretVisible();
    NotifyDirty();
}

void HexEditor::OnKeyDown(int vkCode) {
    if (!IsEnabled()) return;
    const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

    auto moveCaret = [&](int nib) {
        nib = std::clamp(nib, 0, ByteCount() * 2);
        m_caretNibble = nib;
        if (!shift) m_selAnchorNibble = m_caretNibble;
        EnsureCaretVisible();
        NotifyDirty();
    };

    switch (vkCode) {
        case VK_LEFT:  moveCaret(m_caretNibble - 1); break;
        case VK_RIGHT: moveCaret(m_caretNibble + 1); break;
        case VK_UP:    moveCaret(m_caretNibble - m_bytesPerRow * 2); break;
        case VK_DOWN:  moveCaret(m_caretNibble + m_bytesPerRow * 2); break;
        case VK_HOME:  moveCaret((m_caretNibble / 2 / m_bytesPerRow) * m_bytesPerRow * 2); break;
        case VK_END: {
            int rowStart = (m_caretNibble / 2 / m_bytesPerRow) * m_bytesPerRow;
            int rowEnd = (std::min)(rowStart + m_bytesPerRow, ByteCount());
            moveCaret(rowEnd * 2);
            break;
        }
        case VK_PRIOR: m_scrollRow -= 8; ClampScroll(); NotifyDirty(); break;
        case VK_NEXT:  m_scrollRow += 8; ClampScroll(); NotifyDirty(); break;
        case VK_DELETE: DeleteSelectionOrByte(true); break;
        case VK_BACK:   DeleteSelectionOrByte(false); break;
        default: break;
    }
}

void HexEditor::OnCharInput(wchar_t ch) {
    if (!IsEnabled()) return;
    int v = -1;
    if (ch >= L'0' && ch <= L'9') v = ch - L'0';
    else if (ch >= L'a' && ch <= L'f') v = 10 + (ch - L'a');
    else if (ch >= L'A' && ch <= L'F') v = 10 + (ch - L'A');
    if (v >= 0) InsertNibble(v);
}

} // namespace RegeditPlus
