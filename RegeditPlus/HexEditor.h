#pragma once

#include "framework/controls/Control.h"
#include <vector>
#include <functional>
#include <cstdint>

namespace RegeditPlus {

// Virtualized hex dump editor (offset | hex bytes | ASCII), similar to regedit.exe.
class HexEditor : public CUI::Control {
public:
    HexEditor();
    const char* GetClassName() const override { return "HexEditor"; }

    void SetBytes(std::vector<BYTE> data);
    const std::vector<BYTE>& GetBytes() const { return m_data; }

    CUI::Size Measure(CUI::Size availableSize) override;
    void OnRender(CUI::GraphicsContext& ctx) override;
    bool OnAnimationTick() override;
    bool HasSelfAnimation() const override;

    void OnMouseDown(CUI::Point pt) override;
    void OnMouseMove(CUI::Point pt) override;
    void OnMouseUp(CUI::Point pt) override;
    void OnMouseWheel(float delta) override;
    void OnKeyDown(int vkCode) override;
    void OnCharInput(wchar_t ch) override;
    void OnFocus() override;
    void OnBlur() override;

    void SetBytesPerRow(int n) { m_bytesPerRow = (std::max)(1, n); MarkRenderContentDirty(); }
    int GetBytesPerRow() const { return m_bytesPerRow; }

private:
    int ByteCount() const { return static_cast<int>(m_data.size()); }
    int RowCount() const;
    int MaxScroll() const;
    CUI::Rect ContentRect() const;
    float RowHeight() const { return 18.0f; }
    int HitTestNibble(float x, float y) const; // returns nibble index (byte*2 + hi/lo), or -1
    void EnsureCaretVisible();
    void ClampScroll();
    void NotifyDirty();
    void InsertNibble(int value);
    void DeleteSelectionOrByte(bool forward);

    std::vector<BYTE> m_data;
    int m_bytesPerRow = 8;
    int m_scrollRow = 0;
    int m_caretNibble = 0;      // absolute nibble index
    int m_selAnchorNibble = 0;
    bool m_dragging = false;
    bool m_caretBlinkPhase = true;
    bool m_caretBlinkDirty = false;
};

} // namespace RegeditPlus
