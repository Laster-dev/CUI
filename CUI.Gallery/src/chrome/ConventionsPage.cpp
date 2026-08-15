#include "chrome/ConventionsPage.h"

#include "framework/controls/MarkdownView.h"
#include "framework/controls/Panel.h"
#include "framework/controls/ListBox.h"
#include "framework/style/ThemeTokenId.h"

#include <memory>
#include <string>
#include <vector>

using namespace CUI;

namespace Gallery {
namespace {

// 1. 命名规范 (Naming Conventions)
constexpr const char* kNamingConventions = R"markdown(
# 1. 命名规范 (Naming Conventions)

为了保障代码的可读性，避免歧义，所有 C++ 层的类、函数、变量、事件遵循以下统一的命名范式：

## 统一命名规则
*   **类与结构体 (Class & Struct)**: 采用 `PascalCase`（如 `UIElement`, `ButtonFlyoutItem`）。
*   **公共方法与接口 (Public Methods)**: 采用 `PascalCase`（如 `MeasureOverride()`, `SetSelectedIndex()`）。
*   **私有与保护成员变量 (Private/Protected Fields)**: 采用 `m_` 前缀加 `camelCase` 格式（如 `m_selectedIndex`, `m_isDropDownOpen`）。
*   **静态成员变量 (Static Fields)**: 采用 `s_` 前缀加 `camelCase` 格式（如 `s_animationsEnabled`）。
*   **事件 (Events)**: 采用动词加过去式，并以 `Event` 结尾。在属性声明中采用 `PascalCase`（如 `OnThemeChanged`），私有变量中采用 `m_` 前缀且以 `Event` 结尾（如 `m_onSelectionChangedEvent`）。
*   **属性代理 (PropertyRef)**: 采用 `PascalCase`（如 `SelectedIndex`, `ControlValue`）。

## 命名对照表
| 类别 | 格式 | 示例 |
| :--- | :--- | :--- |
| **属性代理** | `PascalCase` | `PropertyRef<int, ...> SelectedIndex;` |
| **外部事件接口** | `On` + 过去式 | `Event<>& OnClosed();` |
| **内部事件源** | `m_` + `on` + 过去式 + `Event` | `Event<> m_onClosedEvent;` |

> [!NOTE]
> 命名规范能够有效提升多人协同开发时的代码清晰度，特别是跨团队或者开源共建场景下，可读性至关重要。
)markdown";

// 2. 生命周期管理 (Lifecycle Management)
constexpr const char* kLifecycle = R"markdown(
# 2. 生命周期管理 (Lifecycle Management)

控件必须具备严格且对称的生命周期管理，以彻底消灭 Windows 应用常见的内存泄漏与悬挂指针：

## 控件创建、初始化与销毁流程
*   **控件创建**: 一律使用 `DSL::Make<T>()` 进行工厂化实例化，返回 `std::shared_ptr<T>`。禁止直接使用裸 `new`。
*   **树的建立 (Attach)**: 子控件通过 `AddChild` 归入父容器。建立树状关系后，自动向上传递 `InvalidateMeasure` 激活布局流。
*   **析构与释放 (Detach)**:
    1.  当控件从排版树中移除（`RemoveChild`）时，其 `m_parent` 被置空。
    2.  如果控件注册了全局动画（`m_animationTicksRegistered`），必须在 `RemoveChild` 或 `OnNavigatedFrom` 生命周期末端同步调用 `CancelAnimationTicks()` 注销时钟，否则动画引擎将永久持有该控件的强引用。
    3.  若控件创建过弹出式解离层（`PopupHost`），须在失去焦点（`OnBlur`）或离开页面（`OnNavigatedFrom`）时，立即清空弹出状态并同步销毁悬浮资源。

## 订阅与解绑机制
*   在对 `Event` 进行 `Connect` 订阅时，若传入的对象生命周期短于被订阅方，必须妥善留存 `Connection` 并在析构函数中执行 `Disconnect()` 解绑，防止悬挂引用崩溃。
*   所有的反应式绑定关系在控件被销毁（`UIElement` 析构）时会自动断开事件订阅。回调不会持有控件的 `shared_ptr`，因此不形成 UI 和数据源的循环引用。
)markdown";

// 3. 属性绑定机制 (Property Binding Mechanism)
constexpr const char* kPropertyBinding = R"markdown(
# 3. 属性绑定机制 (Property Binding Mechanism)

CUI 支持通过 `State<T>` 数据源和 `PropertyRef` 构建数据驱动的 UI 交互：

```cpp
// 示例：双向绑定 CheckBox 选中状态与 Wi-Fi 开关数据源
auto wifi = CheckboxTile("Wi-Fi");
wifi->IsChecked.Bind(stateWifi, BindingMode::TwoWay);
```

## 绑定模式分类
*   **OneWay (单向绑定)**: 数据源 `State<T>` 发生变化时更新 UI 属性，而 UI 的修改不反向修改数据源（适合只读展示）。
*   **TwoWay (双向绑定)**: 数据源变化同步给 UI；同时，UI 的输入交互（如 TextBox 输入、Slider 拖动）也会同步回改数据源。
*   **OneTime (单次绑定)**: 仅在绑定建立时同步一次数据，后续数据源变更不再监听。

