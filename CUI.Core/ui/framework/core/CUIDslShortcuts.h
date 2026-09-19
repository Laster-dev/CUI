#pragma once

// ============================================================================
//  CUIDslShortcuts.h —— 控件名即工厂（快捷宏层）
//
//  背景：CUI::PasswordBox 是类型名，不能再定义同名自由函数——同作用域同名函数
//  会隐藏类名，跨作用域同时 using 则查找二义性。故 DSL 工厂被迫叫
//  PasswordBoxWidget / TextField / CheckboxTile ...
//
//  本文件用函数式宏把控件名还原为工厂入口：
//      auto pwd = PasswordBox(请输入密码).Width(240.0f).Build();
//
//  安全性：函数式宏仅在名字后紧跟左括号时展开，下列类型用法完全不受影响
//      make_shared<PasswordBox>()   后面是 <   不展开
//      void foo(PasswordBox* p)     后面是 *   不展开
//      class MyBox : public PasswordBox  后面是 :  不展开
//      PasswordBox::SomeStatic()    后面是 ::  不展开
//
//  已知限制（遇到时改写为 Fluent::PasswordBox(...) 即可）
//      1. 基类初始化/委托构造: MyBox() : PasswordBox(x) 会被展开
//         规避：using Base = PasswordBox;  MyBox() : Base(x) {}
//      2. 全限定调用 CUI::PasswordBox(x) 同样会被展开（宏不识别限定符）
//      3. 撞 Win32 GDI 的 Rectangle()/Ellipse()，故默认不启用通用词宏
//
//  开关（在 include CUI.h 之前定义）
//      CUI_NO_DSL_SHORTCUTS     完全关闭本宏层
//      CUI_DSL_SHORTCUTS_ALL    额外启用通用词宏 Panel/Grid/Canvas/...
// ============================================================================

#ifndef CUI_NO_DSL_SHORTCUTS

// ---------------------------------------------------------- 基础输入与按钮类
#define Button(...)             CUI::DSL::Fluent::Button(__VA_ARGS__)
#define TextBlock(...)          CUI::DSL::Fluent::TextBlock(__VA_ARGS__)
#define TextBox(...)            CUI::DSL::Fluent::TextBox(__VA_ARGS__)
#define PasswordBox(...)        CUI::DSL::Fluent::PasswordBox(__VA_ARGS__)
#define NumberBox(...)          CUI::DSL::Fluent::NumberBox(__VA_ARGS__)
#define CheckBox(...)           CUI::DSL::Fluent::CheckBox(__VA_ARGS__)
#define RadioButton(...)        CUI::DSL::Fluent::RadioButton(__VA_ARGS__)
#define ToggleButton(...)       CUI::DSL::Fluent::ToggleButton(__VA_ARGS__)
#define DropDownButton(...)     CUI::DSL::Fluent::DropDownButton(__VA_ARGS__)
#define SplitButton(...)        CUI::DSL::Fluent::SplitButton(__VA_ARGS__)
#define HyperlinkButton(...)    CUI::DSL::Fluent::HyperlinkButton(__VA_ARGS__)
#define ToggleSwitch(...)       CUI::DSL::Fluent::ToggleSwitch(__VA_ARGS__)
#define Slider(...)             CUI::DSL::Fluent::Slider(__VA_ARGS__)
#define ComboBox(...)           CUI::DSL::Fluent::ComboBox(__VA_ARGS__)
#define ListBox(...)            CUI::DSL::Fluent::ListBox(__VA_ARGS__)
#define ListView(...)           CUI::DSL::Fluent::ListView(__VA_ARGS__)

// ---------------------------------------------------------- 选择集合与文本类
#define SegmentedControl(...)   CUI::DSL::Fluent::SegmentedControl(__VA_ARGS__)
#define DatePicker(...)         CUI::DSL::Fluent::DatePicker(__VA_ARGS__)
#define TimePicker(...)         CUI::DSL::Fluent::TimePicker(__VA_ARGS__)
#define ColorPicker(...)        CUI::DSL::Fluent::ColorPicker(__VA_ARGS__)
#define TreeView(...)           CUI::DSL::Fluent::TreeView(__VA_ARGS__)
#define AutoSuggestBox(...)     CUI::DSL::Fluent::AutoSuggestBox(__VA_ARGS__)
#define MarkdownView(...)       CUI::DSL::Fluent::MarkdownView(__VA_ARGS__)
#define LogView(...)            CUI::DSL::Fluent::LogView(__VA_ARGS__)

