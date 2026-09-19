---
layout: page
title: CheckBox（复选框）
parent: 控件参考
nav_order: 6
---

# CheckBox（复选框）

> 支持两态（选中 / 未选）与三态（含半选 `Indeterminate`）的复选控件。内置填充、勾选符号与半选横杠三套过渡动画，可绑定到 `Observable<bool>` 或 `Observable<CheckState>` 数据源。

## 一、控件定位

| 场景 | 是否适用 |
|---|---|
| 多选列表中的一项 | ✅ 首选 |
| 树节点的父子半选指示 | ✅ 开 `IsThreeState` |
| 同组内互斥单选 | ❌ 用 [RadioButton](RadioButton.html) |
| 设置项里的开 / 关 | ❌ 用 [ToggleSwitch](ToggleSwitch.html) |
| 工具条上的模式按钮 | ❌ 用 [ToggleButton](ToggleButton.html) |

## 二、类信息

| 项 | 值 |
|---|---|
| 头文件 | `ui/framework/controls/CheckBox.h` |
| 声明 | `class CheckBox : public Control` |
| 继承链 | `UIElement` → `Control` → `CheckBox` → [RadioButton](RadioButton.html) |
| 命名空间 | `CUI` |
| DSL 工厂 | `CheckboxTile(title, onChanged)` |
| 通用工厂 | `DSL::Control<CheckBox>(args...)` |
| Tab 焦点 | ✅（`AcceptsTabFocus()` 返回 `true`，`CheckBox.h:52`） |
| 鼠标指针 | 启用时 `IDC_HAND`，禁用时 `nullptr` |

### 状态枚举

```cpp
enum class CheckState { Unchecked, Checked, Indeterminate };
```

`PropertyValueTraits<CheckState>` 已特化，字符串 `"Unchecked"` / `"Checked"` / `"Indeterminate"` 可与枚举互转（`CheckBox.h:19-30`）——属性系统、样式、序列化里直接写字符串即可。

## 三、构造函数与默认值

```cpp
CheckBox();                                // 无文本
explicit CheckBox(const std::string& text); // 指定文本
```

成员默认值（源码 `CheckBox.h:106-110`）：

| 成员 | 默认值 | 含义 |
|---|---|---|
| `m_state` | `CheckState::Unchecked` | 当前状态 |
| `m_isThreeState` | `false` | 是否启用三态循环 |
| `m_fillAnim` / `m_checkAnim` / `m_indeterminateAnim` | `AnimatedScalar{}` | 方框填充、勾号、半选横杠三段动画 |

## 四、属性

### 4.1 自身属性

| 属性 | 类型 | 读 | 写 | 默认值 | 说明 |
|---|---|:-:|:-:|---|---|
| `State` | `CheckState` | `GetState()` / 代理 `State` | `SetState(CheckState)` / `State = CheckState::Checked` | `Unchecked` | 三态状态本体 |
| `IsChecked` | `bool` | `GetIsChecked()` / 代理 `IsChecked` | `SetIsChecked(bool)` / `IsChecked = true` / DSL `.Checked(bool)` | `false` | 二态视图：读时等价于 `State == Checked`；写时在 `Checked` / `Unchecked` 间切换 |
| `IsThreeState` | `bool` | `GetIsThreeState()` / 代理 `IsThreeState` | `SetIsThreeState(bool)` / `IsThreeState = true` | `false` | 开启后点击按 `Unchecked → Checked → Indeterminate` 循环 |
| `Checked`（绑定代理） | `PropertyRef<bool, PropertyId::ControlValue>` | `Checked.Get()` | `Checked = v`、`Checked.Bind(observable)` | — | 二态双向绑定代理 |
| `State`（绑定代理） | `PropertyRef<CheckState, PropertyId::CheckState>` | `State.Get()` | `State = s`、`State.Bind(observable)` | — | 三态双向绑定代理 |

### 4.2 继承自 `Control` / `UIElement` 的常用属性

| 属性 | 说明 |
|---|---|
| `Text` | 方框右侧的文字 |
| `ForegroundToken` / `BackgroundToken` | 文字与方框颜色 |
| `Width` / `Height` / `MinHeight` | 尺寸 |
| `IsEnabled` | 禁用后不响应鼠标与键盘，指针恢复默认 |
| `Margin` / `Padding` | 参与测量与排布 |

完整列表见 [04 · 属性与绑定](../CUI.Core/04-属性与绑定.html)。

## 五、方法

| 方法 | 说明 |
|---|---|
| `const char* GetClassName() const override` | 返回 `"CheckBox"` |
| `GetProperty / HasProperty / SetProperty` | `PropertyId::ControlValue` / `CheckState` / `IsThreeState` 直通 |
| `HCURSOR GetCursor() const override` | 启用时 `IDC_HAND` |
| `Size Measure(Size) override` / `void OnRender(GraphicsContext&) override` | 测量"方框 + 文字"，绘制方框、勾号 / 半选横杠与文字 |
| `void OnMouseDown(Point pt) override` | 点击循环状态（`IsThreeState` 时走三态） |
| `bool OnKeyDown(int vkCode) override` | 空格 / 回车切换 |
| `bool AcceptsTabFocus() const override` | `true` |
| `OnAnimationTick` / `HasSelfAnimation` | 驱动三段过渡动画 |
| `CheckState GetState() const` / `void SetState(CheckState)` | 三态读写 |
| `bool GetIsChecked() const` / `void SetIsChecked(bool)` | 二态读写 |
| `void Bind(const std::shared_ptr<Observable<bool>>&)` | 绑定到布尔数据源 |
| `void Bind(const std::shared_ptr<Observable<CheckState>>&, bool twoWay = true)` | 绑定到三态数据源，默认双向 |
| `void Unbind()` | 解除绑定 |
| `bool IsUpdatingFromBinding() const` | 当前这次变更是否来自绑定（用于在回调里避免回写循环） |
| `Event<CheckBox*, CheckState>& OnCheckStateChanged()` | 状态变化事件访问器 |

