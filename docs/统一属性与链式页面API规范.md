# 统一属性与链式页面 API 规范

**状态：** 已确认。本文是 CUI 公共页面编写 API 的唯一规范。  
**适用范围：** `CUI.Core` 的对外控件接口、`CUI.Gallery` 全部页面、后续业务页面与 DSL。  
**目标：** 页面作者不再编写 `Get***` / `Set***` 或“先取对象再操作”的冗长代码；控件状态、集合、事件、只读运行时信息统一以命名成员暴露。

---

## 1. 最终使用形式

```cpp
button.Width = 35.0f;
float width = button.Width;
button.Width.Bind(widthState);

canvas.OnDraw = drawHandler;
list.Items = items;
auto bounds = element.Bounds;
```

页面层（Gallery、业务 UI、DSL 组装、示例）禁止新增或继续使用：

```cpp
button->SetWidth(35.0f);
float width = button->GetWidth();
canvas->SetOnDraw(drawHandler);
list->SetItems(items);
```

底层实现为 ABI 兼容、平台适配或内部桥接可暂时保留私有辅助函数，但不能再作为公开作者 API。

## 2. 统一规则

1. **一个 UI 概念只能有一个稳定命名成员。** 宽度只叫 `Width`，选中项只叫 `SelectedItem`，不能并存 `GetX`、`SetX`、`X` 三套作者 API。
2. **赋值、读取、绑定共用入口。** 可写属性必须支持 `=`、读取和 `Bind(...)`。
3. **事件是回调属性。** 页面写 `OnClick = handler`、`OnDraw = handler`，不写 `SetOn...` 或 `.Connect(...)`。
4. **集合是集合属性。** `Children`、`Items`、`Rows`、`Columns`、`SelectedIndices` 不再使用 Get/Set 对。
5. **框架计算结果是只读属性。** `Bounds`、`HWND`、`RowCount` 等可读/可观察，不可由页面赋值。
6. **只有动作保留方法。** 方法必须是动词：`Show()`、`Hide()`、`Navigate()`、`AddChild()`、`RemoveChild()`、`Focus()`、`ScrollIntoView()`。
7. **布局刷新由属性系统负责。** 写入影响 Measure、Arrange、Render、命中测试或数据视图的属性后，必须在同一 UI 更新周期失效并刷新；不得依赖窗口缩放。

## 3. 公共属性模型

| 类型 | 含义 | 页面写法 | 限制 |
|---|---|---|---|
| `Property<T>` | 可读、可写、可绑定状态 | `control.Width = 120;` | 支持读取、赋值、`Bind`、通知 |
| `ReadOnlyProperty<T>` | 布局、平台或控件计算值 | `auto b = element.Bounds;` | 页面不能赋值 |
| `CallbackProperty<Signature>` | 作者级事件回调槽位 | `canvas.OnDraw = handler;` | 新回调替换旧回调；空值清除 |
| `CollectionProperty<T>` | 结构、数据或选择集合 | `list.Items = items;` | 支持整体替换与受控集合操作 |

### 3.1 普通属性

```cpp
Button button("保存");
button.Width = 120.0f;
button.IsEnabled = canSave;
button.Foreground = Color::White;
button.BackgroundToken = ThemeTokenId::AccentFill;
button.Width.Bind(widthState);

float width = button.Width;
```

属性元数据必须标明变更影响：`AffectsMeasure`、`AffectsArrange`、`AffectsRender`、`AffectsHitTest`、`AffectsDataView`。属性系统集中负责绑定、脏标记、重新测量、重新布局、重绘与生命周期安全。

### 3.2 只读属性

```cpp
auto bounds = element.Bounds;
auto hwnd = window.HWND;
int rowCount = list.RowCount;
auto selected = list.SelectedIndices;
```

`Bounds`、`DesiredSize`、`ActualWidth`、`ActualHeight`、`HWND`、`RowCount` 等必须是 `ReadOnlyProperty<T>`。需要监听时使用统一属性订阅/绑定机制，不再额外暴露 `GetBounds()` 之类函数。

### 3.3 回调属性

