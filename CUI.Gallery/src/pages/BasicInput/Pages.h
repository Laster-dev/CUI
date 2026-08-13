#pragma once

#include "framework/controls/UIElement.h"
#include <memory>

namespace Gallery {

std::shared_ptr<CUI::UIElement> BuildButtonPage();
std::shared_ptr<CUI::UIElement> BuildDropDownButtonPage();
std::shared_ptr<CUI::UIElement> BuildHyperlinkButtonPage();
std::shared_ptr<CUI::UIElement> BuildSplitButtonPage();
std::shared_ptr<CUI::UIElement> BuildToggleButtonPage();
std::shared_ptr<CUI::UIElement> BuildCheckBoxPage();
std::shared_ptr<CUI::UIElement> BuildRadioButtonPage();
std::shared_ptr<CUI::UIElement> BuildComboBoxPage();
std::shared_ptr<CUI::UIElement> BuildSliderPage();
std::shared_ptr<CUI::UIElement> BuildRangeSliderPage();
std::shared_ptr<CUI::UIElement> BuildRatingPage();
std::shared_ptr<CUI::UIElement> BuildToggleSwitchPage();
std::shared_ptr<CUI::UIElement> BuildColorPickerPage();
std::shared_ptr<CUI::UIElement> BuildSegmentedPage();

} // namespace Gallery
