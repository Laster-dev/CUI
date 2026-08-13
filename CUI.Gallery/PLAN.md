# CUI.Gallery 重写计划

目标：做一个合格的控件 Gallery，对标 WinUI 3 Gallery 的信息架构与页面纪律。  
工程：`E:\C++project\CUI\CUI.Gallery`（新 exe）。旧 `CUI/` Showcase **冻结**，只作对照，不继续往里塞页面。

本文只定计划，不实现。

---

## 1. 旧 Gallery 为什么不合格

当前入口是 `CUI/showcase`，挂在 `CUI.vcxproj` 上，和框架演示、VS Code 仿壳、流式图、性能叠加层搅在一起。

具体问题：

| 问题 | 现状 |
|------|------|
| 分类自创 | 「基础 Controls / 值/进度 / 导航与数据 / 列表 / 布局 / 交互/浮层 / 画布与渲染 / 终端/媒体」。同类控件拆散，无法当目录用。 |
| 一页塞多家 | `ButtonPage` 同时放 Button、ToggleButton、SplitButton、DropDownButton。WinUI Gallery 是一控件一页。 |
| 控件漏页 | 已有实现但无独立页：ToggleButton、DropDownButton、SplitButton、ProgressRing、MenuBar、ContextMenu、WindowTitleBar、FolderPicker、ContentDialog。 |
| 壳层过重 | `main.cpp` 里导航、缓存、搜索、Settings、流式线程全揉在一个 `StatelessWidget`。 |
| 工程身份错 | Gallery 不是独立产品；`CUI.Gallery.vcxproj` 目前还是空白 Console 工程，未进 `CUI.slnx`。 |
| 演示不干净 | 硬编码 RGB、Toast 刷屏、事件日志和控制混排、部分页没有统一页头/代码/属性面板。 |

重写原则：**壳干净、目录对、一控件一页、能搜、能改属性、能看用法。**

---

## 2. 合格 Gallery 长什么样

对标 WinUI Gallery，但只展示 **CUI 已有能力**。没有的 WinUI 控件不建空页。

壳：

1. 左侧 `NavigationView`
   - 顶部：Home
   - 中部：15 个分类，每类可展开，子项是控件页
   - 底部：Settings
   - Pane 搜索：按控件名 / tag / 分类过滤并跳转
2. 右侧内容区只换页，不换壳
3. Home：欢迎、按分类的控件卡片墙、搜索入口
4. Settings：Light/Dark、Backdrop、动画开关、渲染统计。不是控件页

控件页固定三段（同一控件的变体可以多段 Sample，不同类型必须拆页）：

```
标题 + 一句话说明
────────────────────────────────
实时 Sample（常用交互，可点）
────────────────────────────────
Source（可选 Expander，手写 C++ 片段）
```

一页只服务一个 `GetClassName()`。Button 页不出现 ToggleButton。

---

## 3. 分类与控件目录（唯一真相）

侧栏英文名、顺序固定，与 WinUI Gallery 一致。CUI 特有控件归入语义最近的类，**不新增顶级分类**。

导航 tag 用小写短名（`button`、`dropdownbutton`），稳定后不改，供搜索与深链。

### 3.1 Basic Input

| 页 | tag | 控件 | 备注 |
|----|-----|------|------|
| Button | `button` | `Button` | 默认 / Accent / 禁用 / 图标。波纹是控件行为，不单独占 Motion 页的主 Sample |
| DropDownButton | `dropdownbutton` | `DropDownButton` | 从旧 Button 页拆出 |
| HyperlinkButton | `hyperlinkbutton` | `HyperlinkButton` | 旧页名 Hyperlink |
| SplitButton | `splitbutton` | `SplitButton` | 从旧 Button 页拆出 |
| ToggleButton | `togglebutton` | `ToggleButton` | 从旧 Button 页拆出 |
| CheckBox | `checkbox` | `CheckBox` | |
| RadioButton | `radiobutton` | `RadioButton` | 分组互斥 |
| ComboBox | `combobox` | `ComboBox` | |
| Slider | `slider` | `Slider` | |
| RangeSlider | `rangeslider` | `RangeSlider` | CUI 特有 |
| RatingControl | `rating` | `RatingControl` | |
| ToggleSwitch | `toggleswitch` | `ToggleSwitch` | |
| ColorPicker | `colorpicker` | `ColorPicker` | WinUI 也在 Basic Input |
| SegmentedControl | `segmented` | `SegmentedControl` | CUI 特有，接近 RadioButtons |

