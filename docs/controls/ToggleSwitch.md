---
layout: page
title: ToggleSwitch（开关）
parent: 控件参考
nav_order: 8
---

# ToggleSwitch（开关）

> 手机设置里那种"滑道 + 圆钮"的开关控件。左侧是开关本体，右侧可带一段说明文字；钮在两极之间位移带弹性过渡动画，聚焦时绘制虚线焦点框。

## 一、控件定位

| 场景 | 是否适用 |
|---|---|
| 设置项里的开 / 关（启用通知、深色模式） | ✅ 首选 |
| 需要随附一段说明文字的开关 | ✅ `Header` |
| 工具条上的模式按钮（加粗、对齐） | ❌ 用 [ToggleButton](ToggleButton.html) |
| 需要三态 / 多选 | ❌ 用 [CheckBox](CheckBox.html) |
| 只是触发一次动作 | ❌ 用 [Button](Button.html) |

## 二、类信息

| 项 | 值 |
|---|---|
| 头文件 | `ui/framework/controls/ToggleSwitch.h` |
| 声明 | `class ToggleSwitch : public Control` |
| 继承链 | `UIElement` → `Control` → `ToggleSwitch` |
| 命名空间 | `CUI` |
| DSL 工厂 | `ToggleSwitchWidget()` / `ToggleSwitchTile(header, isOn)` |
| 通用工厂 | `DSL::Control<ToggleSwitch>()` |
| Tab 焦点 | ✅（`AcceptsTabFocus()` 返回 `true`，`ToggleSwitch.h:25`） |
| 鼠标指针 | 启用时 `IDC_HAND`，禁用时 `nullptr` |

## 三、构造函数与默认值

```cpp
ToggleSwitch();   // 唯一构造函数，无参
```

成员默认值（源码 `ToggleSwitch.h:47-50`）：

| 成员 | 默认值 | 含义 |
|---|---|---|
| `m_isOn` | `false` | 开关状态（`true` 为开） |
| `m_header` | `"开关 (ToggleSwitch)"` | 右侧说明文字，建议显式设置 |
| `m_knobPosAnim` | `AnimatedScalar{}` | 滑块位置过渡：`0.0f` = Off，`1.0f` = On |

## 四、属性

### 4.1 自身属性

| 属性 | 类型 | 读 | 写 | 默认值 | 说明 |
|---|---|:-:|:-:|---|---|
| `IsOn` | `bool` | `GetIsOn()` / 代理 `IsOn` | `SetIsOn(bool)` / `IsOn = true` / DSL `.Checked(bool)` | `false` | 开关状态 |
| `Header` | `std::string` | `GetHeader()` | `SetHeader()` / DSL `.Header(str)` | `"开关 (ToggleSwitch)"` | 滑道右侧的说明文字；赋相同值直接返回 |
| `IsOn`（绑定代理） | `PropertyRef<bool, PropertyId::IsOn>` | `IsOn.Get()` / 隐式转 `bool` | `IsOn = v`、`IsOn.Bind(state)` | — | 响应式双向绑定代理 |

`PropertyId` 直通：`IsOn` 可用 `GetProperty / SetProperty / HasProperty` 访问；`Header` 变化会 `NotifyFieldChanged(PropertyId::Header, …)`（`ToggleSwitch.h:40`）。

### 4.2 继承自 `Control` / `UIElement` 的常用属性

完整列表见 [04 · 属性与绑定](../CUI.Core/04-属性与绑定.html)。常用：

| 属性 | 说明 |
|---|---|
| `BackgroundToken` / `ForegroundToken` | 滑道与文字颜色 |
| `Width` / `Height` / `MinWidth` | 尺寸；`Measure` 会按滑道 + 说明文字算总宽 |
| `Margin` / `Padding` | 参与测量与排布 |
| `IsEnabled` | 禁用后鼠标与键盘都不切换，指针恢复默认 |
| `Opacity` / `Visibility` | 常规视觉控制 |

## 五、方法

| 方法 | 说明 |
|---|---|
| `const char* GetClassName() const override` | 返回 `"ToggleSwitch"` |
| `GetProperty / HasProperty / SetProperty` | `PropertyId::IsOn` 直通 |
| `HCURSOR GetCursor() const override` | 启用时 `IDC_HAND` |
| `Size Measure(Size) override` | 计算"开关本体 + 右侧说明文字"的总尺寸 |
| `void OnRender(GraphicsContext&) override` | 绘制滑道背景、滑块钮，以及聚焦时的虚线焦点框 |
| `void OnMouseUp(Point pt) override` | **抬起时切换状态**并派发 `OnToggled` |
| `bool OnKeyDown(int vkCode) override` | 空格 / 回车切换状态 |
| `bool AcceptsTabFocus() const override` | `true` |
| `void OnFocus() override` / `void OnBlur() override` | 标记重绘以显示 / 取消虚线焦点框 |
| `bool OnAnimationTick() override` / `bool HasSelfAnimation() const override` | 驱动滑块位移动画 |
| `bool GetIsOn() const` / `void SetIsOn(bool)` | 状态读写 |
| `GetHeader()` / `SetHeader()` | 说明文字读写 |
| `Event<ToggleSwitch*, bool>& OnToggled()` | 状态变化事件访问器 |

