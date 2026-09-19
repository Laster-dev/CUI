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

class AutoSuggestBox : public WidgetBase<AutoSuggestBox, ::CUI::AutoSuggestBox> {};
class BarChart : public WidgetBase<BarChart, ::CUI::BarChart> {};
class BreadcrumbBar : public WidgetBase<BreadcrumbBar, ::CUI::BreadcrumbBar> {};
class Button : public WidgetBase<Button, ::CUI::Button> {};
class Canvas : public WidgetBase<Canvas, ::CUI::Canvas> {};
class CanvasControl : public WidgetBase<CanvasControl, ::CUI::CanvasControl> {};
class CheckBox : public WidgetBase<CheckBox, ::CUI::CheckBox> {};
class ColorPicker : public WidgetBase<ColorPicker, ::CUI::ColorPicker> {};
class ComboBox : public WidgetBase<ComboBox, ::CUI::ComboBox> {};
class CommandBar : public WidgetBase<CommandBar, ::CUI::CommandBar> {};
class ContentDialog : public WidgetBase<ContentDialog, ::CUI::ContentDialog> {};
class DatePicker : public WidgetBase<DatePicker, ::CUI::DatePicker> {};
class DockManager : public WidgetBase<DockManager, ::CUI::DockManager> {};
class DockPanel : public WidgetBase<DockPanel, ::CUI::DockPanel> {};
class DropDownButton : public WidgetBase<DropDownButton, ::CUI::DropDownButton> {};
class Ellipse : public WidgetBase<Ellipse, ::CUI::Ellipse> {};
class Expander : public WidgetBase<Expander, ::CUI::Expander> {};
class FilePicker : public WidgetBase<FilePicker, ::CUI::FilePicker> {};
class Flyout : public WidgetBase<Flyout, ::CUI::Flyout> {};
class FolderPicker : public WidgetBase<FolderPicker, ::CUI::FolderPicker> {};
class Grid : public WidgetBase<Grid, ::CUI::Grid> {};
class HyperlinkButton : public WidgetBase<HyperlinkButton, ::CUI::HyperlinkButton> {};
class Image : public WidgetBase<Image, ::CUI::Image> {};
class InfoBar : public WidgetBase<InfoBar, ::CUI::InfoBar> {};
class Line : public WidgetBase<Line, ::CUI::Line> {};
class LineChart : public WidgetBase<LineChart, ::CUI::LineChart> {};
class ListBox : public WidgetBase<ListBox, ::CUI::ListBox> {};
class ListView : public WidgetBase<ListView, ::CUI::ListView> {};
class LogView : public WidgetBase<LogView, ::CUI::LogView> {};
class MarkdownView : public WidgetBase<MarkdownView, ::CUI::MarkdownView> {};
class MenuBar : public WidgetBase<MenuBar, ::CUI::MenuBar> {};
class NumberBox : public WidgetBase<NumberBox, ::CUI::NumberBox> {};
class PagingControl : public WidgetBase<PagingControl, ::CUI::PagingControl> {};
class Panel : public WidgetBase<Panel, ::CUI::Panel> {};
class PasswordBox : public WidgetBase<PasswordBox, ::CUI::PasswordBox> {};
class Path : public WidgetBase<Path, ::CUI::Path> {};
class PieChart : public WidgetBase<PieChart, ::CUI::PieChart> {};
class ProgressBar : public WidgetBase<ProgressBar, ::CUI::ProgressBar> {};
class ProgressRing : public WidgetBase<ProgressRing, ::CUI::ProgressRing> {};
class RadioButton : public WidgetBase<RadioButton, ::CUI::RadioButton> {};
class RangeSlider : public WidgetBase<RangeSlider, ::CUI::RangeSlider> {};
class RatingControl : public WidgetBase<RatingControl, ::CUI::RatingControl> {};
class Rectangle : public WidgetBase<Rectangle, ::CUI::Rectangle> {};
class ScrollViewer : public WidgetBase<ScrollViewer, ::CUI::ScrollViewer> {};
class SegmentedControl : public WidgetBase<SegmentedControl, ::CUI::SegmentedControl> {};
class Slider : public WidgetBase<Slider, ::CUI::Slider> {};
class SplitButton : public WidgetBase<SplitButton, ::CUI::SplitButton> {};
class Splitter : public WidgetBase<Splitter, ::CUI::Splitter> {};
class StackPanel : public WidgetBase<StackPanel, ::CUI::StackPanel> {};
class StatusBar : public WidgetBase<StatusBar, ::CUI::StatusBar> {};
class SvgIcon : public WidgetBase<SvgIcon, ::CUI::SvgIcon> {};
class TeachingTip : public WidgetBase<TeachingTip, ::CUI::TeachingTip> {};
class TextBlock : public WidgetBase<TextBlock, ::CUI::TextBlock> {};
class TextBox : public WidgetBase<TextBox, ::CUI::TextBox> {};
class TimePicker : public WidgetBase<TimePicker, ::CUI::TimePicker> {};
class Toast : public WidgetBase<Toast, ::CUI::Toast> {};
class ToggleButton : public WidgetBase<ToggleButton, ::CUI::ToggleButton> {};
class ToggleSwitch : public WidgetBase<ToggleSwitch, ::CUI::ToggleSwitch> {};
class TopologyView : public WidgetBase<TopologyView, ::CUI::TopologyView> {};
class TreeView : public WidgetBase<TreeView, ::CUI::TreeView> {};
class UniformGrid : public WidgetBase<UniformGrid, ::CUI::UniformGrid> {};
class WindowTitleBar : public WidgetBase<WindowTitleBar, ::CUI::WindowTitleBar> {};
class WrapPanel : public WidgetBase<WrapPanel, ::CUI::WrapPanel> {};

} // namespace Widgets
} // namespace CUI

