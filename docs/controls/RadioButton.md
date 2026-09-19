---
layout: page
title: RadioButton（单选按钮）
parent: 控件参考
nav_order: 7
---

# RadioButton（单选按钮）

> 继承 [CheckBox](CheckBox.html)，把方框改成圆形并加上"同组互斥"语义：同一父级树下 `GroupName` 相同的按钮，选中一个会自动取消其它按钮的选中。

## 一、控件定位

| 场景 | 是否适用 |
|---|---|
| 一组选项中只能选一个（性别、模式、尺寸） | ✅ 首选 |
| 可以多选 | ❌ 用 [CheckBox](CheckBox.html) |
| 设置项的开 / 关 | ❌ 用 [ToggleSwitch](ToggleSwitch.html) |
| 一行选项太多、想做成横向分段 | ❌ 用 `SegmentedControl` |

## 二、类信息

| 项 | 值 |
|---|---|
| 头文件 | `ui/framework/controls/RadioButton.h` |
| 声明 | `class RadioButton : public CheckBox` |
| 继承链 | `UIElement` → `Control` → `CheckBox` → `RadioButton` |
| 命名空间 | `CUI` |
| DSL 工厂 | `RadioButtonTile(text, group)` |
| 通用工厂 | `DSL::Control<RadioButton>(args...)` |
| Tab 焦点 | ✅（继承 `CheckBox::AcceptsTabFocus()` → `true`） |
| 鼠标指针 | 启用时 `IDC_HAND`（继承 `CheckBox`） |

## 三、构造函数与默认值

```cpp
RadioButton();                                // 无文本
explicit RadioButton(const std::string& text); // 指定文本
```

成员默认值（源码 `RadioButton.h:42-43`）：

| 成员 | 默认值 | 含义 |
|---|---|---|
| `m_groupName` | `"DefaultGroup"` | 互斥分组名 |
| `m_selectionAnim` | `AnimatedScalar{}` | 选中圆点的缩放淡入动画 |

其余默认值（`m_state = Unchecked`、`m_isThreeState = false` 等）来自 `CheckBox`，见 [CheckBox](CheckBox.html) 第三节。

## 四、属性

### 4.1 自身属性

| 属性 | 类型 | 读 | 写 | 默认值 | 说明 |
|---|---|:-:|:-:|---|---|
| `GroupName` | `std::string` | `GetGroupName()` | `SetGroupName()` / DSL `.GroupName(str)`（`RadioButtonTile(text, group)` 的第二个实参） | `"DefaultGroup"` | 互斥分组标识；赋相同值直接返回 |

### 4.2 继承自 `CheckBox` 的属性

| 属性 | 说明 |
|---|---|
| `State` / `IsChecked` | 状态读写（见 [CheckBox](CheckBox.html) 4.1）。单选按钮上通常用 `IsChecked` |
| `IsThreeState` | ⚠️ 单选按钮语义下不应开启，开启会引入半选态 |
| `Text` | 圆形右侧文字 |
| `ForegroundToken` / `BackgroundToken` | 颜色 |

## 五、方法

| 方法 | 说明 |
|---|---|
| `const char* GetClassName() const override` | 返回 `"RadioButton"` |
| `GetProperty / HasProperty / SetProperty` | 在 `CheckBox` 基础上增加 `GroupName` |
| `void OnRender(GraphicsContext&) override` | 绘制圆形边框、背景与中心选中圆点（覆盖 `CheckBox` 的方框绘制） |
| `void OnMouseDown(Point pt) override` | 点击选中并触发同组互斥 |
| `void OnMouseUp(Point pt) override` | 抬起结束交互 |
| `bool OnKeyDown(int vkCode) override` | 空格 / 回车选中 |
| `OnAnimationTick` / `HasSelfAnimation` | 驱动选中圆点的缩放淡入动画 |
| `GetGroupName()` / `SetGroupName()` | 分组读写 |

私有：`SetChecked(bool)`（强制改状态，不走互斥）、`UncheckSiblingsInGroup()`（把同父级树下、同 `GroupName` 的其它单选钮设为未选中）。

## 六、运行时行为

