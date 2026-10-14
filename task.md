# CUI 框架演进与任务追踪清单 (task.md)

> **当前工程状态**：
> - ✅ **历史里程碑已完成**：DirectX / Win32 底层类型深度解耦与纯净 C++ API 清洗（Phase 1 ~ Phase 5 全部通过，零 DX/Win32 头文件泄漏，通过全量编译与主题门禁）。
> - 🚀 **新一阶段目标**：对标工业级 UI 框架（WinUI 3、Flutter、Qt、Slint），全面补齐语法范式、合成渲染管线、数据虚拟化、富文本排版、无障碍体系与开发者工具链。

---

## 🗺️ 全局演进路线图（Roadmap Overview）

```mermaid
flowchart TD
    subgraph Milestone_Done["已完成里程碑"]
        M0["Phase 1~5: DX/Win32 底层解耦与纯净 C++ 接口 (Done)"]
    end

    subgraph Milestone_Syntax["第一阶段: 语法范式与虚拟化 (核心体验)"]
        P6["Phase 6: 细粒度响应式 (Signals) 与声明式流控制 (If/ForEach)"]
        P7["Phase 7: 通用海量数据虚拟化 (RecyclePool & VirtualizingPanel)"]
        P11["Phase 11: 运行时可视化检查器 (In-App Visual Inspector / DevTools)"]
    end

    subgraph Milestone_Engine["第二阶段: 渲染管线与排版引擎 (性能与能力)"]
        P8["Phase 8: 指令流录制 (DisplayList) 与合成器架构"]
        P9["Phase 9: 富文本控件混排 (InlineUIContainer) 与 TSF 输入法"]
        P10["Phase 10: 响应式视口断点与共享元素转场 (Hero Transitions)"]
    end

    subgraph Milestone_Platform["第三阶段: 工业标准与跨平台架构 (成熟生态)"]
        P12["Phase 12: Windows UI Automation (UIA) 无障碍树"]
        P13["Phase 13: 跨平台硬件抽象层 (HAL: Vulkan/Metal/WebGL)"]
    end

    Milestone_Done --> Milestone_Syntax
    Milestone_Syntax --> Milestone_Engine
    Milestone_Engine --> Milestone_Platform
```

---

## 📌 历史归档里程碑（已全部验收）

<details>
<summary><b>点击展开已完成的 Phase 1 ~ Phase 5（底层纯净化与解耦）</b></summary>

- [x] **Phase 1: 核心基础类型与色彩系统纯净化 (Color & Primitives)**
  - [x] 1.1 扩展并纯化 `Value.h` 中的 `CUI::Color`，剔除公共头文件中 `<d2d1.h>` 依赖。
  - [x] 1.2 重构主题色彩标记表 `ThemeTokens`，全量改用 `CUI::Color`。
  - [x] 1.3 清洗控件中直接引用 `D2D1_COLOR_F` 的属性与模型。
- [x] **Phase 2: 文字排版与字体系统封装 (Typography & Shaping)**
  - [x] 2.1 建立通用文本类型定义头文件 `TextTypes.h`（TextAlignment、FontWeight、TextStyle）。
  - [x] 2.2 封装内部文本布局器与缓存 `TextLayoutCache`，对外隐藏 `IDWriteTextLayout`。
  - [x] 2.3 改造 `GraphicsContext` 文字绘制与测量 API 为纯净 C++ 接口。
  - [x] 2.4 清洗 `TextBlock`、`TextBox`、`Button` 等核心文本渲染。
- [x] **Phase 3: 窗口与输入事件循环解耦 (Windowing & Input)**
  - [x] 3.1 抽象原生光标与键盘枚举 `InputTypes.h`（`CursorType`, `KeyEventArgs`）。
  - [x] 3.2 解耦控件光标返回逻辑（`GetCursorType()` 替代 `HCURSOR`）。
  - [x] 3.3 窗口消息转换与事件派发收敛至 `Window.cpp`，移除控件裸接收 `vkCode`。
  - [x] 3.4 窗口实例语义化 API 封装（隔离 `HWND`）。
- [x] **Phase 4: 渲染上下文与系统特效深度封装 (GraphicsContext & Effects)**
  - [x] 4.1 清洗 `GraphicsContext.h` 公共绘图接口，隐藏 D2D 矩阵与变换。
  - [x] 4.2 封装 Win11 Mica/Acrylic 材质与阴影配置，代码收敛至 `.cpp`。
- [x] **Phase 5: 控件库清洗与全量构建验证 (Clean-up & Verification)**
  - [x] 5.1 全局扫描控件库头文件，实现 0 DX、0 Win32 泄漏。
  - [x] 5.2 全量构建 `CUI.Core`、`CUI.Gallery`（Debug/Release），0 错误通过。

</details>

---

## 🚀 待开展演进任务列表 (New Roadmap Tasks)

---

### Phase 6: 细粒度响应式系统与声明式流控制 (Reactivity & DSL Flow)

