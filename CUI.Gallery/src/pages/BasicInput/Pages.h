#pragma once

#include "framework/controls/UIElement.h"
#include "framework/core/CUIDsl.h"
#include <memory>

namespace Gallery {

using CUI::DSL::Fluent::Button;

// Shell Pages
CUI::Element BuildHomePage();
CUI::Element BuildSettingsPage();

// Basic Input
CUI::Element BuildButtonPage();
CUI::Element BuildDropDownButtonPage();
CUI::Element BuildHyperlinkButtonPage();
CUI::Element BuildSplitButtonPage();
CUI::Element BuildToggleButtonPage();
CUI::Element BuildCheckBoxPage();
CUI::Element BuildRadioButtonPage();
CUI::Element BuildComboBoxPage();
CUI::Element BuildSliderPage();
CUI::Element BuildRangeSliderPage();
CUI::Element BuildRatingControlPage();
CUI::Element BuildToggleSwitchPage();
CUI::Element BuildColorPickerPage();
CUI::Element BuildSegmentedControlPage();

// Collections
CUI::Element BuildListBoxPage();
CUI::Element BuildListViewPage();
CUI::Element BuildTreeViewPage();

// Date and Time
CUI::Element BuildDatePickerPage();
CUI::Element BuildTimePickerPage();

// Dialogs and Flyouts
CUI::Element BuildContentDialogPage();
CUI::Element BuildFlyoutPage();
CUI::Element BuildTeachingTipPage();

// Layout
CUI::Element BuildCanvasPage();
CUI::Element BuildExpanderPage();
CUI::Element BuildGridPage();
CUI::Element BuildStackPanelPage();
CUI::Element BuildWrapPanelPage();
CUI::Element BuildDockPanelPage();
CUI::Element BuildUniformGridPage();
CUI::Element BuildSplitterPage();
CUI::Element BuildDockManagerPage();

// Media
CUI::Element BuildImagePage();
CUI::Element BuildLineChartPage();
CUI::Element BuildBarChartPage();
CUI::Element BuildPieChartPage();
CUI::Element BuildTerminalPage();
CUI::Element BuildTopologyPage();

// Menus and Toolbars
CUI::Element BuildCommandBarPage();
CUI::Element BuildMenuBarPage();
CUI::Element BuildContextMenuPage();

// Motion
CUI::Element BuildAnimationPage();
CUI::Element BuildThemeTransitionPage();
CUI::Element BuildPopupRevealPage();

// Navigation
CUI::Element BuildBreadcrumbBarPage();
CUI::Element BuildNavigationViewPage();
CUI::Element BuildTabViewPage();
CUI::Element BuildPagingControlPage();

// Scrolling
CUI::Element BuildScrollViewerPage();

// Status and Info
CUI::Element BuildInfoBarPage();
CUI::Element BuildProgressBarPage();
CUI::Element BuildProgressRingPage();
CUI::Element BuildStatusBarPage();
CUI::Element BuildToastPage();
CUI::Element BuildToolTipPage();
CUI::Element BuildLogViewPage();

// Styles
CUI::Element BuildThemePage();
CUI::Element BuildTokensPage();
CUI::Element BuildTypographyPage();
CUI::Element BuildShapePage();

// System
CUI::Element BuildFilePickerPage();
CUI::Element BuildFolderPickerPage();
CUI::Element BuildDragDropPage();
CUI::Element BuildCommandsPage();

// Text
CUI::Element BuildAutoSuggestBoxPage();
CUI::Element BuildNumberBoxPage();
CUI::Element BuildPasswordBoxPage();
CUI::Element BuildTextBlockPage();
CUI::Element BuildTextBoxPage();
CUI::Element BuildMarkdownViewPage();

// Windowing
CUI::Element BuildTitleBarPage();
CUI::Element BuildBackdropPage();
CUI::Element BuildWindowPage();

} // namespace Gallery
