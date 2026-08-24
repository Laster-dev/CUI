#include "App.h"
#include "framework/core/CUIDsl.h"
#include <shellscalingapi.h>

using namespace CUI;
using namespace CUI::DSL;

namespace AutoGuardApp {

AutoGuardApp::AutoGuardApp() {
    m_viewModel = std::make_shared<AutoGuard::MainViewModel>();
    m_view = std::make_shared<AutoGuard::MainView>(m_viewModel, &m_window);
}

int AutoGuardApp::Run() {
    if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        SetProcessDPIAware();
    }
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    m_root = m_view->Build();

    // Start initial background scan
    m_viewModel->StartScan([this]() {
        m_view->RefreshContent();
        m_view->UpdateStatusBar();
        m_view->UpdateDetailPanel();
        m_view->ShowToast("全盘扫描完成！");
    });

    auto windowBuilder = m_window.Fluent()
        .Title("AutoGuard - Windows 启动项安全管理专家")
        .Size(1440, 880)
        .Theme(ThemeMode::Dark)
        .Root(m_root)
        .Build();

    if (!windowBuilder) {
        CoUninitialize();
        return -1;
    }

    windowBuilder.Show().Run();
    CoUninitialize();
    return 0;
}

} // namespace AutoGuardApp
