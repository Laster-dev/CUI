#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "RadioButton.h"

namespace CUI {

RadioButton::RadioButton() : CheckBox("RadioButton") {
    SetProperty("groupName", Value("DefaultGroup"));
}

RadioButton::RadioButton(const std::string& text) : CheckBox(text) {
    SetProperty("groupName", Value("DefaultGroup"));
}

std::vector<PropertyMeta> RadioButton::GetPropertyMetas() const {
    auto metas = CheckBox::GetPropertyMetas();
    metas.push_back({ "groupName", "互斥分组 (GroupName)", "单选配置", "string" });
    return metas;
}

void RadioButton::UncheckSiblingsInGroup() {
    if (!m_parent) return;
    std::string myGroup = GetGroupName();
    if (myGroup.empty()) return;

    for (const auto& child : m_parent->GetChildren()) {
        if (child.get() != this) {
            if (auto rb = dynamic_cast<RadioButton*>(child.get())) {
                if (rb->GetGroupName() == myGroup) {
                    rb->SetState(CheckState::Unchecked);
                }
            }
        }
    }
}

void RadioButton::OnMouseUp(Point pt) {
    if (!IsEnabled()) return;
    if (m_isPressed && m_bounds.Contains(pt.x, pt.y)) {
        m_isPressed = false;
        if (GetState() != CheckState::Checked) {
            UncheckSiblingsInGroup();
            SetState(CheckState::Checked);
        }
        OnClick().Invoke(this);
    } else {
        m_isPressed = false;
    }
}

void RadioButton::OnRender(GraphicsContext& ctx) {
    float size = 16.0f;
    Rect checkRect(m_bounds.x, m_bounds.y + (m_bounds.height - size) * 0.5f, size, size);

    D2D1_COLOR_F border = GetProperty("borderBrush").AsColor(D2D1::ColorF(0x66 / 255.0f, 0x66 / 255.0f, 0x66 / 255.0f, 1.0f));
    D2D1_COLOR_F bg = GetProperty("background").AsColor(D2D1::ColorF(0x1E / 255.0f, 0x1E / 255.0f, 0x1E / 255.0f, 1.0f));
    D2D1_COLOR_F accent = GetProperty("accentColor").AsColor(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f));

    // Outer Circle
    ctx.FillRoundedRect(checkRect, size * 0.5f, bg);
    ctx.DrawRoundedRect(checkRect, size * 0.5f, m_isHovered ? accent : border, 1.5f);

    // Inner Circle if checked
    if (GetState() == CheckState::Checked) {
        float innerR = 4.0f;
        Rect innerRect(checkRect.x + 4.0f, checkRect.y + 4.0f, innerR * 2.0f, innerR * 2.0f);
        ctx.FillRoundedRect(innerRect, innerR, accent);
    }

    // Text Header
    std::string txt = GetProperty("text").AsString("");
    if (!txt.empty()) {
        float fontSize = GetProperty("fontSize").AsFloat(13.0f);
        std::string fontFamily = GetProperty("fontFamily").AsString("Segoe UI");
        D2D1_COLOR_F color = GetProperty("color").AsColor(D2D1::ColorF(0xCC / 255.0f, 0xCC / 255.0f, 0xCC / 255.0f, 1.0f));

        Rect textRect(m_bounds.x + size + 8.0f, m_bounds.y, (std::max)(0.0f, m_bounds.width - size - 8.0f), m_bounds.height);
        ctx.DrawText(txt, textRect, color, fontFamily, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
}

} // namespace CUI
