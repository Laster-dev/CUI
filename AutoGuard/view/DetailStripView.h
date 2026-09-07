#pragma once

#include "../viewmodel/MainViewModel.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/Panel.h"
#include <memory>

namespace AutoGuard {

class HIconElement;

class DetailStripView {
public:
    explicit DetailStripView(std::shared_ptr<MainViewModel> viewModel);
    ~DetailStripView();

    std::shared_ptr<CUI::UIElement> Build();
    void Update();

private:
    std::shared_ptr<MainViewModel> m_viewModel;

    std::shared_ptr<CUI::UIElement> m_root;
    std::shared_ptr<HIconElement> m_iconElement;

    std::shared_ptr<CUI::TextBlock> m_titleName;
    std::shared_ptr<CUI::TextBlock> m_statusBadge;
    std::shared_ptr<CUI::TextBlock> m_locationBadge;
    std::shared_ptr<CUI::TextBlock> m_descriptionText;
    std::shared_ptr<CUI::TextBlock> m_commandText;
    std::shared_ptr<CUI::TextBlock> m_reasonText;

    std::shared_ptr<CUI::TextBlock> m_publisherText;
    std::shared_ptr<CUI::TextBlock> m_versionSizeText;
    std::shared_ptr<CUI::TextBlock> m_timeText;
    std::shared_ptr<CUI::TextBlock> m_sourceText;
};

} // namespace AutoGuard
