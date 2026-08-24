# 统一 Fluent Builder 页面 API 规范

**状态：** 已确认并执行
**适用范围：** `CUI.Core`、`CUI.Gallery`、`Demo`、`Calc`、`RegeditPlus`、`EverythingNEO`、`CUI/showcase`

## 唯一作者入口

页面和业务代码只能使用 `CUI::DSL::Fluent` 或现有 Fluent Builder 工厂创建、配置控件：

```cpp
auto root = Fluent::StackPanel()
    .Padding(16)
    .Children({
        Fluent::TextBlock("标题").FontSize(20).Build(),
        Fluent::Button("保存")
            .IsEnabled(true)
            .ForegroundToken(ThemeTokenId::AccentForeground)
            .OnClick(Save)
            .Build()
    })
    .Build();
```

控件创建完成后，如需运行时更新，必须从已有句柄重新进入 Builder：

```cpp
ElementBuilder<TextBlock>(label).Text("已保存");
```

## 命名规则

- 属性使用单一规范名称：`Width`、`Height`、`Text`、`Foreground`、`IsEnabled`。
- 事件使用 `OnXxx(...)`：`OnClick`、`OnTextChanged`、`OnSelectionChanged`。
- 集合使用复数名词或明确动作：`Children(...)`、`Items(...)`、`AddChild(...)`、`AddItem(...)`。
- 只有动作保留方法：`Show()`、`Hide()`、`Navigate()`、`Focus()`、`RemoveChild()`。
- 删除并禁止新增 `GetX`、`SetX`、`Color(...)`、`ColorToken(...)`、`Hover(...)`、`Pressed(...)`、`Enabled(...)` 等作者级入口和别名。
- 规范名称：`ForegroundToken`、`HoverBackground`、`PressedBackground`、`IsEnabled`。
- `Size(width, height)`、`Margin(...)`、`Padding(...)` 是组合便利方法，不产生新的属性概念。

## 禁止写法

```cpp
button->SetWidth(120);
button->SetText("保存");
button->AddChild(child);
auto button = std::make_shared<Button>();
button->SetOnClick(handler);
```

上述接口不得出现在页面、示例和业务代码中。底层控件可保留私有/受保护实现辅助函数，但不得作为作者 API 暴露。

## 构建器约束

- Builder 配置方法返回当前 `ElementBuilder<T>&`，必须支持继续链式调用。
- Builder 的 `.Build()` 返回稳定控件句柄。
- 页面层不得依赖 Builder 的 `operator->()`、`get()` 等逃生接口。
- 事件配置替换当前回调；传入空回调清除回调。
- 属性和集合写入必须触发布局、渲染、命中测试或数据视图所需的失效刷新。
- 旧 `*Widget`、`*Tile`、`*Field` 工厂在迁移完成后删除；迁移期间不得新增使用。

## 验收

- 静态搜索不出现作者级 `GetX/SetX/AddX` 配置调用。
- 静态搜索不出现已删除 Fluent 别名。
- 页面不直接 `std::make_shared<UIElement 派生控件>`。
- `CUI.Core`、Gallery、Demo 和业务项目使用同一套 Fluent Builder 重新编译通过。