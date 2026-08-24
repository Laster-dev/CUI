#pragma once

#include "../viewmodel/MainViewModel.h"
#include "framework/window/Window.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/NavigationView.h"
#include "framework/controls/TreeView.h"
#include "framework/controls/StatusBar.h"
#include "framework/controls/ToastCenter.h"
#include "framework/controls/TextBox.h"
#include "framework/controls/TextBlock.h"

#include <memory>
#include <map>

namespace AutoGuard {

class MainView {
public:
    explicit MainView(std::shared_ptr<MainViewModel> viewModel, CUI::Window* window);
    ~MainView() = default;

    std::shared_ptr<CUI::UIElement> Build();

    void RefreshContent();
    void UpdateTreeView();
    void UpdateStatusBar();
    void UpdateDetailPanel();
    void ShowToast(const std::string& message);

private:
    void SetupMenuBar(std::shared_ptr<CUI::WindowTitleBar> titleBar);
    std::shared_ptr<CUI::UIElement> BuildToolbar();
    std::shared_ptr<CUI::NavigationView> BuildNavigationView();
    
    std::shared_ptr<CUI::UIElement> BuildOverviewPage();
    std::shared_ptr<CUI::UIElement> BuildTreeCategoryPage(const std::string& title, const std::string& desc);
    std::shared_ptr<CUI::UIElement> BuildSettingsPage();
    std::shared_ptr<CUI::UIElement> BuildBottomDetailStrip();
    std::shared_ptr<CUI::UIElement> BuildStatusBar();

private:
    std::shared_ptr<MainViewModel> m_viewModel;
    CUI::Window* m_window = nullptr;

    std::shared_ptr<CUI::NavigationView> m_navView;
    std::shared_ptr<CUI::TreeView> m_treeView;
    std::shared_ptr<CUI::StatusBar> m_statusBar;
    std::shared_ptr<CUI::UIElement> m_detailStrip;
    std::shared_ptr<CUI::ToastCenter> m_toastCenter;
    std::shared_ptr<CUI::TextBox> m_searchBox;

    std::shared_ptr<CUI::StackPanel> m_contentHost;

    std::string m_currentNavTag = "overview";

    // Detail strip widgets
    std::shared_ptr<CUI::TextBlock> m_detailIcon;
    std::shared_ptr<CUI::TextBlock> m_detailLine1Name;
    std::shared_ptr<CUI::TextBlock> m_detailLine1Size;
    std::shared_ptr<CUI::TextBlock> m_detailLine2Desc;
    std::shared_ptr<CUI::TextBlock> m_detailLine2Time;
    std::shared_ptr<CUI::TextBlock> m_detailLine3Pub;
    std::shared_ptr<CUI::TextBlock> m_detailLine3Ver;
    std::shared_ptr<CUI::TextBlock> m_detailLine4Cmd;

    int m_statusLeftId = 0;
    int m_statusCountId = 0;
    int m_statusSelectionId = 0;
};

} // namespace AutoGuard
