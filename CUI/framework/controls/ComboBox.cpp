#include "ComboBox.h"
#include <sstream>

namespace CUI {

ComboBox::ComboBox() {
    SetProperty("placeholder", Value("Select option..."));
    SetProperty("background", Value(D2D1::ColorF(0x3C / 255.0f, 0x3C / 255.0f, 0x3C / 255.0f, 1.0f)));
    SetProperty("hoverBackground", Value(D2D1::ColorF(0x48 / 255.0f, 0x48 / 255.0f, 0x48 / 255.0f, 1.0f)));
    SetProperty("borderBrush", Value(D2D1::ColorF(0x33 / 255.0f, 0x33 / 255.0f, 0x33 / 255.0f, 1.0f)));
    SetProperty("focusedBorderBrush", Value(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f)));
    SetProperty("borderThickness", Value(1.0f));
    SetProperty("color", Value(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f)));
    SetProperty("fontSize", Value(13.0f));
    SetProperty("fontFamily", Value("Segoe UI"));
    SetProperty("padding", Value(Thickness(10, 6, 10, 6)));
    SetProperty("cornerRadius", Value(3.0f));
    SetProperty("width", Value(200.0f));
    SetProperty("height", Value(32.0f));
}

void ComboBox::AddItem(const std::string& item) {
    m_items.push_back(item);
    if (m_selectedIndex == -1) {
        SetSelectedIndex(0);
    }
}

void ComboBox::ClearItems() {
    m_items.clear();
    m_selectedIndex = -1;
}

void ComboBox::SetSelectedIndex(int index) {
    if (index >= 0 && index < static_cast<int>(m_items.size())) {
        if (m_selectedIndex != index) {
            m_selectedIndex = index;
            m_onSelectionChangedEvent.Invoke(this, m_selectedIndex, m_items[m_selectedIndex]);
        }
    }
}

std::string ComboBox::GetSelectedItem() const {
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_items.size())) {
        return m_items[m_selectedIndex];
    }
    return "";
}

