---
layout: page
title: Button（按钮）
parent: 控件参考
nav_order: 1
---

# Button（按钮）

> 触发一次即时动作的标准按钮。内置 Hover / Pressed 状态过渡与点击水波纹（Ripple）反馈，支持鼠标与键盘（空格 / 回车）两种触发方式。

## 一、控件定位

| 场景 | 是否适用 |
|---|---|
| 点击后立即执行一个动作（保存、提交、打开窗口） | ✅ 首选 |
| 表示一个可长期保持的开关状态 | ❌ 用 [ToggleButton](ToggleButton.html) / [ToggleSwitch](ToggleSwitch.html) |
| 点击后弹出菜单让用户二选一 | ❌ 用 [DropDownButton](DropDownButton.html) |
| 只是一个可点击的文字链接 | ❌ 用 [HyperlinkButton](HyperlinkButton.html) |

## 二、类信息

| 项 | 值 |
|---|---|
| 头文件 | `ui/framework/controls/Button.h` |
| 声明 | `class Button : public Control` |
| 继承链 | `UIElement` → `Control` → `Button` |
| 命名空间 | `CUI` |
| DSL 工厂 | `ElevatedButton(text, onPressed)` |
| 通用工厂 | `DSL::Control<Button>(args...)` |
| Tab 焦点 | ✅（`AcceptsTabFocus()` 返回 `true`） |
| 鼠标指针 | 启用时为 `IDC_HAND`（手型） |

## 三、构造函数与出厂默认值

```cpp
Button();                                 // 文本默认为 "Button"
explicit Button(const std::string& text); // 指定文本
```

构造函数内部通过 DSL 预设了下列值（源码 `Button.cpp:20-39`），**这就是按钮的"出厂外观"**：

| 项 | 默认值 |
|---|---|
| `Text` | `"Button"` |
| `BackgroundToken` / `HoverBackgroundToken` / `PressedBackgroundToken` | `ThemeTokenId::AccentColor` |
| `BorderToken` | `ThemeTokenId::AccentColor` |
| `FocusedBorderToken` | `ThemeTokenId::FocusedBorder` |
| `ForegroundToken` | `ThemeTokenId::AccentForeground` |
| `FontFamily` / `FontSize` | `"微软雅黑"` / `12.0f` |
| `Padding` | `(8, 4, 8, 4)` |
| `CornerRadius` | `4.0f` |
| `BorderThickness` | `0.0f` |

> 注意：构造时同时写入了 **Token** 与**具体颜色**（`ThemeManager::Instance().GetColor("accentColor")`）。
> 因此后续切换深浅色主题时，若未显式重设，按钮沿用构造那一刻取到的具体色。想要完全跟随主题，请覆写 `BackgroundToken` 并清掉具体色，或在主题切换回调里重新赋值。

## 四、属性

### 4.1 自身属性

| 属性 | 类型 | 读 | 写 | 默认值 | 说明 |
|---|---|:-:|:-:|---|---|
| `Text` | `std::string` | `GetText()` | `SetText()` | `"Button"` | 按钮文字。等价于继承自 `UIElement` 的同名属性，`Button` 仅做类型收敛；赋值后会触发重新测量与重绘 |
| `Icon` | `std::string` | `GetIcon()` | `SetIcon()` | 空 | 来自 `UIElement`。支持字形文本与 SVG 数据（`GraphicsContext::LooksLikeSvg`），非空时与文字间距 `6.0f` |

### 4.2 继承自 `Control` / `UIElement` 的常用属性

完整列表见 [04 · 属性与绑定](../CUI.Core/04-属性与绑定.html)。按钮上真正会被用到的：

| 属性 | 说明 |
|---|---|
| `Background` / `HoverBackground` / `PressedBackground` | 静止 / 悬停 / 按下三态背景色 |
| `BackgroundToken` / `HoverBackgroundToken` / `PressedBackgroundToken` / `DisabledBackgroundToken` | 三态 + 禁用态的主题 Token（推荐，随主题切换） |
| `BorderBrush` / `BorderThickness` / `BorderToken` | 描边颜色与粗细 |
| `Foreground` / `ForegroundToken` | 文字颜色 |
| `CornerRadius` | 圆角半径，`0` 时为直角矩形 |
| `Padding` | 内容内边距，参与 `Measure` 计算 |
| `Margin` | 外边距 |
| `Width` / `Height` | 显式尺寸，`>= 0` 时**覆盖**自动测量结果 |
| `Opacity` | 整体透明度 `0.0f ~ 1.0f` |
| `IsEnabled` | 禁用后不响应点击、指针恢复默认、不触发水波纹 |

## 五、方法

### 5.1 公有方法

