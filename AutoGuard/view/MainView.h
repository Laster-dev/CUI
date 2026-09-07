#pragma once

#include "../viewmodel/MainViewModel.h"
#include "DetailStripView.h"
#include "framework/window/Window.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/NavigationView.h"
#include "framework/controls/StatusBar.h"
#include "framework/controls/ToastCenter.h"
#include <memory>

namespace AutoGuard {

class MainView {
public:
    explicit MainView(std::shared_ptr<MainViewModel> viewModel, CUI::Window* window);
    ~MainView() = default;

    std::shared_ptr<CUI::UIElement> Build();

    void RefreshContent();
    void UpdateStatusBar();
    void UpdateDetailPanel();
    void ShowToast(const std::string& message);

private:
    std::shared_ptr<CUI::NavigationView> BuildNavigationView();
    std::shared_ptr<CUI::UIElement> BuildStatusBar();

private:
    std::shared_ptr<MainViewModel> m_viewModel;
    CUI::Window* m_window = nullptr;

    std::shared_ptr<CUI::NavigationView> m_navView;
    std::shared_ptr<DetailStripView> m_detailStripView;
    std::shared_ptr<CUI::StatusBar> m_statusBar;
    std::shared_ptr<CUI::ToastCenter> m_toastCenter;

    std::string m_currentNavTag = "overview";

    int m_statusLeftId = 0;
    int m_statusCountId = 0;
    int m_statusSelectionId = 0;
};

} // namespace AutoGuard
