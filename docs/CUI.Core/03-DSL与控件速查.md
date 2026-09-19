---
layout: page
title: 03 · DSL 与控件速查
parent: CUI.Core 开发手册
nav_order: 4
---

# 03 · DSL 与控件速查

> 所有条目摘抄自 `framework/core/CUIDsl.h`（1604 行）。行号可直接跳转核对。

## 一、入口（只有一个 include）

```cpp
#include "CUI.h"        // 伞形头：控件 + DSL + 窗口 + 渲染 + 动画 + 主题
```

`CUI.h` 默认注入 `using namespace CUI;` 与 `using namespace CUI::DSL;`。不想要就：

```cpp
#define CUI_NO_USING_NAMESPACE
#include "CUI.h"
```

## 二、链式三规则

```cpp
auto b = Text("Hi").FontSize(24.0f);   // 1) 用 auto 接（值），别接引用
auto p = b.Build();                    // 2) Build() 给出 shared_ptr<T>，可重复调用
Borrow(existingPtr).Text("改一下");    // 3) Borrow 把已有元素拿回链式能力
```

- `ElementBuilder<T>` 继承 `ElementRef<T>`，**值语义**；`ElementBuilder<T>& r = X().Width(1);` 会悬垂。
- `Build()` 是 `const`，不释放所有权。
- 隐式转换 `operator std::shared_ptr<UIElement>()` 让你能直接塞进 `Children({...})`。

## 三、控件工厂（全部在 `CUI::DSL`）

### 容器 / 布局

| 工厂 | 返回类型 | 说明 |
|---|---|---|
| `Column(float gap = 8)` | StackPanel | 纵向排列 |
| `Row(float gap = 8)` | StackPanel | 横向排列 |
| `Column(gap, {children})` / `Row(gap, {children})` | StackPanel | 带子项的版本 |
| `Container()` | Panel | 空盒子容器 |
| `CanvasWidget()` | Canvas | 绝对定位 |
| `GridWidget()` | Grid | 网格 |
| `WrapPanelWidget("Horizontal")` | WrapPanel | 自动换行 |
| `DockPanelWidget()` | DockPanel | 边缘停靠 |
| `UniformGridWidget(rows = 2, cols = 2)` | UniformGrid | 等分网格 |
| `ScrollViewerWidget()` / `SingleChildScrollView()` | ScrollViewer | 滚动视图 |
| `Expanded(child, flex = 1)` | Panel | 弹性填充包装 |
| `SplitterWidget(Orientation::Horizontal)` | Splitter | 拖拽分隔条 |
| `DockManagerWidget()` | DockManager | 可停靠窗口管理 |

### 文本 / 展示

`Text(content)`、`MarkdownViewWidget()`、`LogViewWidget()`、`ImageWidget()`、`BreadcrumbBarWidget()`

### 按钮

`ElevatedButton(text, onPressed)`、`ToggleButtonWidget(text)`、`DropDownButtonWidget(text)`、`SplitButtonWidget(text)`、`HyperlinkButtonWidget(text, uri)`

### 输入

`TextField(text, onChanged)`（TextBox）、`PasswordBoxWidget(placeholder)`、`NumberBoxWidget(value)`、`AutoSuggestBoxWidget(placeholder)`、`SliderWidget(val, min, max, onChanged)`、`RangeSliderWidget(...)`

### 选择 / 开关

`CheckboxTile(title, onChanged)`、`RadioButtonTile(text, group)`、`ToggleSwitchWidget()` / `ToggleSwitchTile(header, isOn)`、`ComboBoxWidget()`、`ListBoxWidget()`、`ListViewWidget()`、`TreeViewWidget()`、`SegmentedWidget({items})`、`DatePickerWidget()`、`TimePickerWidget()`、`ColorPickerWidget()`、`FilePickerWidget(path)`、`FolderPickerWidget(path)`、`RatingWidget(value, maxRating)`

### 状态 / 反馈

`ProgressBarWidget(val, indeterminate)`、`ProgressRingWidget(val, indeterminate)`、`StatusBarWidget()`、`InfoBarWidget()`、`ToastWidget()`、`TeachingTipWidget()`、`ContentDialogWidget(title, message)`、`PagingControlWidget(current, total)`

### 图表 / 高级

`LineChartWidget()`、`BarChartWidget()`、`PieChartWidget()`、`TopologyWidget()`、`ExpanderWidget(title)`、`CollapsePanelWidget(title)`（旧命名别名）、`CommandBarWidget()`、`MenuBarWidget()`、`TitleBarWidget(title)`

### Shape / 自绘

`RectangleWidget(w, h)`、`EllipseWidget(w, h)`、`LineWidget(x1,y1,x2,y2)`、`PathWidget(svgData)`、`SvgIconWidget(source)`、`CanvasControlWidget(w, h)`

> 未在此列出的控件（如 `TabView`、`NavigationView`、`TerminalControl`）没有专属工厂，用 `CUI::DSL::Control<T>()` 或 `std::make_shared<T>()`。

## 四、通用链式方法

### 标识与尺寸

`Id(str)`、`Width(f)`、`Height(f)`、`Size(w,h)`、`MinWidth(f)`、`MinHeight(f)`、`MaxWidth(f)`、`MaxHeight(f)`

### 对齐与边距

`Align(Alignment)`、`AlignHorizontal(Alignment)`、`AlignVertical(Alignment)`、`Margin(all)`、`Margin(l,t,r,b)`、`Margin(Thickness)`、`Padding(...)`（同 Margin）、`FlexGrow(f)`、`ZIndex(int)`

