#include "CheckBox.h"

namespace CUI {

CheckBox::CheckBox() {
    SetProperty("text", Value("CheckBox"));
    SetProperty("checkState", Value("Unchecked"));
    SetProperty("isThreeState", Value(false));
    SetProperty("background", Value(D2D1::ColorF(0x3C / 255.0f, 0x3C / 255.0f, 0x3C / 255.0f, 1.0f)));
    SetProperty("checkedBackground", Value(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f)));
    SetProperty("color", Value(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f)));
    SetProperty("fontSize", Value(13.0f));
    SetProperty("fontFamily", Value("Segoe UI"));
    SetProperty("padding", Value(Thickness(4, 4, 4, 4)));
    SetProperty("cornerRadius", Value(3.0f));
}

CheckBox::CheckBox(const std::string& text) : CheckBox() {
    SetProperty("text", Value(text));
}

CheckState CheckBox::GetState() const {
    std::string s = GetProperty("checkState").AsString("Unchecked");
    if (s == "Checked") return CheckState::Checked;
    if (s == "Indeterminate") return CheckState::Indeterminate;
    return CheckState::Unchecked;
}

void CheckBox::SetState(CheckState state) {
    std::string s = "Unchecked";
    if (state == CheckState::Checked) s = "Checked";
    else if (state == CheckState::Indeterminate) s = "Indeterminate";

    SetProperty("checkState", Value(s));
    m_onCheckStateChangedEvent.Invoke(this, state);
}

Size CheckBox::Measure(Size availableSize) {
    std::string text = GetProperty("text").AsString("");
    std::string font = GetProperty("fontFamily").AsString("Segoe UI");
    float fontSize = GetProperty("fontSize").AsFloat(13.0f);

    GraphicsContext ctx;
    Size textSize = ctx.MeasureText(text, font, fontSize);

    Thickness margin = GetProperty("margin").AsThickness(Thickness(0));
    Thickness padding = GetProperty("padding").AsThickness(Thickness(0));

    float boxW = 16.0f;
    float gap = 8.0f;

    float w = boxW + gap + textSize.width + margin.left + margin.right + padding.left + padding.right;
    float h = (std::max)(16.0f, textSize.height) + margin.top + margin.bottom + padding.top + padding.bottom;

    float expW = GetProperty("width").AsFloat(-1.0f);
    float expH = GetProperty("height").AsFloat(-1.0f);

    if (expW >= 0.0f) w = expW;
    if (expH >= 0.0f) h = expH;

    m_desiredSize = Size(w, h);
    return m_desiredSize;
}

void CheckBox::OnRender(GraphicsContext& ctx) {
    Thickness padding = GetProperty("padding").AsThickness(Thickness(0));
    float radius = GetProperty("cornerRadius").AsFloat(4.0f);

    CheckState state = GetState();

    float boxSize = 18.0f;
    float boxY = m_bounds.y + (m_bounds.height - boxSize) / 2.0f;
    Rect boxRect(m_bounds.x + padding.left, boxY, boxSize, boxSize);

    D2D1_COLOR_F accentBlue = D2D1::ColorF(0x4C / 255.0f, 0xC2 / 255.0f, 0xFF / 255.0f, 1.0f); // #4CC2FF WinUI Fluent Accent Blue
    D2D1_COLOR_F checkedIconColor = D2D1::ColorF(0x00 / 255.0f, 0x00 / 255.0f, 0x00 / 255.0f, 1.0f); // Black check mark

    if (state == CheckState::Checked || state == CheckState::Indeterminate) {
        // Draw Vibrant Fluent Cyan-Blue Rounded Box
        ctx.FillRoundedRect(boxRect, radius, accentBlue);

        if (state == CheckState::Checked) {
            // Draw Anti-Aliased Vector Checkmark "✓" (2 connected line segments)
            Point p1(boxRect.x + 4.5f, boxRect.y + 9.5f);
            Point p2(boxRect.x + 7.5f, boxRect.y + 12.5f);
            Point p3(boxRect.x + 13.5f, boxRect.y + 5.5f);

            ctx.DrawLine(p1, p2, checkedIconColor, 1.8f);
            ctx.DrawLine(p2, p3, checkedIconColor, 1.8f);
        } else if (state == CheckState::Indeterminate) {
            // Draw Indeterminate "-" solid center rect
            Rect barRect(boxRect.x + 4.5f, boxRect.y + 7.5f, 9.0f, 3.0f);
            ctx.FillRoundedRect(barRect, 1.0f, checkedIconColor);
        }
    } else {
        // Unchecked state: Clean dark background with light gray rounded border (#999999)
        D2D1_COLOR_F bg = GetProperty("background").AsColor(D2D1::ColorF(0x20 / 255.0f, 0x20 / 255.0f, 0x20 / 255.0f, 1.0f));
        D2D1_COLOR_F border = m_isHovered
            ? D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.9f)
            : D2D1::ColorF(0x8E / 255.0f, 0x8E / 255.0f, 0x8E / 255.0f, 0.8f);

        ctx.FillRoundedRect(boxRect, radius, bg);
        ctx.DrawRoundedRect(boxRect, radius, border, 1.2f);
    }

    // Draw Label Text
    std::string text = GetProperty("text").AsString("");
    if (!text.empty()) {
        float textX = boxRect.x + boxSize + 10.0f;
        Rect textRect(textX, m_bounds.y, m_bounds.width - (textX - m_bounds.x), m_bounds.height);

        D2D1_COLOR_F textColor = GetProperty("color").AsColor(D2D1::ColorF(1.0f, 1.0f, 1.0f));
        std::string font = GetProperty("fontFamily").AsString("Segoe UI");
        float fontSize = GetProperty("fontSize").AsFloat(13.0f);

        ctx.DrawText(text, textRect, textColor, font, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
}

void CheckBox::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);

    CheckState currentState = GetState();
    bool threeState = IsThreeState();

    CheckState newState = CheckState::Unchecked;
    if (currentState == CheckState::Unchecked) {
        newState = CheckState::Checked;
    } else if (currentState == CheckState::Checked) {
        newState = threeState ? CheckState::Indeterminate : CheckState::Unchecked;
    } else {
        newState = CheckState::Unchecked;
    }

    SetState(newState);
}

} // namespace CUI
