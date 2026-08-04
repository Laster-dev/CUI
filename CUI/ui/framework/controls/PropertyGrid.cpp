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
#include <cmath>
#include <unordered_map>

namespace CUI {

namespace {
bool TryParseDate(const std::string& value, int& y, int& m, int& d) {
    return sscanf_s(value.c_str(), "%d-%d-%d", &y, &m, &d) == 3;
}

bool TryParseTime(const std::string& value, int& h, int& m) {
    return sscanf_s(value.c_str(), "%d:%d", &h, &m) == 2;
}

const char* ColorPropToTokenProp(const std::string& colorProp) {
    if (colorProp == "background") return "theme.backgroundToken";
    if (colorProp == "hoverBackground") return "theme.hoverBackgroundToken";
    if (colorProp == "pressedBackground") return "theme.pressedBackgroundToken";
    if (colorProp == "disabledBackground") return "theme.disabledBackgroundToken";
    if (colorProp == "borderBrush") return "theme.borderToken";
    if (colorProp == "focusedBorderBrush") return "theme.focusedBorderToken";
    if (colorProp == "color") return "theme.colorToken";
    if (colorProp == "placeholderColor") return "theme.placeholderColorToken";
    if (colorProp == "caretColor") return "theme.colorToken";
    if (colorProp == "dropdownBackground") return "theme.dropdownBackgroundToken";
    if (colorProp == "selectedItemBackground") return "theme.selectedItemBackgroundToken";
    if (colorProp == "selectedBackground") return "theme.selectedBackgroundToken";
    if (colorProp == "checkedBackground") return "theme.checkedBackgroundToken";
    if (colorProp == "underlineColor") return "theme.underlineColorToken";
    if (colorProp == "activeUnderlineColor") return "theme.activeUnderlineColorToken";
    if (colorProp == "fillColor") return "theme.fillColorToken";
    if (colorProp == "trackColor") return "theme.trackColorToken";
    if (colorProp == "activeTrackColor") return "theme.activeTrackColorToken";
    if (colorProp == "thumbColor") return "theme.thumbColorToken";
    if (colorProp == "onColor") return "theme.onColorToken";
    if (colorProp == "offColor") return "theme.offColorToken";
    if (colorProp == "knobColor") return "theme.knobColorToken";
    if (colorProp == "paneBackground") return "theme.paneBackgroundToken";
    if (colorProp == "indicatorColor") return "theme.indicatorColorToken";
    if (colorProp == "accent") return "theme.accentToken";
    if (colorProp == "accentColor") return "theme.accentColorToken";
    if (colorProp == "activeColor") return "theme.activeColorToken";
    if (colorProp == "headerBackground") return "theme.headerBackgroundToken";
    if (colorProp == "gridLineBrush") return "theme.gridLineBrushToken";
    if (colorProp == "titleColor") return "theme.titleColorToken";
    if (colorProp == "messageColor") return "theme.messageColorToken";
    return nullptr;
}

void ApplyColorToken(UIElement* target, const std::string& colorProp, const std::string& tokenName) {
    if (!target || tokenName.empty()) return;
    if (const char* tokenProp = ColorPropToTokenProp(colorProp)) {
        target->SetProperty(tokenProp, Value(tokenName));
    }
    target->SetProperty(colorProp, Value(ThemeManager::Instance().GetColor(tokenName)));
}

std::string ResolveCurrentColorToken(UIElement* target, const std::string& colorProp) {
    if (!target) return "accentColor";
    if (const char* tokenProp = ColorPropToTokenProp(colorProp)) {
        if (target->HasProperty(tokenProp)) {
            std::string token = target->GetProperty(tokenProp).AsString();
            if (!token.empty()) return token;
        }
    }
    // Fall back: match current color against theme tokens
    D2D1_COLOR_F current = target->GetProperty(colorProp).AsColor();
    for (const auto& name : ThemeManager::GetTokenNames()) {
        D2D1_COLOR_F t = ThemeManager::Instance().GetColor(name);
        if (std::abs(t.r - current.r) < 0.02f &&
            std::abs(t.g - current.g) < 0.02f &&
            std::abs(t.b - current.b) < 0.02f &&
            std::abs(t.a - current.a) < 0.02f) {
            return name;
        }
    }
    return "accentColor";
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
} // namespace

PropertyGrid::PropertyGrid() {
    // 右侧检查器是 chrome 玻璃，不是实心 card —— 否则整列盖死 SystemBackdrop。
    SetProperty("theme.backgroundToken", Value("paneBackground"));
    SetProperty("theme.borderToken", Value("cardBorder"));
    SetProperty("borderThickness", Value(1.0f));

    m_container = std::make_shared<StackPanel>();
    m_container->SetProperty("orientation", Value("Vertical"));
    m_container->SetProperty("padding", Value(Thickness(12, 12, 12, 12)));
    m_container->SetProperty("gap", Value(8.0f));
    // 容器本身不铺底，由 PropertyGrid 统一刷 pane 玻璃。
    m_container->SetProperty("background", Value(D2D1::ColorF(0, 0, 0, 0)));
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
    BindThemeToken(titleTb, "theme.colorToken", "textPrimary");
    m_container->AddChild(titleTb);

    // Call Virtual Reflection Method on UIElement Object
    std::vector<PropertyMeta> metas = target->GetPropertyMetas();

    // Root color-system rule: never expose raw RGB color slots in the inspector.
    // Only ColorPicker.selectedColor (data) and theme.*Token bindings are allowed.
    metas.erase(std::remove_if(metas.begin(), metas.end(), [](const PropertyMeta& meta) {
        return meta.type == "color" && meta.name != "selectedColor";
    }), metas.end());

    // Inject whatever theme.*Token bindings the control actually owns.
    static const std::unordered_map<std::string, std::string> kTokenLabels = {
        { "theme.backgroundToken", "背景 Token" },
        { "theme.hoverBackgroundToken", "悬停背景 Token" },
        { "theme.pressedBackgroundToken", "按下背景 Token" },
        { "theme.disabledBackgroundToken", "禁用背景 Token" },
        { "theme.borderToken", "边框 Token" },
        { "theme.focusedBorderToken", "焦点边框 Token" },
        { "theme.colorToken", "文字 Token" },
        { "theme.placeholderColorToken", "占位文字 Token" },
        { "theme.checkedBackgroundToken", "选中背景 Token" },
        { "theme.fillColorToken", "填充 Token" },
        { "theme.trackColorToken", "轨道 Token" },
        { "theme.activeTrackColorToken", "激活轨 Token" },
        { "theme.thumbColorToken", "滑块 Token" },
        { "theme.onColorToken", "开启 Token" },
        { "theme.offColorToken", "关闭 Token" },
        { "theme.knobColorToken", "旋钮 Token" },
        { "theme.accentToken", "强调 Token" },
        { "theme.accentColorToken", "强调色 Token" },
        { "theme.activeColorToken", "激活 Token" },
        { "theme.paneBackgroundToken", "面板背景 Token" },
        { "theme.indicatorColorToken", "指示器 Token" },
        { "theme.dropdownBackgroundToken", "下拉背景 Token" },
        { "theme.selectedItemBackgroundToken", "选中项背景 Token" },
        { "theme.selectedBackgroundToken", "选中背景 Token" },
        { "theme.headerBackgroundToken", "表头背景 Token" },
        { "theme.underlineColorToken", "下划线 Token" },
        { "theme.activeUnderlineColorToken", "激活下划线 Token" },
        { "theme.titleColorToken", "标题 Token" },
        { "theme.messageColorToken", "正文 Token" },
        { "theme.gridLineBrushToken", "网格线 Token" },
    };

    for (const auto& kv : target->GetAllProperties()) {
        const std::string& key = kv.first;
        if (key.rfind("theme.", 0) != 0 || key.find("Token") == std::string::npos) {
            continue;
        }
        bool already = false;
        for (const auto& meta : metas) {
            if (meta.name == key) { already = true; break; }
        }
        if (already) continue;

        auto labelIt = kTokenLabels.find(key);
        std::string display = labelIt != kTokenLabels.end() ? labelIt->second : key;
        metas.push_back({ key, display + " (" + key + ")", "主题色彩", "themeToken", ThemeManager::GetTokenNames() });
    }

    // Put theme tokens before layout/appearance so color-system bindings are visible first.
    std::stable_sort(metas.begin(), metas.end(), [](const PropertyMeta& a, const PropertyMeta& b) {
        auto rank = [](const std::string& cat) {
            if (cat == "基本信息") return 0;
            if (cat == "主题色彩") return 1;
            if (cat == "尺寸布局") return 2;
            if (cat == "外观") return 3;
            if (cat == "字体文本") return 4;
            if (cat == "交互状态") return 5;
            return 10;
        };
        return rank(a.category) < rank(b.category);
    });

    std::string currentCategory = "";
    for (const auto& meta : metas) {
        if (meta.category != currentCategory) {
            currentCategory = meta.category;
            auto catTb = std::make_shared<TextBlock>();
            catTb->SetProperty("text", Value("[" + currentCategory + "]"));
            catTb->SetProperty("fontSize", Value(11.0f));
            catTb->SetProperty("fontWeight", Value("Bold"));
            BindThemeToken(catTb, "theme.colorToken", "textSecondary");
            catTb->SetProperty("margin", Value(Thickness(0, 6, 0, 2)));
            m_container->AddChild(catTb);
        }

        // Label
        auto labelTb = std::make_shared<TextBlock>();
        labelTb->SetProperty("text", Value(meta.displayName + ":"));
        labelTb->SetProperty("fontSize", Value(11.0f));
        BindThemeToken(labelTb, "theme.colorToken", "textSecondary");
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

        } else if (meta.type == "enum" || meta.type == "themeToken" || (meta.type == "color" && meta.name != "selectedColor")) {
            auto combo = std::make_shared<ComboBox>();
            combo->SetProperty("width", Value(260.0f));
            combo->SetProperty("height", Value(26.0f));
            BindThemeToken(combo, "theme.backgroundToken", "inputBackground");
            BindThemeToken(combo, "theme.borderToken", "inputBorder");
            BindThemeToken(combo, "theme.dropdownBackgroundToken", "cardBackground");
            BindThemeToken(combo, "theme.selectedItemBackgroundToken", "selectedBackground");
            BindThemeToken(combo, "theme.colorToken", "textPrimary");

            const bool isThemeToken = (meta.type == "themeToken" || meta.type == "color");
            int selectIdx = 0;
            if (isThemeToken) {
                const auto& tokens = ThemeManager::GetTokenNames();
                std::string currentToken;
                if (meta.type == "themeToken") {
                    currentToken = currentVal.AsString();
                    if (currentToken.empty()) currentToken = "accentColor";
                } else {
                    currentToken = ResolveCurrentColorToken(target.get(), meta.name);
                }
                for (size_t i = 0; i < tokens.size(); ++i) {
                    combo->AddItem(tokens[i]);
                    if (tokens[i] == currentToken) selectIdx = static_cast<int>(i);
                }
            } else {
                std::string strVal = currentVal.AsString();
                for (size_t i = 0; i < meta.options.size(); ++i) {
                    combo->AddItem(meta.options[i]);
                    if (meta.options[i] == strVal) selectIdx = static_cast<int>(i);
                }
            }
            combo->SetSelectedIndex(selectIdx);

            std::string propName = meta.name;
            std::string propType = meta.type;
            std::weak_ptr<UIElement> weakTarget = m_target;
            void* host = m_windowHost;

            combo->OnSelectionChanged().Connect([this, weakTarget, propName, propType, host](ComboBox*, int idx, const std::string& opt) {
                if (m_updatingFromTarget) return;
                auto t = weakTarget.lock();
                if (t) {
                    if (propType == "themeToken") {
                        t->SetProperty(propName, Value(opt));
                        // Mirror into the concrete paint property when mapping is known
                        if (propName == "theme.backgroundToken") t->SetProperty("background", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.hoverBackgroundToken") t->SetProperty("hoverBackground", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.pressedBackgroundToken") t->SetProperty("pressedBackground", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.disabledBackgroundToken") t->SetProperty("disabledBackground", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.borderToken") t->SetProperty("borderBrush", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.focusedBorderToken") t->SetProperty("focusedBorderBrush", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.colorToken") t->SetProperty("color", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.placeholderColorToken") t->SetProperty("placeholderColor", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.checkedBackgroundToken") t->SetProperty("checkedBackground", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.fillColorToken") t->SetProperty("fillColor", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.trackColorToken") t->SetProperty("trackColor", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.activeTrackColorToken") t->SetProperty("activeTrackColor", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.thumbColorToken") t->SetProperty("thumbColor", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.onColorToken") t->SetProperty("onColor", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.offColorToken") t->SetProperty("offColor", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.knobColorToken") t->SetProperty("knobColor", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.accentToken") t->SetProperty("accent", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.accentColorToken") t->SetProperty("accentColor", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.activeColorToken") t->SetProperty("activeColor", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.paneBackgroundToken") t->SetProperty("paneBackground", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.indicatorColorToken") t->SetProperty("indicatorColor", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.dropdownBackgroundToken") t->SetProperty("dropdownBackground", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.selectedItemBackgroundToken") t->SetProperty("selectedItemBackground", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.selectedBackgroundToken") t->SetProperty("selectedBackground", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.headerBackgroundToken") t->SetProperty("headerBackground", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.underlineColorToken") t->SetProperty("underlineColor", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.activeUnderlineColorToken") t->SetProperty("activeUnderlineColor", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.titleColorToken") t->SetProperty("titleColor", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.messageColorToken") t->SetProperty("messageColor", Value(ThemeManager::Instance().GetColor(opt)));
                        else if (propName == "theme.gridLineBrushToken") t->SetProperty("gridLineBrush", Value(ThemeManager::Instance().GetColor(opt)));
                    } else if (propType == "color") {
                        ApplyColorToken(t.get(), propName, opt);
                    } else {
                        ApplyTargetProperty(t.get(), propName, Value(opt));
                    }
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
                        // selectedColor (ColorPicker data) still accepts hex; chrome colors use token combos above
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
