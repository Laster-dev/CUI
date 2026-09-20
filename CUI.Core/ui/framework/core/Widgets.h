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
    AutoSuggestBox& Placeholder(const std::string& v1) { impl_->SetPlaceholder(v1); return *this; }
    AutoSuggestBox& SuggestionItems(const std::vector<std::string>& v1) { impl_->SetSuggestionItems(v1); return *this; }
    AutoSuggestBox& SuggestionProvider(::CUI::AutoSuggestBox::SuggestionProviderFn v1) { impl_->SetSuggestionProvider(v1); return *this; }
    AutoSuggestBox& SuggestionItemHeight(float v1) { impl_->SetSuggestionItemHeight(v1); return *this; }
    AutoSuggestBox& MaxVisibleSuggestions(int v1) { impl_->SetMaxVisibleSuggestions(v1); return *this; }
};
class BarChart : public WidgetBase<BarChart, ::CUI::BarChart> {
public: using WidgetBase::WidgetBase;
    BarChart& Categories(std::vector<std::string> v1, bool v2 = true) { impl_->SetCategories(v1, v2); return *this; }
    BarChart& Series(std::vector<ChartSeries> v1, bool v2 = true) { impl_->SetSeries(v1, v2); return *this; }
    BarChart& LiveData(std::vector<std::string> v1, std::vector<ChartSeries> v2, bool v3 = false) { impl_->SetLiveData(v1, v2, v3); return *this; }
    BarChart& ShowGrid(bool v1) { impl_->SetShowGrid(v1); return *this; }
    BarChart& ShowLegend(bool v1) { impl_->SetShowLegend(v1); return *this; }
    BarChart& ShowTooltip(bool v1) { impl_->SetShowTooltip(v1); return *this; }
};
class BreadcrumbBar : public WidgetBase<BreadcrumbBar, ::CUI::BreadcrumbBar> {
public: using WidgetBase::WidgetBase;
    BreadcrumbBar& Path(const std::vector<std::string>& v1) { impl_->SetPath(v1); return *this; }
};
class Button : public WidgetBase<Button, ::CUI::Button> { public: using WidgetBase::WidgetBase; };
class Canvas : public WidgetBase<Canvas, ::CUI::Canvas> { public: using WidgetBase::WidgetBase; };
class CanvasControl : public WidgetBase<CanvasControl, ::CUI::CanvasControl> {
public: using WidgetBase::WidgetBase;
    CanvasControl& OnDraw(::CUI::CanvasControl::DrawCallback v1) { impl_->SetOnDraw(v1); return *this; }
    CanvasControl& OnCanvasMouseDown(::CUI::CanvasControl::MouseCallback v1) { impl_->SetOnCanvasMouseDown(v1); return *this; }
    CanvasControl& OnCanvasMouseUp(::CUI::CanvasControl::MouseCallback v1) { impl_->SetOnCanvasMouseUp(v1); return *this; }
    CanvasControl& OnCanvasMouseMove(::CUI::CanvasControl::MouseCallback v1) { impl_->SetOnCanvasMouseMove(v1); return *this; }
    CanvasControl& OnTick(::CUI::CanvasControl::TickCallback v1) { impl_->SetOnTick(v1); return *this; }
};
class CheckBox : public WidgetBase<CheckBox, ::CUI::CheckBox> {
public: using WidgetBase::WidgetBase;
    CheckBox& State(CheckState v1) { impl_->SetState(v1); return *this; }
    CheckBox& IsChecked(bool v1) { impl_->SetIsChecked(v1); return *this; }
    CheckBox& IsThreeState(bool v1) { impl_->SetIsThreeState(v1); return *this; }
};
class ColorPicker : public WidgetBase<ColorPicker, ::CUI::ColorPicker> {
public: using WidgetBase::WidgetBase;
    ColorPicker& PopupOpen(bool v1) { impl_->SetPopupOpen(v1); return *this; }
    ColorPicker& SelectedColor(D2D1_COLOR_F v1) { impl_->SetSelectedColor(v1); return *this; }
};
class ComboBox : public WidgetBase<ComboBox, ::CUI::ComboBox> {
public: using WidgetBase::WidgetBase;
    ComboBox& Items(const std::string& v1) { impl_->SetItems(v1); return *this; }
    ComboBox& SelectedIndex(int v1) { impl_->SetSelectedIndex(v1); return *this; }
    ComboBox& DropDownOpen(bool v1) { impl_->SetDropDownOpen(v1); return *this; }
};
class CommandBar : public WidgetBase<CommandBar, ::CUI::CommandBar> {
public: using WidgetBase::WidgetBase;
    CommandBar& LabelPosition(CommandBarLabelPosition v1) { impl_->SetLabelPosition(v1); return *this; }
};
class ContentDialog : public WidgetBase<ContentDialog, ::CUI::ContentDialog> {
public: using WidgetBase::WidgetBase;
    ContentDialog& Title(const std::string& v1) { impl_->SetTitle(v1); return *this; }
    ContentDialog& Message(const std::string& v1) { impl_->SetMessage(v1); return *this; }
    ContentDialog& PrimaryButtonText(const std::string& v1) { impl_->SetPrimaryButtonText(v1); return *this; }
    ContentDialog& SecondaryButtonText(const std::string& v1) { impl_->SetSecondaryButtonText(v1); return *this; }
    ContentDialog& CloseButtonText(const std::string& v1) { impl_->SetCloseButtonText(v1); return *this; }
    ContentDialog& InputEnabled(bool v1, bool v2 = false) { impl_->SetInputEnabled(v1, v2); return *this; }
    ContentDialog& InputText(const std::string& v1) { impl_->SetInputText(v1); return *this; }
};
class DatePicker : public WidgetBase<DatePicker, ::CUI::DatePicker> {
public: using WidgetBase::WidgetBase;
    DatePicker& PopupOpen(bool v1) { impl_->SetPopupOpen(v1); return *this; }
    DatePicker& Date(int v1, int v2, int v3) { impl_->SetDate(v1, v2, v3); return *this; }
};
class DockManager : public WidgetBase<DockManager, ::CUI::DockManager> {
public: using WidgetBase::WidgetBase;
    DockManager& PaneAutoHide(int v1, bool v2) { impl_->SetPaneAutoHide(v1, v2); return *this; }
    DockManager& SideSize(DockSide v1, float v2) { impl_->SetSideSize(v1, v2); return *this; }
};
class DockPanel : public WidgetBase<DockPanel, ::CUI::DockPanel> { public: using WidgetBase::WidgetBase; };
class DropDownButton : public WidgetBase<DropDownButton, ::CUI::DropDownButton> {
public: using WidgetBase::WidgetBase;
    DropDownButton& SelectedIndex(int v1) { impl_->SetSelectedIndex(v1); return *this; }
    DropDownButton& DropDownOpen(bool v1) { impl_->SetDropDownOpen(v1); return *this; }
};
class Ellipse : public WidgetBase<Ellipse, ::CUI::Ellipse> {
public: using WidgetBase::WidgetBase;
    Ellipse& Fill(D2D1_COLOR_F v1) { impl_->SetFill(v1); return *this; }
    Ellipse& Stroke(D2D1_COLOR_F v1) { impl_->SetStroke(v1); return *this; }
    Ellipse& StrokeThickness(float v1) { impl_->SetStrokeThickness(v1); return *this; }
    Ellipse& Stretch(Stretch v1) { impl_->SetStretch(v1); return *this; }
};
class Expander : public WidgetBase<Expander, ::CUI::Expander> {
public: using WidgetBase::WidgetBase;
    Expander& Header(const std::string& v1) { impl_->SetHeader(v1); return *this; }
    Expander& Subtitle(const std::string& v1) { impl_->SetSubtitle(v1); return *this; }
    Expander& IsExpanded(bool v1) { impl_->SetIsExpanded(v1); return *this; }
    Expander& Expanded(bool v1) { impl_->SetExpanded(v1); return *this; }
    Expander& ExpandDirection(ExpandDirection v1) { impl_->SetExpandDirection(v1); return *this; }
    Expander& Content(std::shared_ptr<UIElement> v1) { impl_->SetContent(v1); return *this; }
};
class FilePicker : public WidgetBase<FilePicker, ::CUI::FilePicker> {
public: using WidgetBase::WidgetBase;
    FilePicker& PopupOpen(bool v1) { impl_->SetPopupOpen(v1); return *this; }
    FilePicker& Path(const std::string& v1) { impl_->SetPath(v1); return *this; }
    FilePicker& DialogTitle(const std::string& v1) { impl_->SetDialogTitle(v1); return *this; }
    FilePicker& Filter(const std::string& v1, const std::string& v2) { impl_->SetFilter(v1, v2); return *this; }
    FilePicker& AllowDrop(bool v1) { impl_->SetAllowDrop(v1); return *this; }
};
class Flyout : public WidgetBase<Flyout, ::CUI::Flyout> {
public: using WidgetBase::WidgetBase;
    Flyout& Content(std::shared_ptr<UIElement> v1) { impl_->SetContent(v1); return *this; }
    Flyout& Placement(FlyoutPlacement v1) { impl_->SetPlacement(v1); return *this; }
};
class FolderPicker : public WidgetBase<FolderPicker, ::CUI::FolderPicker> {
public: using WidgetBase::WidgetBase;
    FolderPicker& PopupOpen(bool v1) { impl_->SetPopupOpen(v1); return *this; }
    FolderPicker& Path(const std::string& v1) { impl_->SetPath(v1); return *this; }
    FolderPicker& DialogTitle(const std::string& v1) { impl_->SetDialogTitle(v1); return *this; }
};
class Grid : public WidgetBase<Grid, ::CUI::Grid> {
public: using WidgetBase::WidgetBase;
    Grid& ColumnDefinitions(const std::string& v1) { impl_->SetColumnDefinitions(v1); return *this; }
    Grid& RowDefinitions(const std::string& v1) { impl_->SetRowDefinitions(v1); return *this; }
};
class HyperlinkButton : public WidgetBase<HyperlinkButton, ::CUI::HyperlinkButton> {
public: using WidgetBase::WidgetBase;
    HyperlinkButton& NavigateUri(const std::string& v1) { impl_->SetNavigateUri(v1); return *this; }
};
class Image : public WidgetBase<Image, ::CUI::Image> {
public: using WidgetBase::WidgetBase;
    Image& ImageType(ImageType v1) { impl_->SetImageType(v1); return *this; }
    Image& BadgeText(const std::string& v1) { impl_->SetBadgeText(v1); return *this; }
    Image& BadgeColor(D2D1_COLOR_F v1) { impl_->SetBadgeColor(v1); return *this; }
    Image& Stretch(Stretch v1) { impl_->SetStretch(v1); return *this; }
    Image& Bitmap(ID2D1Bitmap1* v1) { impl_->SetBitmap(v1); return *this; }
};
class InfoBar : public WidgetBase<InfoBar, ::CUI::InfoBar> {
public: using WidgetBase::WidgetBase;
    InfoBar& Title(const std::string& v1) { impl_->SetTitle(v1); return *this; }
    InfoBar& Message(const std::string& v1) { impl_->SetMessage(v1); return *this; }
    InfoBar& Severity(InfoBarSeverity v1) { impl_->SetSeverity(v1); return *this; }
    InfoBar& IsOpen(bool v1) { impl_->SetIsOpen(v1); return *this; }
    InfoBar& IsClosable(bool v1) { impl_->SetIsClosable(v1); return *this; }
    InfoBar& ActionText(const std::string& v1) { impl_->SetActionText(v1); return *this; }
    InfoBar& ActionCommand(std::shared_ptr<::CUI::Command> v1) { impl_->SetActionCommand(v1); return *this; }
};
class Line : public WidgetBase<Line, ::CUI::Line> {
public: using WidgetBase::WidgetBase;
    Line& X1(float v1) { impl_->SetX1(v1); return *this; }
    Line& Y1(float v1) { impl_->SetY1(v1); return *this; }
    Line& X2(float v1) { impl_->SetX2(v1); return *this; }
    Line& Y2(float v1) { impl_->SetY2(v1); return *this; }
    Line& Fill(D2D1_COLOR_F v1) { impl_->SetFill(v1); return *this; }
    Line& Stroke(D2D1_COLOR_F v1) { impl_->SetStroke(v1); return *this; }
    Line& StrokeThickness(float v1) { impl_->SetStrokeThickness(v1); return *this; }
    Line& Stretch(Stretch v1) { impl_->SetStretch(v1); return *this; }
};
class LineChart : public WidgetBase<LineChart, ::CUI::LineChart> {
public: using WidgetBase::WidgetBase;
    LineChart& Categories(std::vector<std::string> v1, bool v2 = true) { impl_->SetCategories(v1, v2); return *this; }
    LineChart& Series(std::vector<ChartSeries> v1, bool v2 = true) { impl_->SetSeries(v1, v2); return *this; }
    LineChart& LiveData(std::vector<std::string> v1, std::vector<ChartSeries> v2, bool v3 = false) { impl_->SetLiveData(v1, v2, v3); return *this; }
    LineChart& ShowGrid(bool v1) { impl_->SetShowGrid(v1); return *this; }
    LineChart& ShowLegend(bool v1) { impl_->SetShowLegend(v1); return *this; }
    LineChart& ShowTooltip(bool v1) { impl_->SetShowTooltip(v1); return *this; }
};
class ListBox : public WidgetBase<ListBox, ::CUI::ListBox> {
public: using WidgetBase::WidgetBase;
    ListBox& Items(const std::vector<std::string>& v1) { impl_->SetItems(v1); return *this; }
    ListBox& SelectionMode(ListBoxSelectionMode v1) { impl_->SetSelectionMode(v1); return *this; }
    ListBox& SelectedIndex(int v1) { impl_->SetSelectedIndex(v1); return *this; }
    ListBox& SelectedItem(const std::string& v1) { impl_->SetSelectedItem(v1); return *this; }
    ListBox& ItemSelected(int v1, bool v2) { impl_->SetItemSelected(v1, v2); return *this; }
    ListBox& CaretIndex(int v1) { impl_->SetCaretIndex(v1); return *this; }
    ListBox& VirtualCount(size_t v1) { impl_->SetVirtualCount(v1); return *this; }
    ListBox& VirtualMode(size_t v1, ::CUI::ListBox::ListBoxDataSource* v2) { impl_->SetVirtualMode(v1, v2); return *this; }
    ListBox& AllowDrag(bool v1) { impl_->SetAllowDrag(v1); return *this; }
    ListBox& AllowDrop(bool v1) { impl_->SetAllowDrop(v1); return *this; }
};
class ListView : public WidgetBase<ListView, ::CUI::ListView> {
public: using WidgetBase::WidgetBase;
    ListView& ColumnVisible(int v1, bool v2) { impl_->SetColumnVisible(v1, v2); return *this; }
    ListView& ShellContextMenuHandler(::CUI::ListView::ShellContextMenuHandler v1) { impl_->SetShellContextMenuHandler(v1); return *this; }
    ListView& Rows(const std::vector<std::vector<std::string>>& v1) { impl_->SetRows(v1); return *this; }
    ListView& RowIcons(const std::vector<HICON>& v1) { impl_->SetRowIcons(v1); return *this; }
    ListView& RowTags(const std::vector<std::string>& v1) { impl_->SetRowTags(v1); return *this; }
    ListView& VirtualMode(int v1, ListViewDataSource* v2) { impl_->SetVirtualMode(v1, v2); return *this; }
    ListView& VirtualRowCount(int v1) { impl_->SetVirtualRowCount(v1); return *this; }
    ListView& RowHeight(float v1) { impl_->SetRowHeight(v1); return *this; }
    ListView& ShowScrollBars(bool v1) { impl_->SetShowScrollBars(v1); return *this; }
    ListView& ShowGridLines(bool v1) { impl_->SetShowGridLines(v1); return *this; }
    ListView& SelectionMode(ListViewSelectionMode v1) { impl_->SetSelectionMode(v1); return *this; }
    ListView& RowSelected(int v1, bool v2) { impl_->SetRowSelected(v1, v2); return *this; }
    ListView& CaretIndex(int v1) { impl_->SetCaretIndex(v1); return *this; }
};
class LogView : public WidgetBase<LogView, ::CUI::LogView> {
public: using WidgetBase::WidgetBase;
    LogView& Expanded(bool v1) { impl_->SetExpanded(v1); return *this; }
    LogView& MaxEntries(uint32_t v1) { impl_->SetMaxEntries(v1); return *this; }
    LogView& PersistEnabled(bool v1) { impl_->SetPersistEnabled(v1); return *this; }
    LogView& PersistPath(std::string v1) { impl_->SetPersistPath(v1); return *this; }
    LogView& LevelEnabled(LogLevel v1, bool v2) { impl_->SetLevelEnabled(v1, v2); return *this; }
    LogView& LevelMask(uint8_t v1) { impl_->SetLevelMask(v1); return *this; }
    LogView& FilterText(const std::string& v1) { impl_->SetFilterText(v1); return *this; }
    LogView& FollowTail(bool v1) { impl_->SetFollowTail(v1); return *this; }
};
class MarkdownView : public WidgetBase<MarkdownView, ::CUI::MarkdownView> {
public: using WidgetBase::WidgetBase;
    MarkdownView& Markdown(const std::string& v1) { impl_->SetMarkdown(v1); return *this; }
    MarkdownView& ShowCodeLineNumbers(bool v1) { impl_->SetShowCodeLineNumbers(v1); return *this; }
};
class MenuBar : public WidgetBase<MenuBar, ::CUI::MenuBar> { public: using WidgetBase::WidgetBase; };
class NumberBox : public WidgetBase<NumberBox, ::CUI::NumberBox> {
public: using WidgetBase::WidgetBase;
    NumberBox& Value(float v1) { impl_->SetValue(v1); return *this; }
    NumberBox& Step(float v1) { impl_->SetStep(v1); return *this; }
    NumberBox& Minimum(float v1) { impl_->SetMinimum(v1); return *this; }
    NumberBox& Maximum(float v1) { impl_->SetMaximum(v1); return *this; }
};
class PagingControl : public WidgetBase<PagingControl, ::CUI::PagingControl> {
public: using WidgetBase::WidgetBase;
    PagingControl& CurrentPage(int v1) { impl_->SetCurrentPage(v1); return *this; }
    PagingControl& TotalPages(int v1) { impl_->SetTotalPages(v1); return *this; }
};
class Panel : public WidgetBase<Panel, ::CUI::Panel> { public: using WidgetBase::WidgetBase; };
class PasswordBox : public WidgetBase<PasswordBox, ::CUI::PasswordBox> {
public: using WidgetBase::WidgetBase;
    PasswordBox& Password(const std::string& v1) { impl_->SetPassword(v1); return *this; }
    PasswordBox& CompositionString(const std::wstring& v1) { impl_->SetCompositionString(v1); return *this; }
    PasswordBox& Placeholder(const std::string& v1) { impl_->SetPlaceholder(v1); return *this; }
    PasswordBox& IsPasswordMode(bool v1) { impl_->SetIsPasswordMode(v1); return *this; }
    PasswordBox& IsPasswordRevealed(bool v1) { impl_->SetIsPasswordRevealed(v1); return *this; }
    PasswordBox& ShowRevealButton(bool v1) { impl_->SetShowRevealButton(v1); return *this; }
    PasswordBox& IsReadOnly(bool v1) { impl_->SetIsReadOnly(v1); return *this; }
    PasswordBox& AcceptsReturn(bool v1) { impl_->SetAcceptsReturn(v1); return *this; }
    PasswordBox& TextWrapping(bool v1) { impl_->SetTextWrapping(v1); return *this; }
    PasswordBox& LineSpacing(float v1) { impl_->SetLineSpacing(v1); return *this; }
    PasswordBox& LineHeight(float v1) { impl_->SetLineHeight(v1); return *this; }
    PasswordBox& CaretBlinkRate(int v1) { impl_->SetCaretBlinkRate(v1); return *this; }
    PasswordBox& CaretWidth(float v1) { impl_->SetCaretWidth(v1); return *this; }
    PasswordBox& AllowDrop(bool v1) { impl_->SetAllowDrop(v1); return *this; }
};
class Path : public WidgetBase<Path, ::CUI::Path> {
public: using WidgetBase::WidgetBase;
    Path& Data(const std::string& v1) { impl_->SetData(v1); return *this; }
    Path& Fill(D2D1_COLOR_F v1) { impl_->SetFill(v1); return *this; }
    Path& Stroke(D2D1_COLOR_F v1) { impl_->SetStroke(v1); return *this; }
    Path& StrokeThickness(float v1) { impl_->SetStrokeThickness(v1); return *this; }
    Path& Stretch(Stretch v1) { impl_->SetStretch(v1); return *this; }
};
class PieChart : public WidgetBase<PieChart, ::CUI::PieChart> {
public: using WidgetBase::WidgetBase;
    PieChart& Categories(std::vector<std::string> v1, bool v2 = true) { impl_->SetCategories(v1, v2); return *this; }
    PieChart& Series(std::vector<ChartSeries> v1, bool v2 = true) { impl_->SetSeries(v1, v2); return *this; }
    PieChart& LiveData(std::vector<std::string> v1, std::vector<ChartSeries> v2, bool v3 = false) { impl_->SetLiveData(v1, v2, v3); return *this; }
    PieChart& ShowGrid(bool v1) { impl_->SetShowGrid(v1); return *this; }
    PieChart& ShowLegend(bool v1) { impl_->SetShowLegend(v1); return *this; }
    PieChart& ShowTooltip(bool v1) { impl_->SetShowTooltip(v1); return *this; }
};
class ProgressBar : public WidgetBase<ProgressBar, ::CUI::ProgressBar> {
public: using WidgetBase::WidgetBase;
    ProgressBar& Value(float v1) { impl_->SetValue(v1); return *this; }
    ProgressBar& Minimum(float v1) { impl_->SetMinimum(v1); return *this; }
    ProgressBar& Maximum(float v1) { impl_->SetMaximum(v1); return *this; }
    ProgressBar& IsIndeterminate(bool v1) { impl_->SetIsIndeterminate(v1); return *this; }
};
class ProgressRing : public WidgetBase<ProgressRing, ::CUI::ProgressRing> {
public: using WidgetBase::WidgetBase;
    ProgressRing& Value(float v1) { impl_->SetValue(v1); return *this; }
    ProgressRing& Minimum(float v1) { impl_->SetMinimum(v1); return *this; }
    ProgressRing& Maximum(float v1) { impl_->SetMaximum(v1); return *this; }
    ProgressRing& IsIndeterminate(bool v1) { impl_->SetIsIndeterminate(v1); return *this; }
};
class RadioButton : public WidgetBase<RadioButton, ::CUI::RadioButton> {
public: using WidgetBase::WidgetBase;
    RadioButton& GroupName(const std::string& v1) { impl_->SetGroupName(v1); return *this; }
    RadioButton& State(CheckState v1) { impl_->SetState(v1); return *this; }
    RadioButton& IsChecked(bool v1) { impl_->SetIsChecked(v1); return *this; }
    RadioButton& IsThreeState(bool v1) { impl_->SetIsThreeState(v1); return *this; }
};
class RangeSlider : public WidgetBase<RangeSlider, ::CUI::RangeSlider> {
public: using WidgetBase::WidgetBase;
    RangeSlider& Minimum(float v1) { impl_->SetMinimum(v1); return *this; }
    RangeSlider& Maximum(float v1) { impl_->SetMaximum(v1); return *this; }
    RangeSlider& Step(float v1) { impl_->SetStep(v1); return *this; }
    RangeSlider& MinimumRange(float v1) { impl_->SetMinimumRange(v1); return *this; }
    RangeSlider& LowerValue(float v1) { impl_->SetLowerValue(v1); return *this; }
    RangeSlider& UpperValue(float v1) { impl_->SetUpperValue(v1); return *this; }
    RangeSlider& Range(float v1, float v2) { impl_->SetRange(v1, v2); return *this; }
};
class RatingControl : public WidgetBase<RatingControl, ::CUI::RatingControl> {
public: using WidgetBase::WidgetBase;
    RatingControl& Value(float v1) { impl_->SetValue(v1); return *this; }
    RatingControl& MaxRating(int v1) { impl_->SetMaxRating(v1); return *this; }
    RatingControl& Step(float v1) { impl_->SetStep(v1); return *this; }
    RatingControl& IsReadOnly(bool v1) { impl_->SetIsReadOnly(v1); return *this; }
    RatingControl& IsClearEnabled(bool v1) { impl_->SetIsClearEnabled(v1); return *this; }
    RatingControl& StarSize(float v1) { impl_->SetStarSize(v1); return *this; }
};
class Rectangle : public WidgetBase<Rectangle, ::CUI::Rectangle> {
public: using WidgetBase::WidgetBase;
    Rectangle& CornerRadius(float v1) { impl_->SetCornerRadius(v1); return *this; }
    Rectangle& Fill(D2D1_COLOR_F v1) { impl_->SetFill(v1); return *this; }
    Rectangle& Stroke(D2D1_COLOR_F v1) { impl_->SetStroke(v1); return *this; }
    Rectangle& StrokeThickness(float v1) { impl_->SetStrokeThickness(v1); return *this; }
    Rectangle& Stretch(Stretch v1) { impl_->SetStretch(v1); return *this; }
};
class ScrollViewer : public WidgetBase<ScrollViewer, ::CUI::ScrollViewer> {
public: using WidgetBase::WidgetBase;
    ScrollViewer& ScrollOffsetY(float v1) { impl_->SetScrollOffsetY(v1); return *this; }
    ScrollViewer& OverlayScrollbar(bool v1) { impl_->SetOverlayScrollbar(v1); return *this; }
};
class SegmentedControl : public WidgetBase<SegmentedControl, ::CUI::SegmentedControl> {
public: using WidgetBase::WidgetBase;
    SegmentedControl& Items(const std::string& v1) { impl_->SetItems(v1); return *this; }
    SegmentedControl& SelectedIndex(int v1) { impl_->SetSelectedIndex(v1); return *this; }
    SegmentedControl& ItemEnabled(int v1, bool v2) { impl_->SetItemEnabled(v1, v2); return *this; }
};
class Slider : public WidgetBase<Slider, ::CUI::Slider> {
public: using WidgetBase::WidgetBase;
    Slider& Value(float v1) { impl_->SetValue(v1); return *this; }
    Slider& Minimum(float v1) { impl_->SetMinimum(v1); return *this; }
    Slider& Maximum(float v1) { impl_->SetMaximum(v1); return *this; }
    Slider& Step(float v1) { impl_->SetStep(v1); return *this; }
};
class SplitButton : public WidgetBase<SplitButton, ::CUI::SplitButton> {
public: using WidgetBase::WidgetBase;
    SplitButton& SelectedIndex(int v1) { impl_->SetSelectedIndex(v1); return *this; }
    SplitButton& DropDownOpen(bool v1) { impl_->SetDropDownOpen(v1); return *this; }
};
class Splitter : public WidgetBase<Splitter, ::CUI::Splitter> { public: using WidgetBase::WidgetBase; };
class StackPanel : public WidgetBase<StackPanel, ::CUI::StackPanel> { public: using WidgetBase::WidgetBase; };
class StatusBar : public WidgetBase<StatusBar, ::CUI::StatusBar> {
public: using WidgetBase::WidgetBase;
    StatusBar& ItemText(int v1, const std::string& v2) { impl_->SetItemText(v1, v2); return *this; }
    StatusBar& ItemIcon(int v1, const std::string& v2) { impl_->SetItemIcon(v1, v2); return *this; }
    StatusBar& ItemProgress(int v1, float v2) { impl_->SetItemProgress(v1, v2); return *this; }
    StatusBar& ItemVisible(int v1, bool v2) { impl_->SetItemVisible(v1, v2); return *this; }
    StatusBar& ItemFixedWidth(int v1, float v2) { impl_->SetItemFixedWidth(v1, v2); return *this; }
};
class SvgIcon : public WidgetBase<SvgIcon, ::CUI::SvgIcon> {
public: using WidgetBase::WidgetBase;
    SvgIcon& Source(const std::string& v1) { impl_->SetSource(v1); return *this; }
    SvgIcon& TintColor(D2D1_COLOR_F v1) { impl_->SetTintColor(v1); return *this; }
    SvgIcon& Fill(D2D1_COLOR_F v1) { impl_->SetFill(v1); return *this; }
    SvgIcon& Stroke(D2D1_COLOR_F v1) { impl_->SetStroke(v1); return *this; }
    SvgIcon& StrokeThickness(float v1) { impl_->SetStrokeThickness(v1); return *this; }
    SvgIcon& Stretch(Stretch v1) { impl_->SetStretch(v1); return *this; }
};
class TeachingTip : public WidgetBase<TeachingTip, ::CUI::TeachingTip> {
public: using WidgetBase::WidgetBase;
    TeachingTip& Title(const std::string& v1) { impl_->SetTitle(v1); return *this; }
    TeachingTip& Message(const std::string& v1) { impl_->SetMessage(v1); return *this; }
    TeachingTip& ActionText(const std::string& v1) { impl_->SetActionText(v1); return *this; }
    TeachingTip& IsCloseVisible(bool v1) { impl_->SetIsCloseVisible(v1); return *this; }
    TeachingTip& IsModal(bool v1) { impl_->SetIsModal(v1); return *this; }
    TeachingTip& PreferredPlacement(BubblePlacement v1) { impl_->SetPreferredPlacement(v1); return *this; }
};
class TextBlock : public WidgetBase<TextBlock, ::CUI::TextBlock> {
public: using WidgetBase::WidgetBase;
    TextBlock& TextAlign(TextAlignment v1) { impl_->SetTextAlign(v1); return *this; }
    TextBlock& VerticalAlign(TextVerticalAlignment v1) { impl_->SetVerticalAlign(v1); return *this; }
    TextBlock& LineSpacing(float v1) { impl_->SetLineSpacing(v1); return *this; }
    TextBlock& LineHeight(float v1) { impl_->SetLineHeight(v1); return *this; }
};
class TextBox : public WidgetBase<TextBox, ::CUI::TextBox> {
public: using WidgetBase::WidgetBase;
    TextBox& CompositionString(const std::wstring& v1) { impl_->SetCompositionString(v1); return *this; }
    TextBox& Placeholder(const std::string& v1) { impl_->SetPlaceholder(v1); return *this; }
    TextBox& IsPasswordMode(bool v1) { impl_->SetIsPasswordMode(v1); return *this; }
    TextBox& IsPasswordRevealed(bool v1) { impl_->SetIsPasswordRevealed(v1); return *this; }
    TextBox& ShowRevealButton(bool v1) { impl_->SetShowRevealButton(v1); return *this; }
    TextBox& IsReadOnly(bool v1) { impl_->SetIsReadOnly(v1); return *this; }
    TextBox& AcceptsReturn(bool v1) { impl_->SetAcceptsReturn(v1); return *this; }
    TextBox& TextWrapping(bool v1) { impl_->SetTextWrapping(v1); return *this; }
    TextBox& LineSpacing(float v1) { impl_->SetLineSpacing(v1); return *this; }
    TextBox& LineHeight(float v1) { impl_->SetLineHeight(v1); return *this; }
    TextBox& CaretBlinkRate(int v1) { impl_->SetCaretBlinkRate(v1); return *this; }
    TextBox& CaretWidth(float v1) { impl_->SetCaretWidth(v1); return *this; }
    TextBox& AllowDrop(bool v1) { impl_->SetAllowDrop(v1); return *this; }
};
class TimePicker : public WidgetBase<TimePicker, ::CUI::TimePicker> {
public: using WidgetBase::WidgetBase;
    TimePicker& PopupOpen(bool v1) { impl_->SetPopupOpen(v1); return *this; }
    TimePicker& Time(int v1, int v2) { impl_->SetTime(v1, v2); return *this; }
};
class Toast : public WidgetBase<Toast, ::CUI::Toast> {
public: using WidgetBase::WidgetBase;
    Toast& Host(ToastCenter* v1) { impl_->SetHost(v1); return *this; }
    Toast& Title(const std::string& v1) { impl_->SetTitle(v1); return *this; }
    Toast& Message(const std::string& v1) { impl_->SetMessage(v1); return *this; }
    Toast& Type(ToastType v1) { impl_->SetType(v1); return *this; }
    Toast& Corner(ToastCorner v1) { impl_->SetCorner(v1); return *this; }
    Toast& DurationMs(int v1) { impl_->SetDurationMs(v1); return *this; }
    Toast& AutoClose(bool v1) { impl_->SetAutoClose(v1); return *this; }
    Toast& Background(const std::string& v1) { impl_->SetBackground(v1); return *this; }
    Toast& Accent(const std::string& v1) { impl_->SetAccent(v1); return *this; }
    Toast& TitleColor(const std::string& v1) { impl_->SetTitleColor(v1); return *this; }
    Toast& MessageColor(const std::string& v1) { impl_->SetMessageColor(v1); return *this; }
    Toast& OffsetX(float v1) { impl_->SetOffsetX(v1); return *this; }
    Toast& OffsetY(float v1) { impl_->SetOffsetY(v1); return *this; }
    Toast& Spacing(float v1) { impl_->SetSpacing(v1); return *this; }
    Toast& Closeable(bool v1) { impl_->SetCloseable(v1); return *this; }
};
class ToggleButton : public WidgetBase<ToggleButton, ::CUI::ToggleButton> {
public: using WidgetBase::WidgetBase;
    ToggleButton& IsChecked(bool v1) { impl_->SetIsChecked(v1); return *this; }
    ToggleButton& Checked(bool v1) { impl_->SetChecked(v1); return *this; }
};
class ToggleSwitch : public WidgetBase<ToggleSwitch, ::CUI::ToggleSwitch> {
public: using WidgetBase::WidgetBase;
    ToggleSwitch& IsOn(bool v1) { impl_->SetIsOn(v1); return *this; }
    ToggleSwitch& Header(const std::string& v1) { impl_->SetHeader(v1); return *this; }
};
class TopologyView : public WidgetBase<TopologyView, ::CUI::TopologyView> {
public: using WidgetBase::WidgetBase;
    TopologyView& Nodes(const std::vector<std::shared_ptr<TopologyNode>>& v1) { impl_->SetNodes(v1); return *this; }
    TopologyView& Edges(const std::vector<TopologyEdge>& v1) { impl_->SetEdges(v1); return *this; }
    TopologyView& LayoutType(TopologyLayoutType v1) { impl_->SetLayoutType(v1); return *this; }
    TopologyView& Zoom(float v1, bool v2 = true) { impl_->SetZoom(v1, v2); return *this; }
    TopologyView& PanOffset(Point v1, bool v2 = true) { impl_->SetPanOffset(v1, v2); return *this; }
    TopologyView& FlowParticlesEnabled(bool v1) { impl_->SetFlowParticlesEnabled(v1); return *this; }
    TopologyView& IsReadOnly(bool v1) { impl_->SetIsReadOnly(v1); return *this; }
    TopologyView& SelectedItem(std::shared_ptr<TopologyNode> v1) { impl_->SetSelectedItem(v1); return *this; }
};
class TreeView : public WidgetBase<TreeView, ::CUI::TreeView> {
public: using WidgetBase::WidgetBase;
    TreeView& Items(const std::vector<std::shared_ptr<TreeViewItem>>& v1) { impl_->SetItems(v1); return *this; }
    TreeView& SelectedItem(std::shared_ptr<TreeViewItem> v1) { impl_->SetSelectedItem(v1); return *this; }
    TreeView& ItemExpanded(std::shared_ptr<TreeViewItem> v1, bool v2) { impl_->SetItemExpanded(v1, v2); return *this; }
    TreeView& IndentWidth(float v1) { impl_->SetIndentWidth(v1); return *this; }
};
class UniformGrid : public WidgetBase<UniformGrid, ::CUI::UniformGrid> { public: using WidgetBase::WidgetBase; };
class WindowTitleBar : public WidgetBase<WindowTitleBar, ::CUI::WindowTitleBar> {
public: using WidgetBase::WidgetBase;
    WindowTitleBar& RightContent(const std::shared_ptr<UIElement>& v1) { impl_->SetRightContent(v1); return *this; }
    WindowTitleBar& Title(const std::string& v1) { impl_->SetTitle(v1); return *this; }
    WindowTitleBar& IconText(const std::string& v1) { impl_->SetIconText(v1); return *this; }
    WindowTitleBar& NativeIcon(HICON v1, bool v2 = false) { impl_->SetNativeIcon(v1, v2); return *this; }
    WindowTitleBar& IsMinimizeButtonVisible(bool v1) { impl_->SetIsMinimizeButtonVisible(v1); return *this; }
    WindowTitleBar& IsMaximizeButtonVisible(bool v1) { impl_->SetIsMaximizeButtonVisible(v1); return *this; }
    WindowTitleBar& IsCloseButtonVisible(bool v1) { impl_->SetIsCloseButtonVisible(v1); return *this; }
    WindowTitleBar& IsMinimizeButtonEnabled(bool v1) { impl_->SetIsMinimizeButtonEnabled(v1); return *this; }
    WindowTitleBar& IsMaximizeButtonEnabled(bool v1) { impl_->SetIsMaximizeButtonEnabled(v1); return *this; }
    WindowTitleBar& IsCloseButtonEnabled(bool v1) { impl_->SetIsCloseButtonEnabled(v1); return *this; }
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

