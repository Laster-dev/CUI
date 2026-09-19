---
layout: page
title: NumberBox（数字输入框）
parent: 控件参考
nav_order: 12
---

# NumberBox（数字输入框）

> 带上下微调箭头的数值输入框。内部持有一个私有 `Field : TextBox` 负责文本编辑，宿主负责画边框、箭头与 hover 高亮；支持 Min/Max 夹取、步长微调、长按连发、滚轮步进，并且在提交（失焦 / 回车 / 点箭头）时会对文本做**四则表达式求值**（`+ - * / % ^` 与括号）。

## 一、控件定位

| 场景 | 是否适用 |
|---|---|
| 输入一个数值并允许键盘微调（数量、透明度、字号） | ✅ 首选 |
| 需要 Min / Max 夹取与固定步长 | ✅ `Minimum / Maximum / Step` |
| 需要"输入 `100/3` 自动算出结果" | ✅ 提交时会求值 |
| 拖动选择数值 | ❌ 用 `Slider` / `RangeSlider` |
| 输入自由格式文本 | ❌ 用 [TextBox](TextBox.html) |

## 二、类信息

| 项 | 值 |
|---|---|
| 头文件 | `ui/framework/controls/NumberBox.h` |
| 声明 | `class NumberBox : public Control` |
| 继承链 | `UIElement` → `Control` → `NumberBox`（内含私有嵌套类 `NumberBox::Field : public TextBox`） |
| 命名空间 | `CUI` |
| DSL 工厂 | `NumberBoxWidget(double val = 0.0)` |
| 通用工厂 | `DSL::Control<NumberBox>()` |
| Tab 焦点 | ⚠️ 自身 `AcceptsTabFocus()` 返回 **`false`**（`NumberBox.h:19`）；真正接收 Tab 焦点的是内部 `Field` |
| 鼠标指针 | `nullptr`（默认箭头）；悬停在 Up / Down 箭头上时 `IDC_HAND` |

## 三、构造函数与出厂默认值

```cpp
NumberBox();                 // 唯一构造函数，无参
```

构造函数体（源码 `NumberBox.cpp:241-281`）：

| 项 | 默认值 |
|---|---|
| `Width` / `Height` | `120.0f` / `28.0f` |
| `Padding` | `(8, 4, 4, 4)` |
| `CornerRadius` | `3.0f` |
| `FontFamily` / `FontSize` | `"Segoe UI"` / `12.0f` |
| `BackgroundToken` / `HoverBackgroundToken` | `ThemeTokenId::InputBackground` / `ThemeTokenId::HoverBackground` |
| `BorderToken` / `FocusedBorderToken` | `ThemeTokenId::InputBorder` / `ThemeTokenId::FocusedBorder` |
| `ForegroundToken` | `ThemeTokenId::TextPrimary` |
| `BorderThickness` | `1.0f` |

成员初始化器（`NumberBox.h:115`、127-138）：

| 成员 | 默认值 |
|---|---|
| `m_value` | `0.0f` |
| `m_minimum` / `m_maximum` | `-100000.0f` / `100000.0f` |
| `m_step` | `1.0f` |
| `kSpinnerW`（箭头列宽，编译期常量） | `18.0f` |
| `m_hotUp` / `m_hotDown` | `AnimatedScalar{0.0f}` |

> 与 `TextBox` 不同，`NumberBox` 出厂就是**有边框的方框**（`BorderThickness 1.0f`），并且同时写入了 `Background` 具体色，切换深浅主题时需自行重设或只依赖 Token。

## 四、属性

### 4.1 自身属性

