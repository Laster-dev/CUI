#pragma once

#include "framework/core/Widget.h"

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

    CUI::Widgets::Ref<::CUI::TextBlock> m_titleName;
    CUI::Widgets::Ref<::CUI::TextBlock> m_statusBadge;
    CUI::Widgets::Ref<::CUI::TextBlock> m_locationBadge;
    CUI::Widgets::Ref<::CUI::TextBlock> m_descriptionText;
    CUI::Widgets::Ref<::CUI::TextBlock> m_commandText;
    CUI::Widgets::Ref<::CUI::TextBlock> m_reasonText;

    CUI::Widgets::Ref<::CUI::TextBlock> m_publisherText;
    CUI::Widgets::Ref<::CUI::TextBlock> m_versionSizeText;
    CUI::Widgets::Ref<::CUI::TextBlock> m_timeText;
    CUI::Widgets::Ref<::CUI::TextBlock> m_sourceText;
};

} // namespace AutoGuard
