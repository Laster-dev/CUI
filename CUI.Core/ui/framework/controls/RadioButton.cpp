#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "RadioButton.h"
#include "../window/Window.h"
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
    SetBackgroundToken(ThemeTokenId::InputBackground);
    SetAccentColorToken(ThemeTokenId::AccentColor);
    SetColorToken(ThemeTokenId::TextSecondary);
    SetBackground(ThemeManager::Instance().GetColor("inputBackground"));
}

RadioButton::RadioButton(const std::string& text) : CheckBox(text) {
    SetBackgroundToken(ThemeTokenId::InputBackground);
    SetAccentColorToken(ThemeTokenId::AccentColor);
    SetColorToken(ThemeTokenId::TextSecondary);
    SetBackground(ThemeManager::Instance().GetColor("inputBackground"));
}

std::vector<PropertyMeta> RadioButton::GetPropertyMetas() const {
    auto metas = CheckBox::GetPropertyMetas();
    metas.push_back({ "groupName", "互斥分组 (GroupName)", "单选配置", "string" });
    return metas;
}

Value RadioButton::GetProperty(PropertyId id) const {
    if (id == PropertyId::GroupName) return Value(m_groupName);
    return CheckBox::GetProperty(id);
}

bool RadioButton::HasProperty(PropertyId id) const {
    return id == PropertyId::GroupName || CheckBox::HasProperty(id);
}

void RadioButton::SetProperty(PropertyId id, const Value& val) {
    if (id == PropertyId::GroupName) {
        SetGroupName(val.AsString());
        return;
    }
    CheckBox::SetProperty(id, val);
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
                    sibling->MarkRenderRectDirty(sibling->GetBounds());
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

bool RadioButton::OnKeyDown(int vkCode) {
    if (!IsEnabled()) {
        return false;
    }
    if (vkCode == VK_SPACE || vkCode == VK_RETURN) {
        if (GetState() != CheckState::Checked) {
            UncheckSiblingsInGroup();
            SetChecked(true);
        }
        ExecuteBoundCommand();
        OnClick().Invoke(this);
        return true;
    }
    if (vkCode == VK_LEFT || vkCode == VK_UP || vkCode == VK_RIGHT || vkCode == VK_DOWN) {
        std::vector<RadioButton*> group;
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
                    auto* radio = dynamic_cast<RadioButton*>(child.get());
                    if (radio && radio->GetGroupName() == GetGroupName()
                        && radio->IsEnabled() && radio->GetVisibility() == Visibility::Visible) {
                        group.push_back(radio);
                        foundAny = true;
                    } else {
                        queue.push_back(child.get());
                    }
                }
            }
            if (foundAny || !searchContainer->GetParent()) {
                break;
            }
            searchContainer = searchContainer->GetParent();
            group.clear();
        }
        if (group.size() < 2) {
            return true;
        }
        int index = 0;
        for (int i = 0; i < static_cast<int>(group.size()); ++i) {
            if (group[i] == this) {
                index = i;
                break;
            }
        }
        const bool forward = (vkCode == VK_RIGHT || vkCode == VK_DOWN);
        int next = forward
            ? (index + 1) % static_cast<int>(group.size())
            : (index <= 0 ? static_cast<int>(group.size()) - 1 : index - 1);
        RadioButton* target = group[next];
        target->UncheckSiblingsInGroup();
        target->SetChecked(true);
        if (Window* win = Window::Current()) {
            win->ApplyFocus(target, FocusState::Keyboard);
        } else {
            target->OnFocus();
        }
        return true;
    }
    return CheckBox::OnKeyDown(vkCode);
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

    Thickness padding = GetPadding();
    float size = 18.0f;
    Rect checkRect(m_bounds.x + padding.left, m_bounds.y + (m_bounds.height - size) * 0.5f, size, size);

    D2D1_COLOR_F accent = ResolveThemeColor(GetAccentColorToken(), ThemeTokenId::AccentColor);
    D2D1_COLOR_F defaultBorder = ThemeManager::Instance().GetColor(ThemeTokenId::InputBorder);
    defaultBorder.a = 0.85f;
    D2D1_COLOR_F hoverBorder = ThemeManager::Instance().GetColor(ThemeTokenId::TextPrimary);

    float selectionProgress = std::clamp(m_selectionAnim.Current(), 0.0f, 1.0f);
    float visualProgress = m_visualStateAnim.Current();

    D2D1_COLOR_F borderUnchecked = BlendColor(defaultBorder, hoverBorder, (std::min)(1.0f, visualProgress / 0.55f));
    D2D1_COLOR_F border = BlendColor(borderUnchecked, accent, selectionProgress);
    D2D1_COLOR_F bg = GetAnimatedBackground(ResolveThemeColor(GetBackgroundToken(), ThemeTokenId::InputBackground));

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

    std::string txt = GetText();
    if (!txt.empty()) {
        float fontSize = GetFontSize();
        std::string fontFamily = GetFontFamily();
        D2D1_COLOR_F color = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextSecondary);

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
    const CheckState next = checked ? CheckState::Checked : CheckState::Unchecked;
    const float target = checked ? 1.0f : 0.0f;
    const bool stateChanged = GetState() != next;
    const bool animSettled = std::abs(m_selectionAnim.Current() - target) <= 0.01f
        && !m_selectionAnim.IsAnimating(0.01f);
    SetState(next);
    if (!stateChanged && animSettled) {
        m_selectionAnim.Reset(target);
        return;
    }
    m_selectionAnim.SetTarget(target);
    if (!UIElement::AreAnimationsEnabled()) {
        m_selectionAnim.Reset(target);
    } else {
        // Siblings unchecked by group exclusion must re-register for ticks,
        // otherwise the filled dot never animates away (looks like two selected).
        RequestAnimationTicks();
    }
    MarkRenderRectDirty(m_bounds);
}

} // namespace CUI
