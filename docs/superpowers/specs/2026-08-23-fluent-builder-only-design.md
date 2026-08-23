# 全仓库 Fluent Builder API 统一重构设计

**日期：** 2026-08-23  
**状态：** 已确认设计，待进入实施计划  
**范围：** `CUI.Core`、`CUI.Gallery`、`Demo`、`Calc`、`RegeditPlus`、`EverythingNEO`、`CUI/showcase`

## 1. 背景与目标

当前仓库同时存在核心控件 `GetX/SetX/AddX` API、直接指针调用、属性对象写法、`DSL::Make<T>()`、`ElementBuilder<T>` 和部分 `Fluent` 工厂，导致同一 UI 概念有多套入口，别名与命名冲突也使页面代码不一致。

本次按破坏兼容的大版本重构，建立唯一作者级 UI 编写方式：**`CUI::DSL::Fluent` 的 Fluent Builder 链式 API**。所有页面、示例和业务工程必须迁移到该 API；旧的非链式作者 API 删除，不保留过渡兼容层。

目标：

- 统一控件创建、属性配置、事件绑定、集合构建和动作调用的表达形式。
- 消除公开的 `GetX/SetX`、直接指针式属性修改和会诱导非链式写法的 Builder 辅助入口。
- 消除 Fluent 层的冗余别名，确保每个 UI 概念只有一个规范名称。
- 保证属性变更继续正确触发布局、渲染、命中测试和数据视图刷新。
- 让核心库和所有仓库内调用方在同一套 API 下重新编译通过。

## 2. 非目标

- 不重写控件内部的绘制、布局算法或业务逻辑。
- 不改变已有控件的用户可见行为，除非该行为依赖旧 API 的错误刷新或生命周期语义。
- 不为外部旧项目保留 ABI/API 兼容层。
- 不把所有内部实现函数机械改名；私有实现可以继续使用内部属性访问器。
- 不引入代码生成系统作为本次重构的前置条件。

## 3. 唯一公共 API 形态

页面作者只能通过 `CUI::DSL::Fluent` 创建和配置控件：

```cpp
auto page = Fluent::StackPanel()
    .Orientation(Orientation::Vertical)
    .Padding(16)
    .Children({
        Fluent::TextBlock("标题")
            .FontSize(20)
            .Foreground(Colors::White),
        Fluent::Button("保存")
            .IsEnabled(canSave)
            .OnClick(Save)
    })
    .Build();
```

Builder 的基础约束：

- 属性配置方法返回当前 Builder 的 `ElementBuilder<T>&`，支持继续链式调用。
- 事件使用 `.OnXxx(handler)`，新 handler 替换旧 handler；传入空 handler 清除回调。
- 集合使用 `.Children(...)`、`.Items(...)`、`.Rows(...)`、`.Columns(...)` 等整体配置方法；增删结构使用明确动词方法。
- 动作用明确动词表达，例如 `.AddChild(...)`、`.AddItem(...)`、`.RemoveItem(...)`。
- `.Build()` 是链式构建终点，返回稳定的控件句柄；控件句柄仅用于保存、传递、注册和框架交互，不作为页面属性配置入口。
- Builder 不提供 `operator->()`、`get()` 等可直接回到底层对象的作者级逃生入口。
- `std::make_shared<T>()`、裸 `Make<T>()` 和直接控件构造不作为页面 API；仅可在 Fluent 实现或框架内部使用。

## 4. 命名规范与别名清理

每个概念只保留一个规范名称。以下别名必须删除并迁移到规范名称：

| 旧别名 | 规范名称 | 说明 |
|---|---|---|
| `Color(...)` | `Foreground(...)` | 文本/前景颜色统一使用 `Foreground` |
| `Hover(...)` | `HoverBackground(...)` | 悬浮态背景统一使用完整语义名 |
| `Pressed(...)` | `PressedBackground(...)` | 按下态背景统一使用完整语义名 |
| `Enabled(...)` | `IsEnabled(...)` | 布尔状态统一 `Is` 前缀 |

以下组合方法可以保留，但不得产生第二套属性概念：

- `Size(width, height)` 仅作为 `Width` + `Height` 的便利组合方法。
- `Margin(...)`、`Padding(...)` 支持均匀值和四边值重载，但名称唯一。
- 颜色 Token 和具体颜色使用同一语义名称的重载，不再保留旧的 `SetColor`/`SetColorToken` 作者入口。

公共命名规则：

- 普通属性：名词或状态名，例如 `Width`、`Text`、`Foreground`、`IsEnabled`。
- 回调：`On` + 事件名，例如 `OnClick`、`OnTextChanged`。
- 集合：复数名词，例如 `Children`、`Items`、`Rows`、`Columns`。
- 动作：动词，例如 `Show`、`Hide`、`Navigate`、`AddChild`、`RemoveChild`、`Focus`。
- 不再新增 `GetX`、`SetX`、`EnableX` 与同一概念的缩写别名。

