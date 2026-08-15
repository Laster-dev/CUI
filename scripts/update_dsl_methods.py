import os
import re

path_dsl = r'E:\C++project\CUI\CUI.Core\ui\framework\core\CUIDsl.h'
with open(path_dsl, 'r', encoding='utf-8') as f:
    dsl = f.read()

new_methods = '''    template<typename ItemsT>
    ElementBuilder& Items(ItemsT&& items) {
        if constexpr (requires { m_ptr->Items = std::forward<ItemsT>(items); }) {
            m_ptr->Items = std::forward<ItemsT>(items);
        } else if constexpr (requires { m_ptr->SetItems(std::forward<ItemsT>(items)); }) {
            m_ptr->SetItems(std::forward<ItemsT>(items));
        }
        return *this;
    }

    ElementBuilder& Items(std::initializer_list<std::string> items) {
        if constexpr (requires { m_ptr->Items = items; }) {
            m_ptr->Items = items;
        } else if constexpr (requires { m_ptr->SetItems(items); }) {
            m_ptr->SetItems(items);
        }
        return *this;
    }

    template<typename V>
    ElementBuilder& Value(V&& v) {
        if constexpr (requires { m_ptr->Value = std::forward<V>(v); }) {
            m_ptr->Value = std::forward<V>(v);
        } else if constexpr (requires { m_ptr->SetValue(std::forward<V>(v)); }) {
            m_ptr->SetValue(std::forward<V>(v));
        }
        return *this;
    }

    ElementBuilder& Minimum(float minVal) {
        if constexpr (requires { m_ptr->Minimum = minVal; }) {
            m_ptr->Minimum = minVal;
        } else if constexpr (requires { m_ptr->SetMinimum(minVal); }) {
            m_ptr->SetMinimum(minVal);
        }
        return *this;
    }

    ElementBuilder& Maximum(float maxVal) {
        if constexpr (requires { m_ptr->Maximum = maxVal; }) {
            m_ptr->Maximum = maxVal;
        } else if constexpr (requires { m_ptr->SetMaximum(maxVal); }) {
            m_ptr->SetMaximum(maxVal);
        }
        return *this;
    }

    ElementBuilder& Step(float s) {
        if constexpr (requires { m_ptr->Step = s; }) {
            m_ptr->Step = s;
        } else if constexpr (requires { m_ptr->SetStep(s); }) {
            m_ptr->SetStep(s);
        }
        return *this;
    }

    ElementBuilder& IsReadOnly(bool ro) {
        if constexpr (requires { m_ptr->IsReadOnly = ro; }) {
            m_ptr->IsReadOnly = ro;
        } else if constexpr (requires { m_ptr->SetIsReadOnly(ro); }) {
            m_ptr->SetIsReadOnly(ro);
        }
        return *this;
    }

    ElementBuilder& IsExpanded(bool exp) {
        if constexpr (requires { m_ptr->IsExpanded = exp; }) {
            m_ptr->IsExpanded = exp;
        } else if constexpr (requires { m_ptr->SetIsExpanded(exp); }) {
            m_ptr->SetIsExpanded(exp);
        }
        return *this;
    }

    ElementBuilder& ColumnDefinitions(const std::string& defs) {
        if constexpr (requires { m_ptr->ColumnDefinitions = defs; }) {
            m_ptr->ColumnDefinitions = defs;
        } else if constexpr (requires { m_ptr->SetColumnDefinitions(defs); }) {
            m_ptr->SetColumnDefinitions(defs);
        }
        return *this;
    }

    ElementBuilder& RowDefinitions(const std::string& defs) {
        if constexpr (requires { m_ptr->RowDefinitions = defs; }) {
            m_ptr->RowDefinitions = defs;
        } else if constexpr (requires { m_ptr->SetRowDefinitions(defs); }) {
            m_ptr->SetRowDefinitions(defs);
        }
        return *this;
    }

    ElementBuilder& Placeholder(const std::string& text) {
        if constexpr (requires { m_ptr->Placeholder = text; }) {
            m_ptr->Placeholder = text;
        } else if constexpr (requires { m_ptr->SetPlaceholder(text); }) {
            m_ptr->SetPlaceholder(text);
        }
        return *this;
    }

    ElementBuilder& Title(const std::string& t) {
        if constexpr (requires { m_ptr->Title = t; }) {
            m_ptr->Title = t;
        } else if constexpr (requires { m_ptr->SetTitle(t); }) {
            m_ptr->SetTitle(t);
        }
        return *this;
    }

    ElementBuilder& Message(const std::string& m) {
        if constexpr (requires { m_ptr->Message = m; }) {
            m_ptr->Message = m;
        } else if constexpr (requires { m_ptr->SetMessage(m); }) {
            m_ptr->SetMessage(m);
        }
        return *this;
    }
'''

if 'ElementBuilder& ColumnDefinitions' not in dsl:
    dsl = dsl.replace('    ElementBuilder& OnValueChanged(std::function<void(RangeSlider*, float, float)> handler) {\n        if constexpr (std::is_same_v<RangeSlider, T>) {\n            m_ptr->OnValueChanged().Connect(handler);\n        }\n        return *this;\n    }',
                      '    ElementBuilder& OnValueChanged(std::function<void(RangeSlider*, float, float)> handler) {\n        if constexpr (std::is_same_v<RangeSlider, T>) {\n            m_ptr->OnValueChanged().Connect(handler);\n        }\n        return *this;\n    }\n\n' + new_methods)
    with open(path_dsl, 'w', encoding='utf-8') as f:
        f.write(dsl)

print("Added generic C++20 fluent methods to ElementBuilder in CUIDsl.h")
