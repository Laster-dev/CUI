---
layout: page
title: AutoSuggestBox（自动建议框）
parent: 控件参考
nav_order: 13
---

# AutoSuggestBox（自动建议框）

> 输入即弹出候选列表的搜索框。控件本体**自绘弹出层**（不复用 `ListBox`），内部持有一个 `AutoSuggestField : TextBox` 负责输入；支持静态目录的模糊子串过滤或自定义 `SuggestionProvider`，带 0.12 秒防抖、键盘上下选择、覆盖式自动隐藏滚动条与 Fluent 弹出动画。

## 一、控件定位

| 场景 | 是否适用 |
|---|---|
| 搜索框，输入时给候选词 | ✅ 首选 |
| 候选来自静态目录（城市、命令名） | ✅ `SuggestionItems` |
| 候选来自网络 / 数据库 | ✅ `SuggestionProvider` |
| 只要普通输入 | ❌ 用 [TextBox](TextBox.html) |
| 需要严格的枚举选择（不允许自由输入） | ❌ 用 `ComboBox` |

## 二、类信息

| 项 | 值 |
|---|---|
| 头文件 | `ui/framework/controls/AutoSuggestBox.h` |
| 声明 | `class AutoSuggestBox : public Control, public IPopup` |
| 继承链 | `UIElement` → `Control` → `AutoSuggestBox`（另实现 `IPopup`；内含 `AutoSuggestField : public TextBox`，定义于 .cpp） |
| 命名空间 | `CUI` |
| DSL 工厂 | `AutoSuggestBoxWidget(placeholder = "搜索…")` |
| 通用工厂 | `DSL::Control<AutoSuggestBox>()` |
| 建议回调类型 | `using SuggestionProviderFn = std::function<std::vector<std::string>(const std::string& query)>` |
| Tab 焦点 | ⚠️ 自身 `AcceptsTabFocus()` 返回 **`false`**（`AutoSuggestBox.h:26`）；焦点落在内部 `AutoSuggestField` 上 |
| 鼠标指针 | `GetCursor()` 直接返回 `nullptr`（指针由内部 `TextBox` 提供 `IDC_IBEAM`） |

## 三、构造函数与出厂默认值

```cpp
AutoSuggestBox();                 // 唯一构造函数，无参
```

构造函数体（源码 `AutoSuggestBox.cpp:72-105`，内部编辑框样式在 `StyleField()` 107-120）：

| 项 | 默认值 |
|---|---|
| `Placeholder` | `"搜索…"` |
| `Background` / `HoverBackground` / `BorderBrush` | `D2D1::ColorF(0,0,0,0)`（全透明） |
| `BackgroundToken` / `HoverBackgroundToken` / `BorderToken` / `FocusedBorderToken` | `ThemeTokenId::Unset` |
| `BorderThickness` / `CornerRadius` / `Padding` | `0.0f` |
| `ForegroundToken` | `ThemeTokenId::TextPrimary` |
| `PlaceholderColorToken` | `ThemeTokenId::TextMuted` |
| `FontFamily` / `FontSize` | `"微软雅黑"` / `12.0f` |
| `Width` / `Height` | `280.0f` / `32.0f` |
| `Text` | `""` |
| 内部 `AutoSuggestField` | 占位符 / 字体 / 前景 Token 跟随宿主，`Padding (8,6,8,6)` |

成员初始化器（`AutoSuggestBox.h:100`、131-152）：

| 成员 | 默认值 |
|---|---|
| `kDebounceSec`（编译期常量） | `0.12f` |
| `m_suggestionItemHeight` | `28.0f` |
| `m_maxVisibleSuggestions` | `8` |
| `m_highlightedIndex` | `-1` |
| `m_suggestScroll` | `0.0f` |
| `m_suggestionsOpen` | `false` |

> `OnRender()` 是**空函数**（`AutoSuggestBox.cpp:467-469`）：本体不画任何像素，可见外观完全来自内部 `TextBox` 加弹出层。给本体设 `Background` / `BorderBrush` 是看不到效果的。