| 属性 | 类型 | 读 | 写 | 默认值 | 说明 |
|---|---|:-:|:-:|---|---|
| `Value` | `float` | `GetValue()` / 代理 `Value` | `SetValue(float)` / `Value = 3.0f` / DSL `.Value(v)` | `0.0f` | 会被 `clamp` 到 `[Minimum, Maximum]`；变化超过 `0.0001f` 才触发 `OnValueChanged` |
| `Minimum` | `float` | `GetMinimum()` / 代理 `Minimum` | `SetMinimum()` / DSL `.Minimum(v)` | `-100000.0f` | 下限 |
| `Maximum` | `float` | `GetMaximum()` / 代理 `Maximum` | `SetMaximum()` / DSL `.Maximum(v)` | `100000.0f` | 上限 |
| `Step` | `float` | `GetStep()` / 代理 `Step` | `SetStep()` / DSL `.Step(v)` | `1.0f` | 微调步长 |
| `ValueProperty` | `PropertyRef<float, PropertyId::ControlValue>` | `operator float()` | `ValueProperty = v`、`ValueProperty.Bind(state)` | — | 响应式双向绑定代理（`NumberBox.cpp:260` 已 `Initialize`） |

`PropertyId` 直通（`NumberBox.cpp:283-329`）：`ControlValue`、`Minimum`、`Maximum`、`Step`；另外 `Color`、`FontFamily`、`FontSize`、`FontWeight` 会**同时转发给内部 `Field`**。

### 4.2 继承自基类的常用属性

| 属性 | 说明 |
|---|---|
| `BackgroundToken` / `HoverBackgroundToken` | 静止 / 悬停底色 |
| `BorderToken` / `FocusedBorderToken` / `BorderThickness` | 常态 / 聚焦描边；聚焦时线宽强制为 `1.5f` |
| `ForegroundToken` | 数值文本颜色（会同步给 `Field`） |
| `CornerRadius` | 圆角，默认 `3.0f` |
| `Width` / `Height` | 默认 `120 / 28` |
| `Margin` | ⚠️ `Measure` **不把它算进** `m_desiredSize` |
| `FontFamily` / `FontSize` / `FontWeight` | 会转发给 `Field`（`FontStyle` / `FontStretch` 不转发） |

完整列表见 [04 · 属性与绑定](../CUI.Core/04-属性与绑定.html)。

## 五、方法

| 方法 | 说明 |
|---|---|
| `const char* GetClassName() const override` | 返回 `"NumberBox"` |
| `GetProperty / HasProperty / SetProperty` | 属性直通 + 字体/颜色转发给 `Field` |
| `HCURSOR GetCursor() const override` | Up / Down 悬停时 `IDC_HAND`，否则 `nullptr` |
| `bool AcceptsTabFocus() const override` | `false` |
| `Size Measure(Size) override` / `void Arrange(Rect) override` | 见 6.1 / 6.2 |
| `void OnRender(GraphicsContext&) override` | 见 6.4 |
| `OnMouseDown / OnMouseUp / OnMouseMove / OnMouseLeave / OnMouseWheel` | 见 6.5 |
| `bool OnKeyDown(int vkCode) override` | 先 `HandleFieldKey`，未命中再交 `Control::OnKeyDown` |
| `float GetValue() const` / `void SetValue(float)` | 数值读写（夹取、同步文本、触发事件） |
| `GetStep / GetMinimum / GetMaximum` + 对应 `SetXxx` | 步长与上下限 |
| `Event<NumberBox*, float>& OnValueChanged()` | 数值变化事件访问器 |
| `bool HandleFieldKey(int vkCode)` | Up / Down / Enter / Esc 处理入口 |
| `void OnFieldTextChanged()` | `Field` 文本变化回调：只做**纯数字**解析 |
| `void StepBy(float dir)` | 先 `CommitEdit()`，再 `SetValue(m_value + dir * m_step)` |
| `void CommitEdit()` | 对当前文本做表达式求值 / 纯数字解析并 `SetValue` |

私有部分（外部不可访问）：嵌套 `class Field : public TextBox`、`enum class HitPart { None, Text, Up, Down }`、`SpinnerCol() / UpBtn() / DownBtn() / TextRect()`、`HitTestPart()`、`FormatValue()`。

## 六、运行时行为

### 6.1 `Measure`（`NumberBox.cpp:331-337`）

```cpp
float expW = GetWidth();  if (expW < 0) expW = 120.0f;
float expH = GetHeight(); if (expH < 0) expH = 28.0f;
m_desiredSize = Size(expW, expH);   // 注意：不加 Margin
```

忽略 `availableSize`；没有显式尺寸就固定 `120×28`。

### 6.2 `Arrange` 与内部布局（339-379）

