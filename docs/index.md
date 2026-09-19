---
layout: page
title: CUI 框架文档
nav_order: 0
---

# CUI 框架文档

`CUI.Core` 是一个基于 **Direct2D / DirectWrite / DirectComposition** 的 C++20 Win32 现代 UI 框架：
自带 90+ 控件、声明式链式 DSL、7 种布局容器、主题 Token、动画系统与可停靠窗口。

---

## 30 秒上手

只需要 **一个 include**：

```cpp
#include "CUI.h"      // 伞形头：控件 + DSL + 窗口 + 渲染 + 动画 + 主题

using namespace CUI;
using namespace CUI::DSL;   // CUI.h 默认已注入，可省略

int main() {
    auto label  = Text("Click count: 0").FontSize(24.0f);
    auto button = ElevatedButton("Click Me!")
                      .OnClick([label](UIElement*) mutable {
                          static int n = 0;
                          Borrow(label).Text("Click count: " + std::to_string(++n));
                      });

    auto root = Column(20, { label, button }).Align(Alignment::Center).Build();

    Window w;
    w.Fluent().Title("CUI Demo").Size(400, 300)
              .Theme(ThemeMode::Dark)
              .Backdrop(BackdropType::Mica)
              .Root(root).Build().Show().Run();
}
```

不想污染全局命名空间：

```cpp
#define CUI_NO_USING_NAMESPACE
#include "CUI.h"
```

---

## 开发手册（CUI.Core）

| 章节 | 什么时候读 |
|---|---|
| [01 · 架构总览](CUI.Core/01-架构总览.html) | 第一次接触：分层、一帧流程、三条不变量 |
| [02 · 快速上手](CUI.Core/02-快速上手.html) | 先跑起来：窗口、容器、事件、自定义控件 |
| [03 · DSL 与控件速查](CUI.Core/03-DSL与控件速查.html) | 查方法名：工厂函数 / 链式方法 / 事件 |
| [04 · 属性与绑定](CUI.Core/04-属性与绑定.html) | 属性读写、`State` / `Observable` 绑定 |
| [05 · 布局系统](CUI.Core/05-布局系统.html) | 尺寸位置不对：Measure / Arrange / Grid 语法 |
| [06 · 渲染与绘制](CUI.Core/06-渲染与绘制.html) | 自绘、`GraphicsContext`、脏区、主题取色 |
| [07 · 动画与帧调度](CUI.Core/07-动画与帧调度.html) | 动画不动 / 一直动 / 空转 |
| [08 · 窗口、输入与弹出层](CUI.Core/08-窗口输入与弹出层.html) | 窗口、路由事件、焦点、拖放、菜单、DPI |
| [09 · 扩展指南](CUI.Core/09-扩展指南.html) | 新增控件 / 容器 / 属性 / Token 的改动清单 |
| [10 · 规约与已知陷阱](CUI.Core/10-规约与已知陷阱.html) | 命名、内存、线程、已踩过的坑 |

> **给 AI 的加载建议**：先读本页 + `01-架构总览` + 任务对应那一篇即可，
> 不要通读 261 个源文件。详见 [手册索引](CUI.Core/README.html)。

---

## 控件参考（逐控件详解）

每篇包含：控件定位 → 类信息与继承链 → 构造函数与出厂默认值 → 全量属性表 → 方法 → 运行时行为（Measure / 渲染 / 输入 / 动画）→ 事件 → 示例 → 主题自定义 → 注意事项。

| 分类 | 控件 |
|---|---|
| 按钮 | [Button](controls/Button.html) · [ToggleButton](controls/ToggleButton.html) · [DropDownButton](controls/DropDownButton.html) · [SplitButton](controls/SplitButton.html) · [HyperlinkButton](controls/HyperlinkButton.html) |
| 选择与开关 | [CheckBox](controls/CheckBox.html) · [RadioButton](controls/RadioButton.html) · [ToggleSwitch](controls/ToggleSwitch.html) |
| 输入 | [TextBox](controls/TextBox.html) · [PasswordBox](controls/PasswordBox.html) · [NumberBox](controls/NumberBox.html) · [AutoSuggestBox](controls/AutoSuggestBox.html) |

完整索引见 [控件参考总览](controls/index.html)。

---

## 三条铁律

1. **单位是 DIP**（逻辑像素）。只有 `Resize`、`EndDraw(RECT*)`、`EnsureCompositionSurface` 用物理像素；Win32 鼠标坐标必须先 `ClientPhysicalToLogical()` 转换。
2. **子用 `shared_ptr`，父是裸指针**。跨回调保存元素请用 `weak_ptr`。
3. **改了必须显式失效**：`InvalidateMeasure()` / `InvalidateArrange()` / `MarkRenderContentDirty()`。框架**没有** `InvalidateVisual()`。

---

## 构建须知

- 产物是静态库 `CUI.Core.lib`，应用工程用 `ProjectReference` 引用即可，无需拷贝 DLL。
- **新增 `.cpp` 必须手工写进 `CUI.Core.vcxproj`**（工程没有通配符），否则链接报 `LNK2019`。
- 附加包含目录：`$(ProjectDir)ui`；语言标准 C++20、`/utf-8`、Unicode。
- NuGet 依赖 `VC-LTL.5.3.1` 需先还原。

---

## 部署本文档到 GitHub Pages

1. 仓库 **Settings → Pages**
2. **Source** 选 `Deploy from a branch`
3. **Branch** 选 `master`（本仓库默认分支），**Folder** 选 `/docs`
4. 保存，等待 1～2 分钟即可访问 `https://laster-dev.github.io/CUI/`

无需任何构建步骤：GitHub 会用 `docs/_config.yml` 里的 Jekyll 主题自动渲染本目录。
