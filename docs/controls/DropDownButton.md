---
layout: page
title: DropDownButton（下拉按钮）
parent: 控件参考
nav_order: 3
---

# DropDownButton（下拉按钮）

> 带箭头的按钮，点击**任意位置**都会弹出自绘下拉菜单（支持分隔线、高亮、键盘导航、点击外部自动关闭）。`SplitButton` 以它为基类，只把"点主区"改成触发 Click。

## 一、控件定位

| 场景 | 是否适用 |
|---|---|
| 点开一列命令 / 选项（更多操作 ▾） | ✅ 首选 |
| 有默认动作 + 若干变体 | ❌ 用 [SplitButton](SplitButton.html) |
| 从固定枚举里选一项并显示当前值 | ❌ 用 `ComboBox` |
| 只要一个动作 | ❌ 用 [Button](Button.html) |

## 二、类信息

| 项 | 值 |
|---|---|
| 头文件 | `ui/framework/controls/DropDownButton.h` |
| 声明 | `class DropDownButton : public Button` |
| 继承链 | `UIElement` → `Control` → `Button` → `DropDownButton` |
| 命名空间 | `CUI` |
| DSL 工厂 | `DropDownButtonWidget(text)` |
| 通用工厂 | `DSL::Control<DropDownButton>(args...)` |
| Tab 焦点 | ✅（继承 `Button::AcceptsTabFocus()` → `true`） |

### 布局常量（`DropDownButton.h:81-85`）

| 常量 | 值 | 含义 |
|---|---|---|
| `kChevronSlot` | `28.0f` | 箭头槽宽度 |
| `kChevronGlyph` | `12.0f` | 箭头字形尺寸 |
| `kItemH` | `32.0f` | 菜单行高 |
| `kSepH` | `8.0f` | 分隔线高度 |
| `kMenuPad` | `4.0f` | 菜单内边距 |

## 三、构造函数与默认值

```cpp
DropDownButton();
explicit DropDownButton(const std::string& text);
```

| 成员 | 默认值 | 含义 |
|---|---|---|
| `m_selectedIndex` | `-1` | 当前选中项索引，`-1` 表示未选 |
| `m_dropOpen` | `false` | 菜单是否展开 |
| `m_highlight` | `-1` | 键盘 / 鼠标高亮行 |

外观默认值继承自 `Button`（Accent 系列 Token、`微软雅黑 12`、Padding `(8,4,8,4)`、圆角 `4`），见 [Button](Button.html) 第三节。

## 四、属性

| 属性 | 类型 | 读 | 写 | 默认值 | 说明 |
|---|---|:-:|:-:|---|---|
| `SelectedIndex` | `int` | `GetSelectedIndex()` | `SetSelectedIndex(int)` | `-1` | 选中项索引 |
| `IsDropDownOpen` | `bool` | `IsDropDownOpen()` | `OpenDropDown()` / `CloseDropDown()` | `false` | 菜单展开状态 |

菜单项通过方法添加（不是属性）：`AddItem(text, callback)`、`AddSeparator()`。外观类属性见 [Button](Button.html) 第四节。

## 五、方法

| 方法 | 说明 |
|---|---|
| `const char* GetClassName() const override` | 返回 `"DropDownButton"` |
| `virtual void OnRender(GraphicsContext&) override` | 绘制按钮本体、箭头与（展开时的）菜单 |
| `virtual void OnMouseDown(Point pt) override` / `OnMouseUp` / `OnMouseMove` / `OnMouseLeave` | 展开 / 收起、行高亮、选中 |
| `virtual bool OnKeyDown(int vkCode) override` | `↑` `↓` 移动高亮，`Enter` 选中，`Esc` 收起 |
| `void AddItem(const std::string& text, std::function<void()> onClick)` | 追加一个菜单项 |
| `void AddSeparator()` | 追加一条分隔线 |
| `int GetSelectedIndex() const` / `void SetSelectedIndex(int)` | 选中项读写 |
| `bool IsDropDownOpen() const` / `void OpenDropDown()` / `void CloseDropDown()` | 手动控制展开 |
| `virtual bool OpensOnPrimaryPress() const` | 返回 **`true`**——点按钮主体即弹菜单（`SplitButton` 覆写为 `false`） |
| `Event<DropDownButton*, int, const std::string&>& OnItemChosen()` | 选中事件访问器 |

