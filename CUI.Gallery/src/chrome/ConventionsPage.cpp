#include "chrome/ConventionsPage.h"

#include "framework/controls/MarkdownView.h"
#include "framework/controls/Panel.h"
#include "framework/style/ThemeTokenId.h"

#include <memory>
#include <string>

using namespace CUI;

namespace Gallery {
namespace {

constexpr const char* kContents = R"markdown(
# 目录

- [框架架构](#architecture)
- [页面与控件使用](#usage)
  - [文本与样式](#typography)
- [响应式数据绑定](#binding)
  - [派生状态](#computed)
  - [生命周期与优先级](#binding-rules)
  - [类型转换器](#converters)
- [复杂控件模型](#models)
- [全局原则](#principles)
)markdown";

constexpr const char* kDocument = R"markdown(
# CUI 全局约定

本文定义 CUI 的架构边界、控件使用方式，以及响应式数据的统一写法。页面只描述 UI 组合和业务状态；共性行为由 CUI.Core 的控件层负责。

## 框架架构

```text
Window
  └─ UIElement（布局 / 绘制 / 焦点 / 文本样式 / 绑定）
       ├─ Panel（组合与布局）
       └─ Control（交互）→ Button / CheckBox / TextBox / ListView ...
```

- **Window** 管理窗口生命周期、标题栏、主题和宿主内容。
- **UIElement** 是所有可视元素的基类，统一处理布局、绘制失效、焦点、文本样式与绑定。
- **Panel**（例如 `StackPanel`、`Grid`、`ScrollViewer`）负责组合与布局，页面不应手工计算坐标。
- **Control** 在 `UIElement` 之上提供交互能力；具体控件只实现自身的值、视觉与输入逻辑。
- **ThemeTokenId** 描述颜色的视觉角色；页面优先使用主题 token，避免散落硬编码颜色。

## 页面与控件使用

Gallery 和应用页面使用声明式方式：创建控件、设置属性、连接业务事件。可复用或跨控件的行为必须修复在 `CUI.Core`，不应在应用层复制实现。

```cpp
auto save = std::make_shared<Button>("保存");
save->SetTextColor(Color::White);
save->SetFontSize(14.0f);
save->SetFontWeight(FontWeight::Bold);
save->SetFontStyle(FontStyle::Italic);
save->SetIsUnderline(true);
save->SetIsStrikethrough(true);
```

### 文本与样式

所有含文字的控件都继承 `UIElement` 的通用文字能力：`SetTextColor`、`SetFontFamily`、`SetFontSize`、`SetFontWeight`、`SetFontStyle`、`SetFontStretch`、`SetIsUnderline` 和 `SetIsStrikethrough`。

字体和视觉状态必须使用强类型枚举，例如 `FontWeight::Bold`、`FontStyle::Italic`，不能以 `std::string` 传递“加粗”“斜体”等语义。

## 响应式数据绑定

页面业务状态优先由 `State<T>` 持有。直接赋值后，已绑定的控件自动刷新；页面不需要再手工更新控件显示。

`Observable<T>` 仍用于共享数据源、动态模型和 `MakeComputed(...)` 的结果。

```cpp
State<bool> wifiValue{ true };
auto wifi = std::make_shared<CheckBox>("Wi-Fi");
wifi->Checked->Bind(wifiValue);

wifiValue = false; // UI 自动刷新

State<std::string> wifiName{ "Wi-Fi" };
wifi->Text->Bind(wifiName);

State<FontStyle> itemFontStyle{ FontStyle::Italic };
wifi->FontStyle->Bind(itemFontStyle);
```

- 基本状态使用 `State<bool>`、`State<int>`、`State<std::string>` 等，并以 `state = value` 直接更新。
- 每个属性通过 `属性->Bind(...)` 绑定；例如 `Checked`、`Text`、`FontStyle`。
- 双向绑定和回写循环防护属于控件职责，页面只表达业务关系。

### 派生状态

多个状态推导一个显示结果时使用 `MakeComputed<T>(...)`，让派生值也保持单一来源。

```cpp
auto allSelected = MakeComputed<CheckState>(
    [wifiValue, bluetoothValue, airplaneValue] {
        const int selected = static_cast<int>(wifiValue.Get())
            + static_cast<int>(bluetoothValue.Get())
            + static_cast<int>(airplaneValue.Get());
        return selected == 3 ? CheckState::Checked
             : selected == 0 ? CheckState::Unchecked
                             : CheckState::Indeterminate;
    }, wifiValue, bluetoothValue, airplaneValue);
selectAll->State->Bind(allSelected, BindingMode::OneWay);
```

### 生命周期与优先级

`Bind(...)` 创建的是属性和 `Observable` 之间可自动解绑的监听关系。属性绑定槽按需创建：未调用 `属性->Bind(...)`、`属性->Set(...)` 或 `属性->Get(...)` 的控件不会为该属性分配绑定对象。

- `BindingMode::OneWay`：数据源驱动 UI。`属性->Set(...)` 返回 `false`，不会短暂覆盖数据源的显示。
- `BindingMode::TwoWay`：数据源和 UI 双向同步。`属性->Set(...)` 会更新控件，控件属性变更再回写绑定的 `Observable`。
- `BindingMode::OneTime`：仅在绑定时同步一次；之后 `属性->Set(...)` 可直接更新本地 UI。
- `SetXxx(...)` 是控件底层属性设置 API；业务层在已绑定属性上应使用 `属性->Set(...)` 或直接修改 `Observable`，避免绕开绑定语义。
- `Observable::Set(...)` 会跳过相等值，不重复通知；每条绑定在从源更新 UI 时也会抑制自己的回写，避免常见的双向回环。
- 绑定对象随控件销毁自动断开事件订阅。回调不会持有控件的 `shared_ptr`，因此不形成 UI 和数据源的循环引用。

### 类型转换器

不同类型之间不要在页面事件中手工同步。使用 `IValueConverter<TSource, TTarget>` 或 `MakeConverter(...)` 把转换规则放在绑定边界；双向转换中 `ConvertBack(...)` 返回 `std::nullopt` 表示当前输入无效，不修改数据源。

```cpp
State<int> count{ 12 };
auto numberText = MakeConverter<int, std::string>(
    [](int value) { return std::to_string(value); },
    [](const std::string& text) -> std::optional<int> {
        try { return std::stoi(text); }
        catch (...) { return std::nullopt; }
    });

textBox->Text->Bind(count, numberText, BindingMode::TwoWay);
```

只需要读取转换后的值时，也可以使用 `ConvertObservable<TTarget>(source, converter)` 得到派生 `Observable`，再按单向方式绑定。

## 复杂控件模型
复杂控件同样必须拥有明确的数据模型，模型是唯一事实来源，不要把数据散落在页面事件或控件视觉状态中。

| 控件 | 推荐状态模型 |
| --- | --- |
| `ListView` / `ListBox` | `Observable<ListModel>` 或 `Observable<std::vector<ListItemModel>>`，描述条目、选择和排序 |
| `Image` | `Observable<ImageSource>` 或专用图片模型，描述路径、位图、加载状态和错误 |
| 三态 `CheckBox` | 多个 `Observable<bool>` 推导出的 `Observable<CheckState>` |

控件需要公开强类型模型，并在模型改变时自动请求布局或绘制失效。

## 全局原则

1. **状态单一来源**：业务值放入 `Observable` 或控件模型，UI 视觉状态不是数据来源。
2. **强类型优先**：颜色、字体、选择状态、图片和列表数据都必须有明确 C++ 类型。
3. **基类共享**：焦点、文本样式、文字色、可见性、禁用状态等通用能力在 `UIElement` 提供。
4. **按需失效**：状态变化后请求布局或重绘，逻辑状态与视觉状态必须同步。
5. **主题一致**：默认使用 `ThemeTokenId`，确保亮暗主题和后续主题扩展自动生效。
)markdown";

const std::string& HeadingForAnchor(const std::string& href) {
    static const std::string empty;
    static const std::string architecture = "框架架构";
    static const std::string usage = "页面与控件使用";
    static const std::string typography = "文本与样式";
    static const std::string binding = "响应式数据绑定";
    static const std::string computed = "派生状态";
    static const std::string bindingRules = "生命周期与优先级";
    static const std::string converters = "类型转换器";
    static const std::string models = "复杂控件模型";
    static const std::string principles = "全局原则";

    if (href == "#architecture") return architecture;
    if (href == "#usage") return usage;
    if (href == "#typography") return typography;
    if (href == "#binding") return binding;
    if (href == "#computed") return computed;
    if (href == "#binding-rules") return bindingRules;
    if (href == "#converters") return converters;
    if (href == "#models") return models;
    if (href == "#principles") return principles;
    return empty;
}

} // namespace

std::shared_ptr<UIElement> BuildConventionsPage() {
    auto contents = std::make_shared<MarkdownView>(kContents);
    contents->SetWidth(220.0f);
    contents->SetHeight(-1.0f);
    contents->SetPadding(Thickness(4.0f));
    contents->SetBackgroundToken(ThemeTokenId::PaneBackground);
    contents->SetBorderToken(ThemeTokenId::CardBorder);
    contents->SetAlign(Alignment::Stretch);

    auto document = std::make_shared<MarkdownView>(kDocument);
    document->SetHeight(-1.0f);
    document->SetFlexGrow(1.0f);
    document->SetAlign(Alignment::Stretch);

    contents->OnLinkClicked().Connect([document](MarkdownView*, const std::string& href) {
        const std::string& heading = HeadingForAnchor(href);
        if (!heading.empty()) {
            document->ScrollToHeading(heading);
        }
    });

    auto page = std::make_shared<StackPanel>(Orientation::Horizontal);
    page->SetGap(16.0f);
    page->SetPadding(Thickness(24.0f));
    page->SetFlexGrow(1.0f);
    page->SetAlign(Alignment::Stretch);
    page->SetBackgroundToken(ThemeTokenId::WindowBackground);
    page->AddChild(contents);
    page->AddChild(document);
    return page;
}

} // namespace Gallery