- [ ] **6.1 引入 C++20 细粒度响应式基语 (`Signal<T>` / `Computed<T>`)**
  - **模块位置**: `ui/framework/reactive/Signal.h`
  - **目标**: 终结“手动监听事件 -> 手动调用 ApplyXxx”的面条代码。
  - **任务要点**:
    - [ ] 实现 `Signal<T>`：读操作自动建立依赖追踪拓扑图，写操作仅触发精确依赖节点的脏标记。
    - [ ] 实现 `Computed<T>`：惰性求值与缓存失效，仅在依赖信号变更时重新计算。
    - [ ] 实现 `Effect(func)`：自动订阅函数内访问的信号，在信号变动时批处理调度执行。
    - [ ] `TextBlock`、`ProgressBar`、`Slider` 等核心控件支持直接接收 `Signal<T>` 传参。

- [ ] **6.2 声明式结构控制流基语 (`If` / `ForEach` / `Switch`)**
  - **模块位置**: `ui/framework/core/CUIDsl.h`
  - **目标**: 允许在 CUI DSL 视图树中直接书写条件渲染与集合迭代。
  - **任务要点**:
    - [ ] 实现 `If(condition_signal, then_builder, else_builder)`，动态切换渲染子树且保持状态平滑。
    - [ ] 实现 `ForEach(collection_signal, item_builder)`，根据数据源自动同步增删节点。
    - [ ] 提供列表局部 Diff 算法（类似 React/Vue Keyed Diff），避免数据单项更新引发整列表重建。

- [ ] **6.3 强类型编译期绑定检查 (Type-Safe Compile-Time Bindings)**
  - **目标**: 逐步减少 `PropertyId` + `Value` 运行时弱类型反射开销，提供编译期静态类型推导的双向绑定语法。

---

### Phase 7: 通用海量数据虚拟化体系 (Universal Virtualization & RecyclePool)

- [ ] **7.1 通用组件回收复用池 (`RecyclePool`)**
  - **模块位置**: `ui/framework/virtualization/RecyclePool.h`
  - **目标**: 将目前特定控件内写死的虚拟化，升级为全框架通用的回收池架构。
  - **任务要点**:
    - [ ] 支持按控件类名或数据模板类型缓存已离开视口的控件实例。
    - [ ] 提供 `Acquire(type)` 和 `Release(element)`，复用实例只重绑数据（Re-bind），不重新分配内存。

- [ ] **7.2 虚拟化布局面板基类 (`VirtualizingPanel` / `VirtualizingStackPanel`)**
  - **模块位置**: `ui/framework/virtualization/VirtualizingStackPanel.h`
  - **目标**: 将视口裁剪、滚动偏移测算与控件复用池深度融合。
  - **任务要点**:
    - [ ] 仅对当前可见视口范围内的项调用 `Measure` 与 `Arrange`。
    - [ ] 支持固定项高（Fixed Item Height）与动态项高测算缓存（Dynamic Item Height Estimator）。
    - [ ] 支持平滑惯性滚动（Pixel-based smooth scrolling）。

- [ ] **7.3 升级核心列表控件**
  - **模块位置**: `ListView.cpp`, `TreeView.cpp`, `DataGrid.cpp` (待建)
  - **目标**: 重构现有列表，使其挂载 `VirtualizingStackPanel` 即可轻松支持 1,000,000+ 条海量数据秒开与 60/120 FPS 丝滑滚动。

---

### Phase 8: 渲染管线升级与现代合成器架构 (Rendering Pipeline & Compositor)

- [ ] **8.1 绘图指令流录制模式 (`DisplayList` / Command Buffer)**
  - **模块位置**: `ui/framework/render/DisplayList.h`
  - **目标**: 从现有的“即时深度遍历绘制”升级为“保留式指令流录制”。
  - **任务要点**:
    - [ ] 控件 `OnRender` 改为向 `DisplayList` 录制纯数据绘图包（DrawOp: FillRect, DrawText, PushClip 等）。
    - [ ] 节点支持指令流缓存：未发生视觉脏标记的控件子树直接复用上帧录制的指令流，跳过 CPU 运算。
    - [ ] 合并连续的同材质/同画刷绘制指令，实现 GPU 批量提交（Batching）。

- [ ] **8.2 独立合成器线程（Compositor / Render Thread）隔离**
  - **目标**: 解决 UI 主线程耗时业务阻断界面动效的问题。
  - **任务要点**:
    - [ ] UI 主线程负责：事件处理、响应式状态、布局（Measure/Arrange）、录制 DisplayList。
    - [ ] 合成线程负责：驱动动画插值、GPU 光栅化、提交呈现（Present）。主线程计算耗时时，界面 Hover 水波纹与动效仍保持 120 FPS。

- [ ] **8.3 自定义 GPU Shader 特效管线**
  - **目标**: 摆脱仅靠固化 Direct2D Effect 的限制。
  - **任务要点**:
    - [ ] 封装通用自定义像素着色器（Custom Pixel Shader）接口。
    - [ ] 支持开发者注入 HLSL 实现流体亚克力、网格渐变（Mesh Gradient）、动态流光等前沿视觉效果。

---

### Phase 9: 富文本与现代输入法系统 (Typography & TSF IME)

