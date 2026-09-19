---
layout: page
title: TextBox（文本框）
parent: 控件参考
nav_order: 10
---

# TextBox（文本框）

> 单行 / 多行文本输入控件。自带占位符（含浮动标签动画）、选区与光标、撤销重做、右键菜单、剪贴板、IME 组字、文件拖放接收与密码遮罩能力；`PasswordBox`、`NumberBox::Field`、`AutoSuggestBox::AutoSuggestField` 都以它为基类。

## 一、控件定位

| 场景 | 是否适用 |
|---|---|
| 用户输入一行文本（用户名、搜索词、路径） | ✅ 首选 |
| 输入多行文本（备注、日志） | ✅ `AcceptsReturn(true)` / `TextWrapping(true)` |
| 输入密码 | ❌ 用 [PasswordBox](PasswordBox.html) |
| 输入数值并带上下微调箭头 | ❌ 用 [NumberBox](NumberBox.html) |
| 输入时弹出候选列表 | ❌ 用 [AutoSuggestBox](AutoSuggestBox.html) |
| 只读展示长文本 | ❌ 用 `TextBlock` |

## 二、类信息

| 项 | 值 |
|---|---|
| 头文件 | `ui/framework/controls/TextBox.h` |
| 声明 | `class TextBox : public Control, public IDropTarget` |
| 继承链 | `UIElement` → `Control` → `TextBox` |
| 命名空间 | `CUI` |
| DSL 工厂 | `TextField(text = "", onChanged = nullptr)` |
| 通用工厂 | `DSL::Control<TextBox>(args...)` |
| Tab 焦点 | ✅（`AcceptsTabFocus()` 返回 `true`，`TextBox.h:29`） |
| 鼠标指针 | `IDC_IBEAM`；密码模式下指针落在"显示密码"眼睛按钮上时切 `IDC_HAND` |

## 三、构造函数与出厂默认值

```cpp
TextBox();                                       // 空文本 + 空占位符
explicit TextBox(const std::string& placeholder); // 指定占位符
```

构造函数体内用 DSL 预设（源码 `TextBox.cpp:72-91`）：

| 项 | 默认值 |
|---|---|
| `Text` | `""` |
| `Placeholder` | `""` |
| `Background` / `HoverBackground` / `BorderBrush` | `D2D1::ColorF(0,0,0,0)`（全透明） |
| `BorderThickness` | `0.0f` |
| `UnderlineColorToken` | `ThemeTokenId::InputBorder` |
| `ActiveUnderlineColorToken` | `ThemeTokenId::AccentColor` |
| `CaretColorToken` | `ThemeTokenId::AccentColor` |
| `ForegroundToken` | `ThemeTokenId::TextPrimary` |
| `PlaceholderColorToken` | `ThemeTokenId::TextMuted` |
| `FontFamily` / `FontSize` | `"微软雅黑"` / `12.0f` |
| `Padding` | `(8, 6, 8, 6)` |
| `KeyboardNavigationMode` | `KeyboardNavigationMode::Contained` |
| `MinHeight` | `32.0f` |

成员初始化器中的其余出厂值（源码 `TextBox.h:178-209`）：

| 成员 | 默认值 |
|---|---|
| `m_cursorPos` / `m_selectionStart` / `m_selectionEnd` | `0 / 0 / 0` |
| `m_isPasswordMode` / `m_isPasswordRevealed` / `m_showRevealButton` | `false / false / true` |
| `m_isReadOnly` | `false` |
| `m_acceptsReturn` / `m_textWrapping` | `false / false` |
| `m_lineSpacing` / `m_lineHeight` | `1.0f / 0.0f` |
| `m_caretBlinkRate` / `m_caretWidth` | `500`（ms）/ `1.5f` |
| `m_allowDrop` / `m_dropHover` | `false / false` |

> 出厂外观是"透明底 + 底部一条下划线"，**不是**带边框的输入框。要方框观感请自行设 `BorderThickness` / `BackgroundToken`，或在主题层改 `inputBackground` / `inputBorder`。

## 四、属性

### 4.1 自身属性

