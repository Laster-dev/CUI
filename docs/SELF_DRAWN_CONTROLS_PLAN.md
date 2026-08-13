# CUI 自绘控件与能力扩展计划

> 创建日期: 2026-08-13  
> 状态: **完成** — A–H 均已落地（Markdown 为自研子集解析 + 自绘 MarkdownView）  
> 原则: **全部纯自绘**（Direct2D 几何/文本/命中/动画自管），禁止用现有 Button/TextBox/ListBox 等控件拼接出“假控件”。可复用的仅限框架基础设施：`UIElement` / `Control` / `PopupHost` / `AnimationManager` / `ThemeManager` / `GraphicsContext`。  
> Showcase: Toggle / Split / DropDown 按钮族统一放在 **Button** 页演示；其余各自独立页。

---

## 1. 目标清单

| ID | 项 | 形态 | Showcase |
|----|----|------|----------|
| A | AutoSuggestBox / SearchBox | 控件 | 独立页 `autosuggest` |
| B | StatusBar | 控件 | 独立页 `statusbar` |
| C | RatingControl | 控件 | 独立页 `rating` |
| D | ToggleButton / SplitButton / DropDownButton | 控件（按钮族） | **全部并入 Button 页** |
| E | RichTextBlock / RichEdit → **Markdown 渲染器** | 控件（只读渲染为主，可选轻量编辑） | 独立页 `markdown` |
| F | TeachingTip / Tooltip 增强 | 控件 + 框架能力 | 独立页 `teachingtip`；Tooltip 可在多页复用 |
| G | Chart | 控件 | 独立页 `chart` |
| H | 拖放 API | 框架 API（非单一控件） | 独立页 `dragdrop` + 若干控件接入示例 |
| I | RangeSlider | 控件（双滑块区间） | 独立页 `rangeslider` |
| J | LogView | 控件（可折叠虚拟化日志框） | 独立页 `logview` |

---

## 2. 硬约束（自绘定义）

### 2.1 允许

- 继承 `Control` / `UIElement`，自己 `OnRender` / `Measure` / `OnMouse*` / `OnKey*` / `OnAnimationTick`
- 使用 `GraphicsContext` 画圆角、路径、文字、图标字形、渐变
- 使用 `PopupHost` / `IPopup` 托管自绘弹出层（菜单、建议列表、TeachingTip 气泡）
- 使用 `AnimationManager` 注册自身动画；popup 内嵌子视觉通过 `SetAnimationHost`
- 主题色走 `ThemeTokenId` / tokens

### 2.2 禁止

- 内部 `AddChild(Button)` / `TextBox` / `ListBox` / `ComboBox` / `Flyout` 内容槽里塞现成控件来“拼”出外观与交互
- 调用 Win32 原生编辑框、Common Control、WebView2、GDI+ Chart 控件等第三方/系统控件充当主体
- Markdown 用 WebView / Scintilla / 外部 HTML 引擎渲染（解析可自研或引入**无 UI** 的纯解析库，绘制必须自绘）

### 2.3 解析库例外（仅 E）

Markdown **解析**可采用 header-only / 无界面依赖的 C/C++ 库（或自研子集解析器）；**排版与绘制**必须走本框架 `GraphicsContext` + 自有 layout。选定库时在实现 PR 中写明许可证。

---

## 3. 推荐实施顺序

依赖与风险决定顺序（先框架能力，再复合控件）：

```
H0  拖放 API 骨架（HitTest / 捕获 / 反馈层）     ← 可与控件并行设计，但先落地协议
P1  Tooltip 增强（轻） + TeachingTip（中）
P2  ToggleButton / SplitButton / DropDownButton
P3  RatingControl
P4  StatusBar
P5  AutoSuggestBox / SearchBox                  ← 依赖自绘弹出列表 + 键盘导航
P6  Chart（折线/柱状/饼图 MVP）
P7  Markdown 渲染器（RichTextBlock 定位）       ← 排版最重，放后
```

说明：

- **H 拖放** 尽早定协议，避免后期每个控件各写一套。
- **按钮族** 相对独立，可快速丰富 Button 页。
- **AutoSuggestBox** 需要成熟的自绘弹出列表与 IME/键盘路径，放在 TeachingTip/Popup 经验之后。
- **Markdown** 工作量最大（解析、块布局、选区、滚动、主题代码块），单独里程碑。

---

## 4. 分项设计要点

### A. AutoSuggestBox / SearchBox

**定位**: WinUI `AutoSuggestBox` 风格：左侧搜索图标、输入区、清除按钮、下拉建议列表。

**自绘结构**（逻辑分区，不是子控件）:

- 边框 / 背景 / 焦点环
- 图标槽、文本排版与光标/选区（可参考 `TextBox` 的 IME/选区算法，但代码独立或抽到共享 `TextEditCore`，**不嵌 TextBox 实例**）
- 清除 “×” 热区
- 建议弹出层：自绘行列表（虚拟化可选），`IPopup` 托管

**API 草案**:

```cpp
class AutoSuggestBox : public Control, public IPopup { /* 或内嵌建议层为 IPopup */ };
void SetText / GetText / SetPlaceholder;
void SetSuggestionItems(std::vector<std::string>); // 或回调式 ProvideSuggestions
Event OnTextChanged / OnSuggestionChosen / OnQuerySubmitted;
```

**行为**:

- 输入防抖后触发建议；↑↓ 选择，Enter 提交，Esc 关闭
- 建议层 light-dismiss；打开时不误关宿主页其它 popup（沿用 `PopupHost` 叠放规则）

**验收**: Showcase 页可过滤水果名列表；键盘全流程可用；高 DPI 下光标与击中正确。

---

### B. StatusBar

**定位**: 窗口底栏，多段（左主消息 + 弹性空白 + 右状态块）。

**自绘**:

- 整条背景与顶部分割线
- `StatusBarItem` 为轻量数据/绘制单元（文本、可选图标、固定/自动宽度），由 `StatusBar` 统一 Measure/Arrange/Render
- 不内嵌 `TextBlock`/`ProgressBar` 控件实例；进度段用自绘细条

**API 草案**:

```cpp
class StatusBar : public Control;
int AddTextItem(std::string text, StatusBarItemAlignment align = Left);
void SetItemText(int id, std::string);
void SetItemProgress(int id, float 0..1); // 可选
```

**验收**: 左消息、右 DPI/缩放/就绪状态；窗口缩放时不重叠、不裁切错乱。

---

### C. RatingControl

**定位**: 星级评分（只读 / 可交互）。

**自绘**:

- N 颗星几何（路径或字体 glyph），填充/描边/半星
- Hover 预览、点击定值、键盘 ←→ 调分
- 动画：填充宽度或透明度短过渡（自管 tick）

**API 草案**:

```cpp
float GetValue / SetValue; // 支持 0.5 步进可选
int GetMaxRating / SetMaxRating; // 默认 5
bool IsReadOnly / IsClearEnabled;
Event OnValueChanged;
```

**验收**: 半星、只读、清除（点同一星或专用清除）；主题色跟随 Accent。

---

### D. ToggleButton / SplitButton / DropDownButton

**定位**: 按钮族扩展；**Showcase 全部放在现有 Button 页**。

| 类型 | 交互 |
|------|------|
| ToggleButton | 按下锁定选中态，再点取消；可做图标+文字 |
| DropDownButton | 主区点击弹出自绘菜单（不嵌 `Button`+`ContextMenu` 拼接；菜单条目自绘或复用 `ContextMenu` 的**绘制协议**需评估——优先自绘轻量 `PopupMenuPainter`，避免“拼控件”） |
| SplitButton | 主按钮区触发 `Click`；分隔线右侧箭头区只打开下拉 |

**硬约束补充**: 下拉面板必须自绘条目命中；若复用 `ContextMenu`，仅允许作为已存在的 **IPopup 宿主实现** 复用其窗口/动画基础设施，条目数据与命中仍由按钮族驱动——实现阶段二选一写清，默认 **自绘菜单层**。

**验收**: Button 页三列对比；Split 主区与箭头区点击互不误触；键盘 Space/Alt+↓。

---

### E. Markdown 渲染器（RichTextBlock / RichEdit 方向）

**定位**: 第一期做 **Markdown → 自绘富文本视图**（`MarkdownView` / `RichTextBlock`），不是完整 Word 级 RichEdit。第二期再考虑源码编辑双模式。

**范围（MVP）**:

- 标题 h1–h6、段落、加粗/斜体/行内代码、链接
- 无序/有序列表、引用块、分隔线
- 围栏代码块（等宽、背景板、可选行号）
- 基础表格（管道表）
- 图片：可选（本地/资源路径），可二期

**架构**:

```
MarkdownParser  →  AST
BlockLayout     →  行盒 / 跑动文本 / 内联盒（自研）
MarkdownView    →  OnRender + 滚动 + 选区/复制（只读）
```

**非目标（本期）**: 完整 WYSIWYG 编辑、协作光标、HTML 全兼容、数学公式、目录侧边栏。

**验收**: Showcase 加载内置样例 `.md`；滚动流畅；选中复制纯文本；主题切换代码块可读。

---

### F. TeachingTip / Tooltip 增强

**Tooltip 增强**:

- 延迟显示 / 延迟隐藏、换行、最大宽度
- 可跟随锚点边缘自动翻转
- 自绘气泡 + 小三角；不嵌 `TextBlock`

**TeachingTip**:

- 模态可选（轻遮罩）或非模态
- 标题、正文、关闭、可选操作区（操作按钮为自绘热区，非 Button 子控件）
- 指向锚点；开关动画

**API 草案**:

```cpp
UIElement::SetToolTip(std::string); // 增强现有路径
class TeachingTip : public Control, public IPopup;
void ShowAround(UIElement* target); Close();
```

**验收**: TeachingTip 页：锚点四边弹出；Tooltip 长文本换行与超时。

