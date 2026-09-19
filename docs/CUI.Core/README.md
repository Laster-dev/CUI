---
layout: page
title: 手册索引与 AI 使用说明
parent: CUI.Core 开发手册
nav_order: 1
---

# CUI.Core 开发文档

> 目标：让开发者 / AI 在**不通读 261 个源文件**的前提下正确改代码。
> 本文所有类型与函数签名均从头文件原文摘抄核对（2026-09-19）。
> 适用版本：`CUI.Core`（静态库，C++20 / MSVC / D2D + DWrite + D3D11 + DXGI + DComp）。

---

## 一、文档地图（按你要做的事选）

| 我要做的事 | 读哪篇 |
|---|---|
| 第一次接触：框架怎么分层、一帧怎么跑、有哪些不变量 | [01-架构总览](./01-架构总览.md) |
| 写个窗口 + 几个控件先跑起来 | [02-快速上手](./02-快速上手.md) |
| 查某个 DSL 方法 / 控件工厂叫什么名字 | [03-DSL与控件速查](./03-DSL与控件速查.md) |
| 属性怎么读写、怎么做数据绑定（State / Observable） | [04-属性与绑定](./04-属性与绑定.md) |
| 布局不对：尺寸、位置、Grid 列宽不回缩 | [05-布局系统](./05-布局系统.md) |
| 自绘控件；画错了 / 改了不刷新 / 闪烁 | [06-渲染与绘制](./06-渲染与绘制.md) |
| 动画不动、一直动、页面切走后空转 | [07-动画与帧调度](./07-动画与帧调度.md) |
| 窗口、鼠标键盘路由、焦点、右键菜单、拖拽、DPI | [08-窗口输入与弹出层](./08-窗口输入与弹出层.md) |
| 新增控件 / 容器 / 属性 / 主题 Token | [09-扩展指南](./09-扩展指南.md) |
| 命名、内存、线程、已修复的历史坑 | [10-规约与已知陷阱](./10-规约与已知陷阱.md) |

---

## 二、AI 必读：最小上下文加载策略

如果你是 AI（或刚接手的人），**按下面顺序加载，不要一开始就遍历源码**：

1. **本文件**（索引，决定去哪篇）
2. **[01-架构总览](./01-架构总览.md)**（分层 + 三条不变量 + 一帧流程）
3. **与你任务对应的那一篇**（通常是 02 / 03 / 06 之一）

> 加载上限：3 篇。只有做底层改动时才读源码，且**只读文档里点名的文件**。
> 原因：CUI.Core 有 140 头文件 / 118 实现文件，全量读入会挤爆上下文并掺入无关实现细节。

### 改动前的"定位三问"

1. **改的是哪一层？** → 控件 / 布局 / 渲染 / 动画 / 窗口，对应目录见 01 的映射表。
2. **该行为由哪个虚函数决定？** → 每篇文档都有"契约"小节直接点名虚函数。
3. **会不会破坏不变量？** → 见 01 第三节与 10：DIP 单位、`shared_ptr` 元素树、脏标记、单 UI 线程。

---

## 三、三条铁律（改代码前先记住）

1. **单位一律是 DIP（逻辑像素）。**
   只有 3 处例外：`GraphicsContext::Resize(w,h)`、`EndDraw(const RECT* dirtyRectPx)`、`EnsureCompositionSurface(..., wPx, hPx)`。
   Win32 消息里的鼠标坐标是物理像素，必须先 `ClientPhysicalToLogical()` 转换。

2. **元素树：子用 `shared_ptr`，父是裸指针（弱引用）。**
   `AddChild(shared_ptr)`；`GetParent()` 不拥有所有权。
   跨回调长期保存元素请用 `std::weak_ptr`（框架内部即用 `CaptureElementRef()` 提升）。

3. **改了东西必须显式失效。**
   - 影响尺寸 → `InvalidateMeasure()`（连带 Arrange）
   - 只影响位置 → `InvalidateArrange()`
   - 只影响外观 → `MarkRenderContentDirty()` 或 `MarkRenderRectDirty(rect)`
   - **没有** `InvalidateVisual()`（别去找，不存在）

---

## 四、一眼速查：常用入口

```cpp
#include "CUI.h"   // 一个头搞定：控件 + DSL + 窗口 + 渲染 + 动画 + 主题
using namespace CUI;
using namespace CUI::DSL;   // CUI.h 已默认注入，可省略

// 窗口
Window w;
w.Fluent().Title("App").Size(1280, 800).Theme(ThemeMode::Dark)
          .Backdrop(BackdropType::Mica).Root(root).Build().Show().Run();

// 元素树
auto root = Column(20, { Text("Hi").FontSize(24), ElevatedButton("OK").OnClick(f) })
                .Align(Alignment::Center).Build();

// 属性（控件成员是 Property<T> 风格）
status->Text = "已点击";                        // 赋值即生效并广播变更
status->Text.Bind(myState, BindingMode::TwoWay);

// 自绘：唯一需要重写的绘制点
void OnRender(GraphicsContext& ctx) override;

// 动画
RequestAnimationTicks();                        // 登记逐帧
bool OnAnimationTick() override;                // 返回 false 自动注销

// 主题取色（绘制路径用枚举，不用字符串）
auto c = ThemeManager::Instance().GetColor(ThemeTokenId::CardBackground);
```

---

## 五、构建须知（最容易卡住的一点）

- 产物是**静态库** `CUI.Core.lib`，无 DLL；消费方通过 `ProjectReference` 自动链接。
- **新增 `.cpp` 必须手工写进 `CUI.Core.vcxproj` 的 ClCompile 列表**——工程没有通配符，忘了加就链接报 `LNK2019`。
- 附加包含目录是 `$(ProjectDir)ui`，所以库内部互相引用写作 `"framework/window/Window.h"`；
  **应用工程只需要 `#include "CUI.h"`**（伞形头位于 `CUI.Core/ui/CUI.h`）。
- C++20、`/utf-8`、Unicode；x64 用 v143 工具集、Win32 用 v145（不统一，改动时注意）。
- 依赖 NuGet：`VC-LTL.5.3.1` 必须已还原，否则 `EnsureNuGetPackageBuildImports` 直接中断构建。

---

## 六、文档维护约定

- 本文档描述的接口若与源码冲突，**以源码为准，并请顺手修正文档**。
- 新增公开 API 时同步更新对应章节的表格（尤其是 03 的工厂/方法表与 09 的扩展清单）。
- 审查类文档在 `docs/review/`（问题清单）；本目录是**开发手册**（怎么用），两者不要混。
