# CUI Gallery / Framework 问题修复计划

> 创建日期: 2026-08-04  
> 修订: 2026-08-04（二次反馈：Splitter 卡顿/过细、禁用态无视觉、顶栏点击穿透、材质无效果）  
> 目标: 对照 WinUI3 Gallery / 顶级桌面 UI 框架体验，系统性消除当前 Gallery 与控件层的体验断点。  
> 原则: **计划落地到仓库 → 按优先级逐项合入 → 每项可验证**。

---

## 问题清单（用户反馈 → 根因摘要）

| ID | 用户问题 | 根因摘要 | 优先级 |
|----|----------|----------|--------|
| A | Splitter 横向/纵向写反 | Showcase 标签与 `Orientation` 映射颠倒；Width/Height 也反了 | P0 |
| B | 示例代码不能复制 | 代码框 `isEnabled=false`，禁用后无法拖选；无真正 ReadOnly | P0 |
| C | 属性全是字符串 `SetProperty("x", …)` | Object 属性袋是运行时 string map；缺编译期强类型 API 层 | P1 |
| D | 所有控件 `IsEnabled` 无效 | 多处绕过 `Control` 输入门闩；HitTest/OnClick 未统一拦截 | P0 |
| E | 界面整体糊 | DPI 已开启，但整帧重绘/层缓存/分数坐标路径仍会导致发糊感 | P1 |
| F | 非常卡 | 焦点定时器整帧重绘；主题/滚动层 thrash；动画脏区过大 | P0 |
| G | 与顶级 UI 框架差什么 | 缺材质透出、强类型属性、稳定输入门闩、局部绘制、成熟控件契约 | 见里程碑 |
| H | 顶部「动画/低性能」经常点不了 | TitleBar 命中/消息路径偶发失效；需加固 hit-test 与消息路由 | P0 |
| I | 顶部暗色/亮色经常点不了 | 同上 + 主题切换后缓存未完全刷新（已修一部分） | P0 |
| J | 根本没有材质效果 | DWM Mica/Acrylic 已调用，但客户区被不透明 `windowBackground` 盖死 | P1 |

---

## 里程碑总览

```
M0  止血（本周）     A B D H I F(局部)
M1  观感与材质       E J + 导航/代码区体验
M2  属性与 API       C（强类型属性门面 + PropertyGrid 双向同步）
M3  性能与绘制架构   F 深挖（脏区、层、定时器、滚动）
M4  框架差距收敛     G（对照 WinUI 契约清单逐项达标）
```

---

## M0 — 止血（立即做）

### A. Splitter 横向 / 纵向修正
**文件**
- `CUI/showcase/app/pages/SplitterPage.cpp`
- `CUI/ui/framework/controls/Splitter.cpp`（补 IsEnabled 门闩）

**做法**
1. 「水平（左右）」→ `Orientation::Vertical`（竖条左右拖）
2. 「垂直（上下）」→ `Orientation::Horizontal`（横条上下拖）
3. 左右分割条用 `Width(6)`；上下分割条用 `Height(6)`
4. `OnMouseDown/Move` 在 disabled 时禁止拖拽

**验收**
- Gallery → Splitter：左右拖改变宽度；上下拖改变高度；标签与行为一致

---

### B. 示例代码可复制
**文件**
- `CUI/showcase/app/ShowcaseHelpers.cpp`
- `CUI/ui/framework/controls/TextBox.h/.cpp`（加 `IsReadOnly`）

**做法**
1. TextBox 增加 `IsReadOnly` / `SetIsReadOnly`（属性 `isReadOnly`）
2. ReadOnly：禁止插入/删除/粘贴，**允许**聚焦、选区、Ctrl+C / 右键复制
3. 示例代码框改为 `isEnabled=true` + `isReadOnly=true`
4. （可选）折叠面板旁加「复制代码」按钮

**验收**
- 展开「示例代码」→ 拖选 → Ctrl+C 能粘贴到记事本

---

### D. IsEnabled 全控件生效
**文件**
- `CUI/ui/framework/controls/UIElement.cpp`（HitTest / OnMouseUp 门闩）
- `CUI/ui/framework/controls/Control.cpp`
- 审计：`Splitter` / `TextBox` / `ScrollViewer` / `TabView` / `CollapsePanel` / `ListView` 等

**做法**
1. `UIElement::OnMouseDown/Up/Move/Wheel/KeyDown`：disabled 时吞掉（或不上浮点击）
2. `OnClick` 仅在 `IsEnabled()` 时触发
3. 各控件自定义路径补齐 `if (!IsEnabled()) return;`
4. PropertyGrid 切换 `isEnabled` 后视觉立即灰显且不可点

**验收**
- 任意控件在属性面板关 `IsEnabled` 后：不可点、不可键入、外观禁用

---

### H / I. 顶部主题 / 动画按钮可靠点击
**文件**
- `CUI/ui/framework/controls/VSCodeControls.cpp`（TitleBar）
- `CUI/ui/framework/window/Window.cpp`（`WM_NCHITTEST` / `WM_APP+42/43/44`）