## 四、属性

### 4.1 自身属性

| 属性 | 类型 | 读 | 写 | 默认值 | 说明 |
|---|---|:-:|:-:|---|---|
| `Text` | `std::string` | `GetText()` | `SetText()` | `""` | 与内部编辑框双向同步；变化会触发 `OnTextChanged` 并排程刷新建议 |
| `Placeholder` | `std::string` | `GetPlaceholder()` | `SetPlaceholder()` | `"搜索…"` | 同步下发给内部编辑框 |
| `SuggestionItems` | `std::vector<std::string>` | `GetSuggestionItems()` / 代理 | `SetSuggestionItems()` / DSL `.SuggestionItems({...})` | 空 | 全量目录；无 provider 时按**大小写不敏感子串**过滤 |
| `FilteredSuggestions` | `const std::vector<std::string>&` | `GetFilteredSuggestions()` | —（只读） | 空 | 当前过滤结果 |
| `SuggestionProvider` | `SuggestionProviderFn` | — | `SetSuggestionProvider(fn)` / DSL `.SuggestionProvider(fn)` | 空 | 一旦设置，**跳过**目录过滤 |
| `SuggestionItemHeight` | `float` | `GetSuggestionItemHeight()` | `SetSuggestionItemHeight(h)` | `28.0f` | 行高（无 DSL 链式方法） |
| `MaxVisibleSuggestions` | `int` | `GetMaxVisibleSuggestions()` / 代理 | `SetMaxVisibleSuggestions(n)` / DSL `.MaxVisibleSuggestions(n)` | `8` | 内部强制 `max(1, n)` |
| `IsPopupOpen` | `bool` | `IsPopupOpen()`（`IPopup`） | — | `false` | 弹出层是否打开 |

### 4.2 继承自基类的常用属性

| 属性 | 说明 |
|---|---|
| `PlaceholderColorToken` | 占位符颜色，会转发给内部编辑框 |
| `ForegroundToken` / `FontFamily` / `FontSize` | 宿主与内部编辑框共用 |
| `Width` / `Height` | 默认 `280 / 32` |
| `Margin` | ⚠️ `Measure` **不把它算进** `m_desiredSize` |
| `Visibility` | `Collapsed` 时 `Arrange` 会 `SetBounds(Rect())` |
| `IsEnabled` | 禁用后 `OnMouseDown` 直接返回 |

> 弹出层的配色（卡片底、描边、高亮）**只跟随全局主题 token**（`cardBackground` / `cardBorder` / `accentColor` / `textPrimary`），不读控件自身属性。

## 五、方法

| 方法 | 说明 |
|---|---|
| `const char* GetClassName() const override` | 返回 `"AutoSuggestBox"` |
| `HCURSOR GetCursor() const override` | 恒返回 `nullptr` |
| `bool AcceptsTabFocus() const override` | `false` |
| `Measure` / `Arrange` | 见 6.1 |
| `void OnRender(GraphicsContext&) override` | **空实现**（467-469） |
| `void OnRenderOverlay(GraphicsContext&) override` | 弹出层未被 `PopupHost` 接管时直接画 |
| `HitTest` / `HitTestOverlay` | 见 6.6 |
| `OnMouseDown / OnMouseMove / OnMouseLeave / OnMouseWheel` | 见 6.7 |
| `bool OnKeyDown(int)` / `void OnCharInput(wchar_t)` | 见 6.8 |
| `void OnFocus()` / `void OnBlur()` | 清 / 设 `m_pendingClose` |
| `bool ShouldClipToBounds() const override` | 返回 `!m_suggestionsOpen`（打开时允许画到边界外） |
| `SetText` / `GetText` / `SetPlaceholder` | 内容读写 |
| `SetSuggestionItems(...)` / `ClearSuggestionItems()` / `GetSuggestionItems()` / `GetFilteredSuggestions()` | 目录维护 |
| `void SetSuggestionProvider(SuggestionProviderFn)` | 设置自定义候选源 |
| `Get/Set SuggestionItemHeight`、`Get/Set MaxVisibleSuggestions` | 行高与可见行数 |
| `OnTextChanged() / OnSuggestionChosen() / OnQuerySubmitted()` | 三个事件访问器 |
| `IsPopupOpen()` / `GetPopupBounds()` / `HitDismissExempt()` / `HitTestPopup()` / `RenderPopup()` / `OnLightDismiss()` | `IPopup` 实现 |