| 属性 | 类型 | 读 | 写 | 默认值 | 说明 |
|---|---|:-:|:-:|---|---|
| `Text` | `std::string` | `GetText()` | `SetText()` | `""` | 编辑内容（**始终明文**）。赋相同值直接返回、不触发事件（`TextBox.cpp:163`） |
| `Placeholder` | `std::string` | `GetPlaceholder()` | `SetPlaceholder()` | `""` | 水印提示；非多行且高度 ≥ `40.0f` 时升级为"浮动标签" |
| `IsPasswordMode` | `bool` | `GetIsPasswordMode()` | `SetIsPasswordMode()` | `false` | 显示遮罩字符 |
| `IsPasswordRevealed` | `bool` | `GetIsPasswordRevealed()` | `SetIsPasswordRevealed()` | `false` | 临时明文显示 |
| `ShowRevealButton` | `bool` | `GetShowRevealButton()` | `SetShowRevealButton()` | `true` | 是否绘制右侧"眼睛"按钮 |
| `PasswordChar` | `wchar_t` | `GetPasswordChar()` | —（硬编码） | `L'•'`（U+2022） | 遮罩字符（`TextBox.h:89-91`） |
| `IsReadOnly` | `bool` | `GetIsReadOnly()` | `SetIsReadOnly()` | `false` | 只读：禁止编辑但仍可选中复制 |
| `AcceptsReturn` | `bool` | 仅 `GetProperty(PropertyId::AcceptsReturn)`（getter 为 private） | `SetAcceptsReturn()` | `false` | 允许回车换行 |
| `TextWrapping` | `bool` | 仅 `GetProperty(PropertyId::TextWrapping)` | `SetTextWrapping()` | `false` | 自动换行 |
| `LineSpacing` | `float` | `GetLineSpacing()` | `SetLineSpacing()` | `1.0f` | 行距系数 |
| `LineHeight` | `float` | `GetLineHeight()` | `SetLineHeight()` | `0.0f` | `0` 表示用字体自然行高 |
| `CaretBlinkRate` | `int` | `GetCaretBlinkRate()` | `SetCaretBlinkRate(ms)` | `500` | `<= 0` 时按 `500` 处理 |
| `CaretWidth` | `float` | `GetCaretWidth()` | `SetCaretWidth()` | `1.5f` | 光标竖条宽度 |
| `AllowDrop` | `bool` | `GetAllowDrop()` | `SetAllowDrop()` | `false` | 接受文本 / 文件拖放 |
| `CompositionString` | `std::wstring` | `GetCompositionString()` | `SetCompositionString()` | 空 | IME 组字串（一般由框架写入） |

`PropertyId` 直通（`TextBox.cpp:17-60`）：`LineSpacing`、`LineHeight`、`CaretWidth`、`CaretBlinkRate`、`TextWrapping`、`AcceptsReturn`、`IsReadOnly` 都可用 `GetProperty / SetProperty / HasProperty` 访问。

### 4.2 继承自基类的常用属性

完整列表见 [04 · 属性与绑定](../CUI.Core/04-属性与绑定.html)。文本框上真正会被用到的：

| 属性 | 说明 |
|---|---|
| `Background` / `BackgroundToken` | 输入框底色；默认全透明且 Token 为 `Unset`，渲染时回落到 `ThemeTokenId::InputBackground` |
| `UnderlineColorToken` / `ActiveUnderlineColorToken` | 静止 / 激活下划线，同时决定选区底色 |
| `PlaceholderColorToken` | 占位符与浮动标签的基色 |
| `CaretColorToken` | 光标颜色 |
| `BorderBrush` / `BorderThickness` / `BorderToken` | 想做方框时改这三个 |
| `Foreground` / `ForegroundToken` | 文本颜色 |
| `Padding` / `Margin` | `Padding` 决定文本矩形，`Margin` 加在 `m_desiredSize` 上 |
| `Width` / `Height` | `>= 0` 时覆盖自动测量 |
| `MinWidth` / `MinHeight` | 测量结果取下界（默认 `0` / `32`） |
| `KeyboardNavigationMode` | 默认 `Contained`，Tab 到末尾不出控件 |

## 五、方法

### 5.1 公有方法