// ---------------------------------------------------------- 进度状态与反馈类
#define ProgressBar(...)        CUI::DSL::Fluent::ProgressBar(__VA_ARGS__)
#define ProgressRing(...)       CUI::DSL::Fluent::ProgressRing(__VA_ARGS__)
#define RangeSlider(...)        CUI::DSL::Fluent::RangeSlider(__VA_ARGS__)
#define StatusBar(...)          CUI::DSL::Fluent::StatusBar(__VA_ARGS__)
#define RatingControl(...)      CUI::DSL::Fluent::RatingControl(__VA_ARGS__)
#define TeachingTip(...)        CUI::DSL::Fluent::TeachingTip(__VA_ARGS__)
#define InfoBar(...)            CUI::DSL::Fluent::InfoBar(__VA_ARGS__)
#define Toast(...)              CUI::DSL::Fluent::Toast(__VA_ARGS__)
#define ContentDialog(...)      CUI::DSL::Fluent::ContentDialog(__VA_ARGS__)

// ---------------------------------------------------------- 布局容器与导航类
#define StackPanel(...)         CUI::DSL::Fluent::StackPanel(__VA_ARGS__)
#define WrapPanel(...)          CUI::DSL::Fluent::WrapPanel(__VA_ARGS__)
#define DockPanel(...)          CUI::DSL::Fluent::DockPanel(__VA_ARGS__)
#define UniformGrid(...)        CUI::DSL::Fluent::UniformGrid(__VA_ARGS__)
#define ScrollViewer(...)       CUI::DSL::Fluent::ScrollViewer(__VA_ARGS__)
#define Splitter(...)           CUI::DSL::Fluent::Splitter(__VA_ARGS__)
#define Expander(...)           CUI::DSL::Fluent::Expander(__VA_ARGS__)
#define Flyout(...)             CUI::DSL::Fluent::Flyout(__VA_ARGS__)
#define MenuBar(...)            CUI::DSL::Fluent::MenuBar(__VA_ARGS__)
#define CommandBar(...)         CUI::DSL::Fluent::CommandBar(__VA_ARGS__)
#define BreadcrumbBar(...)      CUI::DSL::Fluent::BreadcrumbBar(__VA_ARGS__)
#define PagingControl(...)      CUI::DSL::Fluent::PagingControl(__VA_ARGS__)
#define DockManager(...)        CUI::DSL::Fluent::DockManager(__VA_ARGS__)
#define WindowTitleBar(...)     CUI::DSL::Fluent::WindowTitleBar(__VA_ARGS__)

// ---------------------------------------------------------- 媒体图表与绘图类
#define FilePicker(...)         CUI::DSL::Fluent::FilePicker(__VA_ARGS__)
#define FolderPicker(...)       CUI::DSL::Fluent::FolderPicker(__VA_ARGS__)
#define TopologyView(...)       CUI::DSL::Fluent::TopologyView(__VA_ARGS__)
#define LineChart(...)          CUI::DSL::Fluent::LineChart(__VA_ARGS__)
#define BarChart(...)           CUI::DSL::Fluent::BarChart(__VA_ARGS__)
#define PieChart(...)           CUI::DSL::Fluent::PieChart(__VA_ARGS__)
#define SvgIcon(...)            CUI::DSL::Fluent::SvgIcon(__VA_ARGS__)
#define CanvasControl(...)      CUI::DSL::Fluent::CanvasControl(__VA_ARGS__)

#ifdef CUI_DSL_SHORTCUTS_ALL
// ---------------------------------------------------------- 通用词默认关闭区
// 名字过于通用或撞 Win32 GDI，确认工程内无冲突时才打开
#define Panel(...)              CUI::DSL::Fluent::Panel(__VA_ARGS__)
#define Grid(...)               CUI::DSL::Fluent::Grid(__VA_ARGS__)
#define Canvas(...)             CUI::DSL::Fluent::Canvas(__VA_ARGS__)
#define Image(...)              CUI::DSL::Fluent::Image(__VA_ARGS__)
#define Line(...)               CUI::DSL::Fluent::Line(__VA_ARGS__)
#define Path(...)               CUI::DSL::Fluent::Path(__VA_ARGS__)
#define Rectangle(...)          CUI::DSL::Fluent::Rectangle(__VA_ARGS__)
#define Ellipse(...)            CUI::DSL::Fluent::Ellipse(__VA_ARGS__)
#endif // CUI_DSL_SHORTCUTS_ALL

#endif // CUI_NO_DSL_SHORTCUTS