### 3.2 Collections

| 页 | tag | 控件 |
|----|-----|------|
| ListBox | `listbox` | `ListBox` |
| ListView | `listview` | `ListView` |
| TreeView | `treeview` | `TreeView` |

无 FlipView / GridView / ItemsRepeater：不做空页。

### 3.3 Date and Time

| 页 | tag | 控件 |
|----|-----|------|
| DatePicker | `datepicker` | `DatePicker` |
| TimePicker | `timepicker` | `TimePicker` |

无 CalendarView / CalendarDatePicker：不做空页。

### 3.4 Dialogs and Flyouts

| 页 | tag | 控件 |
|----|-----|------|
| ContentDialog | `contentdialog` | `ContentDialog`（`MessageBox.h`） | 旧 Dialog 页 |
| Flyout | `flyout` | `Flyout` / `FlyoutPresenter` | 一页即可 |
| TeachingTip | `teachingtip` | `TeachingTip` | |

Toast 归 Status and Info，不放这里。

### 3.5 Layout

| 页 | tag | 控件 |
|----|-----|------|
| Canvas | `canvas` | `Canvas` |
| Expander | `expander` | `Expander` | 旧 Collapse 页 |
| Grid | `grid` | `Grid` |
| StackPanel | `stackpanel` | `StackPanel` |
| WrapPanel | `wrappanel` | `WrapPanel` | 旧 Wrap 页 |
| DockPanel | `dockpanel` | `DockPanel` | 旧 Dock 页（附加属性 Dock） |
| UniformGrid | `uniformgrid` | `UniformGrid` |
| Splitter | `splitter` | `Splitter` |
| DockManager | `dockmanager` | `DockManager` | CUI 特有 IDE 停靠；旧 Docking 页 |

无 Border / RelativePanel / Viewbox：不做空页。`Panel` 基类不单独做页。

### 3.6 Media

| 页 | tag | 控件 |
|----|-----|------|
| Image | `image` | `Image` | 静态图 + Stretch |
| LineChart | `linechart` | `LineChart` | 从旧 Chart 页拆出 |
| BarChart | `barchart` | `BarChart` | |
| PieChart | `piechart` | `PieChart` | |
| Terminal | `terminal` | `TerminalControl` | |

旧 Stream 页（动态贴图）并入 Image 的第二段 Sample，或作为 Image 页的「Streaming」变体，不单开分类。无 WebView2 / MediaPlayer：不做空页。

MarkdownView 归 Text。

### 3.7 Menus and Toolbars

| 页 | tag | 控件 |
|----|-----|------|
| CommandBar | `commandbar` | `CommandBar` |
| MenuBar | `menubar` | `MenuBar` | 新页 |
| ContextMenu | `contextmenu` | `ContextMenu` / `MenuItem` | 新页，右键触发 |

DropDownButton 已在 Basic Input。

### 3.8 Motion

没有独立 Motion 控件，做 **能力页**（仍挂在本分类下，tag 稳定）：

| 页 | tag | 演示内容 |
|----|-----|----------|
| Implicit animations | `animation` | `AnimatedScalar` / `AnimationSpec`：悬停、展开、数值过渡 |
| Theme transition | `themetransition` | `Window::SetThemeModeWithRipple` |
| Popup reveal | `popupreveal` | `PopupReveal`：Flyout / 菜单进入曲线 |

Button 波纹只在 Button 页出现，这里最多交叉链接，不重复做主 Sample。

### 3.9 Navigation