私有部分：`AutoSuggestField`（定义在 .cpp）、`SetTextInternal`、`ScheduleSuggestRefresh`、`RefreshSuggestionsNow`、`OpenSuggestions`、`CloseSuggestions`、`ChooseSuggestion`、`SubmitQuery`、`EnsureSuggestionVisible`、`MoveHighlightBy`、`HitTestSuggestionIndex` 等。

## 六、运行时行为

### 6.1 `Measure` / `Arrange`（177-200）

```cpp
// Measure：忽略 availableSize
w = Width  >= 0 ? Width  : 280.0f;
h = Height >= 0 ? Height : 48.0f;      // 构造里已设 Height(32)，所以默认 280×32
m_desiredSize = Size(w, h);            // 不加 Margin

// Arrange：Collapsed → SetBounds(Rect())；否则扣掉 Margin 后 SetBounds + LayoutField
```

### 6.2 弹出层几何（403-408）

```cpp
desiredH = itemH * min(max(filtered.size(), 1), maxVisibleSuggestions);
bounds   = PlacePopupNearAnchor(m_bounds, m_bounds.width, desiredH,
                                GetPopupViewportOrDefault(), 2.0f, 4.0f,
                                PopupVerticalPlacement::Below);
```

宽度与宿主同宽，锚在下方、间距 `4.0f`，内边距 `2.0f`；高度按行数封顶。

### 6.3 弹出层绘制（`RenderPopup`，471-528）

1. `ctx.PushPopupReveal(menu, progress, Point(menu.x + width*0.5, menu.y))`。
2. 卡片底 `tokens.cardBackground` 圆角 `4.0f`；描边 `tokens.cardBorder` 线宽 `1.25f`。
3. 行矩形 `(menu.x+2, y, width-4, itemH-2)`；高亮行铺 `tokens.accentColor`（alpha `0.18`）圆角 `3.0f`。
4. 行文本左对齐垂直居中；高亮行用 `accentColor` + `DWRITE_FONT_WEIGHT_SEMI_BOLD`。
5. 滚动条（`maxScroll > 0.001` 且可见）：轨道宽 `8` 贴右，滑块高 `max(16, menu.height² / contentH)`。
6. `ctx.PopPopupReveal()`。

### 6.4 建议刷新（防抖 + 过滤）

- `ScheduleSuggestRefresh()` 把 `m_debounceLeft` 设为 **`0.12f`** 并请求帧调度；归零后 `RefreshSuggestionsNow()`。
- `RefreshSuggestionsNow()`（246-285）：有 provider → 调 provider；否则 **query 非空时**按大小写不敏感子串过滤目录——**query 为空则结果为空**；结果为空或未聚焦 → 关闭。
- `InputActive() = IsFocused() || m_field->IsFocused()`。

### 6.5 打开 / 关闭

- `OpenSuggestions()`：向 `PopupHost::Current()` 注册自己。
- 失焦链路：`AutoSuggestField::OnBlur` → 置 `m_pendingClose` → 下一帧 `FlushPendingClose()` → 若 `!InputActive()` 才真正关闭，避免"点弹出层导致的短暂失焦"误关。
- `OnLightDismiss()` → `CloseSuggestions()`；`HitDismissExempt()` 使点在本体或弹出层内不算 light-dismiss。

### 6.6 命中测试