Size ComboBox::Measure(Size availableSize) {
    float expW = GetProperty("width").AsFloat(200.0f);
    float expH = GetProperty("height").AsFloat(32.0f);
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

void ComboBox::OnRender(GraphicsContext& ctx) {
    std::string itemsProp = GetProperty("items").AsString("");
    if (!itemsProp.empty() && m_items.empty()) {
        std::stringstream ss(itemsProp);
        std::string item;
        while (std::getline(ss, item, ',')) {
            if (!item.empty()) AddItem(item);
        }
    }

    float radius = GetProperty("cornerRadius").AsFloat(3.0f);
    D2D1_COLOR_F bg = GetProperty("background").AsColor(D2D1::ColorF(0x3C / 255.0f, 0x3C / 255.0f, 0x3C / 255.0f, 1.0f));
    if (m_isHovered || m_isDropDownOpen) {
        bg = GetProperty("hoverBackground").AsColor(bg);
    }

    if (radius > 0.0f) {
        ctx.FillRoundedRect(m_bounds, radius, bg);
    } else {
        ctx.FillRect(m_bounds, bg);
    }

    D2D1_COLOR_F borderBrush = (m_isFocused || m_isDropDownOpen)
        ? GetProperty("focusedBorderBrush").AsColor(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f))
        : GetProperty("borderBrush").AsColor(D2D1::ColorF(0x33 / 255.0f, 0x33 / 255.0f, 0x33 / 255.0f, 1.0f));

    float borderThickness = GetProperty("borderThickness").AsFloat(1.0f);
    if (borderThickness > 0.0f) {
        if (radius > 0.0f) {
            ctx.DrawRoundedRect(m_bounds, radius, borderBrush, (m_isFocused || m_isDropDownOpen) ? 1.5f : borderThickness);
        } else {
            ctx.DrawRect(m_bounds, borderBrush, (m_isFocused || m_isDropDownOpen) ? 1.5f : borderThickness);
        }
    }

    Thickness padding = GetProperty("padding").AsThickness(Thickness(10, 6, 10, 6));

    // Render current text
    std::string displayText = GetSelectedItem();
    if (displayText.empty()) {
        displayText = GetProperty("placeholder").AsString("Select option...");
    }

    D2D1_COLOR_F textColor = GetProperty("color").AsColor(D2D1::ColorF(1.0f, 1.0f, 1.0f));
    std::string font = GetProperty("fontFamily").AsString("Segoe UI");
    float fontSize = GetProperty("fontSize").AsFloat(13.0f);

    Rect textRect(m_bounds.x + padding.left, m_bounds.y + padding.top, m_bounds.width - padding.left - padding.right - 20.0f, m_bounds.height - padding.top - padding.bottom);
    ctx.DrawText(displayText, textRect, textColor, font, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Render down arrow "v" icon
    Rect arrowRect(m_bounds.x + m_bounds.width - 24.0f, m_bounds.y, 20.0f, m_bounds.height);
    ctx.DrawText("v", arrowRect, D2D1::ColorF(0xCC / 255.0f, 0xCC / 255.0f, 0xCC / 255.0f), font, 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

void ComboBox::OnRenderOverlay(GraphicsContext& ctx) {
    if (!m_isDropDownOpen || m_items.empty()) return;

    std::string font = GetProperty("fontFamily").AsString("Segoe UI");
    float fontSize = GetProperty("fontSize").AsFloat(13.0f);

    float itemHeight = 28.0f;
    float menuH = itemHeight * m_items.size();
    Rect menuRect(m_bounds.x, m_bounds.y + m_bounds.height + 2.0f, m_bounds.width, menuH);

    // Draw dropdown popup shadow & background (Topmost Overlay Pass)
    ctx.FillRoundedRect(menuRect, 4.0f, D2D1::ColorF(0x25 / 255.0f, 0x25 / 255.0f, 0x26 / 255.0f, 1.0f));
    ctx.DrawRoundedRect(menuRect, 4.0f, D2D1::ColorF(0x45 / 255.0f, 0x45 / 255.0f, 0x45 / 255.0f, 1.0f), 1.0f);

    for (size_t i = 0; i < m_items.size(); ++i) {
        Rect itemRect(menuRect.x + 2, menuRect.y + i * itemHeight + 2, menuRect.width - 4, itemHeight - 4);
        bool isSelected = (static_cast<int>(i) == m_selectedIndex);

        if (isSelected) {
            ctx.FillRoundedRect(itemRect, 2.0f, D2D1::ColorF(0x04 / 255.0f, 0x39 / 255.0f, 0x61 / 255.0f));
        }

        D2D1_COLOR_F itemColor = isSelected ? D2D1::ColorF(1.0f, 1.0f, 1.0f) : D2D1::ColorF(0xCC / 255.0f, 0xCC / 255.0f, 0xCC / 255.0f);
        ctx.DrawText(m_items[i], Rect(itemRect.x + 8, itemRect.y, itemRect.width - 16, itemRect.height), itemColor, font, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
}

UIElement* ComboBox::HitTestOverlay(float x, float y) {
    if (!m_isDropDownOpen || m_items.empty()) return nullptr;

    float itemHeight = 28.0f;
    float menuH = itemHeight * m_items.size();
    Rect menuRect(m_bounds.x, m_bounds.y + m_bounds.height + 2.0f, m_bounds.width, menuH);

    if (menuRect.Contains(x, y)) {
        return this;
    }
    return nullptr;
}

UIElement* ComboBox::HitTest(float x, float y) {
    std::string visStr = GetProperty("visibility").AsString("Visible");
    if (visStr != "Visible") return nullptr;

    if (m_bounds.Contains(x, y)) {
        return this;
    }

    return nullptr;
}

void ComboBox::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);

    if (m_isDropDownOpen && !m_items.empty()) {
        float itemHeight = 28.0f;
        float menuH = itemHeight * m_items.size();
        Rect menuRect(m_bounds.x, m_bounds.y + m_bounds.height + 2.0f, m_bounds.width, menuH);

        if (menuRect.Contains(pt.x, pt.y)) {
            int clickedIdx = static_cast<int>((pt.y - menuRect.y) / itemHeight);
            if (clickedIdx >= 0 && clickedIdx < static_cast<int>(m_items.size())) {
                SetSelectedIndex(clickedIdx);
            }
        }
        m_isDropDownOpen = false;
    } else {
        m_isDropDownOpen = true;
    }
}

void ComboBox::OnBlur() {
    Control::OnBlur();
    m_isDropDownOpen = false;
}

} // namespace CUI