| 方法 | 说明 |
|---|---|
| `const char* GetClassName() const override` | 返回 `"TextBox"` |
| `GetProperty / HasProperty / SetProperty` | 属性系统直通 |
| `HCURSOR GetCursor() const override` | `IDC_IBEAM`；禁用返回 `nullptr`；密码模式且指针在眼睛按钮上返回 `IDC_HAND` |
| `bool AcceptsTabFocus() const override` | `true` |
| `Size Measure(Size) override` | 见 6.1；忽略 `availableSize` |
| `void OnRender(GraphicsContext&) override` | 见 6.5 |
| `OnAnimationTick` / `HasSelfAnimation` | 浮动标签 / 焦点下划线 / 光标闪烁，见 6.6 |
| `OnMouseDown / OnMouseDblClick / OnMouseRightClick / OnMouseUp / OnMouseMove / OnMouseEnter / OnMouseLeave` | 见 6.2 |
| `bool OnKeyDown(int vkCode) override` | 见 6.3 |
| `void OnFocus()` / `void OnBlur()` | 重置闪烁相位、清选区 / 组字串 / 滚动偏移 |
| `void OnCharInput(wchar_t ch)` | 字符输入入口，`ch >= 32` 才插入 |
| `void CommitImeResult(const std::wstring&)` | IME 上屏 |
| `void SelectAll()` / `bool HasSelection() const` / `void DeleteSelection()` | 选区操作 |
| `SetText / GetText`、`SetPlaceholder / GetPlaceholder` | 内容读写 |
| `virtual std::wstring GetDisplayedText() const` | 密码未揭示时返回等长 `•` 串 |
| `Event<TextBox*, const std::string&>& OnTextChanged()` | 文本变化事件访问器 |
| `DragDropEffects OnDragOver(...)` / `void OnDragLeave()` / `bool OnDrop(...)` / `Rect DropHighlightRect() const` | `IDropTarget` 实现，见 6.7 |

### 5.2 可覆写（protected / virtual）

| 方法 | 说明 |
|---|---|
| `virtual std::wstring GetDisplayedText() const` | 覆写即可自定义遮罩策略 |
| `Rect GetRevealButtonRect() const`（protected） | 眼睛按钮矩形：`24×24`，距右边缘 `6.0f`，垂直居中（`TextBox.cpp:189-194`） |
| `virtual void OnRender / OnKeyDown / OnMouseDown …` | 同签名虚函数均可覆写 |

### 5.3 内部状态（private 成员）

| 成员 | 默认值 | 含义 |
|---|---|---|
| `m_cursorPos` | `0` | 光标索引（UTF-16 单位） |
| `m_selectionStart` / `m_selectionEnd` | `0 / 0` | 选区，允许 `start > end` |
| `m_undoStack` / `m_redoStack` | 空 | 撤销 / 重做栈，上限 **200**（`TextBox.cpp:403-405`） |
| `m_labelAnim` / `m_focusLineAnim` | `AnimatedScalar{}` | 浮动标签 / 焦点下划线进度 |
| `m_textLayoutCache` | `TextLayoutCache` | DWrite 布局缓存 |

## 六、运行时行为

### 6.1 `Measure`（`TextBox.cpp:464-494`）

```cpp
text     = GetText().empty() ? GetPlaceholder() : GetText();
tsize    = MeasureText(text, fontFamily, fontSize, weight, style, stretch);
naturalW = tsize.width  + padding.left + padding.right + border*2;
naturalH = tsize.height + padding.top  + padding.bottom + border*2;
w = Width  >= 0 ? Width  : max(naturalW, MinWidth);
h = Height >= 0 ? Height : max(naturalH, MinHeight);
m_desiredSize = Size(w + margin.left + margin.right, h + margin.top + margin.bottom);
```

- **忽略 `availableSize`**：不换行、不被压缩，超长文本靠内部横向滚动显示。
- 只按**当前文本**测量：输入过程中宽度会跳变，表单场景务必显式给 `Width` / `MinWidth`。

### 6.2 鼠标

| 动作 | 行为（源码） |
|---|---|
| 左键按下（859-878） | 未启用直接返回；密码模式命中眼睛按钮 → 切换 `IsPasswordRevealed`；否则聚焦 + 定位光标 + 折叠选区 + 进入拖拽选区 |
| 双击（790-817） | 以 `iswalnum` 为边界选中一个"词" |
| 拖拽（976-1015） | 更新 `m_selectionEnd`；单行模式指针越界时每次滚动 `10.0f` |
| 抬起（1017-1020） | 清 `m_isDraggingSelection` |
| 右键（880-960） | 首次右键自动创建并挂上 `ContextMenu`：撤销 / 重做 / 剪切 / 复制 / 粘贴 / 删除 / 全选；只读时隐藏编辑类项 |