| 方法 | 逻辑 |
|---|---|
| `HitTest`（437-453） | 先问内部 `Field`；再判断 `m_bounds`，命中返回 `m_field.get()` |
| `HitTestOverlay` / `HitTestPopup`（455-465） | `PopupProgress() <= 0.2f` 或候选为空 → `nullptr` |
| `HitTestSuggestionIndex`（417-435） | `index = localY / itemH`，越界返回 `-1` |

### 6.7 鼠标

| 动作 | 行为（源码） |
|---|---|
| 按下（537-549） | 未启用直接返回；命中建议行 → `ChooseSuggestion(index)` |
| 移动（551-571） | 键盘导航激活期间，指针位移平方 `< 4.0f` 直接忽略 |
| 滚轮（577-589） | `m_suggestScroll = clamp(m_suggestScroll - delta * itemH, 0, maxScroll)` |
| 离开（573-575） | 仅转调基类 |

### 6.8 键盘（`HandleSuggestionKey`，591-642）

| 按键 | 行为 |
|---|---|
| `Tab` | 弹出且候选非空 → 选中高亮项并消费按键（此时 Tab **不会移焦**） |
| `Esc` | 已弹出 → 关闭并消费 |
| `Enter` | 弹出且有高亮 → `ChooseSuggestion`；否则 `SubmitQuery()` |
| `↓` | 未弹出 → 立即刷新；已弹出 → `MoveHighlightBy(+1)` |
| `↑` | 已弹出 → `MoveHighlightBy(-1)` |

### 6.9 动画（677-727）

| 动画 | 参数 |
|---|---|
| 弹出 / 收起 | `AnimationSpec{0.20f, 0.005f, 0.22f, EaseOutCubic}`（`AnimationService.h:49-51`） |
| 滚动条自动隐藏 | 空闲 `1.5s` 后隐藏（`ScrollbarAutoHide.h:17`），瞬切不做淡入淡出 |

## 七、事件

| 事件 | 类型 | 回调签名 | 触发时机 | 注册方式 |
|---|---|---|---|---|
| `OnTextChanged()` | `Event<AutoSuggestBox*, const std::string&>&` | `void(AutoSuggestBox* sender, const std::string& text)` | 文本真正变化时（153）；选中建议也会触发 | `box->OnTextChanged().Connect(fn);` |
| `OnSuggestionChosen()` | `Event<AutoSuggestBox*, const std::string&>&` | `void(AutoSuggestBox* sender, const std::string& chosen)` | `ChooseSuggestion` 后（358） | `box->OnSuggestionChosen().Connect(fn);` |
| `OnQuerySubmitted()` | `Event<AutoSuggestBox*, const std::string&>&` | `void(AutoSuggestBox* sender, const std::string& query)` | 回车且未弹出 / 无高亮项时（364） | `box->OnQuerySubmitted().Connect(fn);` |
| `OnClick` | `CallbackProperty<void(UIElement*)>` | `void(UIElement*)` | 继承自 `UIElement`，**不主动触发** | `box->OnClick = fn;` |

> **坑**：三个事件都**没有对应的 DSL 链式方法**，必须先 `.Build()` 拿到 `shared_ptr` 再 `.Connect()`。

## 八、示例

### 8.1 静态目录 + 三个事件

```cpp
#include "CUI.h"

auto box = AutoSuggestBoxWidget("搜索城市")
               .SuggestionItems({"北京", "上海", "广州", "深圳", "成都", "杭州"})
               .MaxVisibleSuggestions(6)
               .Build();

box->OnTextChanged().Connect([](AutoSuggestBox*, const std::string& t) { /* 输入中 */ });
box->OnSuggestionChosen().Connect([](AutoSuggestBox*, const std::string& item) { /* 选中 */ });
box->OnQuerySubmitted().Connect([](AutoSuggestBox*, const std::string& q) { /* 回车 */ });
```

### 8.2 自定义 Provider

```cpp
auto box = AutoSuggestBoxWidget("输入关键字").Build();
box->SetSuggestionProvider([](const std::string& query) -> std::vector<std::string> {
    if (query.empty()) return {};
    return QueryFromIndex(query);   // 自己的检索逻辑
});
```

