---
layout: page
title: PasswordBox（密码框）
parent: 控件参考
nav_order: 11
---

# PasswordBox（密码框）

> 遮罩显示的密码输入控件。它是 `TextBox` 的薄封装：构造时把 `IsPasswordMode` 与 `ShowRevealButton` 打开、给一个中文占位符，并额外提供 `Password` 明文读写代理。编辑、选区、撤销重做、右键菜单、IME、拖放等行为全部沿用 [TextBox](TextBox.html)。

## 一、控件定位

| 场景 | 是否适用 |
|---|---|
| 让用户输入密码并默认遮罩 | ✅ 首选 |
| 需要"显示密码"眼睛按钮 | ✅ 内置（`ShowRevealButton` 默认 `true`） |
| 普通可见文本输入 | ❌ 用 [TextBox](TextBox.html) |
| 需要 PIN 分格输入 / 数字软键盘 | ❌ 需自行派生 |
| 需要密码强度实时提示 | ✅ 派生后监听 `OnTextChanged()` |

## 二、类信息

| 项 | 值 |
|---|---|
| 头文件 | `ui/framework/controls/PasswordBox.h` |
| 声明 | `class PasswordBox : public TextBox` |
| 继承链 | `UIElement` → `Control` → `TextBox` → `PasswordBox` |
| 命名空间 | `CUI` |
| DSL 工厂 | `PasswordBoxWidget(placeholder = "请输入密码")` |
| 通用工厂 | `DSL::Control<PasswordBox>(args...)` |
| Tab 焦点 | ✅（继承 `TextBox::AcceptsTabFocus()` → `true`） |
| 鼠标指针 | `IDC_IBEAM`；指针落在眼睛按钮上时 `IDC_HAND` |

## 三、构造函数与出厂默认值

```cpp
PasswordBox();                                        // 占位符 "请输入密码"
explicit PasswordBox(const std::string& placeholder); // 自定义占位符
```

构造函数体（源码 `PasswordBox.cpp:6-16`）：

| 项 | 默认值 |
|---|---|
| 基类 | `TextBox("请输入密码")` |
| `IsPasswordMode` | `true` |
| `ShowRevealButton` | `true` |

`PasswordBox` **没有新增成员变量**，其余出厂值（透明底、下划线 Token、字体 `微软雅黑 12`、Padding `(8,6,8,6)`、`MinHeight 32`、光标 `1.5f / 500ms` 等）全部来自 `TextBox` 构造函数，见 [TextBox](TextBox.html) 第三节。

## 四、属性

### 4.1 自身属性

| 属性 | 类型 | 读 | 写 | 默认值 | 说明 |
|---|---|:-:|:-:|---|---|
| `Password` | `std::string` | `GetPassword()` / 代理 `Password` | `SetPassword()` / `Password = "..."` / DSL `.Password("...")` | `""` | **明文**密码，实现就是 `GetText()` / `SetText()`（`PasswordBox.h:29-30`） |
| `IsPasswordMode` | `bool` | `GetIsPasswordMode()` | `SetIsPasswordMode()` | `true`（构造时强制置位） | 关闭后即退化成普通 `TextBox` |
| `ShowRevealButton` | `bool` | `GetShowRevealButton()` | `SetShowRevealButton()` / DSL `.ShowRevealButton(bool)` | `true` | 是否绘制右侧眼睛按钮 |
| `IsPasswordRevealed` | `bool` | `GetIsPasswordRevealed()` | `SetIsPasswordRevealed()` / `SetProperty(PropertyId::IsPasswordRevealed, ...)` | `false` | 临时明文显示；**无 DSL 链式方法** |

### 4.2 继承自 `TextBox` 的常用属性

完整表格见 [TextBox](TextBox.html) 4.1 / 4.2。密码框上最常改的：

| 属性 | 说明 |
|---|---|
| `Placeholder` | 水印提示；高度 ≥ `40` 且非多行时会变成浮动标签 |
| `ForegroundToken` / `BackgroundToken` | 文本与底色，默认 `TextPrimary` / 透明 |
| `UnderlineColorToken` / `ActiveUnderlineColorToken` | 静止 / 聚焦下划线 |
| `CaretColorToken` | 光标颜色 |
| `Width` / `Height` / `MinHeight` | 尺寸；`Measure` 忽略 `availableSize`，不给宽度会随内容跳变 |
| `IsEnabled` | 禁用后不响应任何输入 |

## 五、方法

| 方法 | 说明 |
|---|---|
| `const char* GetClassName() const override` | 返回 `"PasswordBox"` |
| `GetProperty / HasProperty / SetProperty` | 新增 `IsPasswordRevealed`、`ShowRevealButton` 两个 id，其余转交 `TextBox` |
| `std::string GetPassword() const` | 等价于 `GetText()`，返回明文 |
| `void SetPassword(const std::string&)` | 等价于 `SetText()`，会触发 `OnTextChanged()` |

继承来的方法全部可用：`SetText/GetText`、`SetPlaceholder/GetPlaceholder`、`SelectAll()`、`HasSelection()`、`DeleteSelection()`、`GetDisplayedText()`、`OnTextChanged()` 等，详见 [TextBox](TextBox.html) 第五节。

## 六、运行时行为

`PasswordBox` 自身没有覆写任何绘制或输入逻辑，行为就是"`IsPasswordMode == true` 的 `TextBox`"：

