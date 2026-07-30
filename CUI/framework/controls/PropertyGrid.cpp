#include "PropertyGrid.h"
#include "../window/Window.h"
#include <sstream>
#include <algorithm>

namespace CUI {

PropertyGrid::PropertyGrid() {
    SetProperty("background", Value("#252526"));
    SetProperty("borderBrush", Value("#333333"));
    SetProperty("borderThickness", Value(1.0f));

    m_container = std::make_shared<StackPanel>();
    m_container->SetProperty("orientation", Value("Vertical"));
    m_container->SetProperty("padding", Value(Thickness(12, 12, 12, 12)));
    m_container->SetProperty("gap", Value(8.0f));
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
    titleTb->SetProperty("color", Value("#569CD6"));
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
            catTb->SetProperty("color", Value("#4EC9B0"));
            catTb->SetProperty("margin", Value(Thickness(0, 6, 0, 2)));
            m_container->AddChild(catTb);
        }

        // Label
        auto labelTb = std::make_shared<TextBlock>();
        labelTb->SetProperty("text", Value(meta.displayName + ":"));
        labelTb->SetProperty("fontSize", Value(11.0f));
        labelTb->SetProperty("color", Value("#AAAAAA"));
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
                    t->SetProperty(propName, Value(st == CheckState::Checked));
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
                    t->SetProperty(propName, Value(opt));
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
                    if (pType == "number") {
                        float f = static_cast<float>(atof(valStr.c_str()));
                        t->SetProperty(propName, Value(f));
                    } else if (pType == "color") {
                        t->SetProperty(propName, Value(Value::ParseColor(valStr)));
                    } else {
                        if (propName == "margin" || propName == "padding") {
                            t->SetProperty(propName, Value(Thickness::Parse(valStr)));
                        } else {
                            t->SetProperty(propName, Value(valStr));
                        }
                    }
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