| 页 | tag | 控件 |
|----|-----|------|
| BreadcrumbBar | `breadcrumb` | `BreadcrumbBar` |
| NavigationView | `navigationview` | `NavigationView` | 页内嵌一套小导航，不要复用 Gallery 外壳 |
| TabView | `tabview` | `TabView` |
| PagingControl | `paging` | `PagingControl` | 接近 PipsPager |

### 3.10 Scrolling

| 页 | tag | 控件 |
|----|-----|------|
| ScrollViewer | `scrollviewer` | `ScrollViewer` | 纵横滚动、程序滚动、嵌套 |

无 AnnotatedScrollBar / SemanticZoom：不做空页。

### 3.11 Status and Info

| 页 | tag | 控件 |
|----|-----|------|
| InfoBar | `infobar` | `InfoBar` |
| ProgressBar | `progressbar` | `ProgressBar` | determinate / indeterminate |
| ProgressRing | `progressring` | `ProgressRing` | 新页，勿再塞进 ProgressBar |
| StatusBar | `statusbar` | `StatusBar` |
| Toast | `toast` | `Toast` / `ToastCenter` | 一页 |
| ToolTip | `tooltip` | `UIElement::SetToolTip` | 能力页，无独立控件类 |
| LogView | `logview` | `LogView` | CUI 特有 |

### 3.12 Styles

能力页，不是控件：

| 页 | tag | 内容 |
|----|-----|------|
| Theme | `theme` | Light / Dark，跟 Settings 联动 |
| Color tokens | `tokens` | `ThemeTokenId` 色板（WindowBackground … FocusedBorder） |
| Typography | `typography` | 字体、字号、字重（TextBlock 矩阵） |
| Shape | `shape` | CornerRadius、描边、卡片 |

Backdrop 归 Windowing。图标演示若有 `DrawIcon`/SVG，作为 Shape 或 Theme 的一段，暂不单开 Iconography 页。

### 3.13 System

| 页 | tag | 内容 |
|----|-----|------|
| FilePicker | `filepicker` | `FilePicker` |
| FolderPicker | `folderpicker` | `FolderPicker` | 新页，勿再塞进 FilePicker |
| Drag and Drop | `dragdrop` | `DragDropService` |
| Commands | `commands` | `CommandManager` / 快捷键 | 有现成 API 再做；没有就延后 |

DPI 变化可放 Windowing。无独立 Clipboard API 则不做 Clipboard 页。

### 3.14 Text

| 页 | tag | 控件 |
|----|-----|------|
| AutoSuggestBox | `autosuggest` | `AutoSuggestBox` |
| NumberBox | `numberbox` | `NumberBox` |
| PasswordBox | `passwordbox` | `PasswordBox` |
| TextBlock | `textblock` | `TextBlock` |
| TextBox | `textbox` | `TextBox` |
| MarkdownView | `markdown` | `MarkdownView` | CUI 特有 |

无 RichEditBox / RichTextBlock：不做空页。

### 3.15 Windowing

| 页 | tag | 内容 |
|----|-----|------|
| TitleBar | `titlebar` | `WindowTitleBar` | 自定义标题栏 + 菜单 |
| Backdrop | `backdrop` | `BackdropType`：None / Mica / MicaAlt / Acrylic |
| Window | `window` | 多窗口、透明模式、`SetTransparentMode` | 以现有 `Window` / `MenuPopupWindow` 为限 |

Gallery 外壳本身用哪套 TitleBar/Backdrop，在 Settings 配；本分类是 **可交互说明页**。

### 3.16 壳层页（不算 15 分类）

| 页 | tag | 作用 |
|----|-----|------|
| Home | `home` | 卡片墙、分类导览 |
| Settings | `settings` | 主题、Backdrop、动画、叠加层 |

合计：**约 70 个内容页**（含 Motion/Styles/Windowing 能力页）+ 2 个壳页。侧栏只出现有实现的项。

---

## 4. 工程与目录

### 4.1 解决方案

