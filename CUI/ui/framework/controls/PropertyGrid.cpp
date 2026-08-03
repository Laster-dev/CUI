#include "PropertyGrid.h"
#include "../window/Window.h"
#include "TextBox.h"
#include "CheckBox.h"
#include "RadioButton.h"
#include "Slider.h"
#include "ProgressBar.h"
#include "NumberBox.h"
#include "ToggleSwitch.h"
#include "DatePicker.h"
#include "TimePicker.h"
#include "ColorPicker.h"
#include "PagingControl.h"
#include "../style/ThemeManager.h"
#include <sstream>
#include <algorithm>
#include <cstdio>

namespace CUI {

namespace {
bool TryParseDate(const std::string& value, int& y, int& m, int& d) {
    return sscanf_s(value.c_str(), "%d-%d-%d", &y, &m, &d) == 3;
}

bool TryParseTime(const std::string& value, int& h, int& m) {
    return sscanf_s(value.c_str(), "%d:%d", &h, &m) == 2;
}

template <typename T>
std::shared_ptr<T> BindThemeToken(const std::shared_ptr<T>& element, const std::string& tokenProp, const std::string& tokenName) {
    if (!element) {
        return element;
    }
    element->SetProperty(tokenProp, Value(tokenName));
    if (tokenProp == "theme.backgroundToken") {
        element->SetProperty("background", Value(ThemeManager::Instance().GetColor(tokenName)));
    } else if (tokenProp == "theme.borderToken") {
        element->SetProperty("borderBrush", Value(ThemeManager::Instance().GetColor(tokenName)));
    } else if (tokenProp == "theme.colorToken") {
        element->SetProperty("color", Value(ThemeManager::Instance().GetColor(tokenName)));
    } else if (tokenProp == "theme.focusedBorderToken") {
        element->SetProperty("focusedBorderBrush", Value(ThemeManager::Instance().GetColor(tokenName)));
    } else if (tokenProp == "theme.dropdownBackgroundToken") {
        element->SetProperty("dropdownBackground", Value(ThemeManager::Instance().GetColor(tokenName)));
    } else if (tokenProp == "theme.selectedItemBackgroundToken") {
        element->SetProperty("selectedItemBackground", Value(ThemeManager::Instance().GetColor(tokenName)));
    } else if (tokenProp == "theme.placeholderColorToken") {
        element->SetProperty("placeholderColor", Value(ThemeManager::Instance().GetColor(tokenName)));
    } else if (tokenProp == "theme.underlineColorToken") {
        element->SetProperty("underlineColor", Value(ThemeManager::Instance().GetColor(tokenName)));
    } else if (tokenProp == "theme.activeUnderlineColorToken") {
        element->SetProperty("activeUnderlineColor", Value(ThemeManager::Instance().GetColor(tokenName)));
    }
    return element;
}

void ApplyTargetProperty(UIElement* target, const std::string& propName, const Value& value) {
    if (!target) return;

    if (auto tb = dynamic_cast<TextBox*>(target)) {
        if (propName == "text") { tb->SetText(value.AsString()); return; }
    }
    if (auto cb = dynamic_cast<CheckBox*>(target)) {
        if (propName == "checkState") {
            std::string s = value.AsString("Unchecked");
            cb->SetState(s == "Checked" ? CheckState::Checked : (s == "Indeterminate" ? CheckState::Indeterminate : CheckState::Unchecked));
            return;
        }
    }
    if (auto slider = dynamic_cast<Slider*>(target)) {
        if (propName == "value") { slider->SetValue(value.AsFloat()); return; }
    }
    if (auto progress = dynamic_cast<ProgressBar*>(target)) {
        if (propName == "value") { progress->SetValue(value.AsFloat()); return; }
        if (propName == "isIndeterminate") { progress->SetIsIndeterminate(value.AsBool()); return; }
    }
    if (auto number = dynamic_cast<NumberBox*>(target)) {
        if (propName == "value") { number->SetValue(value.AsFloat()); return; }
    }
    if (auto toggle = dynamic_cast<ToggleSwitch*>(target)) {
        if (propName == "isOn") { toggle->SetIsOn(value.AsBool()); return; }
    }
    if (auto date = dynamic_cast<DatePicker*>(target)) {
        if (propName == "dateStr") {
            int y = 0, m = 0, d = 0;
            if (TryParseDate(value.AsString(), y, m, d)) { date->SetDate(y, m, d); return; }
        }
    }
    if (auto time = dynamic_cast<TimePicker*>(target)) {
        if (propName == "timeStr") {
            int h = 0, m = 0;
            if (TryParseTime(value.AsString(), h, m)) { time->SetTime(h, m); return; }
        }
    }
    if (auto color = dynamic_cast<ColorPicker*>(target)) {
        if (propName == "selectedColor") { color->SetSelectedColor(value.AsColor()); return; }
    }
    if (auto paging = dynamic_cast<PagingControl*>(target)) {
        if (propName == "currentPage") { paging->SetCurrentPage(value.AsInt()); return; }
        if (propName == "totalPages") { paging->SetTotalPages(value.AsInt()); return; }
    }

    target->SetProperty(propName, value);
}
}

PropertyGrid::PropertyGrid() {
    SetProperty("theme.backgroundToken", Value("cardBackground"));
    SetProperty("background", Value(ThemeManager::Instance().GetColor("cardBackground")));
    SetProperty("theme.borderToken", Value("cardBorder"));
    SetProperty("borderBrush", Value(ThemeManager::Instance().GetColor("cardBorder")));
    SetProperty("borderThickness", Value(1.0f));

    m_container = std::make_shared<StackPanel>();
    m_container->SetProperty("orientation", Value("Vertical"));
    m_container->SetProperty("padding", Value(Thickness(12, 12, 12, 12)));
    m_container->SetProperty("gap", Value(8.0f));
    BindThemeToken(m_container, "theme.backgroundToken", "cardBackground");
    // Prevent last property rows from being clipped if content height is slightly short.
    m_container->SetProperty("clipToBounds", Value(false));

    AddChild(m_container);
}

void PropertyGrid::SetTargetElement(std::shared_ptr<UIElement> target, void* windowHost) {
    m_target = target;
    m_windowHost = windowHost;
    RebuildUI();
}

void PropertyGrid::RebuildUI() {
    m_container->ClearChildren();
    m_inputControls.clear();
    m_checkControls.clear();
    m_comboControls.clear();

    auto target = m_target.lock();
    if (!target) return;

    std::string className = target->GetClassName();

    // Add Title
    auto titleTb = std::make_shared<TextBlock>();
    titleTb->SetProperty("text", Value("自动化属性检查器 (" + className + ")"));
    titleTb->SetProperty("fontSize", Value(12.0f));
    titleTb->SetProperty("fontWeight", Value("Bold"));
    BindThemeToken(titleTb, "theme.colorToken", "accentColor");
    m_container->AddChild(titleTb);

    // Call Virtual Reflection Method on UIElement Object
    std::vector<PropertyMeta> metas = target->GetPropertyMetas();

    std::string currentCategory = "";
    for (const auto& meta : metas) {
        if (meta.category != currentCategory) {
            currentCategory = meta.category;
            auto catTb = std::make_shared<TextBlock>();
            catTb->SetProperty("text", Value("[" + currentCategory + "]"));
            catTb->SetProperty("fontSize", Value(11.0f));
            catTb->SetProperty("fontWeight", Value("Bold"));
            BindThemeToken(catTb, "theme.colorToken", "accentColor");
            catTb->SetProperty("margin", Value(Thickness(0, 6, 0, 2)));
            m_container->AddChild(catTb);
        }

        // Label
        auto labelTb = std::make_shared<TextBlock>();
        labelTb->SetProperty("text", Value(meta.displayName + ":"));
        labelTb->SetProperty("fontSize", Value(11.0f));
        BindThemeToken(labelTb, "theme.colorToken", "textMuted");
        m_container->AddChild(labelTb);

        // Control
        Value currentVal = target->GetProperty(meta.name);

        if (meta.type == "bool") {
            auto chk = std::make_shared<CheckBox>();
            chk->SetProperty("text", Value("启用/开启"));
            bool bVal = currentVal.IsEmpty() ? (meta.name == "isEnabled") : currentVal.AsBool();
            chk->SetState(bVal ? CheckState::Checked : CheckState::Unchecked);

            std::string propName = meta.name;
            std::weak_ptr<UIElement> weakTarget = m_target;
            void* host = m_windowHost;

            chk->OnCheckStateChanged().Connect([this, weakTarget, propName, host](CheckBox*, CheckState st) {
                if (m_updatingFromTarget) return;
                auto t = weakTarget.lock();
                if (t) {
                    ApplyTargetProperty(t.get(), propName, Value(st == CheckState::Checked));
                    if (host) {
                        Window* w = reinterpret_cast<Window*>(host);
                        w->Relayout();
                        InvalidateRect(w->GetHWND(), nullptr, FALSE);
                    }
                }
            });

            m_checkControls[meta.name] = chk;
            m_container->AddChild(chk);

        } else if (meta.type == "enum") {
            auto combo = std::make_shared<ComboBox>();
            combo->SetProperty("width", Value(260.0f));
            combo->SetProperty("height", Value(26.0f));
            BindThemeToken(combo, "theme.backgroundToken", "inputBackground");
            BindThemeToken(combo, "theme.borderToken", "inputBorder");
            BindThemeToken(combo, "theme.dropdownBackgroundToken", "cardBackground");
            BindThemeToken(combo, "theme.selectedItemBackgroundToken", "accentColor");
            BindThemeToken(combo, "theme.colorToken", "textPrimary");

            int selectIdx = 0;
            std::string strVal = currentVal.AsString();
            for (size_t i = 0; i < meta.options.size(); ++i) {
                combo->AddItem(meta.options[i]);
                if (meta.options[i] == strVal) selectIdx = static_cast<int>(i);
            }
            combo->SetSelectedIndex(selectIdx);

            std::string propName = meta.name;
            std::weak_ptr<UIElement> weakTarget = m_target;
            void* host = m_windowHost;

            combo->OnSelectionChanged().Connect([this, weakTarget, propName, host](ComboBox*, int idx, const std::string& opt) {
                if (m_updatingFromTarget) return;
                auto t = weakTarget.lock();
                if (t) {
                    ApplyTargetProperty(t.get(), propName, Value(opt));
                    if (host) {
                        Window* w = reinterpret_cast<Window*>(host);
                        w->Relayout();
                        InvalidateRect(w->GetHWND(), nullptr, FALSE);
                    }
                }
            });

            m_comboControls[meta.name] = combo;
            m_container->AddChild(combo);

        } else {
            auto input = std::make_shared<TextBox>();
            input->SetProperty("width", Value(260.0f));
            input->SetProperty("height", Value(26.0f));
            BindThemeToken(input, "theme.colorToken", "textPrimary");
            BindThemeToken(input, "theme.placeholderColorToken", "textMuted");
            BindThemeToken(input, "theme.underlineColorToken", "cardBorder");
            BindThemeToken(input, "theme.activeUnderlineColorToken", "accentColor");

            std::string displayValStr = "";
            if (!currentVal.IsEmpty()) {
                displayValStr = currentVal.AsString();
            } else {
                if (meta.name == "width" || meta.name == "height") displayValStr = "-1";
                else if (meta.name == "margin" || meta.name == "padding") displayValStr = "0,0,0,0";
                else if (meta.name == "borderThickness") displayValStr = "0";
                else if (meta.name == "cornerRadius") displayValStr = "0";
            }
            input->SetText(displayValStr);

            std::string propName = meta.name;
            std::string pType = meta.type;
            std::weak_ptr<UIElement> weakTarget = m_target;
            void* host = m_windowHost;

            input->OnTextChanged().Connect([this, weakTarget, propName, pType, host](TextBox*, const std::string& valStr) {
                if (m_updatingFromTarget) return;
                auto t = weakTarget.lock();
                if (t) {
                    Value newValue;
                    if (pType == "number") {
                        float f = static_cast<float>(atof(valStr.c_str()));
                        newValue = Value(f);
                    } else if (pType == "color") {
                        newValue = Value(Value::ParseColor(valStr));
                    } else {
                        if (propName == "margin" || propName == "padding") {
                            newValue = Value(Thickness::Parse(valStr));
                        } else {
                            newValue = Value(valStr);
                        }
                    }
                    ApplyTargetProperty(t.get(), propName, newValue);
                    if (host) {
                        Window* w = reinterpret_cast<Window*>(host);
                        w->Relayout();
                        InvalidateRect(w->GetHWND(), nullptr, FALSE);
                    }
                }
            });

            m_inputControls[meta.name] = input;
            m_container->AddChild(input);
        }
    }
}

} // namespace CUI