| 区域 | 计算式 |
|---|---|
| 箭头列 `SpinnerCol` | `x = bounds.x + width - 18 - border`，宽 `18`，高 `max(0, height - border*2)` |
| `UpBtn` | 箭头列上半（高 `col.height * 0.5`） |
| `DownBtn` | 箭头列下半 |
| `TextRect` | `x = bounds.x + border + pad.left`，宽 `max(0, width - border*2 - 18 - pad.left - 2)` |

### 6.3 命中分区（381-392）

先测 `UpBtn`，再测 `DownBtn`，最后 `m_bounds` 内算 `Text`，否则 `None`。

### 6.4 渲染顺序（`OnRender`，501-556）

1. 背景：`GetAnimatedBackground(...)`，圆角 > 0 用 `FillRoundedRect`。
2. 描边：聚焦时用 `FocusedBorderToken` 且线宽 **`1.5f`**，否则 `BorderToken` + `BorderThickness`。
3. Up / Down hover 填充：`tokens.textPrimary`，alpha `0.08 + 0.10 * t`。
4. 分隔线：竖直一条 `tokens.cardBorder`、`1.0f`；Up/Down 之间一条横线。
5. 两个箭头：`ctx.DrawChevron(up/down, tokens.textSecondary, 1.3f)`。

> 箭头列的颜色直接取 `ThemeManager` 的 token，**不随控件自身属性变化**。

### 6.5 鼠标

| 动作 | 行为（源码） |
|---|---|
| 按下（558-570） | 记录 `m_pressed = HitTestPart(pt)`；命中 Up → `StepBy(+1)`，Down → `StepBy(-1)` |
| 长按（614-638） | 按住 ≥ `0.40s` 后进入连发；连发间隔 `0.05s` |
| 抬起（572-577） | `m_pressed = None` |
| 移动（579-589） | 更新 `m_hover` 并设 hover 动画目标 `0/1` |
| 滚轮（600-605） | `StepBy(delta > 0 ? +1 : -1)` |

### 6.6 键盘（`HandleFieldKey`，481-499）

| 按键 | 行为 |
|---|---|
| `↑` | `StepBy(+1)` |
| `↓` | `StepBy(-1)` |
| `Enter` | `CommitEdit()`（求值并提交） |
| `Esc` | `SyncTextFromValue()`（丢弃输入，文本回滚） |

### 6.7 文本解析与格式化

- **输入过滤**（26-37、219-224）：`Field::OnCharInput` 只放行 `0-9`、`.`、`-+*/%^()`、`e`、`E`、空格与控制字符。
- **实时同步**（`OnFieldTextChanged`，447-462）：每次文本变化只尝试**纯数字**解析，成功则 `clamp`，且差值 > `0.0001f` 才更新并触发 `OnValueChanged`。**输入 `1+2` 期间数值不会变。**
- **提交**（`CommitEdit`，464-474）：先 `TryEvalExpression`（支持 `+ - * / % ^` 与括号），失败再 `TryParsePlainNumber`，仍失败则**保持原值**。
- **显示**（`FormatValue`，404-423）：`std::fixed` + `setprecision(6)`，去掉尾部多余的 `0` 与小数点。

### 6.8 动画（614-645）

| 动画 | 参数 |
|---|---|
| Up / Down hover | `AnimationSpec{0.22f, 0.01f}`（`SpinnerHoverSpec()`，19-24） |
| 长按连发 | 首次延迟 `0.40s`，之后每 `0.05s` 一步 |

## 七、事件

| 事件 | 类型 | 回调签名 | 触发时机 | 注册方式 |
|---|---|---|---|---|
| `OnValueChanged()` | `Event<NumberBox*, float>&` | `void(NumberBox* sender, float value)` | `SetValue` 且夹取后新旧值差 > `0.0001f`（443）；箭头、滚轮、`↑/↓`、回车提交、失焦提交都会触发 | `nb->OnValueChanged().Connect(fn);` |
| `OnClick` | `CallbackProperty<void(UIElement*)>` | `void(UIElement*)` | 继承自 `UIElement`，**不主动触发** | `nb->OnClick = fn;` |