---

### G. Chart

**定位**: 轻量数据可视化，纯 D2D。

**MVP 系列**:

1. LineChart（多系列折线 + 坐标轴 + 网格 + 悬停十字线/Tooltip）
2. BarChart（柱状）
3. PieChart（扇区 + 图例）

**数据**:

```cpp
struct ChartSeries { std::string name; std::vector<float> values; /* color optional */ };
void SetCategories(std::vector<std::string>);
void SetSeries(std::vector<ChartSeries>);
```

**非目标**: 3D、实时百万点、金融蜡烛图（可后续）。

**验收**: 三图切换；悬停读值；窗口缩放重算刻度。

---

### H. 拖放 API

**定位**: 框架级 DnD，供控件与应用使用。

**协议草案**:

```cpp
enum class DragDropEffects { None, Copy, Move, Link };
struct DataPackage { /* 文本 / 文件路径列表 / 自定义 mime */ };

class IDragSource {
  virtual DataPackage BeginDrag(...);
};
class IDropTarget {
  virtual DragDropEffects OnDragEnter/Over(Point, DataPackage&);
  virtual void OnDragLeave();
  virtual bool OnDrop(Point, DataPackage&);
};

// Window 消息: WM_DROPFILES 可选；内部拖放走鼠标捕获 + 反馈层
```

**自绘反馈**:

- 拖拽幽灵（半透明预览或光标旁徽章）由框架 `DragDropService` 画在 overlay
- 放置目标高亮矩形由 target 声明或服务绘制

**范围**:

1. 应用内控件间拖放（列表项、Tab、Dock 已有经验可对齐）
2. 外部文件拖入（`WM_DROPFILES` / `IDropTarget` COM 二选一，实现阶段定）
3. 拖出到外壳（可二期）

**验收**: DragDrop 页：两列表互拖；文件拖入显示路径；Esc 取消。

---

## 5. 工程与目录约定

建议新增文件（实现阶段按此落盘，本计划不创建源码）:

```
CUI.Core/ui/framework/controls/
  AutoSuggestBox.h/.cpp
  StatusBar.h/.cpp
  RatingControl.h/.cpp
  RangeSlider.h/.cpp
  ToggleButton.h/.cpp
  SplitButton.h/.cpp
  DropDownButton.h/.cpp
  MarkdownView.h/.cpp          // 或 RichTextBlock + Markdown*
  LogView.h/.cpp
  TeachingTip.h/.cpp
  chart/LineChart.h/.cpp ...
CUI.Core/ui/framework/text/     // Markdown AST / layout（可选）
CUI.Core/ui/framework/dnd/
  DragDropService.h/.cpp
  DataPackage.h
CUI/showcase/app/pages/
  AutoSuggestPage.cpp
  StatusBarPage.cpp
  RatingPage.cpp
  RangeSliderPage.cpp
  MarkdownPage.cpp
  LogViewPage.cpp
  TeachingTipPage.cpp
  ChartPage.cpp
  DragDropPage.cpp
  ButtonPage.cpp               // 扩展：Toggle/Split/DropDown
```

同步：`CUI.Core.vcxproj`、`CUIDsl.h`、导航注册、`PropertyGrid` 元数据。

---

## 6. 里程碑与估算（人天级，粗估）

| 里程碑 | 内容 | 粗估 |
|--------|------|------|
| M1 | 拖放 API 骨架 + Tooltip 增强 | 3–5 |
| M2 | TeachingTip + 按钮族三件套 + Button 页 | 4–6 |
| M3 | RatingControl + StatusBar | 2–3 |
| M4 | AutoSuggestBox | 4–6 |
| M5 | Chart MVP（折线+柱+饼） | 5–8 |
| M6 | Markdown 渲染器 MVP | 8–14 |

合计约 **26–42 人天**（含 Showcase、主题、基础测试）。Markdown 与 Chart 波动最大。

---

## 7. 验收总清单

- [ ] 上述控件均无“子控件拼接”实现（Code Review 门禁）
- [ ] 均支持亮/暗色 tokens
- [ ] 动画走 `RequestAnimationTicks` / `AnimationHost`，无弹层空转帧泵
- [ ] Showcase 可键盘操作核心路径
- [ ] Button 页可看到 Toggle / Split / DropDown
- [x] Markdown 样例只读渲染正确；Chart 三类图可交互读值
- [x] 拖放：应用内 + 文件拖入至少一种外部路径

---

## 8. 明确不做（本计划范围外）

- DataGrid / CalendarView / CommandBar / InfoBar / PersonPicture / Media / WebView
- 用现有控件拼装的“临时版”再重构（直接按自绘终态做）
- Markdown WYSIWYG 完整编辑器（可另开计划）

---

## 9. 下一步

1. 评审本计划（范围、顺序、Markdown 解析库许可）  
2. 冻结 M1 接口草稿（`DataPackage` / `TeachingTip` / 按钮族）  
3. **再开工实现**（本文档批准前不写功能代码）