## 5. 核心层重构

### 5.1 Builder 与属性系统解耦

`ElementBuilder<T>` 不再通过公开 `SetX/GetX` 实现属性配置。Builder 应调用核心内部统一属性写入路径，确保：

- 属性元数据集中定义变更影响。
- 写入后统一执行失效与刷新调度。
- 绑定、值写入和初始化写入遵循一致语义。
- Builder 只暴露作者需要的规范 API，不暴露内部属性桥接细节。

### 5.2 控件公共接口收缩

删除控件头文件中面向作者的 `GetX/SetX`、直接事件注册器和仅为旧写法服务的别名。保留或新增的公共方法仅限：

- Fluent Builder 所需的内部受控入口。
- 真正的动作方法，例如 `Show()`、`Hide()`、`Navigate()`、`Focus()`。
- 框架生命周期、渲染、布局和平台适配所需的受保护/私有接口。

读写状态不再通过页面直接访问控件本体完成；页面应在构建阶段通过 Builder 配置。需要运行时状态时，使用现有状态对象、事件回调或明确的控件动作接口，并在迁移中保持行为等价。

### 5.3 句柄与生命周期

Builder 内部继续使用 `std::shared_ptr` 管理控件生命周期。对作者公开的结果句柄必须支持：

- 保存到局部变量或成员变量。
- 传给父级集合和窗口根节点。
- 与框架需要的控件句柄接口交互。
- 不允许通过句柄重新走 `->SetX`、`->GetX` 或直接属性赋值配置。

## 6. 全仓库迁移策略

按依赖顺序分阶段执行：

1. **建立规范 Fluent 基础层**：整理 `CUIDsl.h` 的 Builder 基础能力、统一工厂和规范方法，删除别名及逃生入口。
2. **迁移核心控件**：逐个控件把公开 `Get/Set` 配置路径收敛到 Builder 所需的内部入口，保留必要动作和框架内部接口。
3. **迁移 Gallery 与展示代码**：先迁移页面工厂和公共页面辅助函数，再迁移各页面、Shell 和 showcase。
4. **迁移业务工程**：迁移 `Demo`、`Calc`、`RegeditPlus`、`EverythingNEO` 的窗口、控件组装和运行时配置。
5. **删除旧 API 与脚本收口**：删除或改造会生成非链式代码的迁移脚本；更新文档、示例和代码审计规则。
6. **全量验证**：执行解决方案构建、静态搜索和关键页面运行验证，确保仓库中不存在作者级非链式入口或别名调用。

机械替换脚本只能用于初步迁移，不能直接处理：

- 多语句配置块到单条链的重组。
- `GetX()` 返回值参与条件、计算或生命周期控制的情况。
- 集合增删与事件多播语义。
- 同名别名的语义判定。
- 需要保存中间句柄的复杂 UI 树。

## 7. 验收标准

### API 静态验收

- 作者代码中不存在 `GetX()`、`SetX(...)`、`->AddX(...)` 等旧配置调用。
- 作者代码中不存在 `Color(...)`、`Hover(...)`、`Pressed(...)`、`Enabled(...)` 等已删除别名。
- 页面层不存在直接 `std::make_shared<控件>`、裸 `Make<T>()` 或 `operator->()` 逃生式配置。
- 所有 Fluent Builder 配置方法命名唯一、返回类型一致、可继续链式调用。

### 编译验收

- `CUI.Core`、`CUI.Gallery`、`Demo`、`Calc`、`RegeditPlus`、`EverythingNEO` 和 `CUI/showcase` 全部重新编译通过。
- 不允许通过添加兼容重载或宏重新引入旧 API。

### 行为验收

- 控件初始属性与重构前一致。
- 事件触发、事件替换/清除和生命周期行为一致。
- `Children`、`Items`、表格行列等集合更新正确触发布局和视图刷新。
- 运行时属性改变仍能在同一 UI 更新周期正确失效和重绘。

## 8. 风险与控制

- **范围大、编译反馈晚**：按核心层、Gallery、业务工程分批提交和构建。
- **同名方法语义不一致**：先建立旧 API 到规范 Fluent 名称的显式映射表，禁止盲替换。
- **运行时句柄需求**：区分“构建配置句柄”和“运行时动作句柄”，必要时设计明确的动作 API，而不是恢复属性 setter。
- **内部/公开边界混淆**：通过访问控制、命名空间和静态搜索共同验证，防止旧入口以辅助函数形式泄漏。
- **文档与脚本继续传播旧写法**：在删除旧 API 的同时更新 README、规范文档、迁移工具和示例字符串。

## 9. 交付物

- 统一后的 `CUI::DSL::Fluent` Builder 基础层和控件 Builder API。
- 全仓库迁移后的页面与业务代码。
- 删除后的旧作者级 `Get/Set` 与别名接口。
- 更新后的 API 规范、README、迁移脚本和静态审计规则。
- 全量构建与静态审计结果。