### 8.3 代码式创建并动态换目录

```cpp
auto box = std::make_shared<AutoSuggestBox>();
box->SetPlaceholder("输入命令");
box->SetSuggestionItems({"open", "save", "close"});
box->SetMaxVisibleSuggestions(4);
box->SetSuggestionItemHeight(32.0f);
```

### 8.4 完整可运行：搜索演示

```cpp
#include "CUI.h"

int main() {
    auto result = Text("（等待输入）").FontSize(15.0f);
    auto box    = AutoSuggestBoxWidget("搜索城市")
                      .SuggestionItems({"北京", "上海", "广州", "深圳", "成都", "杭州", "南京"})
                      .Width(280.0f)
                      .Build();

    box->OnSuggestionChosen().Connect([result](AutoSuggestBox*, const std::string& item) {
        Borrow(result).Text("已选择：" + item);
    });
    box->OnQuerySubmitted().Connect([result](AutoSuggestBox*, const std::string& q) {
        Borrow(result).Text("搜索：" + q);
    });

    auto root = Column(16, { box, result }).Align(Alignment::Center).Build();

    Window w;
    w.Fluent().Title("AutoSuggestBox Demo").Size(460, 240).Root(root).Build().Show().Run();
}
```

### 8.5 用 ↓ 键展开完整目录

```cpp
// 文本为空时按 ↓：源码会用整个目录打开（618-626）
auto box = AutoSuggestBoxWidget("选择一项").SuggestionItems({"A", "B", "C"}).Build();
```

## 九、主题与自定义外观

| 想改的东西 | 改哪个 |
|---|---|
| 占位符文案与颜色 | `AutoSuggestBoxWidget("...")` / `SetPlaceholder(...)`、`PlaceholderColorToken(...)` |
| 输入文本字体 | `FontFamily(...)` / `FontSize(...)` |
| 行高 | `SetSuggestionItemHeight(h)`（默认 `28.0f`） |
| 可见行数 | `.MaxVisibleSuggestions(n)`（默认 `8`） |
| 卡片底 / 描边 / 高亮色 | ❌ 改全局主题 `cardBackground` / `cardBorder` / `accentColor` |
| 本体背景 / 边框 | ❌ `OnRender` 为空，需覆写自行绘制 |
| 尺寸 | `Width(...)` / `Height(...)`；默认 `280×32` |

```cpp
class FramedSuggestBox : public AutoSuggestBox {
public:
    const char* GetClassName() const override { return "FramedSuggestBox"; }
    void OnRender(GraphicsContext& ctx) override {
        ctx.DrawRoundedRect(GetBounds(), 4.0f,
                            ThemeManager::Instance().GetColor(ThemeTokenId::InputBorder), 1.0f);
    }
};
```

## 十、注意事项

1. **没有 provider 时，空查询不产生任何建议**（`AutoSuggestBox.cpp:259`）。想聚焦即列全部请用 provider 或提示按 `↓`。
2. **刷新有 `0.12s` 防抖**：`SetText` 后立刻读 `GetFilteredSuggestions()` 可能还是旧结果。
3. **选中建议会再触发一次 `OnTextChanged`**：注意不要在回调里回写造成递归。
4. **三个事件无 DSL 链式方法**，必须 `.Build()` 后 `.Connect()`。
5. **Tab 的行为随弹出状态变化**：弹出且候选非空时 Tab 变成"选中当前项"而**不会**移焦。
6. **`OnRender` 为空**：给本体设 `Background` / `BorderBrush` 看不到效果。
7. **`Measure` 不加 `Margin`**，且忽略 `availableSize`。
8. **`MaxVisibleSuggestions` 被强制 `max(1, n)`**。
9. **滚动条只支持滚轮 / 键盘**，且 1.5 秒空闲后自动隐藏。
10. **弹出层依赖 `PopupHost::Current()`**；`ShouldClipToBounds()` 在打开时返回 `false`，内容可画到父容器之外。