| 方法 | 说明 |
|---|---|
| `virtual const char* GetClassName() const override` | 返回 `"Button"` |
| `virtual HCURSOR GetCursor() const override` | 启用时返回手型指针，禁用时返回 `nullptr` |
| `virtual Size Measure(Size availableSize) override` | 见"六、测量逻辑"，`availableSize` 未被使用 |
| `virtual void OnRender(GraphicsContext&) override` | 绘制背景（含圆角裁剪）、水波纹、描边与标签 |
| `virtual void OnMouseDown(Point pt) override` | 按下时启动水波纹；未启用时直接返回 |
| `virtual bool OnKeyDown(int vkCode) override` | 见"七、键盘行为" |
| `virtual bool AcceptsTabFocus() const override` | 返回 `true` |
| `virtual bool OnAnimationTick() override` | 推进水波纹帧；基类 `Control` 的状态过渡动画同时推进 |
| `virtual bool HasSelfAnimation() const override` | 水波纹运行中或基类仍在过渡时返回 `true`，用于维持逐帧调度 |
| `void SetText(const std::string&)` / `const std::string& GetText() const` | 文字读写 |

### 5.2 受保护方法（派生自定义按钮时可复用）

| 方法 | 说明 |
|---|---|
| `void BeginRipple(Point pt)` | 以 `pt`（局部坐标）为圆心启动水波纹，并 `RequestAnimationTicks()` |
| `bool TickRipple()` | 推进半径与透明度，返回是否仍需下一帧 |
| `void DrawRipple(GraphicsContext&)` | 在圆角裁剪区域内绘制半透明圆 |
| `void DrawButtonFace(GraphicsContext&, D2D1_COLOR_F bg, D2D1_COLOR_F border, float borderThickness)` | 绘制按钮表面 + 水波纹 + 描边 |
| `void DrawButtonLabel(GraphicsContext&, const Rect& textRect, DWRITE_TEXT_ALIGNMENT align)` | 绘制图标与文字（含居中 / 右对齐排布） |

### 5.3 内部状态（protected 成员）

| 成员 | 默认值 | 含义 |
|---|---|---|
| `m_rippleCenter` | `{0,0}` | 水波纹圆心（局部坐标） |
| `m_rippleRadius` | `0.0f` | 当前半径 |
| `m_rippleOpacity` | `0.0f` | 当前不透明度 |
| `m_rippleActive` | `false` | 动画是否在跑 |

## 六、测量逻辑（`Measure`）

```cpp
contentH = fontSize + 4.0f;                       // 基线高度
if (有 Icon)  contentW += svg ? fontSize + 2 : 文本测量宽;   // contentH 取较大者
if (有 Text)  { if (contentW > 0) contentW += 6.0f; contentW += 文本测量宽; }
w = contentW + margin.left + margin.right + padding.left + padding.right;
h = contentH + margin.top  + margin.bottom + padding.top   + padding.bottom;
if (GetWidth()  >= 0) w = GetWidth();     // 显式尺寸优先
if (GetHeight() >= 0) h = GetHeight();
```

要点：

- **按钮不会自动拉伸填满父容器**，尺寸由内容决定；需要占满一行请显式 `Width(...)` 或放进 `Expanded(...)`。
- `availableSize` 被忽略，因此按钮**不会换行**；超长文本会溢出（需要截断时自行缩短 `Text`）。

## 七、运行时行为

### 7.1 键盘

| 按键 | 行为 |
|---|---|
| `VK_SPACE` | 从按钮中心启动水波纹 → `ExecuteBoundCommand()` → `OnClick` 触发 → 返回 `true`（事件已消费） |
| `VK_RETURN` | 同上 |
| 其它 | 交给 `Control::OnKeyDown` |

未启用（`!IsEnabled()`）时两个按键都不响应，返回 `false`。

### 7.2 水波纹动画

- 起点半径 `4.0f`，透明度 `0.35f`；终点半径为圆心到最远角的距离。
- 半径按帧插值逼近 `maxRadius` 并额外加上 `37.0f * dt` 的推进量；透明度按 `pow(0.958f, dt*60)` 衰减，降到 `<= 0.02f` 时结束。
- 颜色取 `ThemeTokenId::TextPrimary`，仅改 alpha，因此深浅色主题下都可见。
- 全局关闭动画（`UIElement::AreAnimationsEnabled() == false`）时**完全跳过**，不会空转。

### 7.3 绘制顺序

`GetAnimatedBackground()` 取当前 Hover/Pressed 过渡色 → `FillRoundedRect`（圆角为 `0` 时退化为 `FillRect`）→ 水波纹（在圆角裁剪内）→ 描边（`border.a > 0 && thickness > 0` 才画）→ 标签（水平居中、垂直居中）。

## 八、事件

