#pragma once
/**
 * @file Widgets.h
 * @brief 具体控件句柄声明表（自动生成）。
 *
 * 每个句柄只需一行：通用链式方法继承自 WidgetBase，
 * 控件特有接口通过 operator-> 透传，故此处无任何转发代码。
 */
#include "Widget.h"
#include "../controls/AutoSuggestBox.h"
#include "../controls/BreadcrumbBar.h"
#include "../controls/Button.h"
#include "../controls/CanvasControl.h"
#include "../controls/chart/BarChart.h"
#include "../controls/chart/LineChart.h"
#include "../controls/chart/PieChart.h"
#include "../controls/CheckBox.h"
#include "../controls/ColorPicker.h"
#include "../controls/ComboBox.h"
#include "../controls/CommandBar.h"
#include "../controls/DatePicker.h"
#include "../controls/docking/DockManager.h"
#include "../controls/DropDownButton.h"
#include "../controls/Expander.h"
#include "../controls/FilePicker.h"
#include "../controls/Flyout.h"
#include "../controls/FolderPicker.h"
#include "../controls/HyperlinkButton.h"
#include "../controls/Image.h"
#include "../controls/InfoBar.h"
#include "../controls/ListBox.h"
#include "../controls/ListView.h"
#include "../controls/LogView.h"
#include "../controls/MarkdownView.h"
#include "../controls/MenuBar.h"
#include "../controls/MessageBox.h"
#include "../controls/NavigationViewItem.h"
#include "../controls/NumberBox.h"
#include "../controls/PagingControl.h"
#include "../controls/Panel.h"
#include "../controls/PasswordBox.h"
#include "../controls/ProgressBar.h"
#include "../controls/ProgressRing.h"
#include "../controls/RadioButton.h"
#include "../controls/RangeSlider.h"
#include "../controls/RatingControl.h"
#include "../controls/ScrollViewer.h"
#include "../controls/SegmentedControl.h"
#include "../controls/shapes/Shapes.h"
#include "../controls/Slider.h"
#include "../controls/SplitButton.h"
#include "../controls/Splitter.h"
#include "../controls/StatusBar.h"
#include "../controls/TeachingTip.h"
#include "../controls/TextBlock.h"
#include "../controls/TextBox.h"
#include "../controls/TimePicker.h"
#include "../controls/Toast.h"
#include "../controls/ToggleButton.h"
#include "../controls/ToggleSwitch.h"
#include "../controls/topology/TopologyView.h"
#include "../controls/TreeView.h"
#include "../controls/WindowTitleBar.h"

