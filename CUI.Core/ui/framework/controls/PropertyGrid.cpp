#include "PropertyGrid.h"
#include "../window/Window.h"
#include "../core/PropertyDesc.h"
#include "../core/PropertyId.h"
#include "TextBox.h"
#include "CheckBox.h"
#include "RadioButton.h"
#include "Slider.h"
#include "ProgressBar.h"
#include "ProgressRing.h"
#include "NumberBox.h"
#include "ToggleSwitch.h"
#include "ToggleButton.h"
#include "DatePicker.h"
#include "TimePicker.h"
#include "ColorPicker.h"
#include "FilePicker.h"
#include "FolderPicker.h"
#include "PagingControl.h"
#include "RatingControl.h"
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

PropertyId ColorPropToTokenProp(PropertyId colorProp) {
    switch (colorProp) {
    case PropertyId::Background: return PropertyId::BackgroundToken;
    case PropertyId::HoverBackground: return PropertyId::HoverBackgroundToken;
    case PropertyId::PressedBackground: return PropertyId::PressedBackgroundToken;
    case PropertyId::BorderBrush: return PropertyId::BorderToken;
    case PropertyId::Color: return PropertyId::ColorToken;
    default: return PropertyId::None;
    }
}

void SetThemeTokenByPropId(UIElement* target, PropertyId tokenId, ThemeTokenId id) {
    if (!target || tokenId == PropertyId::None) return;
    switch (tokenId) {
    case PropertyId::BackgroundToken: target->SetBackgroundToken(id); break;
    case PropertyId::HoverBackgroundToken: target->SetHoverBackgroundToken(id); break;
    case PropertyId::PressedBackgroundToken: target->SetPressedBackgroundToken(id); break;
    case PropertyId::DisabledBackgroundToken: target->SetDisabledBackgroundToken(id); break;
    case PropertyId::BorderToken: target->SetBorderToken(id); break;
    case PropertyId::FocusedBorderToken: target->SetFocusedBorderToken(id); break;
    case PropertyId::ColorToken: target->SetColorToken(id); break;
    case PropertyId::PlaceholderColorToken: target->SetPlaceholderColorToken(id); break;
    case PropertyId::DropdownBackgroundToken: target->SetDropdownBackgroundToken(id); break;
    case PropertyId::SelectedItemBackgroundToken: target->SetSelectedItemBackgroundToken(id); break;
    case PropertyId::SelectedBackgroundToken: target->SetSelectedBackgroundToken(id); break;
    case PropertyId::CheckedBackgroundToken: target->SetCheckedBackgroundToken(id); break;
    case PropertyId::UnderlineColorToken: target->SetUnderlineColorToken(id); break;
    case PropertyId::ActiveUnderlineColorToken: target->SetActiveUnderlineColorToken(id); break;
    case PropertyId::FillColorToken: target->SetFillColorToken(id); break;
    case PropertyId::TrackColorToken: target->SetTrackColorToken(id); break;
    case PropertyId::ActiveTrackColorToken: target->SetActiveTrackColorToken(id); break;
    case PropertyId::ThumbColorToken: target->SetThumbColorToken(id); break;
    case PropertyId::OnColorToken: target->SetOnColorToken(id); break;
    case PropertyId::OffColorToken: target->SetOffColorToken(id); break;
    case PropertyId::KnobColorToken: target->SetKnobColorToken(id); break;
    case PropertyId::PaneBackgroundToken: target->SetPaneBackgroundToken(id); break;
    case PropertyId::IndicatorColorToken: target->SetIndicatorColorToken(id); break;
    case PropertyId::AccentColorToken: target->SetAccentColorToken(id); break;
    case PropertyId::ActiveColorToken: target->SetActiveColorToken(id); break;
    case PropertyId::HeaderBackgroundToken: target->SetHeaderBackgroundToken(id); break;
    case PropertyId::GridLineBrushToken: target->SetGridLineBrushToken(id); break;
    case PropertyId::TitleColorToken: target->SetTitleColorToken(id); break;
    case PropertyId::MessageColorToken: target->SetMessageColorToken(id); break;
    case PropertyId::SecondaryColorToken: target->SetSecondaryColorToken(id); break;
    case PropertyId::CaretColorToken: target->SetCaretColorToken(id); break;
    case PropertyId::ActiveTabBackgroundToken: target->SetActiveTabBackgroundToken(id); break;
    case PropertyId::InactiveTabBackgroundToken: target->SetInactiveTabBackgroundToken(id); break;
    default: break;
    }
}

