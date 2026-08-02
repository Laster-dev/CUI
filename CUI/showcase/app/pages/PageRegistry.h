#pragma once

#include "../ShowcaseContext.h"
#include "framework/controls/UIElement.h"
#include <memory>
#include <string>

struct ShowcasePage {
    std::string tabTitle;
    std::shared_ptr<CUI::UIElement> content;
};

ShowcasePage BuildButtonPage(const ShowcaseContext& ctx);
ShowcasePage BuildTextBlockPage(const ShowcaseContext& ctx);
ShowcasePage BuildTextBoxPage(const ShowcaseContext& ctx);
ShowcasePage BuildPasswordBoxPage(const ShowcaseContext& ctx);
ShowcasePage BuildCheckBoxPage(const ShowcaseContext& ctx);
ShowcasePage BuildRadioButtonPage(const ShowcaseContext& ctx);
ShowcasePage BuildToggleSwitchPage(const ShowcaseContext& ctx);
ShowcasePage BuildComboBoxPage(const ShowcaseContext& ctx);
ShowcasePage BuildSliderPage(const ShowcaseContext& ctx);
ShowcasePage BuildProgressBarPage(const ShowcaseContext& ctx);
ShowcasePage BuildNumberBoxPage(const ShowcaseContext& ctx);
ShowcasePage BuildDatePickerPage(const ShowcaseContext& ctx);
ShowcasePage BuildTimePickerPage(const ShowcaseContext& ctx);
ShowcasePage BuildColorPickerPage(const ShowcaseContext& ctx);
ShowcasePage BuildBreadcrumbPage(const ShowcaseContext& ctx);
ShowcasePage BuildPagingPage(const ShowcaseContext& ctx);
ShowcasePage BuildSplitterPage(const ShowcaseContext& ctx);
ShowcasePage BuildCollapsePage(const ShowcaseContext& ctx);
ShowcasePage BuildListBoxPage(const ShowcaseContext& ctx);
ShowcasePage BuildListViewPage(const ShowcaseContext& ctx);
ShowcasePage BuildTreeViewPage(const ShowcaseContext& ctx);
ShowcasePage BuildHyperlinkPage(const ShowcaseContext& ctx);
ShowcasePage BuildStreamPage(const ShowcaseContext& ctx);
ShowcasePage BuildDialogPage(const ShowcaseContext& ctx);
ShowcasePage BuildToastPage(const ShowcaseContext& ctx);
ShowcasePage BuildGridPage(const ShowcaseContext& ctx);
ShowcasePage BuildCanvasPage(const ShowcaseContext& ctx);
ShowcasePage BuildWrapPage(const ShowcaseContext& ctx);
ShowcasePage BuildDockPage(const ShowcaseContext& ctx);
ShowcasePage BuildUniformPage(const ShowcaseContext& ctx);
ShowcasePage BuildStackPanelPage(const ShowcaseContext& ctx);
ShowcasePage BuildScrollViewerPage(const ShowcaseContext& ctx);
