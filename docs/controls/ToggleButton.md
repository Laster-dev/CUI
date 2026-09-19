---
layout: page
title: ToggleButton（开关按钮）
parent: 控件参考
nav_order: 2
---

# ToggleButton（开关按钮）

> 可保持按下 / 弹起两种状态的按钮。它完全继承 [Button](Button.html) 的外观与水波纹反馈，只是把"点一下触发一次动作"改成"点一下切换状态并保持"。

## 一、控件定位

| 场景 | 是否适用 |
|---|---|
| 工具条上的"加粗 / 斜体"这类可长期保持的模式开关 | ✅ 首选 |
| 播放 / 暂停这类状态互斥的按钮 | ✅ 首选 |
| 点一下执行一次动作（保存、提交） | ❌ 用 [Button](Button.html) |
| 表达"开 / 关"语义的设置项 | ❌ 用 [ToggleSwitch](ToggleSwitch.html) |
| 需要三态（含半选） | ❌ 用 [CheckBox](CheckBox.html) |

## 二、类信息

| 项 | 值 |
|---|---|
| 头文件 | `ui/framework/controls/ToggleButton.h` |
| 声明 | `class ToggleButton : public Button` |
| 继承链 | `UIElement` → `Control` → `Button` → `ToggleButton` |
| 命名空间 | `CUI` |
| DSL 工厂 | `ToggleButtonWidget(text)` |
| 通用工厂 | `DSL::Control<ToggleButton>(args...)` |
| Tab 焦点 | ✅（继承 `Button::AcceptsTabFocus()` → `true`） |
| 鼠标指针 | 启用时 `IDC_HAND`（继承 `Button`） |

## 三、构造函数与默认值

```cpp
ToggleButton();                                // 文本沿用 Button 的 "Button"
explicit ToggleButton(const std::string& text); // 指定文本
```

成员默认值（源码 `ToggleButton.h:43`）：

| 成员 | 默认值 | 含义 |
|---|---|---|
| `m_isChecked` | `false` | 当前是否处于按下（选中）态 |

其余外观默认值（字号 `12`、字体 `微软雅黑`、Padding `(8,4,8,4)`、圆角 `4`、Accent 系列 Token）全部来自 `Button` 构造函数，见 [Button](Button.html) 第三节。选中态的配色由私有方法 `ApplyCheckedChrome()` 在状态变化时重新写入。

## 四、属性

### 4.1 自身属性

| 属性 | 类型 | 读 | 写 | 默认值 | 说明 |
|---|---|:-:|:-:|---|---|
| `IsChecked` | `bool` | `GetIsChecked()` / 代理 `IsChecked` | `SetIsChecked(bool)` / `SetChecked(bool)` / `IsChecked = true` / DSL `.Checked(bool)` | `false` | 是否处于按下态 |
| `IsOn` | `PropertyRef<bool, PropertyId::IsOn>` | `IsOn.Get()` / 隐式转 `bool` | `IsOn = v`、`IsOn.Bind(state)` | `false` | 与 `IsChecked` 同源的响应式绑定代理，可绑定到 `State<bool>` / `Observable<bool>` |

`PropertyId` 直通：`IsOn` 可用 `GetProperty / SetProperty / HasProperty` 访问（`ToggleButton.h:13-15`）。

### 4.2 继承自 `Button` / `Control` / `UIElement` 的常用属性

见 [Button](Button.html) 第四节。开关按钮上最常改的：

| 属性 | 说明 |
|---|---|
| `Text` / `Icon` | 按钮文字与图标 |
| `BackgroundToken` / `HoverBackgroundToken` / `PressedBackgroundToken` | 三态底色 |
| `ForegroundToken` | 文字颜色 |
| `CornerRadius` / `Padding` / `FontSize` | 外观微调 |
| `Width` / `Height` | 显式尺寸 |
| `IsEnabled` | 禁用后不响应鼠标与键盘 |

## 五、方法

| 方法 | 说明 |
|---|---|
| `const char* GetClassName() const override` | 返回 `"ToggleButton"` |
| `GetProperty / HasProperty / SetProperty` | `PropertyId::IsOn` 的属性系统直通 |
| `virtual void OnMouseUp(Point pt) override` | **在抬起时切换状态**（不是在按下时），随后派发 `OnToggled` |
| `virtual bool OnKeyDown(int vkCode) override` | 空格 / 回车切换状态并返回 `true` |
| `bool GetIsChecked() const` / `void SetIsChecked(bool)` | 状态读写；`SetIsChecked` 会刷新选中态外观 |
| `void SetChecked(bool)` | `SetIsChecked` 的同义写法 |
| `Event<ToggleButton*, bool>& OnToggled()` | 状态变化事件访问器 |

