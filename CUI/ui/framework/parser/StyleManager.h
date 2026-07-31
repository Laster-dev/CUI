#pragma once
#include "../core/Value.h"
#include <string>
#include <unordered_map>

namespace CUI {

class UIElement;

class StyleManager {
public:
    static StyleManager& Instance();

    void RegisterStyle(const std::string& className, const std::string& propertyName, const Value& val);
    void ApplyStyle(UIElement* element);

    void LoadDarkTheme();

private:
    StyleManager();
    std::unordered_map<std::string, std::unordered_map<std::string, Value>> m_styles;
};

} // namespace CUI