```cpp
canvas.OnDraw = [model](CanvasDrawContext& context) {
    DrawScene(context, model);
};

canvas.OnCanvasMouseDown = [state](UIElement*, PointerEventArgs& args) {
    state->BeginDrag(args.Position);
};

button.OnClick = [status](UIElement*) {
    status->Text = "已点击";
};
```

- `OnXxx` 是一个作者级回调槽位，用 `=` 设置、用空回调清除。
- 若底层确实需要多播，使用内部 `Event<T>` / `Signal<T>`；公开页面 API 仍以 `OnXxx` 为主。
- 多订阅扩展使用动词，例如 `SubscribeClick(...)` 并返回订阅令牌；普通页面不能被迫写 `.OnClick().Connect(...)`。

### 3.4 集合属性

```cpp
list.Items = items;
grid.Rows = { GridLength::Auto(), GridLength::Star() };
grid.Columns = { GridLength::Star(), GridLength::Auto() };
panel.Children = { title, body, footer };
combo.SelectedIndices = { 0, 2 };
```

集合整体赋值必须触发正确的布局/视图刷新。读取返回只读视图、快照或受控代理，不能让页面静默绕过失效通知。增删可保留动词方法：`AddChild`、`RemoveChild`、`InsertItem`、`RemoveItem`、`ClearItems`。

## 4. 作者句柄与 `.` 语法

`std::shared_ptr<Button>` 是智能指针，C++ 访问它指向的成员必须用 `->`；C++ 不支持重载 `.` 运算符。因此要得到：

```cpp
auto button = Button("保存");
button.Width = 120.0f;
button.OnClick = Save;
```

公共页面 API 必须引入值语义作者句柄，例如 `ElementRef<T>` / `ControlRef<T>`：

- `ElementRef<T>` 内部持有或引用 `std::shared_ptr<T>`，但页面变量类型不暴露 `shared_ptr`。
- `ElementRef<T>` 以 `.` 暴露属性与作者级方法；只在框架互操作边界提供 `Shared()` / `Native()`。
- `Button(...)`、`Text(...)`、`Row(...)`、`Grid(...)` 等 DSL 工厂返回作者句柄或可无缝转换的构建对象。
- 内部所有权容器和虚函数边界仍可使用 `std::shared_ptr<UIElement>`，但这是实现细节。

不接受以下折中：对 `shared_ptr` 伪装点号、页面继续用 `Make<T>` 返回 `shared_ptr`、或同时公开 `GetX/SetX/X`。

## 5. DSL 与页面写法

页面使用语义工厂与链式配置表达结构和样式：

```cpp
auto btnSuccess = Button("成功状态 (Success)")
    .Background("#2E7D32")
    .Hover("#1B5E20")
    .Pressed("#0D3C10")
    .Foreground(Color::White)
    .OnClick([status](UIElement*) {
        status->Text = "点击了：成功状态按钮 (Emerald)";
    });

Row(12, { btnAccent, btnStandard, btnOutline, btnSubtle, btnDisabled });
```

- 工厂创建正确控件和默认语义；链式方法最终写入同名属性。
- `Row(gap, {...})`、`Column(gap, {...})`、`Grid(...)`、`Wrap(...)` 直接表达结构，不要求 `Children({...}).Build()`。
- 动态子列表允许 `Column(gap).Padding(24).Children(children).Build()`。
- `JustifiedWrapPanel` / Justified 流式模式负责按行填满可用宽度；子项只声明最小、最大、偏好尺寸，不通过页面硬编码宽高补空白。

## 6. 布局与溢出基线

采用 WinUI 风格约束布局：父元素提供可用尺寸，子元素在 Measure 返回期望尺寸，在 Arrange 的最终槽位内排布。

- 页面禁止为修复布局问题强行写固定宽高。
- `Grid`、`StackPanel`、`Canvas`、`WrapPanel`、Justified 流式布局必须正确响应可用宽度与运行时属性变更。
- 子项定位、随机重排、行列定义变化等必须立即触发重排和重绘，不得依赖窗口调整大小。
- 溢出按控件语义处理：文本换行/截断，滚动容器滚动，弹出层裁剪或翻转；默认不得绘制到父边界外。

## 7. 强制 API 映射

