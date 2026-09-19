---
layout: page
title: HyperlinkButton（超链接按钮）
parent: 控件参考
nav_order: 5
---

# HyperlinkButton（超链接按钮）

> 视觉上就是一段带下划线、悬停变色的可点击文字。用于跳转到网页或触发应用内导航，承载一个 `NavigateUri` 字符串。

## 一、控件定位

| 场景 | 是否适用 |
|---|---|
| 页面里"查看详情 →"这类文字链接 | ✅ 首选 |
| 打开外部网页 | ✅ 配合 `NavigateUri` + 自己在 `OnClick` 里启动浏览器 |
| 主操作按钮（保存、提交） | ❌ 用 [Button](Button.html) |
| 点击后弹出菜单 | ❌ 用 [DropDownButton](DropDownButton.html) |
| 纯展示、不可点的文字 | ❌ 用 `TextBlock` |

## 二、类信息

| 项 | 值 |
|---|---|
| 头文件 | `ui/framework/controls/HyperlinkButton.h` |
| 声明 | `class HyperlinkButton : public Control` |
| 继承链 | `UIElement` → `Control` → `HyperlinkButton` |
| 命名空间 | `CUI` |
| DSL 工厂 | `HyperlinkButtonWidget(text, uri)` |
| 通用工厂 | `DSL::Control<HyperlinkButton>(args...)` |
| Tab 焦点 | ✅（`AcceptsTabFocus()` 返回 `true`，`HyperlinkButton.h:25`） |
| 鼠标指针 | 启用时 `IDC_HAND`，禁用时 `nullptr` |

## 三、构造函数与默认值

```cpp
HyperlinkButton();
explicit HyperlinkButton(const std::string& text, const std::string& uri = "");
```

成员默认值（源码 `HyperlinkButton.h:34`）：

| 成员 | 默认值 | 含义 |
|---|---|---|
| `m_navigateUri` | 空串 | 跳转目标地址 |

## 四、属性

### 4.1 自身属性

| 属性 | 类型 | 读 | 写 | 默认值 | 说明 |
|---|---|:-:|:-:|---|---|
| `NavigateUri` | `std::string` | `GetNavigateUri()` | `SetNavigateUri()` / DSL `HyperlinkButtonWidget(text, uri)` | `""` | 目标网址或内部导航标签。**控件不会自动打开它**，只负责保存 |

### 4.2 继承自 `Control` / `UIElement` 的常用属性

| 属性 | 说明 |
|---|---|
| `Text` | 链接文字 |
| `ForegroundToken` | 文字与下划线颜色（悬停态另有过渡色） |
| `FontSize` / `FontFamily` / `FontWeight` | 参与 `Measure`，默认沿用 `UIElement` 的出厂字体 |
| `Width` / `Height` | 显式尺寸；不给时按文字测量 |
| `IsEnabled` | 禁用后不响应键盘，指针恢复默认 |
| `Margin` / `Padding` | 参与测量 |

完整列表见 [04 · 属性与绑定](../CUI.Core/04-属性与绑定.html)。

## 五、方法

| 方法 | 说明 |
|---|---|
| `const char* GetClassName() const override` | 返回 `"HyperlinkButton"` |
| `GetProperty / HasProperty / SetProperty` | 属性系统直通 |
| `HCURSOR GetCursor() const override` | 启用时 `IDC_HAND` |
| `Size Measure(Size) override` | 按文字测量所占尺寸 |
| `void OnRender(GraphicsContext&) override` | 绘制前景文字与下划线 |
| `bool OnKeyDown(int vkCode) override` | 空格 / 回车触发点击行为 |
| `bool AcceptsTabFocus() const override` | `true` |
| `GetNavigateUri()` / `SetNavigateUri()` | 目标地址读写 |

## 六、运行时行为

- **绘制**：先画文字，再在其下方画一条下划线；悬停 / 按下时通过 `Control` 的状态过渡机制改变前景色（这是它区别于 `Button` 的地方——没有背景填充与水波纹）。
- **键盘**：`VK_SPACE` / `VK_RETURN` 触发点击行为并消费按键。
- **导航**：`NavigateUri` **仅作为数据保存**，控件自身不启动浏览器也不做页面路由；跳转逻辑写在 `OnClick` 回调里。
- **测量**：按文字尺寸测量，忽略 `availableSize`，因此**不会自动换行**，长链接会溢出。

## 七、事件

| 事件 | 类型 | 回调签名 | 触发时机 | 注册方式 |
|---|---|---|---|---|
| `OnClick` | `CallbackProperty<void(UIElement*)>` | `void(UIElement* sender)` | 鼠标点击或空格 / 回车触发 | `link->OnClick = fn;` 或 `link->OnClick.Connect(fn);` |

## 八、示例

### 8.1 DSL 最简写法

```cpp
#include "CUI.h"

auto link = HyperlinkButtonWidget("查看文档", "https://example.com/docs").Build();
```

### 8.2 真正打开网页

```cpp
#include <windows.h>

auto link = HyperlinkButtonWidget("访问 GitHub", "https://github.com").Build();
link->OnClick = [](UIElement* sender) {
    auto* l = static_cast<HyperlinkButton*>(sender);
    ShellExecuteA(nullptr, "open", l->GetNavigateUri().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
};
```

### 8.3 应用内导航（把 Uri 当作路由标签）

```cpp
auto link = HyperlinkButtonWidget("返回首页", "home").Build();
link->OnClick = [](UIElement* sender) {
    auto* l = static_cast<HyperlinkButton*>(sender);
    NavigateTo(l->GetNavigateUri());   // 自己的路由函数
});
```

### 8.4 完整可运行

```cpp
#include "CUI.h"

int main() {
    auto tip  = Text("点击下面的链接").FontSize(14.0f);
    auto link = HyperlinkButtonWidget("打开 CUI 文档", "https://laster-dev.github.io/CUI/")
                    .FontSize(14.0f)
                    .Build();

    link->OnClick = [tip](UIElement* sender) {
        auto* l = static_cast<HyperlinkButton*>(sender);
        Borrow(tip).Text("目标：" + l->GetNavigateUri());
    };

    auto root = Column(12, { tip, link }).Align(Alignment::Center).Build();

    Window w;
    w.Fluent().Title("HyperlinkButton Demo").Size(460, 220).Root(root).Build().Show().Run();
}
```

## 九、主题与自定义外观

| 想改的东西 | 改哪个 |
|---|---|
| 链接文字 | 构造实参或 `Text(...)` |
| 文字 / 下划线颜色 | `ForegroundToken(...)`（悬停态由 `Control` 的过渡机制自动混合） |
| 字号 / 字体 | `FontSize(...)` / `FontFamily(...)` |
| 禁用视觉 | `SetIsEnabled(false)` + 自行调整 `ForegroundToken` |

## 十、注意事项

1. **`NavigateUri` 不会自动打开**：必须自己在 `OnClick` 里调用 `ShellExecute` 或写路由逻辑。
2. **没有背景与水波纹**：这是文字型控件，想要按钮质感请用 [Button](Button.html)。
3. **不会自动换行**：`Measure` 只按文字测量，长链接需自行截断或给定 `Width` + 省略处理。
4. **`OnClick` 只有一个**：想同时做埋点和跳转，用 `OnClick.Connect()` 追加第二个订阅者。
5. **链式方法静默降级**：见 [03 · DSL 与控件速查](../CUI.Core/03-DSL与控件速查.html)。