## 六、运行时行为

- **切换时机在鼠标抬起**（`OnMouseUp`），与 `ToggleButton` 一致；按下不切，抬起才切。
- **键盘**：`VK_SPACE` / `VK_RETURN` 切换并消费按键。
- **动画**：`m_knobPosAnim` 在 `0.0f`（Off）与 `1.0f`（On）之间做弹性过渡；`HasSelfAnimation()` 在动画未收敛时返回 `true` 以维持逐帧调度。全局关闭动画时直接跳到终值。
- **焦点反馈**：`OnFocus` / `OnBlur` 只是置脏重绘，绘制时额外画一圈虚线焦点框——这与 `Button` 用 `FocusedBorderToken` 画实线框的做法不同。
- **测量**：`Measure` 把滑道尺寸与 `Header` 文字宽度相加得到总宽，因此**说明文字越长控件越宽**；不给 `Width` 时不会自动拉伸。

## 七、事件

| 事件 | 类型 | 回调签名 | 触发时机 | 注册方式 |
|---|---|---|---|---|
| `OnToggled()` | `Event<ToggleSwitch*, bool>&` | `void(ToggleSwitch* sender, bool isOn)` | 用户点击 / 键盘切换，以及 `SetIsOn` 导致状态变化时 | `ts->OnToggled().Connect(fn);` |
| `OnClick` | `CallbackProperty<void(UIElement*)>` | `void(UIElement*)` | 继承自 `UIElement`，**ToggleSwitch 不主动触发** | `ts->OnClick = fn;` |

## 八、示例

### 8.1 DSL 最简写法

```cpp
#include "CUI.h"

auto sw = ToggleSwitchTile("启用深色模式", true).Build();
```

### 8.2 代码式创建 + 订阅

```cpp
auto sw = std::make_shared<ToggleSwitch>();
sw->SetHeader("自动更新");
sw->SetIsOn(true);
sw->OnToggled().Connect([](ToggleSwitch* sender, bool on) {
    // 持久化 on 到配置
});
```

### 8.3 响应式绑定

```cpp
CUI::State<bool> darkMode{false};
auto sw = std::make_shared<ToggleSwitch>();
sw->SetHeader("深色模式");
sw->IsOn.Bind(darkMode);
```

### 8.4 完整可运行：设置面板

```cpp
#include "CUI.h"

int main() {
    auto tip = Text("全部关闭").FontSize(14.0f);

    auto s1 = ToggleSwitchTile("启用通知", false).Build();
    auto s2 = ToggleSwitchTile("自动保存", true).Build();

    int onCount = (s1->GetIsOn() ? 1 : 0) + (s2->GetIsOn() ? 1 : 0);
    Borrow(tip).Text("已开启 " + std::to_string(onCount) + " 项");

    auto handler = [tip, s1, s2](ToggleSwitch*, bool) {
        int n = (s1->GetIsOn() ? 1 : 0) + (s2->GetIsOn() ? 1 : 0);
        Borrow(tip).Text("已开启 " + std::to_string(n) + " 项");
    };
    s1->OnToggled().Connect(handler);
    s2->OnToggled().Connect(handler);

    auto root = Column(12, { tip, s1, s2 }).Align(Alignment::Center).Build();

    Window w;
    w.Fluent().Title("ToggleSwitch Demo").Size(420, 260).Root(root).Build().Show().Run();
}
```

## 九、主题与自定义外观

| 想改的东西 | 改哪个 |
|---|---|
| 说明文字 | `SetHeader(...)` / DSL `.Header(...)` |
| 滑道与文字颜色 | `BackgroundToken(...)` / `ForegroundToken(...)` |
| 聚焦提示 | 由 `OnFocus` 内部绘制的虚线框，随 `FocusedBorderToken` 体系走 |
| 尺寸 | `Width(...)` / `Height(...)`；宽度默认由滑道 + 文字决定 |
| 滑块颜色 / 滑道圆角 | ❌ 由 `OnRender` 内部实现直接绘制，无公开属性 |

## 十、注意事项

1. **默认 `Header` 是调试串** `"开关 (ToggleSwitch)"`，正式界面务必显式设置。
2. **状态在抬起时切换**，按下不切。
3. **说明文字影响宽度**：`Measure` 把文字宽度算进总宽，长文案会撑宽控件。
4. **没有 `Content`**：右侧只能是纯文本 `Header`，不能塞任意控件。
5. **禁用只停止交互**：需配合颜色 Token 表达禁用视觉。
6. **链式方法静默降级**：见 [03 · DSL 与控件速查](../CUI.Core/03-DSL与控件速查.html)。
