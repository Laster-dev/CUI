---
layout: page
title: SplitButton（拆分按钮）
parent: 控件参考
nav_order: 4
---

# SplitButton（拆分按钮）

> 继承 [DropDownButton](DropDownButton.html)，用一条竖线把按钮分成左右两半：**左侧主区域**触发普通 Click，**右侧箭头区域**弹出下拉菜单。适合"执行默认操作 + 可选变体"的场景（如"保存 / 另存为 / 导出"）。

## 一、控件定位

| 场景 | 是否适用 |
|---|---|
| 有一个默认动作，同时提供若干变体（保存 ▾） | ✅ 首选 |
| 点任意位置都弹菜单选择 | ❌ 用 [DropDownButton](DropDownButton.html) |
| 只有一个动作 | ❌ 用 [Button](Button.html) |
| 需要保持按下 / 弹起状态 | ❌ 用 [ToggleButton](ToggleButton.html) |

## 二、类信息

| 项 | 值 |
|---|---|
| 头文件 | `ui/framework/controls/SplitButton.h` |
| 声明 | `class SplitButton : public DropDownButton` |
| 继承链 | `UIElement` → `Control` → `Button` → `DropDownButton` → `SplitButton` |
| 命名空间 | `CUI` |
| DSL 工厂 | `SplitButtonWidget(text)` |
| 通用工厂 | `DSL::Control<SplitButton>(args...)` |
| Tab 焦点 | ✅（继承 `Button::AcceptsTabFocus()` → `true`） |
| 鼠标指针 | 启用时手型（继承 `Button`） |

## 三、构造函数与默认值

```cpp
SplitButton();
explicit SplitButton(const std::string& text);
```

成员默认值（源码 `SplitButton.h:30`）：

| 成员 | 默认值 | 含义 |
|---|---|---|
| `m_pressInChevron` | `false` | 本次按下是否落在右侧箭头区 |

布局常量继承自 `DropDownButton`：`kChevronSlot = 28.0f`（箭头区宽度）、`kChevronGlyph = 12.0f`、`kItemH = 32.0f`、`kSepH = 8.0f`、`kMenuPad = 4.0f`（`DropDownButton.h:81-85`）。

## 四、属性

`SplitButton` **没有新增属性**，全部沿用 [DropDownButton](DropDownButton.html)：

| 属性 | 说明 |
|---|---|
| `Items` | 菜单项（见 `DropDownButton` 的属性与方法） |
| `SelectedIndex` | 选中项索引（默认 `-1`） |
| `IsDropDownOpen` | 菜单是否展开 |
| `Text` / `Icon` | 左侧主区域文字与图标 |

外观类属性（`BackgroundToken`、`ForegroundToken`、`CornerRadius`、`Padding` 等）见 [Button](Button.html) 第四节。

## 五、方法

| 方法 | 说明 |
|---|---|
| `const char* GetClassName() const override` | 返回 `"SplitButton"` |
| `void OnRender(GraphicsContext&) override` | 在 `DropDownButton` 基础上绘制左右**两态区分**的分割线 |
| `void OnMouseDown(Point pt) override` | 判断落在主区还是箭头区，并记入 `m_pressInChevron` |
| `void OnMouseUp(Point pt) override` | 主区 → 触发普通 Click；箭头区 → 弹出下拉列表 |
| `bool OnKeyDown(int vkCode) override` | 方向键控制菜单高亮，Esc 关闭 |
| `virtual bool OpensOnPrimaryPress() const override` | 返回 **`false`**——这是它与 `DropDownButton`（返回 `true`）的唯一行为开关 |

私有：`Rect PrimaryRect() const` 计算左侧主区域矩形（按钮宽度减去 `kChevronSlot`）；`m_pressInChevron` 决定抬起时走哪条分支。

## 六、运行时行为