`protected: void CycleState()` —— 状态循环的推进逻辑，派生类（如 `RadioButton`）可复用。

## 六、运行时行为

- **点击循环**（`CycleState`）：两态时 `Unchecked ↔ Checked`；`IsThreeState == true` 时按 `Unchecked → Checked → Indeterminate → Unchecked` 循环。
- **三段动画**：`m_fillAnim`（方框填充）、`m_checkAnim`（勾号绘制进度）、`m_indeterminateAnim`（半选横杠）分别过渡，`HasSelfAnimation()` 在任意一段未收敛时返回 `true`。
- **键盘**：空格 / 回车切换并消费按键。
- **绑定**：`Bind(Observable<bool>)` 会自动在 `bool` 与 `CheckState` 间转换；`Bind(Observable<CheckState>, twoWay)` 可关闭反向写回。回调里用 `IsUpdatingFromBinding()` 判断是否来自数据源，避免"回调写回 → 再触发回调"的死循环。
- **禁用**：不响应鼠标与键盘，指针恢复默认；不会自动变灰，需自行配合颜色 Token。

## 七、事件

| 事件 | 类型 | 回调签名 | 触发时机 | 注册方式 |
|---|---|---|---|---|
| `OnCheckStateChanged()` | `Event<CheckBox*, CheckState>&` | `void(CheckBox* sender, CheckState state)` | 状态因用户点击、键盘或 `SetState` / `SetIsChecked` 变化时 | `cb->OnCheckStateChanged().Connect(fn);`；DSL：`CheckboxTile(title, onChanged)` 或 `.OnCheckStateChanged(fn)` |
| `OnClick` | `CallbackProperty<void(UIElement*)>` | `void(UIElement*)` | 继承自 `UIElement`，**CheckBox 不主动触发** | `cb->OnClick = fn;` |

## 八、示例

### 8.1 DSL 最简写法

```cpp
#include "CUI.h"

auto cb = CheckboxTile("记住我的选择", [](CheckBox* sender, CheckState s) {
    // s == CheckState::Checked 时记住
}).Build();
```

### 8.2 代码式创建 + 三态

```cpp
auto cb = std::make_shared<CheckBox>("全选");
cb->SetIsThreeState(true);
cb->SetState(CheckState::Indeterminate);   // 子节点部分选中
cb->OnCheckStateChanged().Connect([](CheckBox* sender, CheckState s) {
    // 向下传播到子节点
});
```

### 8.3 绑定到数据源

```cpp
auto enabled = std::make_shared<Observable<bool>>(false);
auto cb = std::make_shared<CheckBox>("启用日志");
cb->Bind(enabled);          // 双向：改 enabled 会同步勾选框
```

### 8.4 完整可运行：多选清单

```cpp
#include "CUI.h"

int main() {
    auto label = Text("已选 0 项").FontSize(16.0f);

    std::vector<std::shared_ptr<CheckBox>> boxes;
    std::vector<std::string> names = {"文档", "图片", "视频", "音乐"};
    for (auto& n : names) boxes.push_back(CheckboxTile(n).Build());

    auto refresh = [label, boxes]() {
        int n = 0;
        for (auto& b : boxes) if (b->GetIsChecked()) ++n;
        Borrow(label).Text("已选 " + std::to_string(n) + " 项");
    };
    for (auto& b : boxes) b->OnCheckStateChanged().Connect([refresh](CheckBox*, CheckState) { refresh(); });

    auto root = Column(10, {
        label,
        Column(6, { boxes[0], boxes[1], boxes[2], boxes[3] })
    }).Align(Alignment::Center).Build();

    Window w;
    w.Fluent().Title("CheckBox Demo").Size(420, 280).Root(root).Build().Show().Run();
}
```

## 九、主题与自定义外观

| 想改的东西 | 改哪个 |
|---|---|
| 方框与文字颜色 | `BackgroundToken(...)` / `ForegroundToken(...)` |
| 文字 | `Text(...)`（构造函数实参或 `SetText`） |
| 尺寸 | `Width(...)` / `Height(...)`；方框本身尺寸由 `OnRender` 内部确定 |
| 禁用视觉 | `SetIsEnabled(false)` + 自行调整颜色 Token |
| 勾号 / 半选符号形状 | ❌ 由 `OnRender` 内部绘制，无公开属性 |

## 十、注意事项

1. **`IsChecked` 是二态投影**：它读不出 `Indeterminate`——半选时 `GetIsChecked()` 也返回 `false`。要区分三态请用 `GetState()`。
2. **开启 `IsThreeState` 后循环变三段**：如果用户不期望出现半选，别开。
3. **绑定回调要防循环**：在 `OnCheckStateChanged` 里写数据源时，用 `IsUpdatingFromBinding()` 判断来源，或让绑定单向（`twoWay = false`）。
4. **禁用不会自动变灰**，需自行配合 Token。
5. **`RadioButton` 继承 `CheckBox`**：修改 `CheckBox` 的行为会影响单选按钮。
6. **链式方法静默降级**：见 [03 · DSL 与控件速查](../CUI.Core/03-DSL与控件速查.html)。