### 6.3 键盘（`OnKeyDown`，`TextBox.cpp:1022-1259`）

| 按键 | 行为 |
|---|---|
| `Ctrl+Z` / `Ctrl+Y` | 撤销 / 重做 |
| `Ctrl+A` | 全选 |
| `Ctrl+C` | 复制选区到剪贴板（`CF_UNICODETEXT`） |
| `Ctrl+X` | 剪切；只读时吞掉 |
| `Ctrl+V` | 粘贴；`AcceptsReturn` 为假时把 `\r\n` 换成空格 |
| `Enter` | `AcceptsReturn` 为真时插入 `\n` |
| `Backspace` / `Delete` | 有选区删选区，否则删前 / 后一个字符 |
| `←` / `→` | 移动光标；`Shift` 扩展选区 |
| `↑` / `↓` | 仅多行有效 |
| `Home` / `End` | 多行按命中测试移到行首 / 行尾 |

只读时 `Backspace` / `Delete` / `Enter` / `Ctrl+X` / `Ctrl+V` 被吞掉但仍返回 `true`；导航与 `Ctrl+C/A` 不受影响。

### 6.4 撤销 / 重做

编辑动作前调用 `PushUndoState()`（`TextBox.cpp:394-407`），栈深上限 **200**；快照 = `{text, cursorPos, selectionStart, selectionEnd}`。直接调 `SetText()` **不会**入栈。

### 6.5 渲染顺序（`OnRender`，600-788）

1. 背景：`m_hasBackgroundColor` → `BackgroundToken` → 回落 `ThemeTokenId::InputBackground`。
2. 拖放高亮：`accentColor`（alpha `0.18`）+ `2.0f` 描边。
3. 浮动标签 / 占位符：浮动条件 = 占位符非空 **且** 非多行 **且** 高度 ≥ `40.0f`；字号 `fontSize → 11.0f` 插值，Y 从 `bounds.y + 16` 移到 `bounds.y + 4`。
4. `PushClip(textRect)` → 选区（`accentColor` alpha `0.28`，**仅聚焦时**）→ `DrawTextLayout` → IME 组字下划线 → 光标 → `PopClip()`。
5. 光标：按 `GetTickCount64() / blinkRate % 2` 闪烁；高 `max(12.0f, caret.height - 4.0f)`、宽 `CaretWidth`。
6. 底部下划线：静止线宽 `1.0f`；聚焦时叠加一条从中心向两侧展开的激活线。
7. 密码模式且 `ShowRevealButton`：画眼睛图标（外框 `14×9`、圆角 `4.5`）。

### 6.6 动画与光标闪烁（496-559）

- 浮动标签 `AnimationSpec{0.34f, 0.01f}`；焦点线 `AnimationSpec{0.28f, 0.01f}`。
- 光标闪烁周期默认 `500ms`；只脏 `m_lastCaretDirtyRect.Inflate(6.0f)` 这一条窄带；失焦时 `CancelWake`。

### 6.7 拖放（默认关闭）

`AllowDrop(true)` 后：`OnDragOver` 只接受 `HasText() || HasFiles()` 且非只读；`OnDrop` 有文件时**整体替换**文本——多行用 `\n` 连接，单行用 `"; "` 连接（128-151）。

### 6.8 多行判定

`IsMultiline() = AcceptsReturn || TextWrapping`（176-178）。多行时纵向滚动、段落顶对齐；单行时横向滚动、段落垂直居中。

## 七、事件

| 事件 | 类型 | 回调签名 | 触发时机 | 注册方式 |
|---|---|---|---|---|
| `OnTextChanged()` | `Event<TextBox*, const std::string&>&` | `void(TextBox* sender, const std::string& text)` | 仅 `SetText()` 且新值 != 旧值时（`TextBox.cpp:173`）；插入、退格、删除、粘贴、拖放、IME 上屏、撤销重做都会触发 | `tb->OnTextChanged().Connect(fn);`；DSL：`TextField(text, onChanged)` |
| `OnClick` | `CallbackProperty<void(UIElement*)>` | `void(UIElement*)` | 继承自 `UIElement`，**TextBox 不主动触发** | `tb->OnClick = fn;` |