私有：`ApplyCheckedChrome()`（按当前状态写入配色）、`ToggleFromUser()`（用户交互引发的切换，内部统一走它以保证事件与外观同步）。

## 六、运行时行为

- **状态在鼠标"抬起"时翻转**（`OnMouseUp`），这与 `Button` 在抬起时触发 `OnClick` 的时机一致；因此点击一次 = 一次状态翻转 + 一次 `OnToggled`。
- **键盘**：`VK_SPACE` / `VK_RETURN` 翻转状态并消费按键。
- **外观**：`m_isChecked` 变化后 `ApplyCheckedChrome()` 重新设置背景 / 前景等配色，选中与未选中是两套不同的 chrome；因此**不要**在外部同时手改 `BackgroundToken`，否则会被下一次状态切换覆盖。
- **水波纹**：完全沿用 `Button`，点击仍有 Ripple 反馈。`HasSelfAnimation()` 同样包含 Ripple 状态。
- **禁用**：`IsEnabled() == false` 时鼠标与键盘都不切换状态，指针恢复默认。

## 七、事件

| 事件 | 类型 | 回调签名 | 触发时机 | 注册方式 |
|---|---|---|---|---|
| `OnToggled()` | `Event<ToggleButton*, bool>&` | `void(ToggleButton* sender, bool isChecked)` | 状态由用户交互或 `SetIsChecked` 改变时 | `tb->OnToggled().Connect(fn);` |
| `OnClick` | `CallbackProperty<void(UIElement*)>` | `void(UIElement*)` | 继承自 `UIElement`；`Button` 的键盘路径会触发，**鼠标路径由 `OnMouseUp` 承载** | `tb->OnClick = fn;` |

> **坑**：`OnClick` 与 `OnToggled` 语义不同——前者是"被点了"，后者是"状态变了"。程序化调用 `SetIsChecked()` 会触发 `OnToggled`，但不会触发 `OnClick`。

## 八、示例

### 8.1 DSL 最简写法

```cpp
#include "CUI.h"

auto bold = ToggleButtonWidget("B")
                .FontWeight(FontWeight::Bold)
                .Width(36.0f)
                .Build();
```

### 8.2 代码式创建 + 订阅状态

```cpp
auto play = std::make_shared<ToggleButton>("播放");
play->SetIsChecked(false);
play->OnToggled().Connect([](ToggleButton* sender, bool on) {
    // on == true 时开始播放，false 时暂停
});
```

### 8.3 响应式双向绑定

```cpp
CUI::State<bool> muted{false};
auto btn = ToggleButtonWidget("静音").Build();
btn->IsOn.Bind(muted);   // 改 muted 会同步按钮，点按钮也会写回 muted
```

### 8.4 完整可运行：播放 / 暂停

```cpp
#include "CUI.h"

int main() {
    auto label = Text("已暂停").FontSize(18.0f);
    auto play  = ToggleButtonWidget("播放").Width(120.0f).Build();

    play->OnToggled().Connect([label](ToggleButton* sender, bool on) {
        Borrow(label).Text(on ? "正在播放" : "已暂停");
        Borrow(sender).Text(on ? "暂停" : "播放");
    });

    auto root = Column(16, { label, play }).Align(Alignment::Center).Build();

    Window w;
    w.Fluent().Title("ToggleButton Demo").Size(420, 240).Root(root).Build().Show().Run();
}
```

## 九、主题与自定义外观

| 想改的东西 | 改哪个 |
|---|---|
| 常态 / 悬停 / 按下底色 | `BackgroundToken(...)` / `HoverBackgroundToken(...)` / `PressedBackgroundToken(...)` |
| 文字颜色 | `ForegroundToken(...)` |
| 圆角、内边距、字号 | `CornerRadius(...)` / `Padding(...)` / `FontSize(...)` |
| 选中态配色 | ❌ 由 `ApplyCheckedChrome()` 内部写入，改上面的 Token 会在下一次状态切换时被覆盖；需要完全自定义请派生并覆写该方法（它是 private，只能通过改源码或整体自绘实现） |

## 十、注意事项

1. **状态在抬起时翻转**：按下不切、抬起才切，拖出去再抬起不会触发。
2. **不要和外部改色混用**：`ApplyCheckedChrome()` 会在每次状态变化时重写配色，手动 `Background(...)` 会被冲掉。
3. **`OnClick` 与 `OnToggled` 不等价**：程序改状态只触发 `OnToggled`。
4. **继承链副作用**：`ToggleButton` 继承 `Button`，`Button` 出厂默认值的任何改动都会影响它。
5. **链式方法静默降级**：完整清单见 [03 · DSL 与控件速查](../CUI.Core/03-DSL与控件速查.html)。