### 容器附加属性

`GridRow(int)`、`GridColumn(int)`、`GridRowSpan(int)`、`GridColumnSpan(int)`、`CanvasLeft/Top/Right/Bottom(float)`、`Dock(Dock)`、`RowHeight(f)`、`ItemWidth/ItemHeight(f)`、`Rows(n)`、`Columns(n)`

### 子项

`Children({child, ...})`、`AddChild(shared_ptr)`、`AddChild(ElementBuilder<ChildT>)`、`Items(...)`、`AddItem(...)`、`ItemText(id,text)`、`ItemIcon(id,icon)`、`ItemProgress(id,v)`

### 外观

`Background(color)` / `Background("hexString")`、`HoverBackground(...)`、`PressedBackground(...)`、`Foreground(color)`、`Border(color, thickness)`、`BorderBrush(color)`、`BorderThickness(f)`、`CornerRadius(f)`、`Opacity(f)`、`Visibility(Visibility)` / `Visibility("...")`

### 字体与文本

`Text(str)`、`FontSize(f)`、`FontFamily(str)`、`FontWeight(FontWeight)`、`FontStyle(FontStyle)`、`FontStretch(FontStretch)`、`ToolTip(str)`、`Icon(str)`、`Placeholder(str)`、`Header(str)`、`Content(child)`

### 主题 Token（推荐，随深浅色自动切换）

`ForegroundToken(id)`、`BackgroundToken(id)`、`BorderToken(id, thickness)`、`HoverBackgroundToken(id)`、`PressedBackgroundToken(id)`、`SelectedBackgroundToken(id)`，以及各控件专属 Token：`FillColorToken`、`TrackColorToken`、`ActiveTrackColorToken`、`ThumbColorToken`、`CaretColorToken`、`UnderlineColorToken`、`PlaceholderColorToken`、`DropdownBackgroundToken`、`GridLineBrushToken`、`IndicatorColorToken`、`SecondaryColorToken`、`DisabledBackgroundToken`、`OnColorToken`、`PaneBackgroundToken`、`AccentColorToken`、`TitleColorToken`、`MessageColorToken`、`ColorToken`（= ForegroundToken 别名）

### 其它常见

`Orientation(...)`、`Gap(f)`、`Spacing(f)`、`Value(v)`、`Minimum(f)`、`Maximum(f)`、`SelectedIndex(int)`、`SelectedItem(...)`、`Checked(bool)`、`IsSeparator(bool)`、`Expanded(bool)`、`MaxRating(int)`、`MaxTabWidth(f)`、`MinTabWidth(f)`、`OverlayScrollbar(bool)`、`ClipToBounds(bool)`、`KeyboardNavigationMode(mode)`

## 五、事件链式方法

| 方法 | 签名 |
|---|---|
| `OnClick` | `void(UIElement*)` |
| `OnTextChanged` | `void(TextBox*, const std::string&)` |
| `OnCheckStateChanged` / `OnCheckChanged` | `void(CheckBox*, CheckState)` |
| `OnValueChanged` | `void(Slider*, float)` 或 `void(RangeSlider*, float, float)` |
| `OnSelectionChanged` | `void(TreeView*, shared_ptr<TreeViewItem>)` / `void(ComboBox*, int, const std::string&)` / `void(SegmentedControl*, int, const std::string&)` |
| `OnItemToggled` / `OnItemDoubleClicked` | `void(TreeView*, shared_ptr<TreeViewItem>)` |
| `OnBreadcrumbItemClicked` | `void(BreadcrumbBar*, int, const std::string&)` |
| `OnToggled` | `void(ToggleButton*, bool)` |
| `OnItemChosen` | `void(DropDownButton*, int, const std::string&)` |
| `OnInvoked` / `OnExpandChanged` | `void(NavigationViewItem*)` |
| `OnNavigationItemInvoked` / `OnNavigationBackRequested` / `OnNavigationDisplayModeChanged` | `void(NavigationView*, …)` |
| `OnPathChanged` | `void(FilePicker*, const std::string&)` |
| `OnDraw` | 自绘回调（CanvasControl） |
| `OnCanvasMouseDown` / `OnCanvasMouseMove` / `OnCanvasMouseUp` | 画布输入 |
| `OnTick` | 逐帧回调 |

## 六、陷阱：静默降级（最重要的一条）

绝大多数链式方法长这样：

```cpp
ElementBuilder& RowHeight(float v) {
    if constexpr (requires { m_ptr->SetRowHeight(v); }) m_ptr->SetRowHeight(v);
    return *this;
}
```

即：**控件不支持该方法时，编译通过、运行无效、无任何提示。**

对策：

1. 不确定控件是否支持 → 打开 `CUIDsl.h` 看该方法体的 `requires` 目标（就是它实际调用的 setter）。
2. 或直接操作属性：`elem->SetRowHeight(v)`（不支持就编译报错，反而更安全）。
3. 事件方法同理：`OnClick` 要求 `T` 派生自 `Control`，对 `TextBlock` 调用无效。

## 七、其它注意

- `Control<T>(args...)` 是通用工厂：`DSL::Control<MyWidget>(ctorArgs...)`。
- `Column` / `Row` 同时是自由函数，与 Windows 头文件无冲突（框架已在多个示例工程验证）。
- `ElementBuilder` 的 `Borrow` 有 4 个重载：`ElementBuilder&`、`shared_ptr<T>&`、`T*`、`T&`。
