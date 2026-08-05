#include "StyleManager.h"
#include "../controls/UIElement.h"
#include "../style/ThemeManager.h"

namespace CUI {

StyleManager& StyleManager::Instance() {
    static StyleManager instance;
    return instance;
}

StyleManager::StyleManager() {
    LoadDarkTheme();
}

void StyleManager::LoadDarkTheme() {
    // Class styles resolve colors through ThemeManager (single color source).
    auto hex = [](const char* token) {
        return Value(ThemeManager::Instance().GetColorHex(token));
    };

    RegisterStyle("vscode-titlebar", "background", hex("titleBarBackground"));
    RegisterStyle("vscode-titlebar", "height", Value(30.0f));

    RegisterStyle("vscode-activitybar", "background", hex("activityBarBackground"));
    RegisterStyle("vscode-activitybar", "width", Value(48.0f));

    RegisterStyle("vscode-sidebar", "background", hex("sideBarBackground"));
    RegisterStyle("vscode-sidebar", "width", Value(240.0f));

    RegisterStyle("vscode-tabbar", "background", hex("tabBarBackground"));
    RegisterStyle("vscode-tabbar", "height", Value(35.0f));

    RegisterStyle("vscode-editor", "background", hex("editorBackground"));

    RegisterStyle("vscode-statusbar", "background", hex("statusBarBackground"));
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
                PropertyId propId = PropertyIdFromName(kv.first);
                if (propId == PropertyId::None) continue;
                if (!element->HasProperty(propId)) {
                    element->SetProperty(propId, kv.second);
                }
            }
        }
    }
}

void StyleManager::ReloadFromTheme() {
    m_styles.clear();
    LoadDarkTheme();
}

} // namespace CUI
