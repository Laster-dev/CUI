#include "catalog/CategoryRegistrations.h"
#include "pages/BasicInput/Pages.h"

namespace Gallery {

void BasicInputCatalog::Register(std::vector<Entry>& entries) {
    entries.push_back({ "button", "Button", "按钮用于触发操作。", Category::BasicInput, BuildButtonPage });
    entries.push_back({ "dropdownbutton", "DropDownButton", "整个按钮打开菜单。", Category::BasicInput, BuildDropDownButtonPage });
    entries.push_back({ "hyperlinkbutton", "HyperlinkButton", "用于导航的文本样式按钮。", Category::BasicInput, BuildHyperlinkButtonPage });
    entries.push_back({ "splitbutton", "SplitButton", "默认操作外加更多命令。", Category::BasicInput, BuildSplitButtonPage });
    entries.push_back({ "togglebutton", "ToggleButton", "保持开或关的按钮。", Category::BasicInput, BuildToggleButtonPage });
    entries.push_back({ "checkbox", "CheckBox", "打开或关闭某个选项。", Category::BasicInput, BuildCheckBoxPage });
    entries.push_back({ "radiobutton", "RadioButton", "从一组中选择一项。", Category::BasicInput, BuildRadioButtonPage });
    entries.push_back({ "combobox", "ComboBox", "显示当前值，并打开列表进行更改。", Category::BasicInput, BuildComboBoxPage });
    entries.push_back({ "slider", "Slider", "从范围内选取一个值。", Category::BasicInput, BuildSliderPage });
    entries.push_back({ "rangeslider", "RangeSlider", "两个滑块分别设置下限和上限。", Category::BasicInput, BuildRangeSliderPage });
    entries.push_back({ "rating", "RatingControl", "用星级表示评分。", Category::BasicInput, BuildRatingControlPage });
    entries.push_back({ "toggleswitch", "ToggleSwitch", "打开或关闭某项设置。", Category::BasicInput, BuildToggleSwitchPage });
    entries.push_back({ "colorpicker", "ColorPicker", "从色板或色谱中选取颜色。", Category::BasicInput, BuildColorPickerPage });
    entries.push_back({ "segmented", "SegmentedControl", "紧凑的互斥选择。", Category::BasicInput, BuildSegmentedControlPage });
}

void CollectionsCatalog::Register(std::vector<Entry>& entries) {
    entries.push_back({ "listbox", "ListBox", "显示选项列表。", Category::Collections, BuildListBoxPage });
    entries.push_back({ "listview", "ListView", "显示数据项的列表。", Category::Collections, BuildListViewPage });
    entries.push_back({ "treeview", "TreeView", "显示分层数据。", Category::Collections, BuildTreeViewPage });
}

void DateAndTimeCatalog::Register(std::vector<Entry>& entries) {
    entries.push_back({ "datepicker", "DatePicker", "选择日期。", Category::DateAndTime, BuildDatePickerPage });
    entries.push_back({ "timepicker", "TimePicker", "选择时间。", Category::DateAndTime, BuildTimePickerPage });
}

void DialogsAndFlyoutsCatalog::Register(std::vector<Entry>& entries) {
    entries.push_back({ "contentdialog", "ContentDialog", "显示模式对话框。", Category::DialogsAndFlyouts, BuildContentDialogPage });
    entries.push_back({ "flyout", "Flyout", "显示上下文信息。", Category::DialogsAndFlyouts, BuildFlyoutPage });
    entries.push_back({ "teachingtip", "TeachingTip", "提供上下文相关的指导。", Category::DialogsAndFlyouts, BuildTeachingTipPage });
}

void LayoutCatalog::Register(std::vector<Entry>& entries) {
    entries.push_back({ "canvas", "Canvas", "通过坐标定位子元素。", Category::Layout, BuildCanvasPage });
    entries.push_back({ "expander", "Expander", "可折叠的容器。", Category::Layout, BuildExpanderPage });
    entries.push_back({ "grid", "Grid", "基于行列的布局。", Category::Layout, BuildGridPage });
    entries.push_back({ "stackpanel", "StackPanel", "水平或垂直排列子元素。", Category::Layout, BuildStackPanelPage });
    entries.push_back({ "wrappanel", "WrapPanel", "自动换行的布局。", Category::Layout, BuildWrapPanelPage });
    entries.push_back({ "dockpanel", "DockPanel", "将子元素停靠在边缘。", Category::Layout, BuildDockPanelPage });
    entries.push_back({ "uniformgrid", "UniformGrid", "网格中的所有单元格大小相同。", Category::Layout, BuildUniformGridPage });
    entries.push_back({ "splitter", "Splitter", "允许用户调整区域大小。", Category::Layout, BuildSplitterPage });
    entries.push_back({ "dockmanager", "DockManager", "类似 IDE 的停靠管理。", Category::Layout, BuildDockManagerPage });
}

void MediaCatalog::Register(std::vector<Entry>& entries) {
    entries.push_back({ "image", "Image", "显示图片。", Category::Media, BuildImagePage });
    entries.push_back({ "linechart", "LineChart", "折线图。", Category::Media, BuildLineChartPage });
    entries.push_back({ "barchart", "BarChart", "柱状图。", Category::Media, BuildBarChartPage });
    entries.push_back({ "piechart", "PieChart", "饼图。", Category::Media, BuildPieChartPage });
    entries.push_back({ "terminal", "Terminal", "终端控件。", Category::Media, BuildTerminalPage });
    entries.push_back({ "topology", "TopologyView", "可折叠带动画拓扑图。", Category::Media, BuildTopologyPage });
}

void MenusAndToolbarsCatalog::Register(std::vector<Entry>& entries) {
    entries.push_back({ "commandbar", "CommandBar", "显示命令的工具栏。", Category::MenusAndToolbars, BuildCommandBarPage });
    entries.push_back({ "menubar", "MenuBar", "顶级菜单。", Category::MenusAndToolbars, BuildMenuBarPage });
    entries.push_back({ "contextmenu", "ContextMenu", "右键菜单。", Category::MenusAndToolbars, BuildContextMenuPage });
}

void MotionCatalog::Register(std::vector<Entry>& entries) {
    entries.push_back({ "animation", "Implicit animations", "悬停、展开、数值过渡。", Category::Motion, BuildAnimationPage });
    entries.push_back({ "themetransition", "Theme transition", "主题切换波纹效果。", Category::Motion, BuildThemeTransitionPage });
    entries.push_back({ "popupreveal", "Popup reveal", "Flyout/菜单进入曲线。", Category::Motion, BuildPopupRevealPage });
}

void NavigationCatalog::Register(std::vector<Entry>& entries) {
    entries.push_back({ "breadcrumb", "BreadcrumbBar", "显示导航路径。", Category::Navigation, BuildBreadcrumbBarPage });
    entries.push_back({ "navigationview", "NavigationView", "顶级导航框架。", Category::Navigation, BuildNavigationViewPage });
    entries.push_back({ "tabview", "TabView", "多标签页。", Category::Navigation, BuildTabViewPage });
    entries.push_back({ "paging", "PagingControl", "分页控件。", Category::Navigation, BuildPagingControlPage });
}

void ScrollingCatalog::Register(std::vector<Entry>& entries) {
    entries.push_back({ "scrollviewer", "ScrollViewer", "滚动视图内容。", Category::Scrolling, BuildScrollViewerPage });
}

void StatusAndInfoCatalog::Register(std::vector<Entry>& entries) {
    entries.push_back({ "infobar", "InfoBar", "内联信息提示。", Category::StatusAndInfo, BuildInfoBarPage });
    entries.push_back({ "progressbar", "ProgressBar", "显示进度条。", Category::StatusAndInfo, BuildProgressBarPage });
    entries.push_back({ "progressring", "ProgressRing", "显示进度环。", Category::StatusAndInfo, BuildProgressRingPage });
    entries.push_back({ "statusbar", "StatusBar", "状态栏。", Category::StatusAndInfo, BuildStatusBarPage });
    entries.push_back({ "toast", "Toast", "应用内通知。", Category::StatusAndInfo, BuildToastPage });
    entries.push_back({ "tooltip", "ToolTip", "悬停提示。", Category::StatusAndInfo, BuildToolTipPage });
    entries.push_back({ "logview", "LogView", "日志视图。", Category::StatusAndInfo, BuildLogViewPage });
}

void StylesCatalog::Register(std::vector<Entry>& entries) {
    entries.push_back({ "theme", "Theme", "Light/Dark 主题。", Category::Styles, BuildThemePage });
    entries.push_back({ "tokens", "Color tokens", "主题色板。", Category::Styles, BuildTokensPage });
    entries.push_back({ "typography", "Typography", "字体排印。", Category::Styles, BuildTypographyPage });
    entries.push_back({ "shape", "Shape", "形状与圆角。", Category::Styles, BuildShapePage });
}

void SystemCatalog::Register(std::vector<Entry>& entries) {
    entries.push_back({ "filepicker", "FilePicker", "文件选择。", Category::System, BuildFilePickerPage });
    entries.push_back({ "folderpicker", "FolderPicker", "文件夹选择。", Category::System, BuildFolderPickerPage });
    entries.push_back({ "dragdrop", "Drag and Drop", "拖放操作。", Category::System, BuildDragDropPage });
    entries.push_back({ "commands", "Commands", "命令和快捷键。", Category::System, BuildCommandsPage });
}

void TextCatalog::Register(std::vector<Entry>& entries) {
    entries.push_back({ "autosuggest", "AutoSuggestBox", "自动建议的输入框。", Category::Text, BuildAutoSuggestBoxPage });
    entries.push_back({ "numberbox", "NumberBox", "数字输入框。", Category::Text, BuildNumberBoxPage });
    entries.push_back({ "passwordbox", "PasswordBox", "密码输入框。", Category::Text, BuildPasswordBoxPage });
    entries.push_back({ "textblock", "TextBlock", "显示文本。", Category::Text, BuildTextBlockPage });
    entries.push_back({ "textbox", "TextBox", "单行或多行文本输入。", Category::Text, BuildTextBoxPage });
    entries.push_back({ "markdown", "MarkdownView", "Markdown 渲染。", Category::Text, BuildMarkdownViewPage });
}

void WindowingCatalog::Register(std::vector<Entry>& entries) {
    entries.push_back({ "titlebar", "TitleBar", "自定义标题栏。", Category::Windowing, BuildTitleBarPage });
    entries.push_back({ "backdrop", "Backdrop", "窗口背景材质。", Category::Windowing, BuildBackdropPage });
    entries.push_back({ "window", "Window", "多窗口与透明。", Category::Windowing, BuildWindowPage });
}

} // namespace Gallery