```cpp
EventId id = tb->OnTextChanged().Connect([](TextBox* s, const std::string& t) { /* ... */ });
tb->OnTextChanged().Disconnect(id);
```

> **坑**：如果在回调里再调 `SetText()`，值不同会形成递归链，建议回调里只读不回写。

## 八、示例

### 8.1 DSL 最简写法

```cpp
#include "CUI.h"

auto input = TextField("admin")
                 .Placeholder("请输入用户名")
                 .Width(240.0f)
                 .Build();
```

### 8.2 代码式创建 + 订阅

```cpp
auto box = std::make_shared<TextBox>("搜索关键字");  // 实参是占位符
box->SetWidth(260.0f);
box->OnTextChanged().Connect([](TextBox* sender, const std::string& text) {
    // 实时过滤逻辑
});
```

### 8.3 多行只读日志框

```cpp
auto logBox = TextField()
                  .AcceptsReturn(true)
                  .TextWrapping(true)
                  .IsReadOnly(true)
                  .Width(420.0f)
                  .Height(160.0f)
                  .Build();
```

### 8.4 完整可运行：文件拖放接收

```cpp
#include "CUI.h"

int main() {
    auto label = Text("把文件拖到下面的框里").FontSize(14.0f);
    auto drop  = TextField()
                     .Placeholder("拖放文件到这里")
                     .AllowDrop(true)
                     .AcceptsReturn(true)
                     .Width(360.0f)
                     .MinHeight(64.0f)
                     .Build();
    drop->OnTextChanged().Connect([label](TextBox*, const std::string& text) {
        Borrow(label).Text("已接收 " + std::to_string(text.size()) + " 字符");
    });

    auto root = Column(12, { label, drop }).Align(Alignment::Center).Build();

    Window w;
    w.Fluent().Title("TextBox Demo").Size(520, 260).Root(root).Build().Show().Run();
}
```

## 九、主题与自定义外观

| 想改的东西 | 改哪个 |
|---|---|
| 输入框底色 | `BackgroundToken(ThemeTokenId::InputBackground)`（推荐）或 `Background(color)` |
| 下划线（静止 / 激活） | `UnderlineColorToken(...)` / `ActiveUnderlineColorToken(...)` |
| 占位符颜色 | `PlaceholderColorToken(...)`（默认 `TextMuted`） |
| 光标颜色与粗细 | `CaretColorToken(...)` + `SetCaretWidth()` / `SetCaretBlinkRate()` |
| 变成有边框的方框 | `BorderThickness(1.0f)` + `BorderToken(ThemeTokenId::InputBorder)` |
| 高度 | `MinHeight(...)`（默认 `32.0f`）；≥ `40.0f` 且非多行时启用浮动标签 |

自定义遮罩字符：

```cpp
class MaskedBox : public TextBox {
public:
    std::wstring GetDisplayedText() const override {
        return std::wstring(GetText().length(), L'*');
    }
    const char* GetClassName() const override { return "MaskedBox"; }
};
```

## 十、注意事项

1. **`TextBox(text)` 传进去的是占位符**，不是文本（`TextBox.h:21`）。要设初始文本请用 `TextField(text)` 或 `.Text(...)`。
2. **宽度随内容跳变**：`Measure` 只按当前文本算且忽略 `availableSize`，表单场景务必固定 `Width` / `MinWidth`。
3. **`SetText()` 相同值静默返回**，不触发 `OnTextChanged`。
4. **`SetText()` 不入撤销栈**，程序化赋值后再 `Ctrl+Z` 会跳到更早的用户编辑状态。
5. **只读 ≠ 禁用**：只读下仍能聚焦、选中、`Ctrl+C`。
6. **密码模式的剪贴板是明文**：复制出来的是明文。
7. **多行判定是"或"关系**：只开 `TextWrapping` 也会进入多行模式。
8. **浮动标签有高度门槛**：`Height >= 40` 且非多行且占位符非空才启用。
9. **拖放默认关闭**，且拖入文件是**整体替换**文本。
10. **继承链副作用**：`PasswordBox` / `NumberBox::Field` / `AutoSuggestBox::AutoSuggestField` 都派生自 `TextBox`。
