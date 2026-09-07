#pragma once

#include "../viewmodel/MainViewModel.h"
#include "framework/window/Window.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/ListView.h"
#include "framework/controls/ContextMenu.h"
#include <memory>
#include <functional>
#include <vector>

namespace AutoGuard {

class TreeCategoryView {
public:
    TreeCategoryView(
        std::shared_ptr<MainViewModel> viewModel,
        CUI::Window* window,
        std::function<void()> onSelectionChanged,
        std::function<void(const std::string&)> onShowToast,
        std::function<void()> onRefresh);
    ~TreeCategoryView() = default;

    std::shared_ptr<CUI::UIElement> Build(const std::string& title, const std::string& desc);

private:
    std::shared_ptr<MainViewModel> m_viewModel;
    CUI::Window* m_window = nullptr;
    std::function<void()> m_onSelectionChanged;
    std::function<void(const std::string&)> m_onShowToast;
    std::function<void()> m_onRefresh;
};

} // namespace AutoGuard