- **区域划分**：右侧 `28.0f` 宽的箭头区（`kChevronSlot`），其余为左侧主区（`PrimaryRect()`）。
- **主区点击 → Click**：只触发 `OnClick`，不弹菜单（因为 `OpensOnPrimaryPress()` 返回 `false`）。
- **箭头区点击 → 弹菜单**：走 `DropDownButton` 的弹出逻辑，选中项后触发 `OnItemChosen`。
- **视觉反馈**：主区与箭头区是**两态区分**——鼠标在哪半边，哪半边高亮；中间一条竖分割线（`OnRender` 绘制）。
- **键盘**：方向键移动菜单高亮，Esc 收起；空格 / 回车走 `Button` 的主区点击路径。
- **失焦 / 页面切走**：与 `DropDownButton` 一致，自动收起菜单。

## 七、事件

| 事件 | 类型 | 回调签名 | 触发时机 | 注册方式 |
|---|---|---|---|---|
| `OnClick` | `CallbackProperty<void(UIElement*)>` | `void(UIElement* sender)` | 点击**左侧主区**或空格 / 回车时 | `sb->OnClick = fn;` |
| `OnItemChosen()` | `Event<DropDownButton*, int, const std::string&>&` | `void(DropDownButton* sender, int index, const std::string& text)` | 从**右侧箭头区**弹出的菜单里选中一项 | `sb->OnItemChosen().Connect(fn);` |

> 两个事件对应两个区域，语义不要混用：主区是"执行默认动作"，箭头区是"选择变体"。

## 八、示例

### 8.1 DSL 最简写法

```cpp
#include "CUI.h"

auto save = SplitButtonWidget("保存")
                .AddItem("保存",        []{ /* 直接保存 */ })
                .AddItem("另存为…",     []{ /* 弹另存为 */ })
                .AddSeparator()
                .AddItem("导出为 PDF",  []{ /* 导出 */ })
                .Build();
```

### 8.2 代码式创建

```cpp
auto sb = std::make_shared<SplitButton>("保存");
sb->AddItem("保存",    []{ /* ... */ });
sb->AddItem("另存为…", []{ /* ... */ });

sb->OnClick = [](UIElement*) { /* 主区：执行默认保存 */ };
sb->OnItemChosen().Connect([](DropDownButton*, int index, const std::string& text) {
    /* 箭头区：按 index / text 分支 */
});
```

### 8.3 完整可运行

```cpp
#include "CUI.h"

int main() {
    auto label = Text("等待操作").FontSize(16.0f);

    auto sb = std::make_shared<SplitButton>("保存");
    sb->AddItem("另存为…",    [label]{ Borrow(label).Text("另存为"); });
    sb->AddItem("导出为 PDF", [label]{ Borrow(label).Text("导出 PDF"); });

    sb->OnClick = [label](UIElement*) { Borrow(label).Text("已保存"); };

    auto root = Column(16, { label, sb }).Align(Alignment::Center).Build();

    Window w;
    w.Fluent().Title("SplitButton Demo").Size(420, 240).Root(root).Build().Show().Run();
}
```

## 九、主题与自定义外观

与 [DropDownButton](DropDownButton.html) 完全一致：

| 想改的东西 | 改哪个 |
|---|---|
| 按钮底色 / 悬停 / 按下 | `BackgroundToken(...)` / `HoverBackgroundToken(...)` / `PressedBackgroundToken(...)` |
| 文字颜色 | `ForegroundToken(...)` |
| 箭头区宽度 | ❌ 编译期常量 `kChevronSlot = 28.0f`，不可运行时改 |
| 菜单行高 | ❌ `kItemH = 32.0f`，不可运行时改 |

## 十、注意事项

1. **两个区域两个事件**：主区 `OnClick`、箭头区 `OnItemChosen`，别把业务逻辑放错地方。
2. **`OpensOnPrimaryPress()` 返回 `false` 是核心差异**：把它覆写成 `true` 就退化成 `DropDownButton`。
3. **箭头区宽度固定 `28.0f`**：按钮太窄时主区会被挤没，建议 `Width` 不小于 `100`。
4. **`SelectedIndex` 默认 `-1`**：菜单项被选中后才会更新，不要假定它总是有效值。
5. **菜单项回调与 `OnItemChosen` 都会触发**：两者都写逻辑会重复执行，选一种即可。
6. **链式方法静默降级**：见 [03 · DSL 与控件速查](../CUI.Core/03-DSL与控件速查.html)。