**做法**
1. 主题/材质/动画三个热区扩大命中，避免落在 caption 拖拽区
2. `WM_NCHITTEST`：落在热区一律 `HTCLIENT`
3. 点击处理改用稳定 `Window*`（避免 `WindowFromPoint` 偶发失败）
4. 切换后强制 `ApplyVisualState` + 内容层失效（已有一部分，补全）

**验收**
- 连续快速点「亮色/暗色」「动画/低性能」「材质」各 20 次，全部生效

---

### F0. 卡顿止血（最小改动）
**文件**
- `CUI/ui/framework/window/Window.cpp`（焦点定时器勿整帧）

**做法**
1. 去掉「有焦点就 `RequestFullRepaint`」的 500ms 定时器整帧路径
2. TextBox 光标闪烁改为局部脏矩形 / `InvalidateAnimatedRegions`

**验收**
- 空闲时「帧模式」多为局部；打字时 FPS 稳定、不全屏闪

---

## M1 — 观感与材质

### E. 去糊
1. 布局坐标在绘制前对齐到物理像素（文本/1px 边）
2. 减少无必要整帧；层缓存失效策略与主题切换对齐
3. 审计无 HWND 的临时 `GraphicsContext` 测字路径

### J. 真正看见 Mica / Acrylic
1. `backdrop != None` 时根背景 alpha 降低或局部透明
2. `OnPaint` 清屏使用预乘透明，让 DWM 透出
3. TitleBar / Pane 区分实心与透出区域（对齐 WinUI）

**验收**
- 切换材质后，窗口背后桌面壁纸透过标题栏/侧栏可见（非纯色盖死）

---

## M2 — 属性与 API（回应「为什么要是字符串」）

### C. 分层改造（不一次拆光）

**现状**
- 运行时：`Object::SetProperty(string, Value)` 属性袋（反射/主题/PropertyGrid 依赖它）
- 部分控件已有薄封装：`SetText` / `SetIsEnabled`

**目标形态（WinUI 风格门面）**
```cpp
button->SetIsEnabled(false);
textBox->SetAcceptsReturn(true);
// 内部仍可落到 SetProperty，但调用方不再手写字符串
```

**步骤**
1. 为常用属性补齐强类型 getter/setter（IsEnabled, AcceptsReturn, Orientation…）
2. Showcase / 示例代码生成器优先输出强类型 API
3. PropertyGrid 继续走 metas + 属性袋（框架内部）
4. 文档明确：字符串 `SetProperty` = 框架/工具链；应用代码用强类型

**验收**
- Gallery 示例代码不再满屏 `SetProperty("..." )`；改为 `SetXxx(...)`

---

## M3 — 性能与绘制架构

1. 脏区绘制成为默认；整帧仅用于 resize/主题/首次显示
2. ScrollViewer 层：主题切换必失效；滚动优先 blit patch
3. NavigationView / 属性面板避免每帧 ClearChildren
4. 低性能模式：真正降采样动画 + 停用不必要层

---

## M4 — 与顶级 UI 框架的差距清单（长期）

| 能力 | 当前 | 目标 |
|------|------|------|
| 系统材质 | DWM 调用但被不透明客户区盖住 | 可见 Mica/Acrylic |
| 控件契约 | IsEnabled/ReadOnly/焦点不一致 | 统一视觉状态机 |
| 属性模型 | 字符串袋 + 少量封装 | 强类型 API + 反射 metas |
| 导航/Gallery | 在追 WinUI Gallery | 分类、搜索、代码、属性三栏稳定 |
| 绘制 | 易整帧、层缓存易脏 | 局部脏区 + 稳定 60/120Hz |
| 输入 | 多路径绕过 | 单一命中/捕获/手势管道 |
| 无障碍 | 基本无 | 焦点可视、键盘完整 |

---

## 执行顺序（本仓库落地）

1. [x] **A** Splitter 方向与尺寸  
2. [x] **B** 示例代码 ReadOnly 可复制  
3. [x] **D** IsEnabled 统一门闩  
4. [x] **H/I** TitleBar 主题/动画热区  
5. [x] **F0** 去掉焦点定时器整帧  
6. [x] **J** 材质透出（最小可见版）  
7. [ ] **E** 去糊专项  
8. [ ] **C** 强类型属性门面 + 示例代码生成改造  
9. [ ] **F/M3** 性能深挖  
10. [ ] **M4** 逐项对齐 WinUI 契约  

每完成一项：更新本文件对应复选框，并在 PR/提交说明写清验收方式。

---

## 相关路径速查

- Splitter: `CUI/ui/framework/controls/Splitter.*` · `showcase/app/pages/SplitterPage.cpp`
- 示例代码: `CUI/showcase/app/ShowcaseHelpers.cpp`
- 属性: `CUI/ui/framework/core/Object.*` · `PropertyGrid.*`
- 启用态: `Control.cpp` · `UIElement.cpp`
- 标题栏: `VSCodeControls.cpp` · `Window.cpp`
- 材质: `WindowBackdrop.cpp` · `Window::OnPaint`
- 主题: `ThemeManager.*` · `Window::SetThemeMode` / `ApplyVisualState`