- **遮罩**：`GetDisplayedText()` 在未揭示时返回 `std::wstring(明文长度, L'•')`（U+2022）。绘制、命中测试、光标定位都基于这份遮罩串。
- **眼睛按钮**：`GetRevealButtonRect()` = `24×24`，距右边缘 `6.0f`，垂直居中。点击切换 `IsPasswordRevealed`（`TextBox.cpp:864-867`），条件为 `IsPasswordMode && !IsReadOnly && ShowRevealButton`——**只读时点眼睛无效**。
- **文本区**：密码模式且显示按钮时，`GetTextRect()` 右侧额外让出 `32.0f`。
- **指针**：按钮矩形内为 `IDC_HAND`，其余为 `IDC_IBEAM`，禁用时 `nullptr`。
- **键盘 / 右键菜单 / 撤销重做 / IME / 拖放**：与 `TextBox` 完全一致。
- **剪贴板是明文**：复制 / 剪切写的是 `GetText()`。

## 七、事件

| 事件 | 类型 | 回调签名 | 触发时机 | 注册方式 |
|---|---|---|---|---|
| `OnTextChanged()` | `Event<TextBox*, const std::string&>&` | `void(TextBox* sender, const std::string& text)` | 密码内容（明文）实际变化时（`TextBox.cpp:173`） | `pwd->OnTextChanged().Connect(fn);` |
| `OnClick` | `CallbackProperty<void(UIElement*)>` | `void(UIElement*)` | 继承自 `UIElement`，**不主动触发** | `pwd->OnClick = fn;` |

`sender` 的静态类型是 `TextBox*`，需要时转回来：

```cpp
pwd->OnTextChanged().Connect([](TextBox* sender, const std::string& pwd) {
    auto* box = static_cast<PasswordBox*>(sender);
});
```

## 八、示例

### 8.1 DSL 最简写法

```cpp
#include "CUI.h"

auto pwd = PasswordBoxWidget("请输入密码")
               .Width(240.0f)
               .Build();
```

### 8.2 代码式创建 + 预填与读取

```cpp
auto pwd = std::make_shared<PasswordBox>("Password");
pwd->SetWidth(240.0f);
pwd->Password = "p@ssw0rd";                 // 代理写法，等价于 SetText
std::string plain = pwd->GetPassword();      // 明文
```

### 8.3 登录表单（完整可运行）

```cpp
#include "CUI.h"

int main() {
    auto tip  = Text("请输入口令").FontSize(12.0f);
    auto pwd  = PasswordBoxWidget("请输入密码").Width(240.0f).Build();
    auto btn  = ElevatedButton("登录").Width(240.0f).Build();

    btn->OnClick = [pwd, tip](UIElement*) {
        Borrow(tip).Text(pwd->GetPassword().empty()
                             ? "密码不能为空"
                             : "已提交，长度 " + std::to_string(pwd->GetPassword().size()));
    };

    auto root = Column(10, { tip, pwd, btn }).Align(Alignment::Center).Build();

    Window w;
    w.Fluent().Title("PasswordBox Demo").Size(420, 260).Root(root).Build().Show().Run();
}
```

### 8.4 关闭眼睛按钮

```cpp
auto pwd = std::make_shared<PasswordBox>("Passphrase");
pwd->SetShowRevealButton(false);   // 不画眼睛，也就不允许明文切换
```

### 8.5 实时强度提示

```cpp
pwd->OnTextChanged().Connect([tip](TextBox*, const std::string& text) {
    Borrow(tip).Text(text.size() >= 8 ? "强度：足够" : "强度：至少 8 位");
});
```

## 九、主题与自定义外观

`PasswordBox` 没有自己的绘制代码，外观改动全部落在 `TextBox` 的属性 / Token 上：

| 想改的东西 | 改哪个 |
|---|---|
| 占位符文案 | 构造函数实参或 `Placeholder(...)` |
| 占位符 / 浮动标签颜色 | `PlaceholderColorToken(...)` |
| 输入框底色、下划线 | `BackgroundToken(...)`、`UnderlineColorToken(...)` |
| 变成方框 | `BorderThickness(1.0f)` + `BorderToken(ThemeTokenId::InputBorder)` |
| 眼睛按钮 | `SetShowRevealButton(false)`（颜色取自 `ThemeTokenId::TextMuted`，不可改） |
| 遮罩字符 | 覆写 `virtual std::wstring GetDisplayedText() const` |

```cpp
class StarPasswordBox : public PasswordBox {
public:
    std::wstring GetDisplayedText() const override {
        return std::wstring(GetText().length(), L'*');
    }
    const char* GetClassName() const override { return "StarPasswordBox"; }
};
```

## 十、注意事项

1. **薄封装**：`PasswordBox` 只在构造时打开两个开关并给默认占位符，没有任何渲染 / 输入逻辑。排查行为问题请直接看 [TextBox](TextBox.html)。
2. **明文到处都是**：`GetText()`、`Password`、`OnTextChanged` 回调参数、撤销栈快照里存的都是明文——**不要打日志、不要埋点上报**。
3. **复制 / 剪切导出明文**：遮罩只作用于屏幕显示。
4. **`IsPasswordRevealed` 无 DSL 链式方法**，只能 `SetIsPasswordRevealed(true)`。
5. **只读模式下眼睛按钮失效**。
6. **默认占位符是中文** `"请输入密码"`，多语言界面请显式传参。
7. **宽度仍随内容跳变**：`Measure` 按遮罩文本测量，表单场景请固定 `Width`。
8. **密码模式会压缩文本区**：右侧固定留 `32.0f` 给按钮。