- [ ] **9.1 行内富文本混排树 (`InlineUIContainer` / `Run` / `Span`)**
  - **模块位置**: `ui/framework/text/Inline.h`, `ui/framework/controls/RichTextBlock.h`
  - **目标**: 允许文本段落中内嵌任意 UI 控件或跨字符样式混合。
  - **任务要点**:
    - [ ] 建立 Inline 模型树：`Run`（纯文本）、`Span`（带样式分组）、`InlineUIContainer`（内嵌任意控件）。
    - [ ] 在 `Measure` 阶段保留内嵌控件的基线与占位，在 `Arrange` 阶段将内嵌控件精确定位至对应行与文字基线。

- [ ] **9.2 现代 Windows 文本服务框架（Text Services Framework, TSF）深度集成**
  - **模块位置**: `ui/framework/text/TSFManager.h`, `TextBox.cpp`
  - **目标**: 全面接入 Windows 11 原生输入体验。
  - **任务要点**:
    - [ ] 实现 `ITextStoreACP` 接口以支持原生 TSF。
    - [ ] 支持 Win11 原生表情选择器（Win + .）、剪贴板历史浮窗（Win + V）、语音听写（Win + H）。
    - [ ] 支持触摸屏虚拟键盘呼出与界面动态避让推顶。

- [ ] **9.3 国际化多语言复杂排版与双向文本 (BiDi)**
  - **目标**: 完善阿拉伯语/希伯来语等 RTL 文本的反向排版与断词换行。

---

### Phase 10: 响应式视口断点与转场动效 (Adaptive Layout & Transitions)

- [ ] **10.1 响应式视口断点触发器 (`AdaptiveTrigger` / `Breakpoints`)**
  - **目标**: 界面在窗口缩放时能无缝自适应桌面大屏与小窗紧凑排版。
  - **任务要点**:
    - [ ] 类似 WinUI 3 `AdaptiveTrigger`：可声明在视口跨越断点（如 600px、900px、1200px）时动态应用不同的布局模式与可见性规则。

- [ ] **10.2 共享元素转场与隐式布局动效 (Hero Transitions & Implicit Layout Animations)**
  - **目标**: 跨页面导航或列表重排时消除视觉突变。
  - **任务要点**:
    - [ ] 实现卡片展开至详情页的共享元素缩放（Hero Animation）。
    - [ ] 列表增删项时，未删除的项自动平滑平移至新位置（Animated Layout）。

---

### Phase 11: 开发者工具链与工程化质量保障 (DevTools & DX)

- [ ] **11.1 运行时可视树检查器 (In-App Visual Inspector / "F12 DevTools")**
  - **目标**: 在运行时按快捷键呼出调试面板，极大提升界面排查效率。
  - **任务要点**:
    - [ ] 悬停高亮控件真实 Bounds、Padding、Margin 盒模型。
    - [ ] 树状展示当前整个 UI 元素层级树与实时属性值。
    - [ ] 支持在面板上实时切换亮暗色、修改 Token 颜色并即时查看结果。

- [ ] **11.2 重绘区与性能实时分析仪 (Overdraw & Frame Profiler Overlay)**
  - **目标**: 量化分析界面卡顿与冗余绘制。
  - **任务要点**:
    - [ ] 调试绘制开关：实时使用热力图色彩高亮每帧发生重绘的区域（Overdraw Visualizer）。
    - [ ] 顶部悬浮展示实时 FPS、UI 线程耗时、Render 耗时折线图。

- [ ] **11.3 自动化像素快照回归测试 (Golden Image Testing)**
  - **目标**: 杜绝样式改动意外引发其它页面退化破损。
  - **任务要点**:
    - [ ] 在脱机无头（Headless）环境下为 `CUI.Gallery` 各页面截屏并与基准图片进行像素 Diff 校验。

---

### Phase 12: 无障碍系统与跨平台硬件抽象 (Accessibility & Cross-Platform HAL)

- [ ] **12.1 Windows UI Automation (UIA) 深度接入**
  - **模块位置**: `ui/framework/accessibility/`
  - **目标**: 赋能视障用户读屏体验，并支持行业标准自动化测试工具驱动。
  - **任务要点**:
    - [ ] 各核心控件实现 `IRawElementProviderSimple` 与常用模式（`IInvokeProvider`, `IValueProvider`, `ISelectionProvider`）。
    - [ ] 接入 Windows 讲述人（Narrator），读出焦点所在控件的名称、角色（Role）与状态。
    - [ ] 支持 Appium / WinAppDriver / 自动化测试框架识别控件的 `AutomationId` 并模拟点击与输入。

- [ ] **12.2 跨平台硬件抽象层 (HAL: Hardware Abstraction Layer)**
  - **模块位置**: `ui/framework/hal/`
  - **目标**: 为走出 Windows 生态铺平道路。
  - **任务要点**:
    - [ ] 抽象 `GraphicsBackend` 纯虚接口（支持 D2D、Metal、Vulkan、Skia 等后端替换）。
    - [ ] 抽象 `WindowSystem` 纯虚接口（剥离 HWND 裸系统调用，预留 SDL / GLFW / 原生 Wayland 接入点）。