| 旧 API | 新 API | 类型 / 说明 |
|---|---|---|
| `SetOnDraw(...)` | `OnDraw = ...` | `CallbackProperty` |
| `SetOnTick(...)` | `OnTick = ...` | `CallbackProperty` |
| `SetOnCanvasMouseDown/Move/Up(...)` | `OnCanvasMouseDown/Move/Up = ...` | `CallbackProperty` |
| `SetContent(...)` | `Content = ...` | 内容属性 |
| `SetContentFactory(...)` | `ContentFactory = ...` | 工厂属性 |
| `SetSuggestionProvider(...)` | `SuggestionProvider = ...` | 提供器属性 |
| `SetItems(...)` / `GetItems()` | `Items = ...` / `Items` | 集合属性 |
| `GetChildren()` | `Children` | 集合属性 / 只读视图 |
| `SetColumnDefinitions(...)` | `Columns = ...` | Grid 列定义 |
| `SetRowDefinitions(...)` | `Rows = ...` | Grid 行定义 |
| `SetRows(...)` | `Rows = ...` | 类型按控件语义区分 |
| `GetBounds()` | `Bounds` | 只读布局属性 |
| `GetHWND()` | `HWND` | 只读平台属性 |
| `GetSelectedIndices()` | `SelectedIndices` | 选择集合 |
| `GetRowCount()` | `RowCount` | 只读计数属性 |
| `SetSelectedIndex(...)` / `GetSelectedIndex()` | `SelectedIndex = ...` / `SelectedIndex` | 状态属性 |
| `SetSelectedItem(...)` / `GetSelectedItem()` | `SelectedItem = ...` / `SelectedItem` | 状态属性 |
| `SetText(...)` / `GetText()` | `Text = ...` / `Text` | 状态属性 |
| `SetValue(...)` | `Value = ...` | 状态属性 |
| `SetMinimum/SetMaximum/SetStep(...)` | `Minimum` / `Maximum` / `Step` | 数值属性 |
| `SetDate/SetTime(...)` | `Date` / `Time` | 状态属性 |
| `SetInputText/GetInputText()` | `InputText = ...` / `InputText` | 输入属性 |
| `SetPassword/GetPassword()` | `Password = ...` / `Password` | 输入属性 |
| `SetFill/SetStroke/SetStrokeThickness(...)` | `Fill` / `Stroke` / `StrokeThickness` | 画布/形状属性 |
| `SetViewport(...)` | `Viewport = ...` | 画布属性 |
| `SetTitle/SetMessage/SetSubtitle(...)` | `Title` / `Message` / `Subtitle` | 壳层/对话框属性 |
| `SetPrimaryButtonText/SetSecondaryButtonText(...)` | `PrimaryButtonText` / `SecondaryButtonText` | 对话框属性 |
| `SetThemeMode/SetBackdropType(...)` | `ThemeMode` / `BackdropType` | 窗口属性 |

其余现存 Setter 必须同名转为属性：

- 输入显示：`AcceptsReturn`、`CaretIndex`、`InputEnabled`、`IsReadOnly`、`IsPasswordMode`、`ShowRevealButton`、`TextAlign`、`TextWrapping`、`LineSpacing`、`Markdown`。
- 状态：`IsChecked`、`IsOn`、`IsExpanded`、`IsCloseVisible`、`IsModal`、`IsPaneOpen`、`IsSettingsVisible`、`State`、`Visibility`。
- 列表树：`SelectionMode`、`SelectsOnInvoked`、`VirtualMode`、`ShowGridLines`、`ColumnVisible`、`RowHeight`、`IndentWidth`、`MaxVisibleSuggestions`、`SuggestionItems`。
- 布局壳层：`Header`、`RightContent`、`PaneTitle`、`PaneDisplayMode`、`Placement`、`PreferredPlacement`、`ExpandDirection`。
- 标识样式：`Id`、`Tag`、`GroupName`、`ActionText`、`CloseButtonText`、`BorderThickness`、`CornerRadius`。
- 其他：`MaxRating`、`RenderStatsOverlayVisible`、`AllowDrag`、`AllowDrop`。

