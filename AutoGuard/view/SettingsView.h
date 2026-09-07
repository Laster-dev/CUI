#pragma once

#include "framework/window/Window.h"
#include "framework/core/CUIDsl.h"
#include <memory>
#include <functional>

namespace AutoGuard {

class SettingsView {
public:
    SettingsView(CUI::Window* window, std::function<void(const std::string&)> onShowToast);
    ~SettingsView() = default;

    std::shared_ptr<CUI::UIElement> Build();

private:
    CUI::Window* m_window = nullptr;
    std::function<void(const std::string&)> m_onShowToast;
};

} // namespace AutoGuard