| 事件 | 类型 | 签名 | 触发时机 | 注册方式 |
|---|---|---|---|---|
| `OnClick` | `CallbackProperty<void(UIElement*)>` | `void(UIElement* sender)` | 鼠标释放后的点击，或空格 / 回车键 | `btn->OnClick = fn;` 或 `btn->OnClick.Connect(fn)` |
| `OnMouseDownEvent()` | `Event<UIElement*, Point>&` | `void(UIElement*, Point)` | 鼠标按下瞬间（早于水波纹） | `btn->OnMouseDownEvent().Add(fn)` |
| `Command` | `std::shared_ptr<Command>` | — | 与 `OnClick` 同批执行（`ExecuteBoundCommand()`） | 通过命令绑定接口设置 |

`CallbackProperty` 两种用法：

```cpp
btn->OnClick = [](UIElement* sender) { /* 单处理器，覆盖式 */ };
EventId id = btn->OnClick.Connect([](UIElement* sender) { /* 多订阅 */ });
```

> **坑**：`OnClick` 定义在 `UIElement` 上，所有元素都有它，但只有 `Button` 一类控件会主动触发。
> 在 `TextBlock`、`Panel` 上写 `OnClick` 会编译通过却永不触发——详见 [03 · DSL 与控件速查](../CUI.Core/03-DSL与控件速查.html) 的"静默降级"。

## 九、示例

### 9.1 DSL 最简写法

```cpp
#include "CUI.h"

auto btn = ElevatedButton("保存")
               .Width(120.0f)
               .OnClick([](UIElement*) {
                   // 提交逻辑
               })
               .Build();
```

### 9.2 代码式创建 + 多订阅

```cpp
auto btn = std::make_shared<Button>("删除");
btn->SetWidth(100.0f);
btn->SetBackgroundToken(ThemeTokenId::SystemCritical); // 危险操作用红
btn->OnClick = [](UIElement*) { /* 主逻辑 */ };
btn->OnClick.Connect([](UIElement*) { /* 埋点 */ });
```

### 9.3 图标按钮

```cpp
auto btn = ElevatedButton("设置").Icon("⚙").Build();
// 图标与文字间距固定 6.0f；纯图标时不带间距，图标居中
```

### 9.4 改文本与禁用

```cpp
Borrow(btn).Text("处理中…");
btn->SetIsEnabled(false);   // 指针恢复默认、不再触发点击、无水波纹
```

### 9.5 计数器（完整可运行）

```cpp
#include "CUI.h"

int main() {
    auto label  = Text("Click count: 0").FontSize(24.0f);
    auto button = ElevatedButton("Click Me!")
                      .OnClick([label](UIElement*) mutable {
                          static int n = 0;
                          Borrow(label).Text("Click count: " + std::to_string(++n));
                      });
    auto root = Column(20, { label, button }).Align(Alignment::Center).Build();

    Window w;
    w.Fluent().Title("Button Demo").Size(400, 300).Root(root).Build().Show().Run();
}
```

## 十、主题与自定义外观

| 想改的东西 | 改哪个 |
|---|---|
| 常态背景 | `BackgroundToken(...)`（推荐）或 `Background(color)` |
| 悬停 / 按下反馈 | `HoverBackgroundToken(...)` / `PressedBackgroundToken(...)` |
| 描边与聚焦描边 | `BorderToken(...)` / `FocusedBorderToken(...)` |
| 文字颜色 | `ForegroundToken(...)` |
| 圆角 | `CornerRadius(...)`（设为 `0` 变直角） |
| 尺寸感 | `Padding(...)` + `FontSize(...)` |

自定义派生示例（只换水波纹颜色）：

```cpp
class DangerButton : public Button {
public:
    DangerButton(const std::string& t) : Button(t) {
        DSL::Borrow(this).BackgroundToken(ThemeTokenId::SystemCritical);
    }
    const char* GetClassName() const override { return "DangerButton"; }
};
```

## 十一、注意事项

1. **尺寸不自适应父容器**：`Measure` 忽略 `availableSize`，也不会换行。需要撑满请显式给 `Width` 或用 `Expanded`。
2. **`OnClick` 与 `Command` 都会执行**：按下时先 `ExecuteBoundCommand()` 再触发 `OnClick`，避免在两处写同样逻辑造成重复提交。
3. **禁用状态要显式设置**：`SetIsEnabled(false)` 只是停止交互，不会自动变灰——通常配合 `DisabledBackgroundToken` 一起改。
4. **动画可被全局关闭**：`AreAnimationsEnabled()` 为假时水波纹直接跳过，不要依赖水波纹来表达"点击已生效"，关键反馈请同步改 UI 状态。
5. **链式方法静默降级**：`ElevatedButton(...).CornerRadius(...)` 这类写法若方法不被支持会静默无效；`Button` 支持的完整列表见 [03 · DSL 与控件速查](../CUI.Core/03-DSL与控件速查.html)。
