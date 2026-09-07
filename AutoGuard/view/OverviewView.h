#pragma once

#include "../viewmodel/MainViewModel.h"
#include "framework/core/CUIDsl.h"
#include <memory>
#include <functional>

namespace AutoGuard {

class OverviewView {
public:
    OverviewView(
        std::shared_ptr<MainViewModel> viewModel,
        std::function<void(const std::string&)> onNavigate);
    ~OverviewView() = default;

    std::shared_ptr<CUI::UIElement> Build();

private:
    std::shared_ptr<MainViewModel> m_viewModel;
    std::function<void(const std::string&)> m_onNavigate;
};

} // namespace AutoGuard