void ApplyColorToken(UIElement* target, PropertyId colorProp, const std::string& tokenName) {
    if (!target || tokenName.empty()) return;
    if (PropertyId tokenId = ColorPropToTokenProp(colorProp); tokenId != PropertyId::None) {
        SetThemeTokenByPropId(target, tokenId, ThemeTokenIdFromName(tokenName));
    }
}

std::string ResolveCurrentColorToken(UIElement* target, PropertyId colorProp) {
    if (!target) return "accentColor";
    if (PropertyId tokenId = ColorPropToTokenProp(colorProp); tokenId != PropertyId::None) {
        if (target->HasProperty(tokenId)) {
            std::string token = target->GetProperty(tokenId).AsString();
            if (!token.empty()) return token;
        }
    }
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
std::shared_ptr<T> BindThemeToken(const std::shared_ptr<T>& element, PropertyId tokenId, const std::string& tokenName) {
    if (!element) {
        return element;
    }
    SetThemeTokenByPropId(element.get(), tokenId, ThemeTokenIdFromName(tokenName));
    return element;
}

void ApplyTargetProperty(UIElement* target, PropertyId propId, const Value& value) {
    if (!target || propId == PropertyId::None) return;

    if (auto tb = dynamic_cast<TextBox*>(target)) {
        if (propId == PropertyId::Text) { tb->SetText(value.AsString()); return; }
    }
    if (auto cb = dynamic_cast<CheckBox*>(target)) {
        if (propId == PropertyId::CheckState) {
            std::string s = value.AsString("Unchecked");
            cb->SetState(s == "Checked" ? CheckState::Checked : (s == "Indeterminate" ? CheckState::Indeterminate : CheckState::Unchecked));
            return;
        }
    }
    if (auto slider = dynamic_cast<Slider*>(target)) {
        if (propId == PropertyId::ControlValue) { slider->SetValue(value.AsFloat()); return; }
    }
    if (auto progress = dynamic_cast<ProgressBar*>(target)) {
        if (propId == PropertyId::ControlValue) { progress->SetValue(value.AsFloat()); return; }
        if (propId == PropertyId::IsIndeterminate) { progress->SetIsIndeterminate(value.AsBool()); return; }
    }
    if (auto ring = dynamic_cast<ProgressRing*>(target)) {
        if (propId == PropertyId::ControlValue) { ring->SetValue(value.AsFloat()); return; }
        if (propId == PropertyId::IsIndeterminate) { ring->SetIsIndeterminate(value.AsBool()); return; }
    }
    if (auto filePicker = dynamic_cast<FilePicker*>(target)) {
        if (propId == PropertyId::Text) { filePicker->SetPath(value.AsString()); return; }
    }
    if (auto folderPicker = dynamic_cast<FolderPicker*>(target)) {
        if (propId == PropertyId::Text) { folderPicker->SetPath(value.AsString()); return; }
    }
    if (auto number = dynamic_cast<NumberBox*>(target)) {
        if (propId == PropertyId::ControlValue) { number->SetValue(value.AsFloat()); return; }
    }
    if (auto toggleBtn = dynamic_cast<ToggleButton*>(target)) {
        if (propId == PropertyId::IsOn) { toggleBtn->SetIsChecked(value.AsBool()); return; }
    }
    if (auto toggle = dynamic_cast<ToggleSwitch*>(target)) {
        if (propId == PropertyId::IsOn) { toggle->SetIsOn(value.AsBool()); return; }
    }
    if (auto date = dynamic_cast<DatePicker*>(target)) {
        if (propId == PropertyId::DateStr) {
            int y = 0, m = 0, d = 0;
            if (TryParseDate(value.AsString(), y, m, d)) { date->SetDate(y, m, d); return; }
        }
    }
    if (auto time = dynamic_cast<TimePicker*>(target)) {
        if (propId == PropertyId::TimeStr) {
            int h = 0, m = 0;
            if (TryParseTime(value.AsString(), h, m)) { time->SetTime(h, m); return; }
        }
    }
    if (auto color = dynamic_cast<ColorPicker*>(target)) {
        if (propId == PropertyId::SelectedColor) { color->SetSelectedColor(value.AsColor()); return; }
    }
    if (auto paging = dynamic_cast<PagingControl*>(target)) {
        if (propId == PropertyId::CurrentPage) { paging->SetCurrentPage(value.AsInt()); return; }
        if (propId == PropertyId::TotalPages) { paging->SetTotalPages(value.AsInt()); return; }
    }
    if (auto rating = dynamic_cast<RatingControl*>(target)) {
        if (propId == PropertyId::ControlValue) { rating->SetValue(value.AsFloat()); return; }
        if (propId == PropertyId::Maximum) { rating->SetMaxRating(static_cast<int>(value.AsFloat())); return; }
        if (propId == PropertyId::Step) { rating->SetStep(value.AsFloat()); return; }
        if (propId == PropertyId::IsReadOnly) { rating->SetIsReadOnly(value.AsBool()); return; }
        if (propId == PropertyId::IsClearEnabled) { rating->SetIsClearEnabled(value.AsBool()); return; }
    }

    if (const PropertyDesc* desc = FindPropertyDescForElement(target, propId)) {
        if (desc->set) {
            desc->set(target, value);
            return;
        }
    }
    target->SetProperty(propId, value);
}

const char* PropertyKindToMetaType(PropertyKind kind) {
    switch (kind) {
    case PropertyKind::Bool: return "bool";
    case PropertyKind::Int: return "int";
    case PropertyKind::Float: return "float";
    case PropertyKind::String: return "string";
    case PropertyKind::Color: return "color";
    case PropertyKind::Thickness: return "thickness";
    case PropertyKind::Enum: return "enum";
    case PropertyKind::ThemeToken: return "themeToken";
    default: return "string";
    }
}

PropertyMeta MetaFromDesc(const PropertyDesc& desc) {
    PropertyMeta meta;
    meta.id = desc.id;
    meta.displayName = desc.displayName ? desc.displayName : PropertyIdToName(desc.id);
    meta.category = desc.category ? desc.category : "";
    meta.type = PropertyKindToMetaType(desc.kind);
    if (desc.enumOptions) {
        for (const char* const* p = desc.enumOptions; *p; ++p) {
            meta.options.emplace_back(*p);
        }
    }
    return meta;
}

bool MetaListContains(const std::vector<PropertyMeta>& metas, PropertyId id) {
    for (const auto& m : metas) {
        if (m.id == id) return true;
    }
    return false;
}
} // namespace