`SetItemExpanded`、`SetRowSelected` 这类修改指定目标的 API 是命令，不改成单个属性；分别改为 `ExpandItem(item)` / `CollapseItem(item)`、`SelectRow(row)` / `DeselectRow(row)`。

## 8. 当前改造清单

### P0：页面层清零旧 API

1. 清除全部 Gallery 页面中的 `Get***` / `Set***` 调用。
2. 将 `SetOnDraw`、`SetOnTick`、`SetOnCanvasMouseDown/Move/Up` 和页面中的 `.Connect(...)` 改为 `On... = handler`。
3. 将 `SetItems`、`GetItems`、`GetChildren`、`SetColumnDefinitions`、`SetRowDefinitions`、`SetRows` 改为 `Items`、`Children`、`Columns`、`Rows`。
4. 将 `GetBounds`、`GetHWND`、`GetSelectedIndices`、`GetRowCount` 改为同名只读属性。
5. 页面变量不再以 `std::shared_ptr<UIElement>` 作为作者类型；改用 `ElementRef<T>` 或 DSL 作者句柄，确保使用 `.`。

### P1：控件层补齐

1. 为 P0 调用点提供相应的 `Property`、`ReadOnlyProperty`、`CallbackProperty`、`CollectionProperty`。
2. 属性写入连接统一失效路径，保证无需调整窗口也能立即刷新布局与渲染。
3. 旧 Get/Set 只可作为临时内部兼容包装并标记废弃，Gallery 与业务页面禁止引用。
4. 将共有外观属性稳定地放入 `UIElement` 公开表面：`Foreground`、`BackgroundToken`、`BorderToken`、`CornerRadius`、`BorderThickness` 等。

### P2：DSL 和批量迁移

1. 维护并扩展 `Button(...)`、`Text(...)`、`Row(...)`、`Column(...)`、`Grid(...)`、`Wrap(...)`、`JustifiedWrap(...)`。
2. 批量迁移脚本处理安全模式：`Make<T>`、`std::make_shared<T>`、`SetX(v)`、`GetX()`、`.Connect(...)`、固定集合的 `Children({...}).Build()`。
3. 脚本只改可确定的模式；重载歧义、有副作用和命令型 API 输出人工清单，禁止盲改。

### P3：门禁和验收

1. 公共头文件移除或废弃公开 `Get***` / `Set***`。
2. 增加扫描门禁：页面目录拒绝 `->Get[A-Z]`、`->Set[A-Z]`、`.Get[A-Z]`、`.Set[A-Z]`。
3. 设置动词方法白名单：`AddChild`、`RemoveChild`、`Navigate`、`Show`、`Hide`、`Focus`、`ExpandItem` 等。
4. 添加编译示例，覆盖属性读写绑定、只读属性、回调属性、集合属性、链式 DSL、动态 Children。

## 9. 验收标准

1. `CUI.Gallery/src` 的页面代码不含公开 `Get***` / `Set***`。
2. 页面事件不含 `SetOn...`，一般场景不含 `.Connect(...)`。
3. 所有已审计 API 均有同名属性或明确动词替代，没有无迁移路径的调用。
4. 属性写入无需调整窗口大小即可正确刷新 Measure、Arrange、Render 或数据视图。
5. Grid、StackPanel、Canvas、WrapPanel、Justified 流式布局在运行时变更后正确重排，且不依赖页面硬编码尺寸。
6. 演示页面不出现未处理的子项越界；溢出行为符合控件 WinUI 风格语义。
7. Gallery Debug x64 构建通过；迁移脚本可重复运行且不会破坏已迁移页面。

## 10. 实施顺序

1. 建立 `ElementRef<T>` / 作者句柄，先解决页面必须使用 `->` 的根因。
2. 补齐 P0 属性类型并接入统一失效元数据。
3. 执行可机械化批量迁移，生成无法安全迁移的人工清单。
4. 按控件类别处理命令型 API 与语义冲突。
5. 启用扫描门禁，完成全量构建和运行时布局验证。

这个顺序不可反转：没有作者句柄与统一属性对象，仅把 `SetWidth` 改名为 `Width` 仍会迫使页面写 `button->Width`，无法达到最终页面 API 目标。