- `CUI.Gallery.vcxproj` 改为 **Windows 子系统**（`Windows`，不是 `Console`）
- 链接 `CUI.Core`，配置/平台与现有 Gallery 一致：优先 **Debug|x64**
- 拷贝/引用 `CUI/app.manifest`（DPI Aware）
- 加入 `CUI.slnx`
- 旧 `CUI.vcxproj` 保留，不再作为 Gallery 产品入口

### 4.2 源码树

```
CUI.Gallery/
  PLAN.md                          ← 本文件
  CUI.Gallery.vcxproj
  src/
    main.cpp                       ← 只创建 Window、挂壳、跑消息循环
    GalleryApp.cpp / .h
    catalog/
      Catalog.h / Catalog.cpp      ← 分类顺序、页注册、搜索索引（唯一清单）
    chrome/
      GalleryShell.cpp / .h        ← NavigationView + 搜索 + 缓存
      HomePage.cpp
      SettingsPage.cpp
    page/
      SamplePage.cpp / .h          ← 标准页骨架
      SampleSection.cpp / .h
    pages/
      BasicInput/ButtonPage.cpp
      BasicInput/DropDownButtonPage.cpp
      ...
      Collections/ListBoxPage.cpp
      DateTime/DatePickerPage.cpp
      Dialogs/ContentDialogPage.cpp
      Layout/GridPage.cpp
      Media/ImagePage.cpp
      Menus/CommandBarPage.cpp
      Motion/AnimationPage.cpp
      Navigation/TabViewPage.cpp
      Scrolling/ScrollViewerPage.cpp
      Status/InfoBarPage.cpp
      Styles/ThemePage.cpp
      System/FilePickerPage.cpp
      Text/TextBoxPage.cpp
      Windowing/TitleBarPage.cpp
    assets/                        ← 示例图、图标；按需
```

每个 `XxxPage.cpp` 只导出：

```cpp
std::shared_ptr<UIElement> BuildButtonPage();
```

禁止再引入 `ShowcaseContext` 那套散落依赖。需要 `Window*` 时用 `Window::Current()`。

### 4.3 Catalog 合同

`Catalog` 是侧栏、Home 卡片、搜索的唯一数据源。页面工厂不得在 `GalleryShell` 里再手写一份列表。

```cpp
enum class GalleryCategory {
    BasicInput,
    Collections,
    DateAndTime,
    DialogsAndFlyouts,
    Layout,
    Media,
    MenusAndToolbars,
    Motion,
    Navigation,
    Scrolling,
    StatusAndInfo,
    Styles,
    System,
    Text,
    Windowing,
};

struct GalleryEntry {
    const char* tag;          // "button"
    const char* title;        // "Button"
    const char* subtitle;     // 一句话
    GalleryCategory category;
    std::function<std::shared_ptr<UIElement>()> build;
};
```

分类显示名固定英文（与侧栏一致）。subtitle 可用中文。

### 4.4 页缓存

沿用「首次导航才构建 + LRU」：默认最多缓存 12 页。Motion / Terminal / DockManager 等重页离开时可主动丢缓存。

---

## 5. 页面纪律

1. **一控件一页。** 变体（禁用、强调色、横向 Slider）用同一页的多个 `SampleSection`。
2. **说明短。** 标题下不超过两句：做什么、何时用。
3. **Source 是可编译片段。** 10～30 行，展示创建 + 关键 API，不贴 Measure/OnRender。
4. **禁止演示里硬编码与主题打架的 RGB**（除非该页就是在讲 Color / Token）。
5. **禁止页内再造一套 Gallery 导航**（NavigationView 示例页除外，且必须是嵌套小型实例）。
6. **事件输出可选且克制。** 一行状态即可，不要每页一个伪终端。
7. **不从旧 Showcase 整页复制。** 布局、文案、分类全部按本计划重写；旧文件只当 API 用法备忘。
8. **没有的控件不占位。** 目录里不出现「Coming soon」。
9. **中文 UI 文案可以，分类名必须英文。**

---

## 6. 分阶段

每阶段结束时：工程可编译、壳能导航、已做分类在 Home 和侧栏都出现。

### P0 — 壳（先于任何控件页）