PropertyGrid::PropertyGrid() {
    // 右侧检查器是 chrome 玻璃，不是实心 card —— 否则整列盖死 SystemBackdrop。
    SetBackgroundToken(ThemeTokenId::PaneBackground);
    SetBorderToken(ThemeTokenId::CardBorder);
    // No full box; left hairline is drawn in OnRender.
    SetBorderThickness(0.0f);
    SetCornerRadius(0.0f);

    m_container = std::make_shared<StackPanel>();
    m_container->SetOrientation(Orientation::Vertical);
    m_container->SetPadding(Thickness(12, 12, 12, 12));
    m_container->SetGap(8.0f);
    // 容器本身不铺底，由 PropertyGrid 统一刷 pane 玻璃。
    m_container->SetBackground(D2D1::ColorF(0, 0, 0, 0));
    m_container->SetClipToBounds(false);

    AddChild(m_container);
}

void PropertyGrid::OnRender(GraphicsContext& ctx) {
    ScrollViewer::OnRender(ctx);
    if (m_bounds.IsEmpty()) {
        return;
    }
    D2D1_COLOR_F border = (GetBorderToken() != ThemeTokenId::Unset)
        ? ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder)
        : ThemeManager::Instance().GetColor(ThemeTokenId::CardBorder);
    if (border.a <= 0.0f) {
        return;
    }
    const float x = m_bounds.x + 0.5f;
    ctx.DrawLine(
        Point(x, m_bounds.y),
        Point(x, m_bounds.y + m_bounds.height),
        border,
        1.0f);
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
    titleTb->SetText("自动化属性检查器 (" + className + ")");
    titleTb->SetFontSize(12.0f);
    titleTb->SetFontWeight("Bold");
    BindThemeToken(titleTb, PropertyId::ColorToken, "textPrimary");
    m_container->AddChild(titleTb);

    // Prefer static PropertyDesc tables; fall back to PropertyMeta for compatibility.
    std::vector<PropertyMeta> metas;
    PropertyDescSpan descSpan = target->GetPropertyDescs();
    if (descSpan.count > 0 && descSpan.data) {
        metas.reserve(descSpan.count);
        for (size_t i = 0; i < descSpan.count; ++i) {
            metas.push_back(MetaFromDesc(descSpan.data[i]));
        }
        // Keep subclass GetPropertyMetas extras (controls only override metas today).
        for (const auto& extra : target->GetPropertyMetas()) {
            if (!MetaListContains(metas, extra.id)) {
                metas.push_back(extra);
            }
        }
    } else {
        metas = target->GetPropertyMetas();
    }

    // Root color-system rule: never expose raw RGB color slots in the inspector.
    // Only ColorPicker.selectedColor (data) and theme.*Token bindings are allowed.
    metas.erase(std::remove_if(metas.begin(), metas.end(), [](const PropertyMeta& meta) {
        return meta.type == "color" && meta.id != PropertyId::SelectedColor;
    }), metas.end());

    static const std::unordered_map<PropertyId, std::string> kTokenLabels = {
        { PropertyId::BackgroundToken, "背景 Token" },
        { PropertyId::HoverBackgroundToken, "悬停背景 Token" },
        { PropertyId::PressedBackgroundToken, "按下背景 Token" },
        { PropertyId::DisabledBackgroundToken, "禁用背景 Token" },
        { PropertyId::BorderToken, "边框 Token" },
        { PropertyId::FocusedBorderToken, "焦点边框 Token" },
        { PropertyId::ColorToken, "文字 Token" },
        { PropertyId::PlaceholderColorToken, "占位文字 Token" },
        { PropertyId::CheckedBackgroundToken, "选中背景 Token" },
        { PropertyId::FillColorToken, "填充 Token" },
        { PropertyId::TrackColorToken, "轨道 Token" },
        { PropertyId::ActiveTrackColorToken, "激活轨 Token" },
        { PropertyId::ThumbColorToken, "滑块 Token" },
        { PropertyId::OnColorToken, "开启 Token" },
        { PropertyId::OffColorToken, "关闭 Token" },
        { PropertyId::KnobColorToken, "旋钮 Token" },
        { PropertyId::AccentColorToken, "强调色 Token" },
        { PropertyId::ActiveColorToken, "激活 Token" },
        { PropertyId::PaneBackgroundToken, "面板背景 Token" },
        { PropertyId::IndicatorColorToken, "指示器 Token" },
        { PropertyId::DropdownBackgroundToken, "下拉背景 Token" },
        { PropertyId::SelectedItemBackgroundToken, "选中项背景 Token" },
        { PropertyId::SelectedBackgroundToken, "选中背景 Token" },
        { PropertyId::HeaderBackgroundToken, "表头背景 Token" },
        { PropertyId::UnderlineColorToken, "下划线 Token" },
        { PropertyId::ActiveUnderlineColorToken, "激活下划线 Token" },
        { PropertyId::TitleColorToken, "标题 Token" },
        { PropertyId::MessageColorToken, "正文 Token" },
        { PropertyId::GridLineBrushToken, "网格线 Token" },
        { PropertyId::SecondaryColorToken, "次要文字 Token" },
        { PropertyId::CaretColorToken, "光标 Token" },
        { PropertyId::ActiveTabBackgroundToken, "活动标签 Token" },
        { PropertyId::InactiveTabBackgroundToken, "非活动标签 Token" },
    };

    auto injectThemeTokenMeta = [&](PropertyId key) {
        if (MetaListContains(metas, key)) return;
        auto labelIt = kTokenLabels.find(key);
        const std::string keyName = PropertyIdToName(key);
        std::string display = labelIt != kTokenLabels.end() ? labelIt->second : keyName;
        metas.push_back({ keyName.c_str(), display + " (" + keyName + ")", "主题色彩", "themeToken", ThemeManager::GetTokenNames() });
    };

    // Prefer ThemeToken PropertyDescs; fall back to SnapshotProperties for owned tokens.
    bool usedDescThemeTokens = false;
    if (descSpan.count > 0 && descSpan.data) {
        for (size_t i = 0; i < descSpan.count; ++i) {
            const PropertyDesc& d = descSpan.data[i];
            if (d.kind != PropertyKind::ThemeToken || d.id == PropertyId::None) continue;
            usedDescThemeTokens = true;
            if (!target->HasProperty(d.id)) continue;
            injectThemeTokenMeta(d.id);
        }
    }
    if (!usedDescThemeTokens) {
        for (const auto& kv : target->SnapshotProperties()) {
            const PropertyDesc* desc = FindPropertyDescById(kv.first);
            if (!desc || desc->kind != PropertyKind::ThemeToken) continue;
            injectThemeTokenMeta(kv.first);
        }
    }

    metas.erase(std::remove_if(metas.begin(), metas.end(), [&](const PropertyMeta& meta) {
        return meta.type == "themeToken" && !target->HasProperty(meta.id);
    }), metas.end());

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
            catTb->SetText("[" + currentCategory + "]");
            catTb->SetFontSize(11.0f);
            catTb->SetFontWeight("Bold");
            BindThemeToken(catTb, PropertyId::ColorToken, "textSecondary");
            catTb->SetMargin(Thickness(0, 6, 0, 2));
            m_container->AddChild(catTb);
        }

        // Label
        auto labelTb = std::make_shared<TextBlock>();
        labelTb->SetText(meta.displayName + ":");
        labelTb->SetFontSize(11.0f);
        BindThemeToken(labelTb, PropertyId::ColorToken, "textSecondary");
        m_container->AddChild(labelTb);

        Value currentVal = target->GetProperty(meta.id);

        if (meta.type == "bool") {
            auto chk = std::make_shared<CheckBox>();
            chk->SetText("启用/开启");
            bool bVal = currentVal.IsEmpty() ? (meta.id == PropertyId::IsEnabled) : currentVal.AsBool();
            chk->SetState(bVal ? CheckState::Checked : CheckState::Unchecked);

            PropertyId propId = meta.id;
            std::weak_ptr<UIElement> weakTarget = m_target;
            void* host = m_windowHost;

            chk->OnCheckStateChanged().Connect([this, weakTarget, propId, host](CheckBox*, CheckState st) {
                if (m_updatingFromTarget) return;
                auto t = weakTarget.lock();
                if (t) {
                    ApplyTargetProperty(t.get(), propId, Value(st == CheckState::Checked));
                    if (host) {
                        Window* w = reinterpret_cast<Window*>(host);
                        w->Relayout();
                        InvalidateRect(w->GetHWND(), nullptr, FALSE);
                    }
                }
            });

            m_checkControls[meta.id] = chk;
            m_container->AddChild(chk);

        } else if (meta.type == "enum" || meta.type == "themeToken" || (meta.type == "color" && meta.id != PropertyId::SelectedColor)) {
            auto combo = std::make_shared<ComboBox>();
            combo->SetWidth(260.0f);
            combo->SetHeight(32.0f);
            BindThemeToken(combo, PropertyId::BackgroundToken, "inputBackground");
            BindThemeToken(combo, PropertyId::BorderToken, "inputBorder");
            BindThemeToken(combo, PropertyId::DropdownBackgroundToken, "cardBackground");
            BindThemeToken(combo, PropertyId::SelectedItemBackgroundToken, "selectedBackground");
            BindThemeToken(combo, PropertyId::ColorToken, "textPrimary");

            const bool isThemeToken = (meta.type == "themeToken" || meta.type == "color");
            int selectIdx = 0;
            if (isThemeToken) {
                const auto& tokens = ThemeManager::GetTokenNames();
                std::string currentToken;
                if (meta.type == "themeToken") {
                    currentToken = currentVal.AsString();
                    if (currentToken.empty()) currentToken = "accentColor";
                } else {
                    currentToken = ResolveCurrentColorToken(target.get(), meta.id);
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

            PropertyId propId = meta.id;
            std::string propType = meta.type;
            std::weak_ptr<UIElement> weakTarget = m_target;
            void* host = m_windowHost;

            combo->OnSelectionChanged().Connect([this, weakTarget, propId, propType, host](ComboBox*, int idx, const std::string& opt) {
                if (m_updatingFromTarget) return;
                auto t = weakTarget.lock();
                if (t) {
                    if (propType == "themeToken") {
                        ApplyTargetProperty(t.get(), propId, Value(opt));
                    } else if (propType == "color") {
                        ApplyColorToken(t.get(), propId, opt);
                    } else {
                        ApplyTargetProperty(t.get(), propId, Value(opt));
                    }
                    if (host) {
                        Window* w = reinterpret_cast<Window*>(host);
                        w->Relayout();
                        InvalidateRect(w->GetHWND(), nullptr, FALSE);
                    }
                }
            });

            m_comboControls[meta.id] = combo;
            m_container->AddChild(combo);

        } else {
            auto input = std::make_shared<TextBox>();
            input->SetWidth(260.0f);
            input->SetHeight(40.0f);
            input->SetPadding(Thickness(8.0f, 6.0f, 8.0f, 6.0f));
            input->SetPlaceholder("");
            BindThemeToken(input, PropertyId::ColorToken, "textPrimary");
            BindThemeToken(input, PropertyId::PlaceholderColorToken, "textMuted");
            BindThemeToken(input, PropertyId::UnderlineColorToken, "cardBorder");
            BindThemeToken(input, PropertyId::ActiveUnderlineColorToken, "accentColor");

            std::string displayValStr = "";
            if (!currentVal.IsEmpty()) {
                displayValStr = currentVal.AsString();
            } else {
                if (meta.id == PropertyId::Width || meta.id == PropertyId::Height) displayValStr = "-1";
                else if (meta.id == PropertyId::Margin || meta.id == PropertyId::Padding) displayValStr = "0,0,0,0";
                else if (meta.id == PropertyId::BorderThickness) displayValStr = "0";
                else if (meta.id == PropertyId::CornerRadius) displayValStr = "0";
            }
            input->SetText(displayValStr);

            PropertyId propId = meta.id;
            std::string pType = meta.type;
            std::weak_ptr<UIElement> weakTarget = m_target;
            void* host = m_windowHost;

            input->OnTextChanged().Connect([this, weakTarget, propId, pType, host](TextBox*, const std::string& valStr) {
                if (m_updatingFromTarget) return;
                auto t = weakTarget.lock();
                if (t) {
                    Value newValue;
                    if (pType == "number" || pType == "float") {
                        float f = static_cast<float>(atof(valStr.c_str()));
                        newValue = Value(f);
                    } else if (pType == "int") {
                        newValue = Value(atoi(valStr.c_str()));
                    } else if (pType == "color") {
                        newValue = Value(Value::ParseColor(valStr));
                    } else if (pType == "thickness" || propId == PropertyId::Margin || propId == PropertyId::Padding) {
                        newValue = Value(Thickness::Parse(valStr));
                    } else {
                        newValue = Value(valStr);
                    }
                    ApplyTargetProperty(t.get(), propId, newValue);
                    if (host) {
                        Window* w = reinterpret_cast<Window*>(host);
                        w->Relayout();
                        InvalidateRect(w->GetHWND(), nullptr, FALSE);
                    }
                }
            });

            m_inputControls[meta.id] = input;
            m_container->AddChild(input);
        }
    }
}

} // namespace CUI