> **坑**：`CUIDsl.h` 里的 `OnValueChanged` 链式方法只针对 `Slider` / `RangeSlider`，**`NumberBox` 没有 DSL 版**。必须拿到底层指针后 `.Connect(...)`。

## 八、示例

### 8.1 DSL 最简写法

```cpp
#include "CUI.h"

auto count = NumberBoxWidget(1)
                 .Minimum(0).Maximum(99).Step(1)
                 .Width(160.0f)
                 .Build();
```

### 8.2 代码式创建 + 订阅

```cpp
auto nb = std::make_shared<NumberBox>();
nb->SetMinimum(0.0f);
nb->SetMaximum(100.0f);
nb->SetStep(0.5f);
nb->SetValue(12.5f);
nb->OnValueChanged().Connect([](NumberBox* sender, float v) {
    // v 已被夹取到 [0, 100]
});
```

### 8.3 响应式绑定

```cpp
CUI::State<float> count{1.0f};
auto nb = NumberBoxWidget(0).Build();
nb->ValueProperty.Bind(count);   // 双向绑定
```

### 8.4 完整可运行：数量选择器

```cpp
#include "CUI.h"

int main() {
    auto label = Text("数量：1").FontSize(16.0f);
    auto nb    = NumberBoxWidget(1).Minimum(1).Maximum(20).Step(1).Width(160.0f).Build();

    nb->OnValueChanged().Connect([label](NumberBox*, float v) {
        Borrow(label).Text("数量：" + std::to_string(static_cast<int>(v)));
    });

    auto root = Column(16, { label, nb }).Align(Alignment::Center).Build();

    Window w;
    w.Fluent().Title("NumberBox Demo").Size(420, 240).Root(root).Build().Show().Run();
}
```

### 8.5 允许输入表达式（默认即支持）

```cpp
// 用户键入 "100/3" 后失焦或回车 → CommitEdit 求值 → 33.333333
auto nb = NumberBoxWidget(0).Minimum(-1000).Maximum(1000).Build();
```

## 九、主题与自定义外观

| 想改的东西 | 改哪个 |
|---|---|
| 底色 / 悬停底色 | `BackgroundToken(...)` / `HoverBackgroundToken(...)` |
| 常态描边 | `BorderToken(...)` + `BorderThickness(...)` |
| 聚焦描边 | `FocusedBorderToken(...)`（聚焦时线宽固定 `1.5f`） |
| 数值文本颜色 / 字体 | `ForegroundToken(...)`；`FontFamily / FontSize / FontWeight` 会同步给内部 `Field` |
| 圆角 | `CornerRadius(...)`（默认 `3.0f`） |
| 尺寸 | `Width(...)` / `Height(...)`（默认 `120×28`）；箭头列固定 `18` 宽 |
| 箭头 / 分隔线颜色 | ❌ 不可改：直接取 `tokens.textPrimary / cardBorder / textSecondary` |

## 十、注意事项

1. **Tab 焦点在内部 `Field` 上**：`NumberBox::AcceptsTabFocus()` 返回 `false`（`NumberBox.h:19`）。
2. **表达式只在提交时生效**：`1+2` 要等失焦 / 回车 / 点箭头。
3. **变化阈值 `0.0001f`**：小于它的变化既不更新 `m_value` 也不触发事件。
4. **改 `Minimum` / `Maximum` 不会回夹已有值**：改完请手动再 `SetValue(GetValue())`。
5. **`Measure` 不加 `Margin`**：`m_desiredSize` 就是 `Width/Height`（或 `120×28`）。
6. **显示格式化不可替换**：`FormatValue` 是 private，固定 6 位小数去尾零。
7. **箭头与滚轮都会先 `CommitEdit()`**：编辑框里留着未提交的表达式时点箭头，会先求值再加减步长。
8. **非法文本静默回落**：提交解析失败时保持旧值，不给任何错误提示。
9. **DSL 没有 `NumberBox` 版 `OnValueChanged`**，只能拿到 `shared_ptr` 后 `.Connect()`。
10. **`Field`、`TextRect()`、`FormatValue()` 全是 private**，无法在不改源码的前提下换布局或格式化规则。