- 修正 vcxproj：子系统、包含路径、链接 CUI.Core、清单、进 slnx
- `Catalog` + `GalleryShell` + Home + Settings
- `SamplePage` 骨架（标题 / Sample / 手写 Source）
- 用 **Button 一页** 打通全链路，作为模板

完成标准：能搜到 Button、能点示例、能切主题。

### P1 — Basic Input + Text

Basic Input 14 页 + Text 6 页。最常用，先把「一页一样」的手感做稳。

### P2 — Collections / Date and Time / Layout / Navigation / Scrolling

列表、日期、布局、导航、滚动。Layout 页要能看懂附加属性（Grid.Row、Canvas.Left、Dock）。

### P3 — Dialogs / Menus / Status and Info / Media

浮层、菜单、状态、图/表/终端。Toast、Flyout、ContentDialog 必须用框架 API，不要页内私画一层。

### P4 — Motion / Styles / System / Windowing

能力页。Settings 与 Theme / Backdrop 页数据同源（改一处两处都变）。

### P5 — 抛光

- Home 卡片按 15 类分组，可点
- 搜索支持中文说明（若 subtitle 含中文）
- 缺页补齐（对照第 3 节清单打勾）
- 去掉旧 Showcase 特有的调试噪音（`ProgressBarDiag` 等不进 Gallery）
- 统一页边距、Sample 卡片、代码字体

旧 `CUI.exe` 不删除，等 Gallery 可独立承担演示后再说是否降级为内部沙盒。

---

## 7. 明确不做

- 不把 Gallery 做成 IDE / VS Code 仿壳（旧 `VSCodeControls` 不搬）
- 不在 Gallery 里继续改控件实现；发现绘制/属性 bug 回 `CUI.Core` 另开任务
- 不实现 WinUI 有而 CUI 无的控件来「凑目录」
- 不新增第 16 个顶级分类
- 不把 DockManager、Terminal、Chart 塞进 Basic Input
- 本阶段不写自动化 UI 测试（P5 后再议）

---

## 8. 旧页对照（迁移时用，不要复制）

| 旧 Showcase | 新位置 |
|-------------|--------|
| ButtonPage（含三钮） | Basic Input ×4 页 |
| HyperlinkPage | Basic Input / HyperlinkButton |
| TextBlock/TextBox/PasswordBox/NumberBox/AutoSuggest | Text |
| CheckBox/Radio/ToggleSwitch/ComboBox/Segmented/Rating | Basic Input |
| Slider/RangeSlider/ColorPicker | Basic Input |
| DatePicker/TimePicker | Date and Time |
| FilePicker | System；FolderPicker 新页 |
| ListBox/ListView/TreeView | Collections |
| Breadcrumb/Paging/NavigationView/TabView | Navigation |
| Grid/Canvas/Wrap/Dock/Uniform/Stack/Splitter/Collapse | Layout |
| Docking | Layout / DockManager |
| ScrollViewer | Scrolling |
| Flyout/TeachingTip/Dialog | Dialogs and Flyouts |
| Toast/InfoBar/StatusBar/ProgressBar | Status and Info |
| CommandBar | Menus and Toolbars |
| Chart | Media ×3 |
| Image/Stream | Media / Image |
| Markdown | Text |
| LogView | Status and Info |
| Terminal | Media |
| DragDrop | System |
| Settings（main.cpp 内嵌） | chrome/SettingsPage |

---

## 9. 开工顺序（P0 任务列表）

1. 配 `CUI.Gallery.vcxproj` 与 `CUI.slnx`
2. `src/main.cpp` 创建窗口
3. `catalog/Catalog.cpp` 写入第 3 节清单（未实现的 build 先不注册）
4. `chrome/GalleryShell.cpp`：分类侧栏、搜索、LRU
5. `page/SamplePage.cpp` 标准骨架
6. `pages/BasicInput/ButtonPage.cpp` + Home + Settings

P0 通过后再按 P1→P5 批量加页。加页只改 `Catalog` 注册 + 对应 `pages/...` 文件，不改壳。
