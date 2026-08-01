#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "RadioButton.h"
#include <algorithm>
#include <cmath>

namespace CUI {
namespace {
float EaseLine(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return 1.0f - std::pow(1.0f - t, 2.4f);
}

}

RadioButton::RadioButton() : CheckBox("RadioButton") {
    SetProperty("groupName", Value("DefaultGroup"));
    SetProperty("background", Value(D2D1::ColorF(0x20 / 255.0f, 0x20 / 255.0f, 0x20 / 255.0f, 1.0f)));
    SetProperty("accentColor", Value(D2D1::ColorF(0x51 / 255.0f, 0xA8 / 255.0f, 0xF7 / 255.0f, 1.0f)));
}

RadioButton::RadioButton(const std::string& text) : CheckBox(text) {
    SetProperty("groupName", Value("DefaultGroup"));
    SetProperty("background", Value(D2D1::ColorF(0x20 / 255.0f, 0x20 / 255.0f, 0x20 / 255.0f, 1.0f)));
    SetProperty("accentColor", Value(D2D1::ColorF(0x51 / 255.0f, 0xA8 / 255.0f, 0xF7 / 255.0f, 1.0f)));
}

std::vector<PropertyMeta> RadioButton::GetPropertyMetas() const {
    auto metas = CheckBox::GetPropertyMetas();
    metas.push_back({ "groupName", "互斥分组 (GroupName)", "单选配置", "string" });
    return metas;
}

void RadioButton::OnMouseDown(Point pt) {
    if (!IsEnabled()) return;
    Control::OnMouseDown(pt);
}

void RadioButton::UncheckSiblingsInGroup() {
    std::string myGroup = GetGroupName();
    if (myGroup.empty()) return;

    UIElement* parent = GetParent();
    if (!parent) return;

    for (const auto& child : parent->GetChildren()) {
        auto* sibling = dynamic_cast<RadioButton*>(child.get());
        if (!sibling || sibling == this) {
            continue;
        }
        if (sibling->GetGroupName() == myGroup) {
            sibling->SetChecked(false);
        }
    }
}

void RadioButton::OnMouseUp(Point pt) {
    if (!IsEnabled()) return;
    if (m_isPressed && m_bounds.Contains(pt.x, pt.y)) {
        m_isPressed = false;
        if (GetState() != CheckState::Checked) {
            UncheckSiblingsInGroup();
            SetChecked(true);
        }
        OnClick().Invoke(this);
    } else {
        m_isPressed = false;
    }
}

void RadioButton::OnRender(GraphicsContext& ctx) {
    Thickness padding = GetProperty("padding").AsThickness(Thickness(4, 4, 4, 4));
    float size = 18.0f;
    Rect checkRect(m_bounds.x + padding.left, m_bounds.y + (m_bounds.height - size) * 0.5f, size, size);

    D2D1_COLOR_F accent = GetProperty("accentColor").AsColor(D2D1::ColorF(0x51 / 255.0f, 0xA8 / 255.0f, 0xF7 / 255.0f, 1.0f));
    D2D1_COLOR_F border = BlendColor(
        D2D1::ColorF(0x8E / 255.0f, 0x8E / 255.0f, 0x8E / 255.0f, 0.85f),
        accent,
        (std::min)(1.0f, m_visualStateAnim.Current() / 0.55f)
    );
    D2D1_COLOR_F bg = GetAnimatedBackground(GetProperty("background").AsColor(D2D1::ColorF(0x20 / 255.0f, 0x20 / 255.0f, 0x20 / 255.0f, 1.0f)));

    ctx.FillRoundedRect(checkRect, size * 0.5f, bg);
    ctx.DrawRoundedRect(checkRect, size * 0.5f, border, 1.4f);

    float dotFactor = std::clamp(m_selectionAnim.Current(), 0.0f, 1.0f);
    if (dotFactor > 0.01f) {
        float eased = EaseLine(dotFactor);
        float maxDiameter = 8.0f;
        float diameter = maxDiameter * eased;
        float dotX = checkRect.x + (checkRect.width - diameter) * 0.5f;
        float dotY = checkRect.y + (checkRect.height - diameter) * 0.5f;
        ctx.FillRoundedRect(Rect(dotX, dotY, diameter, diameter), diameter * 0.5f, accent);
    }

    std::string txt = GetProperty("text").AsString("");
    if (!txt.empty()) {
        float fontSize = GetProperty("fontSize").AsFloat(13.0f);
        std::string fontFamily = GetProperty("fontFamily").AsString("Segoe UI");
        D2D1_COLOR_F color = GetProperty("color").AsColor(D2D1::ColorF(0xCC / 255.0f, 0xCC / 255.0f, 0xCC / 255.0f, 1.0f));

        float textX = checkRect.x + size + 10.0f;
        Rect textRect(textX, m_bounds.y, (std::max)(0.0f, m_bounds.width - (textX - m_bounds.x)), m_bounds.height);
        ctx.DrawText(txt, textRect, color, fontFamily, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
}

bool RadioButton::OnAnimationTick() {
    bool base = CheckBox::OnAnimationTick();
    bool animating = base;

    float target = GetState() == CheckState::Checked ? 1.0f : 0.0f;
    m_selectionAnim.SetTarget(target);
    animating = m_selectionAnim.Tick(UIElement::GetAnimationDeltaSeconds(), AnimationSpec{ 0.18f, 0.01f }) || animating;

    return animating;
}

bool RadioButton::HasSelfAnimation() const {
    float target = GetState() == CheckState::Checked ? 1.0f : 0.0f;
    return CheckBox::HasSelfAnimation() || std::abs(target - m_selectionAnim.Current()) > 0.01f;
}

void RadioButton::SetChecked(bool checked) {
    SetState(checked ? CheckState::Checked : CheckState::Unchecked);
}

} // namespace CUI
