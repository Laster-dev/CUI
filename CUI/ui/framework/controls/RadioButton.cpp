#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "RadioButton.h"
#include "../style/ThemeManager.h"
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
    SetProperty("theme.backgroundToken", Value("inputBackground"));
    SetProperty("theme.accentColorToken", Value("accentColor"));
    SetProperty("theme.colorToken", Value("textSecondary"));
    SetProperty("background", Value(ThemeManager::Instance().GetColor("inputBackground")));
    SetProperty("accentColor", Value(ThemeManager::Instance().GetColor("accentColor")));
}

RadioButton::RadioButton(const std::string& text) : CheckBox(text) {
    SetProperty("groupName", Value("DefaultGroup"));
    SetProperty("theme.backgroundToken", Value("inputBackground"));
    SetProperty("theme.accentColorToken", Value("accentColor"));
    SetProperty("theme.colorToken", Value("textSecondary"));
    SetProperty("background", Value(ThemeManager::Instance().GetColor("inputBackground")));
    SetProperty("accentColor", Value(ThemeManager::Instance().GetColor("accentColor")));
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

    UIElement* searchContainer = GetParent();
    while (searchContainer) {
        bool foundAny = false;
        std::vector<UIElement*> queue;
        queue.push_back(searchContainer);

        size_t head = 0;
        while (head < queue.size()) {
            UIElement* curr = queue[head++];
            for (const auto& child : curr->GetChildren()) {
                if (!child) continue;
                auto* sibling = dynamic_cast<RadioButton*>(child.get());
                if (sibling && sibling != this && sibling->GetGroupName() == myGroup) {
                    sibling->SetChecked(false);
                    sibling->MarkRenderContentDirty();
                    foundAny = true;
                } else {
                    queue.push_back(child.get());
                }
            }
        }

        if (foundAny || !searchContainer->GetParent()) break;
        searchContainer = searchContainer->GetParent();
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
    float target = GetState() == CheckState::Checked ? 1.0f : 0.0f;
    m_selectionAnim.SetTarget(target);

    if (!UIElement::AreAnimationsEnabled()) {
        m_selectionAnim.Reset(target);
        UpdateVisualStateTarget();
        m_visualStateAnim.Reset(m_visualStateTarget);
    }

    Thickness padding = GetProperty("padding").AsThickness(Thickness(4, 4, 4, 4));
    float size = 18.0f;
    Rect checkRect(m_bounds.x + padding.left, m_bounds.y + (m_bounds.height - size) * 0.5f, size, size);

    D2D1_COLOR_F accent = ResolveThemeColor("theme.accentColorToken", "accentColor");
    D2D1_COLOR_F defaultBorder = ThemeManager::Instance().GetColor("inputBorder");
    defaultBorder.a = 0.85f;
    D2D1_COLOR_F hoverBorder = ThemeManager::Instance().GetColor("textPrimary");

    float selectionProgress = std::clamp(m_selectionAnim.Current(), 0.0f, 1.0f);
    float visualProgress = m_visualStateAnim.Current();

    D2D1_COLOR_F borderUnchecked = BlendColor(defaultBorder, hoverBorder, (std::min)(1.0f, visualProgress / 0.55f));
    D2D1_COLOR_F border = BlendColor(borderUnchecked, accent, selectionProgress);
    D2D1_COLOR_F bg = GetAnimatedBackground(ResolveThemeColor("theme.backgroundToken", "inputBackground"));

    ctx.FillRoundedRect(checkRect, size * 0.5f, bg);
    ctx.DrawRoundedRect(checkRect, size * 0.5f, border, 1.4f);

    if (selectionProgress > 0.01f) {
        float eased = UIElement::AreAnimationsEnabled() ? EaseLine(selectionProgress) : selectionProgress;
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
        D2D1_COLOR_F color = ResolveThemeColor("theme.colorToken", "textSecondary");

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
    if (!UIElement::AreAnimationsEnabled()) {
        m_selectionAnim.Reset(target);
        return base;
    }
    animating = m_selectionAnim.Tick(UIElement::GetAnimationDeltaSeconds(), AnimationSpec{ 0.18f, 0.01f }) || animating;

    return animating;
}

bool RadioButton::HasSelfAnimation() const {
    float target = GetState() == CheckState::Checked ? 1.0f : 0.0f;
    return CheckBox::HasSelfAnimation() || std::abs(target - m_selectionAnim.Current()) > 0.01f;
}

void RadioButton::SetChecked(bool checked) {
    SetState(checked ? CheckState::Checked : CheckState::Unchecked);
    float target = checked ? 1.0f : 0.0f;
    m_selectionAnim.SetTarget(target);
    if (!UIElement::AreAnimationsEnabled()) {
        m_selectionAnim.Reset(target);
    }
}

} // namespace CUI