namespace CUI {
namespace Widgets {

class AutoSuggestBox : public WidgetBase<AutoSuggestBox, ::CUI::AutoSuggestBox> {
public: using WidgetBase::WidgetBase;
    AutoSuggestBox& Placeholder(const std::string& v1) const { impl_->ApplyPlaceholder(v1); return self(); }
    AutoSuggestBox& SuggestionItems(const std::vector<std::string>& v1) const { impl_->ApplySuggestionItems(v1); return self(); }
    AutoSuggestBox& SuggestionProvider(::CUI::AutoSuggestBox::SuggestionProviderFn v1) const { impl_->ApplySuggestionProvider(v1); return self(); }
    AutoSuggestBox& SuggestionItemHeight(float v1) const { impl_->ApplySuggestionItemHeight(v1); return self(); }
    AutoSuggestBox& MaxVisibleSuggestions(int v1) const { impl_->ApplyMaxVisibleSuggestions(v1); return self(); }
};
class BarChart : public WidgetBase<BarChart, ::CUI::BarChart> {
public: using WidgetBase::WidgetBase;
    BarChart& Categories(std::vector<std::string> v1, bool v2 = true) const { impl_->ApplyCategories(v1, v2); return self(); }
    BarChart& Series(std::vector<ChartSeries> v1, bool v2 = true) const { impl_->ApplySeries(v1, v2); return self(); }
    BarChart& LiveData(std::vector<std::string> v1, std::vector<ChartSeries> v2, bool v3 = false) const { impl_->ApplyLiveData(v1, v2, v3); return self(); }
    BarChart& ShowGrid(bool v1) const { impl_->ApplyShowGrid(v1); return self(); }
    BarChart& ShowLegend(bool v1) const { impl_->ApplyShowLegend(v1); return self(); }
    BarChart& ShowTooltip(bool v1) const { impl_->ApplyShowTooltip(v1); return self(); }
};
class BreadcrumbBar : public WidgetBase<BreadcrumbBar, ::CUI::BreadcrumbBar> {
public: using WidgetBase::WidgetBase;
    BreadcrumbBar& Path(const std::vector<std::string>& v1) const { impl_->ApplyPath(v1); return self(); }
};
class Button : public WidgetBase<Button, ::CUI::Button> { public: using WidgetBase::WidgetBase; };
class Canvas : public WidgetBase<Canvas, ::CUI::Canvas> { public: using WidgetBase::WidgetBase; };
class CanvasControl : public WidgetBase<CanvasControl, ::CUI::CanvasControl> {
public: using WidgetBase::WidgetBase;
    CanvasControl& OnDraw(::CUI::CanvasControl::DrawCallback v1) const { impl_->ApplyOnDraw(v1); return self(); }
    CanvasControl& OnCanvasMouseDown(::CUI::CanvasControl::MouseCallback v1) const { impl_->ApplyOnCanvasMouseDown(v1); return self(); }
    CanvasControl& OnCanvasMouseUp(::CUI::CanvasControl::MouseCallback v1) const { impl_->ApplyOnCanvasMouseUp(v1); return self(); }
    CanvasControl& OnCanvasMouseMove(::CUI::CanvasControl::MouseCallback v1) const { impl_->ApplyOnCanvasMouseMove(v1); return self(); }
    CanvasControl& OnTick(::CUI::CanvasControl::TickCallback v1) const { impl_->ApplyOnTick(v1); return self(); }
};
class CheckBox : public WidgetBase<CheckBox, ::CUI::CheckBox> {
public: using WidgetBase::WidgetBase;
    CheckBox& State(CheckState v1) const { impl_->ApplyState(v1); return self(); }
    CheckBox& IsChecked(bool v1) const { impl_->ApplyIsChecked(v1); return self(); }
    CheckBox& IsThreeState(bool v1) const { impl_->ApplyIsThreeState(v1); return self(); }
};
class ColorPicker : public WidgetBase<ColorPicker, ::CUI::ColorPicker> {
public: using WidgetBase::WidgetBase;
    ColorPicker& PopupOpen(bool v1) const { impl_->ApplyPopupOpen(v1); return self(); }
    ColorPicker& SelectedColor(D2D1_COLOR_F v1) const { impl_->ApplySelectedColor(v1); return self(); }
};
class ComboBox : public WidgetBase<ComboBox, ::CUI::ComboBox> {
public: using WidgetBase::WidgetBase;
    ComboBox& Items(const std::string& v1) const { impl_->ApplyItems(v1); return self(); }
    ComboBox& SelectedIndex(int v1) const { impl_->ApplySelectedIndex(v1); return self(); }
    ComboBox& DropDownOpen(bool v1) const { impl_->ApplyDropDownOpen(v1); return self(); }
};
class CommandBar : public WidgetBase<CommandBar, ::CUI::CommandBar> {
public: using WidgetBase::WidgetBase;
    CommandBar& LabelPosition(CommandBarLabelPosition v1) const { impl_->ApplyLabelPosition(v1); return self(); }
};
class ContentDialog : public WidgetBase<ContentDialog, ::CUI::ContentDialog> {
public: using WidgetBase::WidgetBase;
    ContentDialog& Title(const std::string& v1) const { impl_->ApplyTitle(v1); return self(); }
    ContentDialog& Message(const std::string& v1) const { impl_->ApplyMessage(v1); return self(); }
    ContentDialog& PrimaryButtonText(const std::string& v1) const { impl_->ApplyPrimaryButtonText(v1); return self(); }
    ContentDialog& SecondaryButtonText(const std::string& v1) const { impl_->ApplySecondaryButtonText(v1); return self(); }
    ContentDialog& CloseButtonText(const std::string& v1) const { impl_->ApplyCloseButtonText(v1); return self(); }
    ContentDialog& InputEnabled(bool v1, bool v2 = false) const { impl_->ApplyInputEnabled(v1, v2); return self(); }
    ContentDialog& InputText(const std::string& v1) const { impl_->ApplyInputText(v1); return self(); }
};
class DatePicker : public WidgetBase<DatePicker, ::CUI::DatePicker> {
public: using WidgetBase::WidgetBase;
    DatePicker& PopupOpen(bool v1) const { impl_->ApplyPopupOpen(v1); return self(); }
    DatePicker& Date(int v1, int v2, int v3) const { impl_->ApplyDate(v1, v2, v3); return self(); }
};
class DockManager : public WidgetBase<DockManager, ::CUI::DockManager> {
public: using WidgetBase::WidgetBase;
    DockManager& PaneAutoHide(int v1, bool v2) const { impl_->ApplyPaneAutoHide(v1, v2); return self(); }
    DockManager& SideSize(DockSide v1, float v2) const { impl_->ApplySideSize(v1, v2); return self(); }
};
class DockPanel : public WidgetBase<DockPanel, ::CUI::DockPanel> { public: using WidgetBase::WidgetBase; };
class DropDownButton : public WidgetBase<DropDownButton, ::CUI::DropDownButton> {
public: using WidgetBase::WidgetBase;
    DropDownButton& SelectedIndex(int v1) const { impl_->ApplySelectedIndex(v1); return self(); }
    DropDownButton& DropDownOpen(bool v1) const { impl_->ApplyDropDownOpen(v1); return self(); }
};
class Ellipse : public WidgetBase<Ellipse, ::CUI::Ellipse> {
public: using WidgetBase::WidgetBase;
    Ellipse& Fill(D2D1_COLOR_F v1) const { impl_->ApplyFill(v1); return self(); }
    Ellipse& Stroke(D2D1_COLOR_F v1) const { impl_->ApplyStroke(v1); return self(); }
    Ellipse& StrokeThickness(float v1) const { impl_->ApplyStrokeThickness(v1); return self(); }
    Ellipse& Stretch(::CUI::Stretch v1) const { impl_->ApplyStretch(v1); return self(); }
};
class Expander : public WidgetBase<Expander, ::CUI::Expander> {
public: using WidgetBase::WidgetBase;
    Expander& Header(const std::string& v1) const { impl_->ApplyHeader(v1); return self(); }
    Expander& Subtitle(const std::string& v1) const { impl_->ApplySubtitle(v1); return self(); }
    Expander& IsExpanded(bool v1) const { impl_->ApplyIsExpanded(v1); return self(); }
    Expander& Expanded(bool v1) const { impl_->ApplyExpanded(v1); return self(); }
    Expander& ExpandDirection(::CUI::ExpandDirection v1) const { impl_->ApplyExpandDirection(v1); return self(); }
    Expander& Content(std::shared_ptr<UIElement> v1) const { impl_->ApplyContent(v1); return self(); }
};
class FilePicker : public WidgetBase<FilePicker, ::CUI::FilePicker> {
public: using WidgetBase::WidgetBase;
    FilePicker& PopupOpen(bool v1) const { impl_->ApplyPopupOpen(v1); return self(); }
    FilePicker& Path(const std::string& v1) const { impl_->ApplyPath(v1); return self(); }
    FilePicker& DialogTitle(const std::string& v1) const { impl_->ApplyDialogTitle(v1); return self(); }
    FilePicker& Filter(const std::string& v1, const std::string& v2) const { impl_->ApplyFilter(v1, v2); return self(); }
    FilePicker& AllowDrop(bool v1) const { impl_->ApplyAllowDrop(v1); return self(); }
};
class Flyout : public WidgetBase<Flyout, ::CUI::Flyout> {
public: using WidgetBase::WidgetBase;
    Flyout& Content(std::shared_ptr<UIElement> v1) const { impl_->ApplyContent(v1); return self(); }
    Flyout& Placement(FlyoutPlacement v1) const { impl_->ApplyPlacement(v1); return self(); }
};
class FolderPicker : public WidgetBase<FolderPicker, ::CUI::FolderPicker> {
public: using WidgetBase::WidgetBase;
    FolderPicker& PopupOpen(bool v1) const { impl_->ApplyPopupOpen(v1); return self(); }
    FolderPicker& Path(const std::string& v1) const { impl_->ApplyPath(v1); return self(); }
    FolderPicker& DialogTitle(const std::string& v1) const { impl_->ApplyDialogTitle(v1); return self(); }
};
class Grid : public WidgetBase<Grid, ::CUI::Grid> {
public: using WidgetBase::WidgetBase;
    Grid& ColumnDefinitions(const std::string& v1) const { impl_->ApplyColumnDefinitions(v1); return self(); }
    Grid& RowDefinitions(const std::string& v1) const { impl_->ApplyRowDefinitions(v1); return self(); }
};
class HyperlinkButton : public WidgetBase<HyperlinkButton, ::CUI::HyperlinkButton> {
public: using WidgetBase::WidgetBase;
    HyperlinkButton& NavigateUri(const std::string& v1) const { impl_->ApplyNavigateUri(v1); return self(); }
};
class Image : public WidgetBase<Image, ::CUI::Image> {
public: using WidgetBase::WidgetBase;
    Image& ImageType(::CUI::ImageType v1) const { impl_->ApplyImageType(v1); return self(); }
    Image& BadgeText(const std::string& v1) const { impl_->ApplyBadgeText(v1); return self(); }
    Image& BadgeColor(D2D1_COLOR_F v1) const { impl_->ApplyBadgeColor(v1); return self(); }
    Image& Stretch(::CUI::Stretch v1) const { impl_->ApplyStretch(v1); return self(); }
    Image& Bitmap(ID2D1Bitmap1* v1) const { impl_->ApplyBitmap(v1); return self(); }
};
class InfoBar : public WidgetBase<InfoBar, ::CUI::InfoBar> {
public: using WidgetBase::WidgetBase;
    InfoBar& Title(const std::string& v1) const { impl_->ApplyTitle(v1); return self(); }
    InfoBar& Message(const std::string& v1) const { impl_->ApplyMessage(v1); return self(); }
    InfoBar& Severity(InfoBarSeverity v1) const { impl_->ApplySeverity(v1); return self(); }
    InfoBar& IsOpen(bool v1) const { impl_->ApplyIsOpen(v1); return self(); }
    InfoBar& IsClosable(bool v1) const { impl_->ApplyIsClosable(v1); return self(); }
    InfoBar& ActionText(const std::string& v1) const { impl_->ApplyActionText(v1); return self(); }
    InfoBar& ActionCommand(std::shared_ptr<::CUI::Command> v1) const { impl_->ApplyActionCommand(v1); return self(); }
};
class Line : public WidgetBase<Line, ::CUI::Line> {
public: using WidgetBase::WidgetBase;
    Line& X1(float v1) const { impl_->ApplyX1(v1); return self(); }
    Line& Y1(float v1) const { impl_->ApplyY1(v1); return self(); }
    Line& X2(float v1) const { impl_->ApplyX2(v1); return self(); }
    Line& Y2(float v1) const { impl_->ApplyY2(v1); return self(); }
    Line& Fill(D2D1_COLOR_F v1) const { impl_->ApplyFill(v1); return self(); }
    Line& Stroke(D2D1_COLOR_F v1) const { impl_->ApplyStroke(v1); return self(); }
    Line& StrokeThickness(float v1) const { impl_->ApplyStrokeThickness(v1); return self(); }
    Line& Stretch(::CUI::Stretch v1) const { impl_->ApplyStretch(v1); return self(); }
};
class LineChart : public WidgetBase<LineChart, ::CUI::LineChart> {
public: using WidgetBase::WidgetBase;
    LineChart& Categories(std::vector<std::string> v1, bool v2 = true) const { impl_->ApplyCategories(v1, v2); return self(); }
    LineChart& Series(std::vector<ChartSeries> v1, bool v2 = true) const { impl_->ApplySeries(v1, v2); return self(); }
    LineChart& LiveData(std::vector<std::string> v1, std::vector<ChartSeries> v2, bool v3 = false) const { impl_->ApplyLiveData(v1, v2, v3); return self(); }
    LineChart& ShowGrid(bool v1) const { impl_->ApplyShowGrid(v1); return self(); }
    LineChart& ShowLegend(bool v1) const { impl_->ApplyShowLegend(v1); return self(); }
    LineChart& ShowTooltip(bool v1) const { impl_->ApplyShowTooltip(v1); return self(); }
};
class ListBox : public WidgetBase<ListBox, ::CUI::ListBox> {
public: using WidgetBase::WidgetBase;
    ListBox& Items(const std::vector<std::string>& v1) const { impl_->ApplyItems(v1); return self(); }
    ListBox& SelectionMode(ListBoxSelectionMode v1) const { impl_->ApplySelectionMode(v1); return self(); }
    ListBox& SelectedIndex(int v1) const { impl_->ApplySelectedIndex(v1); return self(); }
    ListBox& SelectedItem(const std::string& v1) const { impl_->ApplySelectedItem(v1); return self(); }
    ListBox& ItemSelected(int v1, bool v2) const { impl_->ApplyItemSelected(v1, v2); return self(); }
    ListBox& CaretIndex(int v1) const { impl_->ApplyCaretIndex(v1); return self(); }
    ListBox& VirtualCount(size_t v1) const { impl_->ApplyVirtualCount(v1); return self(); }
    ListBox& VirtualMode(size_t v1, ::CUI::ListBox::ListBoxDataSource* v2) const { impl_->ApplyVirtualMode(v1, v2); return self(); }
    ListBox& AllowDrag(bool v1) const { impl_->ApplyAllowDrag(v1); return self(); }
    ListBox& AllowDrop(bool v1) const { impl_->ApplyAllowDrop(v1); return self(); }
};
class ListView : public WidgetBase<ListView, ::CUI::ListView> {
public: using WidgetBase::WidgetBase;
    ListView& ColumnVisible(int v1, bool v2) const { impl_->ApplyColumnVisible(v1, v2); return self(); }
    ListView& ShellContextMenuHandler(::CUI::ListView::ShellContextMenuHandler v1) const { impl_->ApplyShellContextMenuHandler(v1); return self(); }
    ListView& Rows(const std::vector<std::vector<std::string>>& v1) const { impl_->ApplyRows(v1); return self(); }
    ListView& RowIcons(const std::vector<HICON>& v1) const { impl_->ApplyRowIcons(v1); return self(); }
    ListView& RowTags(const std::vector<std::string>& v1) const { impl_->ApplyRowTags(v1); return self(); }
    ListView& VirtualMode(int v1, ListViewDataSource* v2) const { impl_->ApplyVirtualMode(v1, v2); return self(); }
    ListView& VirtualRowCount(int v1) const { impl_->ApplyVirtualRowCount(v1); return self(); }
    ListView& RowHeight(float v1) const { impl_->ApplyRowHeight(v1); return self(); }
    ListView& ShowScrollBars(bool v1) const { impl_->ApplyShowScrollBars(v1); return self(); }
    ListView& ShowGridLines(bool v1) const { impl_->ApplyShowGridLines(v1); return self(); }
    ListView& SelectionMode(ListViewSelectionMode v1) const { impl_->ApplySelectionMode(v1); return self(); }
    ListView& RowSelected(int v1, bool v2) const { impl_->ApplyRowSelected(v1, v2); return self(); }
    ListView& CaretIndex(int v1) const { impl_->ApplyCaretIndex(v1); return self(); }
};
class LogView : public WidgetBase<LogView, ::CUI::LogView> {
public: using WidgetBase::WidgetBase;
    LogView& Expanded(bool v1) const { impl_->ApplyExpanded(v1); return self(); }
    LogView& MaxEntries(uint32_t v1) const { impl_->ApplyMaxEntries(v1); return self(); }
    LogView& PersistEnabled(bool v1) const { impl_->ApplyPersistEnabled(v1); return self(); }
    LogView& PersistPath(std::string v1) const { impl_->ApplyPersistPath(v1); return self(); }
    LogView& LevelEnabled(LogLevel v1, bool v2) const { impl_->ApplyLevelEnabled(v1, v2); return self(); }
    LogView& LevelMask(uint8_t v1) const { impl_->ApplyLevelMask(v1); return self(); }
    LogView& FilterText(const std::string& v1) const { impl_->ApplyFilterText(v1); return self(); }
    LogView& FollowTail(bool v1) const { impl_->ApplyFollowTail(v1); return self(); }
};
class MarkdownView : public WidgetBase<MarkdownView, ::CUI::MarkdownView> {
public: using WidgetBase::WidgetBase;
    MarkdownView& Markdown(const std::string& v1) const { impl_->ApplyMarkdown(v1); return self(); }
    MarkdownView& ShowCodeLineNumbers(bool v1) const { impl_->ApplyShowCodeLineNumbers(v1); return self(); }
};
class MenuBar : public WidgetBase<MenuBar, ::CUI::MenuBar> { public: using WidgetBase::WidgetBase; };
class NumberBox : public WidgetBase<NumberBox, ::CUI::NumberBox> {
public: using WidgetBase::WidgetBase;
    NumberBox& Value(float v1) const { impl_->ApplyValue(v1); return self(); }
    NumberBox& Step(float v1) const { impl_->ApplyStep(v1); return self(); }
    NumberBox& Minimum(float v1) const { impl_->ApplyMinimum(v1); return self(); }
    NumberBox& Maximum(float v1) const { impl_->ApplyMaximum(v1); return self(); }
};
class PagingControl : public WidgetBase<PagingControl, ::CUI::PagingControl> {
public: using WidgetBase::WidgetBase;
    PagingControl& CurrentPage(int v1) const { impl_->ApplyCurrentPage(v1); return self(); }
    PagingControl& TotalPages(int v1) const { impl_->ApplyTotalPages(v1); return self(); }
};
class Panel : public WidgetBase<Panel, ::CUI::Panel> { public: using WidgetBase::WidgetBase; };
class PasswordBox : public WidgetBase<PasswordBox, ::CUI::PasswordBox> {
public: using WidgetBase::WidgetBase;
    PasswordBox& Password(const std::string& v1) const { impl_->ApplyPassword(v1); return self(); }
    PasswordBox& CompositionString(const std::wstring& v1) const { impl_->ApplyCompositionString(v1); return self(); }
    PasswordBox& Placeholder(const std::string& v1) const { impl_->ApplyPlaceholder(v1); return self(); }
    PasswordBox& IsPasswordMode(bool v1) const { impl_->ApplyIsPasswordMode(v1); return self(); }
    PasswordBox& IsPasswordRevealed(bool v1) const { impl_->ApplyIsPasswordRevealed(v1); return self(); }
    PasswordBox& ShowRevealButton(bool v1) const { impl_->ApplyShowRevealButton(v1); return self(); }
    PasswordBox& IsReadOnly(bool v1) const { impl_->ApplyIsReadOnly(v1); return self(); }
    PasswordBox& AcceptsReturn(bool v1) const { impl_->ApplyAcceptsReturn(v1); return self(); }
    PasswordBox& TextWrapping(bool v1) const { impl_->ApplyTextWrapping(v1); return self(); }
    PasswordBox& LineSpacing(float v1) const { impl_->ApplyLineSpacing(v1); return self(); }
    PasswordBox& LineHeight(float v1) const { impl_->ApplyLineHeight(v1); return self(); }
    PasswordBox& CaretBlinkRate(int v1) const { impl_->ApplyCaretBlinkRate(v1); return self(); }
    PasswordBox& CaretWidth(float v1) const { impl_->ApplyCaretWidth(v1); return self(); }
    PasswordBox& AllowDrop(bool v1) const { impl_->ApplyAllowDrop(v1); return self(); }
};
class Path : public WidgetBase<Path, ::CUI::Path> {
public: using WidgetBase::WidgetBase;
    Path& Data(const std::string& v1) const { impl_->ApplyData(v1); return self(); }
    Path& Fill(D2D1_COLOR_F v1) const { impl_->ApplyFill(v1); return self(); }
    Path& Stroke(D2D1_COLOR_F v1) const { impl_->ApplyStroke(v1); return self(); }
    Path& StrokeThickness(float v1) const { impl_->ApplyStrokeThickness(v1); return self(); }
    Path& Stretch(::CUI::Stretch v1) const { impl_->ApplyStretch(v1); return self(); }
};
class PieChart : public WidgetBase<PieChart, ::CUI::PieChart> {
public: using WidgetBase::WidgetBase;
    PieChart& Categories(std::vector<std::string> v1, bool v2 = true) const { impl_->ApplyCategories(v1, v2); return self(); }
    PieChart& Series(std::vector<ChartSeries> v1, bool v2 = true) const { impl_->ApplySeries(v1, v2); return self(); }
    PieChart& LiveData(std::vector<std::string> v1, std::vector<ChartSeries> v2, bool v3 = false) const { impl_->ApplyLiveData(v1, v2, v3); return self(); }
    PieChart& ShowGrid(bool v1) const { impl_->ApplyShowGrid(v1); return self(); }
    PieChart& ShowLegend(bool v1) const { impl_->ApplyShowLegend(v1); return self(); }
    PieChart& ShowTooltip(bool v1) const { impl_->ApplyShowTooltip(v1); return self(); }
};
class ProgressBar : public WidgetBase<ProgressBar, ::CUI::ProgressBar> {
public: using WidgetBase::WidgetBase;
    ProgressBar& Value(float v1) const { impl_->ApplyValue(v1); return self(); }
    ProgressBar& Minimum(float v1) const { impl_->ApplyMinimum(v1); return self(); }
    ProgressBar& Maximum(float v1) const { impl_->ApplyMaximum(v1); return self(); }
    ProgressBar& IsIndeterminate(bool v1) const { impl_->ApplyIsIndeterminate(v1); return self(); }
};
class ProgressRing : public WidgetBase<ProgressRing, ::CUI::ProgressRing> {
public: using WidgetBase::WidgetBase;
    ProgressRing& Value(float v1) const { impl_->ApplyValue(v1); return self(); }
    ProgressRing& Minimum(float v1) const { impl_->ApplyMinimum(v1); return self(); }
    ProgressRing& Maximum(float v1) const { impl_->ApplyMaximum(v1); return self(); }
    ProgressRing& IsIndeterminate(bool v1) const { impl_->ApplyIsIndeterminate(v1); return self(); }
};
class RadioButton : public WidgetBase<RadioButton, ::CUI::RadioButton> {
public: using WidgetBase::WidgetBase;
    RadioButton& GroupName(const std::string& v1) const { impl_->ApplyGroupName(v1); return self(); }
    RadioButton& State(CheckState v1) const { impl_->ApplyState(v1); return self(); }
    RadioButton& IsChecked(bool v1) const { impl_->ApplyIsChecked(v1); return self(); }
    RadioButton& IsThreeState(bool v1) const { impl_->ApplyIsThreeState(v1); return self(); }
};
class RangeSlider : public WidgetBase<RangeSlider, ::CUI::RangeSlider> {
public: using WidgetBase::WidgetBase;
    RangeSlider& Minimum(float v1) const { impl_->ApplyMinimum(v1); return self(); }
    RangeSlider& Maximum(float v1) const { impl_->ApplyMaximum(v1); return self(); }
    RangeSlider& Step(float v1) const { impl_->ApplyStep(v1); return self(); }
    RangeSlider& MinimumRange(float v1) const { impl_->ApplyMinimumRange(v1); return self(); }
    RangeSlider& LowerValue(float v1) const { impl_->ApplyLowerValue(v1); return self(); }
    RangeSlider& UpperValue(float v1) const { impl_->ApplyUpperValue(v1); return self(); }
    RangeSlider& Range(float v1, float v2) const { impl_->ApplyRange(v1, v2); return self(); }
};
class RatingControl : public WidgetBase<RatingControl, ::CUI::RatingControl> {
public: using WidgetBase::WidgetBase;
    RatingControl& Value(float v1) const { impl_->ApplyValue(v1); return self(); }
    RatingControl& MaxRating(int v1) const { impl_->ApplyMaxRating(v1); return self(); }
    RatingControl& Step(float v1) const { impl_->ApplyStep(v1); return self(); }
    RatingControl& IsReadOnly(bool v1) const { impl_->ApplyIsReadOnly(v1); return self(); }
    RatingControl& IsClearEnabled(bool v1) const { impl_->ApplyIsClearEnabled(v1); return self(); }
    RatingControl& StarSize(float v1) const { impl_->ApplyStarSize(v1); return self(); }
};
class Rectangle : public WidgetBase<Rectangle, ::CUI::Rectangle> {
public: using WidgetBase::WidgetBase;
    Rectangle& CornerRadius(float v1) const { impl_->ApplyCornerRadius(v1); return self(); }
    Rectangle& Fill(D2D1_COLOR_F v1) const { impl_->ApplyFill(v1); return self(); }
    Rectangle& Stroke(D2D1_COLOR_F v1) const { impl_->ApplyStroke(v1); return self(); }
    Rectangle& StrokeThickness(float v1) const { impl_->ApplyStrokeThickness(v1); return self(); }
    Rectangle& Stretch(::CUI::Stretch v1) const { impl_->ApplyStretch(v1); return self(); }
};
class ScrollViewer : public WidgetBase<ScrollViewer, ::CUI::ScrollViewer> {
public: using WidgetBase::WidgetBase;
    ScrollViewer& ScrollOffsetY(float v1) const { impl_->ApplyScrollOffsetY(v1); return self(); }
    ScrollViewer& OverlayScrollbar(bool v1) const { impl_->ApplyOverlayScrollbar(v1); return self(); }
};
class SegmentedControl : public WidgetBase<SegmentedControl, ::CUI::SegmentedControl> {
public: using WidgetBase::WidgetBase;
    SegmentedControl& Items(const std::string& v1) const { impl_->ApplyItems(v1); return self(); }
    SegmentedControl& SelectedIndex(int v1) const { impl_->ApplySelectedIndex(v1); return self(); }
    SegmentedControl& ItemEnabled(int v1, bool v2) const { impl_->ApplyItemEnabled(v1, v2); return self(); }
};
class Slider : public WidgetBase<Slider, ::CUI::Slider> {
public: using WidgetBase::WidgetBase;
    Slider& Value(float v1) const { impl_->ApplyValue(v1); return self(); }
    Slider& Minimum(float v1) const { impl_->ApplyMinimum(v1); return self(); }
    Slider& Maximum(float v1) const { impl_->ApplyMaximum(v1); return self(); }
    Slider& Step(float v1) const { impl_->ApplyStep(v1); return self(); }
};
class SplitButton : public WidgetBase<SplitButton, ::CUI::SplitButton> {
public: using WidgetBase::WidgetBase;
    SplitButton& SelectedIndex(int v1) const { impl_->ApplySelectedIndex(v1); return self(); }
    SplitButton& DropDownOpen(bool v1) const { impl_->ApplyDropDownOpen(v1); return self(); }
};
class Splitter : public WidgetBase<Splitter, ::CUI::Splitter> { public: using WidgetBase::WidgetBase; };
class StackPanel : public WidgetBase<StackPanel, ::CUI::StackPanel> { public: using WidgetBase::WidgetBase; };
class StatusBar : public WidgetBase<StatusBar, ::CUI::StatusBar> {
public: using WidgetBase::WidgetBase;
    StatusBar& ItemText(int v1, const std::string& v2) const { impl_->ApplyItemText(v1, v2); return self(); }
    StatusBar& ItemIcon(int v1, const std::string& v2) const { impl_->ApplyItemIcon(v1, v2); return self(); }
    StatusBar& ItemProgress(int v1, float v2) const { impl_->ApplyItemProgress(v1, v2); return self(); }
    StatusBar& ItemVisible(int v1, bool v2) const { impl_->ApplyItemVisible(v1, v2); return self(); }
    StatusBar& ItemFixedWidth(int v1, float v2) const { impl_->ApplyItemFixedWidth(v1, v2); return self(); }
};
class SvgIcon : public WidgetBase<SvgIcon, ::CUI::SvgIcon> {
public: using WidgetBase::WidgetBase;
    SvgIcon& Source(const std::string& v1) const { impl_->ApplySource(v1); return self(); }
    SvgIcon& TintColor(D2D1_COLOR_F v1) const { impl_->ApplyTintColor(v1); return self(); }
    SvgIcon& Fill(D2D1_COLOR_F v1) const { impl_->ApplyFill(v1); return self(); }
    SvgIcon& Stroke(D2D1_COLOR_F v1) const { impl_->ApplyStroke(v1); return self(); }
    SvgIcon& StrokeThickness(float v1) const { impl_->ApplyStrokeThickness(v1); return self(); }
    SvgIcon& Stretch(::CUI::Stretch v1) const { impl_->ApplyStretch(v1); return self(); }
};
class TeachingTip : public WidgetBase<TeachingTip, ::CUI::TeachingTip> {
public: using WidgetBase::WidgetBase;
    TeachingTip& Title(const std::string& v1) const { impl_->ApplyTitle(v1); return self(); }
    TeachingTip& Message(const std::string& v1) const { impl_->ApplyMessage(v1); return self(); }
    TeachingTip& ActionText(const std::string& v1) const { impl_->ApplyActionText(v1); return self(); }
    TeachingTip& IsCloseVisible(bool v1) const { impl_->ApplyIsCloseVisible(v1); return self(); }
    TeachingTip& IsModal(bool v1) const { impl_->ApplyIsModal(v1); return self(); }
    TeachingTip& PreferredPlacement(BubblePlacement v1) const { impl_->ApplyPreferredPlacement(v1); return self(); }
};
class TextBlock : public WidgetBase<TextBlock, ::CUI::TextBlock> {
public: using WidgetBase::WidgetBase;
    TextBlock& TextAlign(TextAlignment v1) const { impl_->ApplyTextAlign(v1); return self(); }
    TextBlock& VerticalAlign(TextVerticalAlignment v1) const { impl_->ApplyVerticalAlign(v1); return self(); }
    TextBlock& LineSpacing(float v1) const { impl_->ApplyLineSpacing(v1); return self(); }
    TextBlock& LineHeight(float v1) const { impl_->ApplyLineHeight(v1); return self(); }
};
class TextBox : public WidgetBase<TextBox, ::CUI::TextBox> {
public: using WidgetBase::WidgetBase;
    TextBox& CompositionString(const std::wstring& v1) const { impl_->ApplyCompositionString(v1); return self(); }
    TextBox& Placeholder(const std::string& v1) const { impl_->ApplyPlaceholder(v1); return self(); }
    TextBox& IsPasswordMode(bool v1) const { impl_->ApplyIsPasswordMode(v1); return self(); }
    TextBox& IsPasswordRevealed(bool v1) const { impl_->ApplyIsPasswordRevealed(v1); return self(); }
    TextBox& ShowRevealButton(bool v1) const { impl_->ApplyShowRevealButton(v1); return self(); }
    TextBox& IsReadOnly(bool v1) const { impl_->ApplyIsReadOnly(v1); return self(); }
    TextBox& AcceptsReturn(bool v1) const { impl_->ApplyAcceptsReturn(v1); return self(); }
    TextBox& TextWrapping(bool v1) const { impl_->ApplyTextWrapping(v1); return self(); }
    TextBox& LineSpacing(float v1) const { impl_->ApplyLineSpacing(v1); return self(); }
    TextBox& LineHeight(float v1) const { impl_->ApplyLineHeight(v1); return self(); }
    TextBox& CaretBlinkRate(int v1) const { impl_->ApplyCaretBlinkRate(v1); return self(); }
    TextBox& CaretWidth(float v1) const { impl_->ApplyCaretWidth(v1); return self(); }
    TextBox& AllowDrop(bool v1) const { impl_->ApplyAllowDrop(v1); return self(); }
};
class TimePicker : public WidgetBase<TimePicker, ::CUI::TimePicker> {
public: using WidgetBase::WidgetBase;
    TimePicker& PopupOpen(bool v1) const { impl_->ApplyPopupOpen(v1); return self(); }
    TimePicker& Time(int v1, int v2) const { impl_->ApplyTime(v1, v2); return self(); }
};
class Toast : public WidgetBase<Toast, ::CUI::Toast> {
public: using WidgetBase::WidgetBase;
    Toast& Host(ToastCenter* v1) const { impl_->ApplyHost(v1); return self(); }
    Toast& Title(const std::string& v1) const { impl_->ApplyTitle(v1); return self(); }
    Toast& Message(const std::string& v1) const { impl_->ApplyMessage(v1); return self(); }
    Toast& Type(ToastType v1) const { impl_->ApplyType(v1); return self(); }
    Toast& Corner(ToastCorner v1) const { impl_->ApplyCorner(v1); return self(); }
    Toast& DurationMs(int v1) const { impl_->ApplyDurationMs(v1); return self(); }
    Toast& AutoClose(bool v1) const { impl_->ApplyAutoClose(v1); return self(); }
    Toast& Background(const std::string& v1) const { impl_->ApplyBackground(v1); return self(); }
    Toast& Accent(const std::string& v1) const { impl_->ApplyAccent(v1); return self(); }
    Toast& TitleColor(const std::string& v1) const { impl_->ApplyTitleColor(v1); return self(); }
    Toast& MessageColor(const std::string& v1) const { impl_->ApplyMessageColor(v1); return self(); }
    Toast& OffsetX(float v1) const { impl_->ApplyOffsetX(v1); return self(); }
    Toast& OffsetY(float v1) const { impl_->ApplyOffsetY(v1); return self(); }
    Toast& Spacing(float v1) const { impl_->ApplySpacing(v1); return self(); }
    Toast& Closeable(bool v1) const { impl_->ApplyCloseable(v1); return self(); }
};
class ToggleButton : public WidgetBase<ToggleButton, ::CUI::ToggleButton> {
public: using WidgetBase::WidgetBase;
    ToggleButton& IsChecked(bool v1) const { impl_->ApplyIsChecked(v1); return self(); }
    ToggleButton& Checked(bool v1) const { impl_->ApplyChecked(v1); return self(); }
};
class ToggleSwitch : public WidgetBase<ToggleSwitch, ::CUI::ToggleSwitch> {
public: using WidgetBase::WidgetBase;
    ToggleSwitch& IsOn(bool v1) const { impl_->ApplyIsOn(v1); return self(); }
    ToggleSwitch& Header(const std::string& v1) const { impl_->ApplyHeader(v1); return self(); }
};
class TopologyView : public WidgetBase<TopologyView, ::CUI::TopologyView> {
public: using WidgetBase::WidgetBase;
    TopologyView& Nodes(const std::vector<std::shared_ptr<TopologyNode>>& v1) const { impl_->ApplyNodes(v1); return self(); }
    TopologyView& Edges(const std::vector<TopologyEdge>& v1) const { impl_->ApplyEdges(v1); return self(); }
    TopologyView& LayoutType(TopologyLayoutType v1) const { impl_->ApplyLayoutType(v1); return self(); }
    TopologyView& Zoom(float v1, bool v2 = true) const { impl_->ApplyZoom(v1, v2); return self(); }
    TopologyView& PanOffset(Point v1, bool v2 = true) const { impl_->ApplyPanOffset(v1, v2); return self(); }
    TopologyView& FlowParticlesEnabled(bool v1) const { impl_->ApplyFlowParticlesEnabled(v1); return self(); }
    TopologyView& IsReadOnly(bool v1) const { impl_->ApplyIsReadOnly(v1); return self(); }
    TopologyView& SelectedItem(std::shared_ptr<TopologyNode> v1) const { impl_->ApplySelectedItem(v1); return self(); }
};
class TreeView : public WidgetBase<TreeView, ::CUI::TreeView> {
public: using WidgetBase::WidgetBase;
    TreeView& Items(const std::vector<std::shared_ptr<TreeViewItem>>& v1) const { impl_->ApplyItems(v1); return self(); }
    TreeView& SelectedItem(std::shared_ptr<TreeViewItem> v1) const { impl_->ApplySelectedItem(v1); return self(); }
    TreeView& ItemExpanded(std::shared_ptr<TreeViewItem> v1, bool v2) const { impl_->ApplyItemExpanded(v1, v2); return self(); }
    TreeView& IndentWidth(float v1) const { impl_->ApplyIndentWidth(v1); return self(); }
};
class UniformGrid : public WidgetBase<UniformGrid, ::CUI::UniformGrid> { public: using WidgetBase::WidgetBase; };
class WindowTitleBar : public WidgetBase<WindowTitleBar, ::CUI::WindowTitleBar> {
public: using WidgetBase::WidgetBase;
    WindowTitleBar& RightContent(const std::shared_ptr<UIElement>& v1) const { impl_->ApplyRightContent(v1); return self(); }
    WindowTitleBar& Title(const std::string& v1) const { impl_->ApplyTitle(v1); return self(); }
    WindowTitleBar& IconText(const std::string& v1) const { impl_->ApplyIconText(v1); return self(); }
    WindowTitleBar& NativeIcon(HICON v1, bool v2 = false) const { impl_->ApplyNativeIcon(v1, v2); return self(); }
    WindowTitleBar& IsMinimizeButtonVisible(bool v1) const { impl_->ApplyIsMinimizeButtonVisible(v1); return self(); }
    WindowTitleBar& IsMaximizeButtonVisible(bool v1) const { impl_->ApplyIsMaximizeButtonVisible(v1); return self(); }
    WindowTitleBar& IsCloseButtonVisible(bool v1) const { impl_->ApplyIsCloseButtonVisible(v1); return self(); }
    WindowTitleBar& IsMinimizeButtonEnabled(bool v1) const { impl_->ApplyIsMinimizeButtonEnabled(v1); return self(); }
    WindowTitleBar& IsMaximizeButtonEnabled(bool v1) const { impl_->ApplyIsMaximizeButtonEnabled(v1); return self(); }
    WindowTitleBar& IsCloseButtonEnabled(bool v1) const { impl_->ApplyIsCloseButtonEnabled(v1); return self(); }
};
class WrapPanel : public WidgetBase<WrapPanel, ::CUI::WrapPanel> { public: using WidgetBase::WidgetBase; };

class ContextMenu : public WidgetBase<ContextMenu, ::CUI::ContextMenu> { public: using WidgetBase::WidgetBase; };

    class MenuItem : public WidgetBase<MenuItem, ::CUI::MenuItem> { public: using WidgetBase::WidgetBase; };
    class NavigationViewItem : public WidgetBase<NavigationViewItem, ::CUI::NavigationViewItem> { public: using WidgetBase::WidgetBase; };
    class NavigationViewItemHeader : public WidgetBase<NavigationViewItemHeader, ::CUI::NavigationViewItemHeader> { public: using WidgetBase::WidgetBase; };
    class NavigationViewItemSeparator : public WidgetBase<NavigationViewItemSeparator, ::CUI::NavigationViewItemSeparator> { public: using WidgetBase::WidgetBase; };
    class NavigationView : public WidgetBase<NavigationView, ::CUI::NavigationView> { public: using WidgetBase::WidgetBase; };

} // namespace Widgets
} // namespace CUI

