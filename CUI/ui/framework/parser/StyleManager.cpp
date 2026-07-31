#include "StyleManager.h"
#include "../controls/UIElement.h"

namespace CUI {

StyleManager& StyleManager::Instance() {
    static StyleManager instance;
    return instance;
}

StyleManager::StyleManager() {
    LoadDarkTheme();
}

void StyleManager::LoadDarkTheme() {
    // VS Code Dark+ Theme Tokens
    RegisterStyle("vscode-titlebar", "background", Value("#1F1F1F"));
    RegisterStyle("vscode-titlebar", "height", Value(30.0f));

    RegisterStyle("vscode-activitybar", "background", Value("#333333"));
    RegisterStyle("vscode-activitybar", "width", Value(48.0f));

    RegisterStyle("vscode-sidebar", "background", Value("#252526"));
    RegisterStyle("vscode-sidebar", "width", Value(240.0f));

    RegisterStyle("vscode-tabbar", "background", Value("#252526"));
    RegisterStyle("vscode-tabbar", "height", Value(35.0f));

    RegisterStyle("vscode-editor", "background", Value("#1E1E1E"));

    RegisterStyle("vscode-statusbar", "background", Value("#007ACC"));
    RegisterStyle("vscode-statusbar", "height", Value(22.0f));
}

void StyleManager::RegisterStyle(const std::string& className, const std::string& propertyName, const Value& val) {
    m_styles[className][propertyName] = val;
}

void StyleManager::ApplyStyle(UIElement* element) {
    if (!element) return;

    std::string styleClass = element->GetStyleClass();
    if (!styleClass.empty()) {
        auto it = m_styles.find(styleClass);
        if (it != m_styles.end()) {
            for (const auto& kv : it->second) {
                if (!element->HasProperty(kv.first)) {
                    element->SetProperty(kv.first, kv.second);
                }
            }
        }
    }
}

} // namespace CUI
