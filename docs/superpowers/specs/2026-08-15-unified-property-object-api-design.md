# 统一属性对象与全量绑定 API 设计

## 目标

将 CUI 的公开属性 API 统一为可赋值、可读取、可绑定的属性对象。业务代码不再依赖 `GetX()/SetX()` 或 `GetOrCreatePropertyBinding<T, Id>()`；所有已注册的可绑定属性都必须以一致、强类型的方式使用。

## 公开 API

每个属性使用 `Property<T, Id>` 对象，并支持：

```cpp
button->Width = 35.0f;
float width = button->Width;

State<float> widthState{ 35.0f };
button->Width.Bind(widthState, BindingMode::TwoWay);
```

普通变量只用于读取或赋值；响应式绑定必须使用 `State<T>` 或 `Observable<T>`，以提供变更通知。

## 覆盖范围

- 覆盖全部 `PropertyId`，包括 UIElement 通用属性、布局属性、Canvas/Grid/Dock 附加属性，以及各控件专属属性。
- 保留现有 `Text`、`Width`、`ValueProperty` 等代理名称作为兼容的快捷 API。
- `GetX()/SetX()` 和属性桥接继续保留为框架内部、反射、布局与序列化的实现边界；新业务示例不直接调用它们。

## 实现结构

1. 建立单一属性清单，至少描述属性 ID、C++ 值类型、公开成员名、所属控件类型和桥接 getter/setter。
2. 由该清单生成或验证 PropertyId 名称表、属性描述、类型化属性对象声明和初始化，避免手工清单失配。
3. 属性对象的赋值操作调用现有 `SetProperty`/专用 setter，读取操作调用现有 `GetProperty`/专用 getter；`.Bind()` 复用 `BindableProperty` 的现有双向保护逻辑。
4. 对无法安全提供强类型公开代理的动态/集合属性，提供等价的强类型集合属性对象，而不是回退为字符串属性名。

## 迁移

- 先建立通用属性对象基础设施和 UIElement/布局/附加属性代理。
- 再按控件类别补齐控件专属属性，并为每个类别增加编译期覆盖检查。
- 将 Gallery 中命令式的“事件 + SetX”演示迁移为 `State + Bind`，以验证 API 的实际可用性。
- 旧 API 保持可用，不做破坏性删除。

## 验收标准

- 任一已注册 `PropertyId` 都有一条强类型的、无字符串名称的绑定路径。
- 常用属性满足 `element->PropertyName = value`、`T value = element->PropertyName` 与 `element->PropertyName.Bind(state)`。
- 双向绑定不产生循环更新；布局、绘制和命中相关属性仍触发原有失效机制。
- CUI.Core 和 CUI.Gallery 的相关示例能够编译；Gallery 的绑定示例不再需要事件回调中调用 `SetGap`。