## Set 与 Bind 的优先级语义
*   手动调用属性 `Set(value)` 会强制切断当前的单向/双向绑定，使控件回退到硬编码值模式。
*   绑定后的 `Bind()` 拥有最高更新级，任何对绑定属性的交互修改，必须经由 `IsUpdating()` 过滤锁判定，以阻止产生因双向同步而导致的死循环更新。

## 类型转换器的统一接口规范
*   在类型不一致的绑定场景中（如 `float` 数值绑定到 `std::string` 文本显示），必须提供统一的类型转换器接口 `IValueConverter<TSource, TTarget>` 或使用 `MakeConverter(...)` 进行隐式翻译，禁止在 UI 业务方法中手工强转。
)markdown";

// 4. 事件与消息传递 (Events & Message Dispatching)
constexpr const char* kEventsAndMessages = R"markdown(
# 4. 事件与消息传递 (Events & Message Dispatching)

CUI 交互事件不依赖简单的直接回调，而是基于声明式的三级事件路由网：

## 三级路由机制
*   **隧穿 (Tunneling/Preview)**: 事件自最外围的根级窗口（Window）向下逐层检索，探针深入至发生交互的具体叶子节点，便于拦截全局快捷键或拖放事件。
*   **冒泡 (Bubbling)**: 事件从被命中的最内层控件（`HitTest`）向上传播至父容器，直至被某一级处理（如 `args.handled = true`）或到达根窗口。
*   **广播 (Broadcasting)**: 全局系统变更（如系统暗黑模式切换广播 `OnThemeChanged`）自根元素向下进行整棵树的 DFS 广播通知。

## 异步事件处理的一致性
*   在多线程与网络/IO 回调中（如终端 PTY 异步读取写入），严禁直接操作 `UIElement` 的属性和绘图笔刷。
*   必须在回调中将数据写进临界区双缓冲，置脏原子状态量（`std::atomic<bool>`），并向主渲染线程申请主事件队列重绘，在主线程 `OnAnimationTick` 或 `SyncRenderState` 中安全刷新 UI。
)markdown";

// 5. 渲染与刷新策略 (Rendering & Redraw Strategies)
constexpr const char* kRenderingStrategy = R"markdown(
# 5. 渲染与刷新策略 (Rendering & Redraw Strategies)

为保障每秒 60 帧以上的极致流畅度，严禁任何全量全局重绘，一律采用增量局部脏化与合成层提权策略：

## 脏标记机制 (Dirty Flags)
*   **测量脏 (Measure Dirty)**: 当控件文字、内外边距或尺寸改变时，置位 `m_measureDirty = true`。这不会立即触发重绘，而是将该节点推入待处理排版链，在下一帧由 `FlushLayoutIfNeeded` 统一进行自底向上的合并尺寸计算。
*   **排列脏 (Arrange Dirty)**: 控件边界 `m_bounds` 改变时，置位 `m_arrangeDirty = true`，重新排布子项位置。
*   **渲染脏 (Render Dirty)**: 当控件外观需要重绘时（如 Hover 状态变化），调用 `MarkRenderContentDirty()`。通过 `CollectRenderDirtyRegion` 递归提取包含该子树在内的最小脏矩形 `DirtyRegion`，仅在 Direct2D 表面擦除并绘制脏矩形区域，其余未脏区域使用缓存帧拷贝。

## 局部刷新与性能优化
*   **组合图层提升 (Promoted Layer)**: 针对需要高频移动或变透明度的复杂子树，调用 `PromoteLayer(true)` 提权为独立的 `ID2D1Bitmap1` 合成图块。
*   这使得其位置移动或透明度变化只需在 DComp（DirectComposition）合成层重组，无需触发 CPU/GPU 的重新光栅化（Rasterization），极大节省重绘开销。
)markdown";

// 6. 样式与主题系统 (Style & Theme Systems)
constexpr const char* kThemeSystem = R"markdown(
# 6. 样式与主题系统 (Style & Theme Systems)

CUI 框架采用统一的主题字典与资源管理，隔离硬编码数值：

## 统一主题管理与样式继承
*   **资源集中管理**: 所有的色彩值一律登记在 `ThemeManager` 的 `ThemeTokens` 中，控件只管通过 `ThemeTokenId` 获取（如 `ThemeTokenId::AccentColor`），禁止在渲染代码中使用手写 RGB 浮点数。
*   **自动主题响应**: 主窗口监控 Win32 `WM_SETTINGCHANGE` 广播。当操作系统主题在亮/暗色间切换时，`ThemeManager` 会全自动重刷 Token 映射，并通过 `OnThemeChanged` 虚函数递归整棵控件树，使所有已加载的控件平滑重绘渲染。

## 毛玻璃背景自动适配 (Material Role)
*   控件背景色通过 `ResolveThemeColor(token, fallback)` 进行检索。
*   当窗口背景模糊（亚克力/云母）被启用时，`ThemeManager` 会根据控件在布局中的角色级别 `MaterialRole`（Chrome=高透、Surface=中透、Solid=实色）自动调整并重构颜色 Alpha 通道权重，无需控件开发者手动调整。
)markdown";