- **互斥范围**是"同一父级树 + 相同 `GroupName`"：`UncheckSiblingsInGroup()` 只在当前父节点的子树里查找，因此**两个不同容器里的同名组互不干扰**。
- **默认组名是 `"DefaultGroup"`**：不显式设 `GroupName` 时，同一容器下所有单选按钮都属同一组，恰好符合"一个容器一组"的常见用法。
- **选中圆点动画**：`m_selectionAnim` 控制中心圆点的缩放淡入；`HasSelfAnimation()` 在动画未收敛时返回 `true`。
- **点击流程**走 `OnMouseDown`（与 `ToggleButton` 在 `OnMouseUp` 切换不同），配合 `OnMouseUp` 完成交互。
- **互斥是单向的**：选中 A 会取消 B，但取消 A（把 A 设为未选中）不会自动选中任何其它项——这与原生单选组"始终有一项选中"的语义不同。

## 七、事件

| 事件 | 类型 | 回调签名 | 触发时机 | 注册方式 |
|---|---|---|---|---|
| `OnCheckStateChanged()` | `Event<CheckBox*, CheckState>&` | `void(CheckBox* sender, CheckState state)` | 继承自 `CheckBox`；本按钮被选中或被互斥取消时都会触发 | `rb->OnCheckStateChanged().Connect(fn);` |
| `OnClick` | `CallbackProperty<void(UIElement*)>` | `void(UIElement*)` | 继承自 `UIElement`，**不主动触发** | `rb->OnClick = fn;` |

回调参数里的 `sender` 静态类型是 `CheckBox*`，需要时 `static_cast<RadioButton*>(sender)`。

> **坑**：互斥取消时同样会触发 `OnCheckStateChanged`，回调里要用 `state == CheckState::Checked` 过滤，否则一次点击会收到两条通知（旧项 Unchecked + 新项 Checked）。

## 八、示例

### 8.1 DSL 最简写法

```cpp
#include "CUI.h"

auto r1 = RadioButtonTile("小杯", "size").Build();
auto r2 = RadioButtonTile("中杯", "size").Build();
auto r3 = RadioButtonTile("大杯", "size").Build();
```

### 8.2 代码式创建 + 订阅

```cpp
auto rb = std::make_shared<RadioButton>("深色模式");
rb->SetGroupName("theme");
rb->SetIsChecked(true);
rb->OnCheckStateChanged().Connect([](CheckBox* sender, CheckState s) {
    if (s != CheckState::Checked) return;   // 过滤互斥导致的取消通知
    // 应用主题
});
```

### 8.3 完整可运行：主题选择

```cpp
#include "CUI.h"

int main() {
    auto label = Text("当前：浅色").FontSize(16.0f);

    auto light = RadioButtonTile("浅色", "theme").Build();
    auto dark  = RadioButtonTile("深色", "theme").Build();
    light->SetIsChecked(true);

    auto onChanged = [label](CheckBox* sender, CheckState s) {
        if (s != CheckState::Checked) return;
        auto* rb = static_cast<RadioButton*>(sender);
        Borrow(label).Text(std::string("当前：") + rb->GetText());
    };
    light->OnCheckStateChanged().Connect(onChanged);
    dark->OnCheckStateChanged().Connect(onChanged);

    auto root = Column(10, { label, light, dark }).Align(Alignment::Center).Build();

    Window w;
    w.Fluent().Title("RadioButton Demo").Size(420, 260).Root(root).Build().Show().Run();
}
```

### 8.4 两组互不干扰

```cpp
auto a1 = RadioButtonTile("A1", "groupA").Build();
auto a2 = RadioButtonTile("A2", "groupA").Build();
auto b1 = RadioButtonTile("B1", "groupB").Build();   // 不同组名，互斥范围不同
```

## 九、主题与自定义外观

| 想改的东西 | 改哪个 |
|---|---|
| 文字 | `Text(...)`（构造实参或 `SetText`） |
| 圆形与文字颜色 | `BackgroundToken(...)` / `ForegroundToken(...)` |
| 分组 | `SetGroupName(...)` / `RadioButtonTile(text, group)` |
| 尺寸 | `Width(...)` / `Height(...)` |
| 圆形半径、中心圆点大小 | ❌ 由 `OnRender` 内部确定，无公开属性 |

## 十、注意事项

1. **互斥只在同父级子树内生效**：跨容器的同名组不会互斥。
2. **默认组名 `"DefaultGroup"`**：忘记设分组时，同一容器下所有单选按钮会意外互斥——这通常正是想要的，但多组场景必须显式命名。
3. **取消不会补选**：把当前项设为未选中后，组内可能没有任何选中项。
4. **`IsThreeState` 不应开启**：它是 `CheckBox` 遗留属性，开启会让单选按钮出现半选态。
5. **回调会收到取消通知**：用 `s == CheckState::Checked` 过滤。
6. **继承链副作用**：`CheckBox` 的行为变化会直接影响单选按钮。
