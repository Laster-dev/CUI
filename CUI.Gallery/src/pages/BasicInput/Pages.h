#pragma once

#include "framework/controls/UIElement.h"
#include <memory>

namespace Gallery {

// Shell Pages
std::shared_ptr<CUI::UIElement> BuildHomePage();
std::shared_ptr<CUI::UIElement> BuildSettingsPage();

// Basic Input
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
std::shared_ptr<CUI::UIElement> BuildRatingControlPage();
std::shared_ptr<CUI::UIElement> BuildToggleSwitchPage();
std::shared_ptr<CUI::UIElement> BuildColorPickerPage();
std::shared_ptr<CUI::UIElement> BuildSegmentedControlPage();

// Collections
std::shared_ptr<CUI::UIElement> BuildListBoxPage();
std::shared_ptr<CUI::UIElement> BuildListViewPage();
std::shared_ptr<CUI::UIElement> BuildTreeViewPage();

// Date and Time
std::shared_ptr<CUI::UIElement> BuildDatePickerPage();
std::shared_ptr<CUI::UIElement> BuildTimePickerPage();

// Dialogs and Flyouts
std::shared_ptr<CUI::UIElement> BuildContentDialogPage();
std::shared_ptr<CUI::UIElement> BuildFlyoutPage();
std::shared_ptr<CUI::UIElement> BuildTeachingTipPage();

// Layout
std::shared_ptr<CUI::UIElement> BuildCanvasPage();
std::shared_ptr<CUI::UIElement> BuildExpanderPage();
std::shared_ptr<CUI::UIElement> BuildGridPage();
std::shared_ptr<CUI::UIElement> BuildStackPanelPage();
std::shared_ptr<CUI::UIElement> BuildWrapPanelPage();
std::shared_ptr<CUI::UIElement> BuildDockPanelPage();
std::shared_ptr<CUI::UIElement> BuildUniformGridPage();
std::shared_ptr<CUI::UIElement> BuildSplitterPage();
std::shared_ptr<CUI::UIElement> BuildDockManagerPage();

// Media
std::shared_ptr<CUI::UIElement> BuildImagePage();
std::shared_ptr<CUI::UIElement> BuildLineChartPage();
std::shared_ptr<CUI::UIElement> BuildBarChartPage();
std::shared_ptr<CUI::UIElement> BuildPieChartPage();
std::shared_ptr<CUI::UIElement> BuildTerminalPage();

// Menus and Toolbars
std::shared_ptr<CUI::UIElement> BuildCommandBarPage();
std::shared_ptr<CUI::UIElement> BuildMenuBarPage();
std::shared_ptr<CUI::UIElement> BuildContextMenuPage();

// Motion
std::shared_ptr<CUI::UIElement> BuildAnimationPage();
std::shared_ptr<CUI::UIElement> BuildThemeTransitionPage();
std::shared_ptr<CUI::UIElement> BuildPopupRevealPage();

// Navigation
std::shared_ptr<CUI::UIElement> BuildBreadcrumbBarPage();
std::shared_ptr<CUI::UIElement> BuildNavigationViewPage();
std::shared_ptr<CUI::UIElement> BuildTabViewPage();
std::shared_ptr<CUI::UIElement> BuildPagingControlPage();

// Scrolling
std::shared_ptr<CUI::UIElement> BuildScrollViewerPage();

// Status and Info
std::shared_ptr<CUI::UIElement> BuildInfoBarPage();
std::shared_ptr<CUI::UIElement> BuildProgressBarPage();
std::shared_ptr<CUI::UIElement> BuildProgressRingPage();
std::shared_ptr<CUI::UIElement> BuildStatusBarPage();
std::shared_ptr<CUI::UIElement> BuildToastPage();
std::shared_ptr<CUI::UIElement> BuildToolTipPage();
std::shared_ptr<CUI::UIElement> BuildLogViewPage();

// Styles
std::shared_ptr<CUI::UIElement> BuildThemePage();
std::shared_ptr<CUI::UIElement> BuildTokensPage();
std::shared_ptr<CUI::UIElement> BuildTypographyPage();
std::shared_ptr<CUI::UIElement> BuildShapePage();

// System
std::shared_ptr<CUI::UIElement> BuildFilePickerPage();
std::shared_ptr<CUI::UIElement> BuildFolderPickerPage();
std::shared_ptr<CUI::UIElement> BuildDragDropPage();
std::shared_ptr<CUI::UIElement> BuildCommandsPage();

// Text
std::shared_ptr<CUI::UIElement> BuildAutoSuggestBoxPage();
std::shared_ptr<CUI::UIElement> BuildNumberBoxPage();
std::shared_ptr<CUI::UIElement> BuildPasswordBoxPage();
std::shared_ptr<CUI::UIElement> BuildTextBlockPage();
std::shared_ptr<CUI::UIElement> BuildTextBoxPage();
std::shared_ptr<CUI::UIElement> BuildMarkdownViewPage();

// Windowing
std::shared_ptr<CUI::UIElement> BuildTitleBarPage();
std::shared_ptr<CUI::UIElement> BuildBackdropPage();
std::shared_ptr<CUI::UIElement> BuildWindowPage();

} // namespace Gallery