// 7. 容器与子元素管理 (Containers & Child Management)
constexpr const char* kContainerManagement = R"markdown(
# 7. 容器与子元素管理 (Containers & Child Management)

所有复合容器（`Panel` 派生类、`ScrollViewer`、`TabView` 等）管理子控件时，必须遵循以下结构规范：

## 子元素管理与同步
*   **统一接口操作**: 增删子元素一律通过 `AddChild()` 和 `RemoveChild()`。该方法会自动维护父子树双向指针引用（`m_parent` / `m_children`），并自动向上一级层层置脏 `InvalidateMeasure`。
*   **布局与测量隔离**:
    - 父容器在 `MeasureOverride` 中遍历子控件调用其 `Measure(availableSize)`。
    - 在 `ArrangeOverride` 中计算子项坐标并调用其 `Arrange(finalRect)`。
    - 禁止在非布局生命周期的业务逻辑中手动修改子控件的 `m_bounds`。

## 溢出剪切 (Clipping)
*   若容器开启了 `GetClipToBounds() == true`，在 `Render` 时必须通过 Direct2D 的 `PushAxisAlignedClip` 塞入剪切矩形保护边缘，并在子控件绘制完成后对称执行 `PopAxisAlignedClip`，防止子控件图形画到容器之外。
)markdown";

// 8. 错误与异常处理 (Error & Exception Handling)
constexpr const char* kErrorHandling = R"markdown(
# 8. 错误与异常处理 (Error & Exception Handling)

为防止底层图形或文件句柄故障引起程序闪退，建立起完善的错误恢复与环境差异化约定：

## 健壮度与异常捕获
*   **资源异常安全**: 所有的底层 Direct2D、WIC 或 DirectWrite 接口获取（如 `EnsureNativeIconBitmap`），必须用 `SUCCEEDED` / `FAILED` 宏或 `wrl::ComPtr` 进行空指针防御判定。如果加载失败，允许回退到默认 placeholder 图案，严禁抛出 C++ 未捕获异常导致整个程序崩溃。

## 调试模式与生产模式的差异
*   **调试模式 (Debug Mode)**:
    - 默认开启性能 Overlay 监视器，滚动显示实际 Fps 以及被重绘脏矩形的外框边界（以绿色线框描边表示，辅助检查是否存在过度重绘渲染）。
    - 关键的 PTY 后端通信和资源解码错误会详细输出在 `LogView` 组件中。
*   **生产模式 (Release Mode)**:
    - 自动关闭重绘脏框调试红绿线。
    - 当遇到异常故障（如底层 D3D 设备丢失 `DXGI_ERROR_DEVICE_REMOVED`）时，系统静默在后台重构 `GraphicsContext` 并自动重新解码装载资源，保证 UI 在显卡驱动重置或休眠唤醒后能够 100% 恢复。
)markdown";

} // namespace

Element BuildConventionsPage() {
    // 章节目录定义
    const std::vector<std::string> chapters = {
        "1. 命名规范",
        "2. 生命周期",
        "3. 属性绑定",
        "4. 消息路由",
        "5. 渲染刷新",
        "6. 样式主题",
        "7. 容器管理",
        "8. 容错异常"
    };

    const std::vector<std::string> documents = {
        kNamingConventions,
        kLifecycle,
        kPropertyBinding,
        kEventsAndMessages,
        kRenderingStrategy,
        kThemeSystem,
        kContainerManagement,
        kErrorHandling
    };

    // 左侧章节选择列表
    auto listBox = std::make_shared<ListBox>();
    listBox->Width = 200.0f;
    listBox->Height = -1.0f;
    listBox->Align = Alignment::Stretch;
    listBox->BackgroundToken = ThemeTokenId::PaneBackground;
    listBox->BorderToken = ThemeTokenId::CardBorder;
    listBox->BorderThickness = 1.0f;

    for (const auto& ch : chapters) {
        listBox->AddItem(ch);
    }

    // 右侧 Markdown 文档视图
    auto docView = std::make_shared<MarkdownView>(documents[0]);
    docView->Height = -1.0f;
    docView->FlexGrow = 1.0f;
    docView->Align = Alignment::Stretch;

    // 连接选中修改事件，点击菜单项时动态切换右侧展示的 Markdown 内容
    listBox->OnSelectionChanged().Connect([docView, documents](ListBox*, int index, const std::string&) {
        if (index >= 0 && index < static_cast<int>(documents.size())) {
            docView->SetMarkdown(documents[index]);
        }
    });

    // 默认选中第一章
    listBox->SetSelectedIndex(0);

    // 水平线性布局组装
    auto page = std::make_shared<StackPanel>(Orientation::Horizontal);
    page->Gap = 16.0f;
    page->Padding = Thickness(24.0f);
    page->FlexGrow = 1.0f;
    page->Align = Alignment::Stretch;
    page->BackgroundToken = ThemeTokenId::WindowBackground;
    page->AddChild(listBox);
    page->AddChild(docView);

    return page;
}

} // namespace Gallery
