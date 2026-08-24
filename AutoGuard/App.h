#pragma once

#include "viewmodel/MainViewModel.h"
#include "view/MainView.h"
#include "framework/window/Window.h"
#include <memory>

namespace AutoGuardApp {

class AutoGuardApp {
public:
    AutoGuardApp();
    ~AutoGuardApp() = default;

    int Run();

private:
    CUI::Window m_window;
    std::shared_ptr<AutoGuard::MainViewModel> m_viewModel;
    std::shared_ptr<AutoGuard::MainView> m_view;
    std::shared_ptr<CUI::UIElement> m_root;
};

} // namespace AutoGuardApp