## 六、运行时行为

- **点击即展开**：因为 `OpensOnPrimaryPress()` 返回 `true`，点按钮任意位置都弹菜单，不会触发 `OnClick` 语义的"执行动作"。
- **菜单几何**：宽度至少等于按钮宽度，行高 `kItemH = 32.0f`，分隔线 `kSepH = 8.0f`，内边距 `kMenuPad = 4.0f`；锚在按钮下方。
- **高亮**：`m_highlight` 记录当前行，`OnMouseMove` 更新，`↑` `↓` 移动，越界不循环（以源码为准）。
- **选中**：`Enter` 或点击行 → `SetSelectedIndex` + 触发 `OnItemChosen` + 执行该项回调 + 收起菜单。
- **自动收起**：鼠标移出菜单、点击外部（light dismiss）、`Esc`、失去焦点都会 `CloseDropDown()`。

## 七、事件

| 事件 | 类型 | 回调签名 | 触发时机 | 注册方式 |
|---|---|---|---|---|
| `OnItemChosen()` | `Event<DropDownButton*, int, const std::string&>&` | `void(DropDownButton* sender, int index, const std::string& text)` | 选中某一菜单项时 | `ddb->OnItemChosen().Connect(fn);` |
| `OnClick` | `CallbackProperty<void(UIElement*)>` | `void(UIElement*)` | 继承自 `UIElement`；本控件把点击消费成"展开菜单"，**不要依赖它** | `ddb->OnClick = fn;` |

> **坑**：`AddItem` 的回调与 `OnItemChosen` 都会执行，二选一即可，别写两份逻辑。

## 八、示例

### 8.1 DSL 写法

```cpp
#include "CUI.h"

auto more = DropDownButtonWidget("更多")
                .AddItem("重命名", []{ /* ... */ })
                .AddItem("复制",   []{ /* ... */ })
                .AddSeparator()
                .AddItem("删除",   []{ /* ... */ })
                .Build();
```

### 8.2 代码式 + 事件

```cpp
auto ddb = std::make_shared<DropDownButton>("操作");
ddb->AddItem("导出", []{});
ddb->AddItem("打印", []{});
ddb->OnItemChosen().Connect([](DropDownButton* sender, int index, const std::string& text) {
    // index / text 即被选中项
});
```

### 8.3 完整可运行

```cpp
#include "CUI.h"

int main() {
    auto label = Text("未选择").FontSize(16.0f);
    auto ddb   = std::make_shared<DropDownButton>("选择操作");
    ddb->AddItem("导出 CSV", []{});
    ddb->AddItem("导出 JSON", []{});
    ddb->OnItemChosen().Connect([label](DropDownButton*, int, const std::string& text) {
        Borrow(label).Text("已选择：" + text);
    });

    auto root = Column(16, { label, ddb }).Align(Alignment::Center).Build();
    Window w;
    w.Fluent().Title("DropDownButton Demo").Size(420, 240).Root(root).Build().Show().Run();
}
```

## 九、主题与自定义外观

| 想改的东西 | 改哪个 |
|---|---|
| 按钮底色 / 悬停 / 按下 | `BackgroundToken(...)` / `HoverBackgroundToken(...)` / `PressedBackgroundToken(...)` |
| 文字颜色 | `ForegroundToken(...)` |
| 箭头槽宽 / 行高 / 分隔线高 | ❌ 编译期常量，运行时不可改 |

## 十、注意事项

1. **点击即展开，不执行动作**：需要"默认动作 + 变体"请用 [SplitButton](SplitButton.html)。
2. **`AddItem` 回调与 `OnItemChosen` 都触发**，别重复写业务逻辑。
3. **`SelectedIndex` 默认 `-1`**，用前先判断是否有效。
4. **菜单宽度以按钮宽度为准**：按钮太窄会让菜单项文字被截断。
5. **链式方法静默降级**：见 [03 · DSL 与控件速查](../CUI.Core/03-DSL与控件速查.html)。
