#pragma once

#include "../controls/UIElement.h"
#include "../controls/Panel.h"
#include "../controls/Button.h"
#include "../controls/ToggleButton.h"
#include "../controls/DropDownButton.h"
#include "../controls/SplitButton.h"
#include "../controls/TextBox.h"
#include "../controls/PasswordBox.h"
#include "../controls/TextBlock.h"
#include "../controls/CheckBox.h"
#include "../controls/HyperlinkButton.h"
#include "../controls/ComboBox.h"
#include "../controls/SegmentedControl.h"
#include "../controls/ListBox.h"
#include "../controls/ListView.h"
#include "../controls/Image.h"
#include "../controls/ScrollViewer.h"
#include "../controls/TabView.h"
#include "../controls/MenuBar.h"
#include "../controls/TreeView.h"
#include "../controls/Slider.h"
#include "../controls/RangeSlider.h"
#include "../controls/NumberBox.h"
#include "../controls/RadioButton.h"
#include "../controls/ToggleSwitch.h"
#include "../controls/DatePicker.h"
#include "../controls/TimePicker.h"
#include "../controls/ColorPicker.h"
#include "../controls/BreadcrumbBar.h"
#include "../controls/ProgressBar.h"
#include "../controls/ProgressRing.h"
#include "../controls/AutoSuggestBox.h"
#include "../controls/StatusBar.h"
#include "../controls/RatingControl.h"
#include "../controls/TeachingTip.h"
#include "../controls/chart/Chart.h"
#include "../controls/MarkdownView.h"
#include "../controls/LogView.h"
#include "../controls/InfoBar.h"
#include "../controls/CommandBar.h"
#include "../dnd/DragDropService.h"
#include "../controls/FilePicker.h"
#include "../controls/FolderPicker.h"
#include "../controls/PagingControl.h"
#include "../controls/Splitter.h"
#include "../controls/Expander.h"
#include "../controls/Flyout.h"
#include "../controls/MessageBox.h"
#include "../controls/shapes/Shapes.h"
#include "../controls/CanvasControl.h"
#include "../controls/topology/TopologyView.h"
#include "../style/ThemeManager.h"

#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <initializer_list>
#include <functional>

namespace CUI {
class Window;
namespace DSL {

/**
 * @brief 实例化 UI 控件的工厂包装模板。
 * @tparam T 继承自 UIElement 的具体控件类型。
 * @tparam Args 构造函数参数列表。
 */
template<typename T, typename... Args>
std::shared_ptr<T> Make(Args&&... args) { // 实例化 UI 控件并返回智能指针
    static_assert(std::is_base_of_v<UIElement, T>, "DSL::Make only creates UIElement types.");
    return std::make_shared<T>(std::forward<Args>(args)...);
}

/**
 * @brief 将 RGB 十六进制整数格式颜色转换为 Direct2D 所需的 D2D1_COLOR_F 结构。
 * @param rgb 十六进制颜色代码。
 * @param alpha 透明度等级。
 */
inline D2D1_COLOR_F Rgb(unsigned int rgb, float alpha = 1.0f) { // 十六进制 RGB 颜色值转换
    return D2D1::ColorF(
        static_cast<float>((rgb >> 16) & 0xFF) / 255.0f,
        static_cast<float>((rgb >> 8) & 0xFF) / 255.0f,
        static_cast<float>(rgb & 0xFF) / 255.0f,
        alpha
    );
}

/**
 * @brief 链式调用声明（Fluent API）的控件生成建造者模板。
 * 允许使用 Flutter/SwiftUI 风格编写声明式 UI 树。
 * @tparam T 控件类型。
 */
template <typename T>
class ElementBuilder : public ElementRef<T> {
public:
    using ElementRef<T>::m_ptr;
    ElementBuilder() : ElementRef<T>(std::make_shared<T>()) {}
    explicit ElementBuilder(std::shared_ptr<T> elem) : ElementRef<T>(std::move(elem)) {}
    template<typename U, typename = std::enable_if_t<!std::is_same_v<U, T> && std::is_convertible_v<U*, T*>>>
    ElementBuilder(const ElementRef<U>& ref) : ElementRef<T>(ref.Shared()) {}

    operator std::shared_ptr<T>() const { return m_ptr; } // 隐式转换为具体的智能指针类型
    operator std::shared_ptr<UIElement>() const { return m_ptr; } // 隐式转换为 UIElement 基类智能指针
    std::shared_ptr<T> Build() const { return m_ptr; } // 链式终点：返回构造完成的共享指针
    T* operator->() const { return m_ptr.get(); } // 重载指针运算符以便快捷存取成员
    T* get() const { return m_ptr.get(); }

    ElementBuilder& Id(const std::string& id) { // 设定控件检索 ID
        m_ptr->SetId(id);
        return *this;
    }

    ElementBuilder& Width(float w) { // 设定显式排版宽度
        m_ptr->SetWidth(w);
        return *this;
    }

    ElementBuilder& Height(float h) { // 设定显式排版高度
        m_ptr->SetHeight(h);
        return *this;
    }

    ElementBuilder& MinWidth(float w) { // 设定布局最小限制宽度
        m_ptr->SetMinWidth(w);
        return *this;
    }

    ElementBuilder& MinHeight(float h) { // 设定布局最小限制高度
        m_ptr->SetMinHeight(h);
        return *this;
    }

    ElementBuilder& MaxWidth(float w) { m_ptr->SetMaxWidth(w); return *this; }
    ElementBuilder& MaxHeight(float h) { m_ptr->SetMaxHeight(h); return *this; }

    ElementBuilder& Size(float w, float h) { // 设定宽高尺寸
        m_ptr->SetWidth(w);
        m_ptr->SetHeight(h);
        return *this;
    }

    ElementBuilder& Margin(float all) { // 设定四向均匀外边距
        m_ptr->SetMargin(Thickness(all));
        return *this;
    }

    ElementBuilder& Margin(float l, float t, float r, float b) { // 设定具体外边距数值
        m_ptr->SetMargin(Thickness(l, t, r, b));
        return *this;
    }

    ElementBuilder& Padding(float all) { // 设定四向均匀内边距
        m_ptr->SetPadding(Thickness(all));
        return *this;
    }

    ElementBuilder& Padding(float l, float t, float r, float b) { // 设定具体内边距数值
        m_ptr->SetPadding(Thickness(l, t, r, b));
        return *this;
    }

    ElementBuilder& FlexGrow(float flex) { // 设定弹性伸展权重
        m_ptr->SetFlexGrow(flex);
        return *this;
    }

    ElementBuilder& ZIndex(int zIndex) { // 设定 Canvas 子项的绘制与命中层级
        m_ptr->SetZIndex(zIndex);
        return *this;
    }

    ElementBuilder& BackgroundToken(ThemeTokenId id) { // 绑定背景色主题 Token
        m_ptr->SetBackgroundToken(id);
        return *this;
    }

    ElementBuilder& Background(D2D1_COLOR_F color) { // 设定硬编码背景颜色
        m_ptr->Background = CUI::Color(color);
        return *this;
    }

    ElementBuilder& Background(const std::string& color) {
        m_ptr->Background = Color::Hex(color);
        return *this;
    }

    ElementBuilder& HoverBackgroundToken(ThemeTokenId id) { // 绑定悬浮背景色主题 Token
        m_ptr->SetHoverBackgroundToken(id);
        return *this;
    }

    ElementBuilder& HoverBackground(D2D1_COLOR_F color) { // 设定硬编码悬浮背景颜色
        m_ptr->HoverBackground = CUI::Color(color);
        return *this;
    }

    ElementBuilder& Hover(D2D1_COLOR_F color) { return HoverBackground(color); }
    ElementBuilder& Hover(const std::string& color) { return HoverBackground(Color::Hex(color)); }

    ElementBuilder& PressedBackgroundToken(ThemeTokenId id) { // 绑定按下背景色主题 Token
        m_ptr->SetPressedBackgroundToken(id);
        return *this;
    }

    ElementBuilder& PressedBackground(D2D1_COLOR_F color) { // 设定硬编码按下背景颜色
        m_ptr->PressedBackground = CUI::Color(color);
        return *this;
    }

    ElementBuilder& Pressed(D2D1_COLOR_F color) { return PressedBackground(color); }
    ElementBuilder& Pressed(const std::string& color) { return PressedBackground(Color::Hex(color)); }

    ElementBuilder& ColorToken(ThemeTokenId id) { // 绑定字元前景主题 Token
        m_ptr->SetColorToken(id);
        return *this;
    }

    ElementBuilder& Color(D2D1_COLOR_F color) { // 设定硬编码前景/文本颜色
        m_ptr->Foreground = CUI::Color(color);
        return *this;
    }

    ElementBuilder& Foreground(D2D1_COLOR_F color) {
        m_ptr->Foreground = CUI::Color(color);
        return *this;
    }

    ElementBuilder& FontSize(float size) { // 设定字体大小 (px)
        m_ptr->SetFontSize(size);
        return *this;
    }

    ElementBuilder& FontFamily(const std::string& family) { // 指定渲染字体族名称
        m_ptr->SetFontFamily(family);
        return *this;
    }

    ElementBuilder& FontWeight(CUI::FontWeight weight) { // 设置文本字重粗细
        m_ptr->SetFontWeight(weight);
        return *this;
    }

    ElementBuilder& FontStyle(CUI::FontStyle style) { // 设置文本字形直立/倾斜
        m_ptr->SetFontStyle(style);
        return *this;
    }

    ElementBuilder& FontStretch(CUI::FontStretch stretch) { // 设置字体拉伸方向
        m_ptr->SetFontStretch(stretch);
        return *this;
    }

    ElementBuilder& Underline(bool underline = true) { // 设定是否增加下划线修饰
        m_ptr->SetIsUnderline(underline);
        return *this;
    }

    ElementBuilder& Strikethrough(bool strikethrough = true) { // 设定是否增加删除线修饰
        m_ptr->SetIsStrikethrough(strikethrough);
        return *this;
    }

    ElementBuilder& CornerRadius(float r) { // 设定矩形边角圆角像素半径
        m_ptr->SetCornerRadius(r);
        return *this;
    }

    ElementBuilder& BorderToken(ThemeTokenId id, float thickness = 1.0f) { // 设定边框主题颜色和粗细
        m_ptr->SetBorderToken(id);
        m_ptr->SetBorderThickness(thickness);
        return *this;
    }

    ElementBuilder& Border(D2D1_COLOR_F color, float thickness = 1.0f) { // 设定硬编码边框颜色和粗细
        m_ptr->SetBorderBrush(color);
        m_ptr->SetBorderThickness(thickness);
        return *this;
    }

    ElementBuilder& Enabled(bool enabled) { // 设定控件交互可用状态
        m_ptr->SetIsEnabled(enabled);
        return *this;
    }

    ElementBuilder& Visibility(const std::string& vis) { // 设定控件的可见性模式
        if (vis == "Hidden") m_ptr->SetVisibility(CUI::Visibility::Hidden);
        else if (vis == "Collapsed") m_ptr->SetVisibility(CUI::Visibility::Collapsed);
        else m_ptr->SetVisibility(CUI::Visibility::Visible);
        return *this;
    }

    // 容器元素添加子元素
    ElementBuilder& Children(std::initializer_list<std::shared_ptr<UIElement>> list) { // 批量导入添加子控件集合
        for (auto& child : list) {
            if (child) m_ptr->AddChild(child);
        }
        return *this;
    }

    ElementBuilder& Add(std::shared_ptr<UIElement> child) { // 单次添加一个直系子控件
        if (child) m_ptr->AddChild(child);
        return *this;
    }


    ElementBuilder& Text(const std::string& text) { // 设定核心文字展示信息
        m_ptr->SetText(text);
        return *this;
    }

    ElementBuilder& ToolTip(const std::string& tip) { // 设定鼠标停留信息气泡内容
        m_ptr->SetToolTip(tip);
        return *this;
    }

    ElementBuilder& Icon(const std::string& icon) { // 赋予图标特征
        m_ptr->SetIcon(icon);
        return *this;
    }

    ElementBuilder& Subtitle(const std::string& subtitle) { // 设定 Expander 副标题文本
        auto expander = std::dynamic_pointer_cast<Expander>(m_ptr);
        if (expander) {
            expander->SetSubtitle(subtitle);
        }
        return *this;
    }

    ElementBuilder& Orientation(const std::string& orient) { // 设定布局的分布朝向
        if (orient == "Horizontal" || orient == "Row") {
            m_ptr->SetOrientation(CUI::Orientation::Horizontal);
        } else {
            m_ptr->SetOrientation(CUI::Orientation::Vertical);
        }
        return *this;
    }

    ElementBuilder& Gap(float gap) { // 设定子控件之间分隔的像素间距
        m_ptr->SetGap(gap);
        return *this;
    }

    ElementBuilder& Justified(bool enabled = true) { m_ptr->SetJustifyLines(enabled); return *this; }
    ElementBuilder& FillLastLine(bool enabled = true) { m_ptr->SetFillLastLine(enabled); return *this; }

    ElementBuilder& OnClick(std::function<void(UIElement*)> handler) { // 连接 Click 单击事件回调
        if constexpr (std::is_base_of_v<Control, T> || std::is_same_v<CUI::Button, T> || std::is_same_v<HyperlinkButton, T>) {
            m_ptr->OnClick.Connect(handler);
        }
        return *this;
    }

    ElementBuilder& Command(std::shared_ptr<CUI::Command> command) { // 绑定触发执行的 Action 命令
        m_ptr->SetCommand(std::move(command));
        return *this;
    }

    ElementBuilder& OnTextChanged(std::function<void(TextBox*, const std::string&)> handler) { // 连接文本编辑内容更改回调
        if constexpr (std::is_same_v<TextBox, T>) {
            m_ptr->OnTextChanged().Connect(handler);
        }
        return *this;
    }

    ElementBuilder& OnCheckChanged(std::function<void(CheckBox*, CheckState)> handler) { // 连接复选框/单选钮选中状态更改回调
        if constexpr (std::is_base_of_v<CheckBox, T> || std::is_same_v<CheckBox, T> || std::is_same_v<RadioButton, T>) {
            m_ptr->OnCheckStateChanged().Connect(handler);
        }
        return *this;
    }

    ElementBuilder& OnValueChanged(std::function<void(Slider*, float)> handler) { // 连接单数值滑块分值更改回调
        if constexpr (std::is_same_v<Slider, T>) {
            m_ptr->OnValueChanged().Connect(handler);
        }
        return *this;
    }

    ElementBuilder& OnValueChanged(std::function<void(RangeSlider*, float, float)> handler) { // 连接双滑块区间段滑动更改回调
        if constexpr (std::is_same_v<RangeSlider, T>) {
            m_ptr->OnValueChanged().Connect(handler);
        }
        return *this;
    }


    ElementBuilder& BorderThickness(float thickness) {
        m_ptr->SetBorderThickness(thickness);
        return *this;
    }

    ElementBuilder& Orientation(CUI::Orientation o) {
        m_ptr->SetOrientation(o);
        return *this;
    }

    template<typename ItemsT>
    ElementBuilder& Items(ItemsT&& items) {
        if constexpr (requires { m_ptr->Items = std::forward<ItemsT>(items); }) {
            m_ptr->Items = std::forward<ItemsT>(items);
        } else if constexpr (requires { m_ptr->SetItems(std::forward<ItemsT>(items)); }) {
            m_ptr->SetItems(std::forward<ItemsT>(items));
        }
        return *this;
    }

    ElementBuilder& Items(std::initializer_list<std::string> items) {
        if constexpr (requires { m_ptr->Items = items; }) {
            m_ptr->Items = items;
        } else if constexpr (requires { m_ptr->SetItems(items); }) {
            m_ptr->SetItems(items);
        }
        return *this;
    }

    template<typename V>
    ElementBuilder& Value(V&& v) {
        if constexpr (requires { m_ptr->Value = std::forward<V>(v); }) {
            m_ptr->Value = std::forward<V>(v);
        } else if constexpr (requires { m_ptr->SetValue(std::forward<V>(v)); }) {
            m_ptr->SetValue(std::forward<V>(v));
        }
        return *this;
    }

    ElementBuilder& Minimum(float minVal) {
        if constexpr (requires { m_ptr->Minimum = minVal; }) {
            m_ptr->Minimum = minVal;
        } else if constexpr (requires { m_ptr->SetMinimum(minVal); }) {
            m_ptr->SetMinimum(minVal);
        }
        return *this;
    }

    ElementBuilder& Maximum(float maxVal) {
        if constexpr (requires { m_ptr->Maximum = maxVal; }) {
            m_ptr->Maximum = maxVal;
        } else if constexpr (requires { m_ptr->SetMaximum(maxVal); }) {
            m_ptr->SetMaximum(maxVal);
        }
        return *this;
    }

    ElementBuilder& Step(float s) {
        if constexpr (requires { m_ptr->Step = s; }) {
            m_ptr->Step = s;
        } else if constexpr (requires { m_ptr->SetStep(s); }) {
            m_ptr->SetStep(s);
        }
        return *this;
    }

    ElementBuilder& IsReadOnly(bool ro) {
        if constexpr (requires { m_ptr->IsReadOnly = ro; }) {
            m_ptr->IsReadOnly = ro;
        } else if constexpr (requires { m_ptr->SetIsReadOnly(ro); }) {
            m_ptr->SetIsReadOnly(ro);
        }
        return *this;
    }

    ElementBuilder& IsExpanded(bool exp) {
        if constexpr (requires { m_ptr->IsExpanded = exp; }) {
            m_ptr->IsExpanded = exp;
        } else if constexpr (requires { m_ptr->SetIsExpanded(exp); }) {
            m_ptr->SetIsExpanded(exp);
        }
        return *this;
    }

    ElementBuilder& ColumnDefinitions(const std::string& defs) {
        if constexpr (requires { m_ptr->ColumnDefinitions = defs; }) {
            m_ptr->ColumnDefinitions = defs;
        } else if constexpr (requires { m_ptr->SetColumnDefinitions(defs); }) {
            m_ptr->SetColumnDefinitions(defs);
        }
        return *this;
    }

    ElementBuilder& RowDefinitions(const std::string& defs) {
        if constexpr (requires { m_ptr->RowDefinitions = defs; }) {
            m_ptr->RowDefinitions = defs;
        } else if constexpr (requires { m_ptr->SetRowDefinitions(defs); }) {
            m_ptr->SetRowDefinitions(defs);
        }
        return *this;
    }

    ElementBuilder& Placeholder(const std::string& text) {
        if constexpr (requires { m_ptr->Placeholder = text; }) {
            m_ptr->Placeholder = text;
        } else if constexpr (requires { m_ptr->SetPlaceholder(text); }) {
            m_ptr->SetPlaceholder(text);
        }
        return *this;
    }

    ElementBuilder& Title(const std::string& t) {
        if constexpr (requires { m_ptr->Title = t; }) {
            m_ptr->Title = t;
        } else if constexpr (requires { m_ptr->SetTitle(t); }) {
            m_ptr->SetTitle(t);
        }
        return *this;
    }

    ElementBuilder& Message(const std::string& m) {
        if constexpr (requires { m_ptr->Message = m; }) {
            m_ptr->Message = m;
        } else if constexpr (requires { m_ptr->SetMessage(m); }) {
            m_ptr->SetMessage(m);
        }
        return *this;
    }

    ElementBuilder& Nodes(const std::vector<std::shared_ptr<TopologyNode>>& nodes) {
        if constexpr (requires { m_ptr->Nodes = nodes; }) {
            m_ptr->Nodes = nodes;
        } else if constexpr (requires { m_ptr->SetNodes(nodes); }) {
            m_ptr->SetNodes(nodes);
        }
        return *this;
    }

    ElementBuilder& Edges(const std::vector<TopologyEdge>& edges) {
        if constexpr (requires { m_ptr->Edges = edges; }) {
            m_ptr->Edges = edges;
        } else if constexpr (requires { m_ptr->SetEdges(edges); }) {
            m_ptr->SetEdges(edges);
        }
        return *this;
    }

    ElementBuilder& LayoutType(TopologyLayoutType t) {
        if constexpr (requires { m_ptr->LayoutType = t; }) {
            m_ptr->LayoutType = t;
        } else if constexpr (requires { m_ptr->SetLayoutType(t); }) {
            m_ptr->SetLayoutType(t);
        }
        return *this;
    }

    ElementBuilder& FlowParticles(bool enabled = true) {
        if constexpr (requires { m_ptr->FlowParticles = enabled; }) {
            m_ptr->FlowParticles = enabled;
        } else if constexpr (requires { m_ptr->SetFlowParticlesEnabled(enabled); }) {
            m_ptr->SetFlowParticlesEnabled(enabled);
        }
        return *this;
    }
};

struct ChildArgument {
    Element element;
    ChildArgument() = default;
    ChildArgument(Element value) : element(std::move(value)) {}
    template<typename T> ChildArgument(std::shared_ptr<T> value) : element(std::move(value)) {}
    template<typename T> ChildArgument(const ElementRef<T>& ref) : element(ref.Shared()) {}
    template<typename T> ChildArgument(const ElementBuilder<T>& builder) : element(builder.Shared()) {}
};

// Flutter-Style Widget Aliases (快速构建语法别名)

inline ElementBuilder<StackPanel> Column(float gap = 8.0f) { // 快速生成垂直方向排列的容器面板
    return ElementBuilder<StackPanel>().Orientation("Vertical").Gap(gap);
}

inline ElementBuilder<StackPanel> Row(float gap = 8.0f) { // 快速生成水平方向排列的容器面板
    return ElementBuilder<StackPanel>().Orientation("Horizontal").Gap(gap);
}

namespace Fluent {

inline ElementBuilder<CUI::Button> Button(const std::string& text = "") {
    return ElementBuilder<CUI::Button>(Make<CUI::Button>(text));
}

} // namespace Fluent

inline ElementBuilder<StackPanel> Row(float gap, std::initializer_list<ChildArgument> children) {
    auto row = Row(gap);
    for (const auto& child : children) {
        if (child.element) row->AddChild(child.element);
    }
    return row;
}

inline ElementBuilder<StackPanel> Column(float gap, std::initializer_list<ChildArgument> children) {
    auto column = Column(gap);
    for (const auto& child : children) {
        if (child.element) column->AddChild(child.element);
    }
    return column;
}

inline ElementBuilder<TextBlock> Text(const std::string& content = "") { // 快速生成只读文本块组件
    auto l = ElementBuilder<TextBlock>();
    if (!content.empty()) l.Text(content);
    return l;
}

inline ElementBuilder<Button> ElevatedButton(const std::string& text = "", std::function<void(UIElement*)> onPressed = nullptr) { // 快速生成普通点击式凸起按钮
    auto b = ElementBuilder<Button>();
    if (!text.empty()) b.Text(text);
    if (onPressed) b.OnClick(onPressed);
    return b;
}

inline ElementBuilder<ToggleButton> ToggleButtonWidget(const std::string& text = "") { // 快速生成带按下/弹回两态切换的开关按钮
    auto b = ElementBuilder<ToggleButton>();
    if (!text.empty()) b.Text(text);
    return b;
}

inline ElementBuilder<DropDownButton> DropDownButtonWidget(const std::string& text = "") { // 快速生成带向下小三角箭头的下拉弹窗按钮
    auto b = ElementBuilder<DropDownButton>();
    if (!text.empty()) b.Text(text);
    return b;
}

inline ElementBuilder<SplitButton> SplitButtonWidget(const std::string& text = "") { // 快速生成拆分式下拉按钮
    auto b = ElementBuilder<SplitButton>();
    if (!text.empty()) b.Text(text);
    return b;
}

inline ElementBuilder<TextBox> TextField(const std::string& text = "", std::function<void(TextBox*, const std::string&)> onChanged = nullptr) { // 快速生成普通文本输入框
    auto t = ElementBuilder<TextBox>();
    if (!text.empty()) t.Text(text);
    if (onChanged) t.OnTextChanged(onChanged);
    return t;
}

inline ElementBuilder<CheckBox> CheckboxTile(const std::string& title = "", std::function<void(CheckBox*, CheckState)> onChanged = nullptr) { // 快速生成复选卡选项组
    auto c = ElementBuilder<CheckBox>();
    if (!title.empty()) c.Text(title);
    if (onChanged) c.OnCheckChanged(onChanged);
    return c;
}

inline ElementBuilder<Panel> Container() { // 快速生成空泛的排版盒模型容器
    return ElementBuilder<Panel>();
}

inline ElementBuilder<TopologyView> TopologyWidget() {
    return ElementBuilder<TopologyView>();
}

inline ElementBuilder<Canvas> CanvasWidget() { // 快速生成支持绝对坐标手工摆放子项的画布容器
    return ElementBuilder<Canvas>();
}

inline ElementBuilder<Grid> GridWidget() { // 快速生成网格栅格排版定位 Grid 容器
    return ElementBuilder<Grid>();
}

inline ElementBuilder<WrapPanel> WrapPanelWidget(const std::string& orient = "Horizontal") { // 快速生成自动溢出换行的流式布局容器
    return ElementBuilder<WrapPanel>().Orientation(orient);
}

inline ElementBuilder<DockPanel> DockPanelWidget() { // 快速生成边缘停靠容器面板
    return ElementBuilder<DockPanel>();
}

inline ElementBuilder<UniformGrid> UniformGridWidget(int rows = 2, int cols = 2) { // 快速生成单元格等宽等高的均分网格容器
    auto u = ElementBuilder<UniformGrid>();
    u->SetRows(rows);
    u->SetColumns(cols);
    return u;
}

inline ElementBuilder<ComboBox> ComboBoxWidget() { return ElementBuilder<ComboBox>(); }
inline ElementBuilder<ListBox> ListBoxWidget() { return ElementBuilder<ListBox>(); }
inline ElementBuilder<ToggleSwitch> ToggleSwitchWidget() { return ElementBuilder<ToggleSwitch>(); }
inline ElementBuilder<TreeView> TreeViewWidget() { return ElementBuilder<TreeView>(); }
inline ElementBuilder<Flyout> FlyoutWidget() { return ElementBuilder<Flyout>(); }

inline ElementBuilder<ScrollViewer> SingleChildScrollView() { // 快速生成单子控件滚动查看器
    return ElementBuilder<ScrollViewer>();
}

inline ElementBuilder<Panel> Expanded(std::shared_ptr<UIElement> child, float flex = 1.0f) { // 快速生成弹性延伸填充块
    auto p = ElementBuilder<Panel>();
    p.FlexGrow(flex);
    if (child) {
        child->SetFlexGrow(1.0f);
        p.Add(child);
    }
    return p;
}

inline ElementBuilder<Slider> SliderWidget(float val = 0.0f, float min = 0.0f, float max = 100.0f, std::function<void(Slider*, float)> onChanged = nullptr) { // 快速生成游标滑动条
    auto s = ElementBuilder<Slider>();
    s->SetMinimum(min);
    s->SetMaximum(max);
    s->SetValue(val);
    if (onChanged) s.OnValueChanged(onChanged);
    return s;
}

inline ElementBuilder<RangeSlider> RangeSliderWidget(
    float lower = 20.0f,
    float upper = 80.0f,
    float min = 0.0f,
    float max = 100.0f,
    std::function<void(RangeSlider*, float, float)> onChanged = nullptr) { // 快速生成双滑手柄区间段选择滑动器
    auto s = ElementBuilder<RangeSlider>();
    s->SetMinimum(min);
    s->SetMaximum(max);
    s->SetRange(lower, upper);
    if (onChanged) s.OnValueChanged(onChanged);
    return s;
}

inline ElementBuilder<ProgressBar> ProgressBarWidget(float val = 0.0f, bool isIndeterminate = false) { // 快速生成水平条形进度显示表
    auto p = ElementBuilder<ProgressBar>();
    p->SetValue(val);
    p->SetIsIndeterminate(isIndeterminate);
    return p;
}

inline ElementBuilder<ProgressRing> ProgressRingWidget(float val = 0.0f, bool isIndeterminate = true) { // 快速生成圆形旋转进度加载环
    auto p = ElementBuilder<ProgressRing>();
    p->SetValue(val);
    p->SetIsIndeterminate(isIndeterminate);
    return p;
}

inline ElementBuilder<AutoSuggestBox> AutoSuggestBoxWidget(const std::string& placeholder = "搜索…") { // 快速生成带模糊关联建议匹配的输入框
    auto a = ElementBuilder<AutoSuggestBox>();
    a->SetPlaceholder(placeholder);
    return a;
}

inline ElementBuilder<StatusBar> StatusBarWidget() { // 快速生成底部状态控制条
    return ElementBuilder<StatusBar>();
}

inline ElementBuilder<RatingControl> RatingWidget(float value = 3.5f, int maxRating = 5) { // 快速生成五星级评分控件
    auto r = ElementBuilder<RatingControl>();
    r->SetMaxRating(maxRating);
    r->SetValue(value);
    return r;
}

inline ElementBuilder<TeachingTip> TeachingTipWidget() { // 快速生成新手气泡指引小浮框
    return ElementBuilder<TeachingTip>();
}

inline ElementBuilder<LineChart> LineChartWidget() { // 快速生成折线统计图表
    return ElementBuilder<LineChart>();
}

inline ElementBuilder<BarChart> BarChartWidget() { // 快速生成柱状统计图表
    return ElementBuilder<BarChart>();
}

inline ElementBuilder<PieChart> PieChartWidget() { // 快速生成饼图百分比统计图表
    return ElementBuilder<PieChart>();
}

inline ElementBuilder<MarkdownView> MarkdownViewWidget() { // 快速生成自适应 Markdown 排版富文本视图
    return ElementBuilder<MarkdownView>();
}

inline ElementBuilder<LogView> LogViewWidget() { // 快速生成带分级着色和搜索的高频滚动日志监视窗
    return ElementBuilder<LogView>();
}

inline ElementBuilder<InfoBar> InfoBarWidget() { // 快速生成用于头部提示消息的各种状态通知条
    return ElementBuilder<InfoBar>();
}

inline ElementBuilder<CommandBar> CommandBarWidget() { // 快速生成可伸缩、带溢出点按式横条工具栏
    return ElementBuilder<CommandBar>();
}

inline ElementBuilder<MenuBar> MenuBarWidget() { // 快速生成水平顶级菜单栏
    return ElementBuilder<MenuBar>();
}

inline ElementBuilder<Image> ImageWidget() { // 快速生成图片加载盒
    return ElementBuilder<Image>();
}

inline ElementBuilder<FilePicker> FilePickerWidget(const std::string& path = "") { // 快速生成文件路径拾取器
    auto f = ElementBuilder<FilePicker>();
    if (!path.empty()) {
        f->SetPath(path);
    }
    return f;
}

inline ElementBuilder<FolderPicker> FolderPickerWidget(const std::string& path = "") { // 快速生成文件夹目录拾取器
    auto f = ElementBuilder<FolderPicker>();
    if (!path.empty()) {
        f->SetPath(path);
    }
    return f;
}

inline ElementBuilder<SegmentedControl> SegmentedWidget(std::initializer_list<const char*> items = {}) { // 快速生成 iOS 风格的左右滑动分段选择单选组
    auto s = ElementBuilder<SegmentedControl>();
    for (const char* item : items) {
        if (item && *item) {
            s->AddItem(item);
        }
    }
    return s;
}

inline ElementBuilder<NumberBox> NumberBoxWidget(double val = 0.0) { // 快速生成带上下微调箭头数值框
    auto n = ElementBuilder<NumberBox>();
    n->SetValue(static_cast<float>(val));
    return n;
}

inline ElementBuilder<PasswordBox> PasswordBoxWidget(const std::string& placeholder = "请输入密码") { // 快速生成遮罩密码安全输入框
    auto p = ElementBuilder<PasswordBox>();
    p->SetPlaceholder(placeholder);
    return p;
}

inline ElementBuilder<RadioButton> RadioButtonTile(const std::string& text = "", const std::string& group = "DefaultGroup") { // 快速生成单选按钮卡片项
    auto r = ElementBuilder<RadioButton>();
    if (!text.empty()) r.Text(text);
    r->SetGroupName(group);
    return r;
}

inline ElementBuilder<ToggleSwitch> ToggleSwitchTile(const std::string& header = "", bool isOn = false) { // 快速生成滑道式物理开关
    auto t = ElementBuilder<ToggleSwitch>();
    if (!header.empty()) t.Text(header);
    t->SetIsOn(isOn);
    return t;
}

inline ElementBuilder<DatePicker> DatePickerWidget() { // 快速生成日期年月日下拉滚轮选择器
    return ElementBuilder<DatePicker>();
}

inline ElementBuilder<TimePicker> TimePickerWidget() { // 快速生成时间时分秒下拉滚轮选择器
    return ElementBuilder<TimePicker>();
}

inline ElementBuilder<ColorPicker> ColorPickerWidget() { // 快速生成 HSV 环形加色板颜色选择盘
    return ElementBuilder<ColorPicker>();
}

inline ElementBuilder<BreadcrumbBar> BreadcrumbBarWidget() { // 快速生成树形面包屑路标导航条
    return ElementBuilder<BreadcrumbBar>();
}

inline ElementBuilder<PagingControl> PagingControlWidget(int current = 1, int total = 10) { // 快速生成列表分页翻页控制器
    auto p = ElementBuilder<PagingControl>();
    p->SetTotalPages(total);
    p->SetCurrentPage(current);
    return p;
}

inline ElementBuilder<Splitter> SplitterWidget(Orientation orientation = Orientation::Horizontal) { // 快速生成拖拽式布局调整分割条
    auto s = ElementBuilder<Splitter>();
    if (orientation == Orientation::Horizontal) {
        s->SetOrientation(Orientation::Horizontal);
        s->SetWidth(-1.0f);
        s->SetHeight(10.0f);
    } else {
        s->SetOrientation(Orientation::Vertical);
        s->SetWidth(10.0f);
        s->SetHeight(-1.0f);
    }
    s->SetAlign(Alignment::Stretch);
    return s;
}

inline ElementBuilder<Expander> ExpanderWidget(const std::string& title = "Expander") { // 快速生成可拉伸折拢的内容卡片 Expander
    auto c = ElementBuilder<Expander>();
    c->SetHeader(title);
    return c;
}

inline ElementBuilder<Expander> CollapsePanelWidget(const std::string& title = "Expander") { // 兼容性老命名：折叠面板组件
    return ExpanderWidget(title);
}

inline ElementBuilder<ListView> ListViewWidget() { // 快速生成纵向数据项目展示列表
    return ElementBuilder<ListView>();
}

inline ElementBuilder<HyperlinkButton> HyperlinkButtonWidget(const std::string& text = "", const std::string& uri = "") { // 快速生成超链接字元按钮
    auto h = ElementBuilder<HyperlinkButton>();
    if (!text.empty()) h.Text(text);
    if (!uri.empty()) h->SetNavigateUri(uri);
    return h;
}

inline ElementBuilder<ContentDialog> ContentDialogWidget(const std::string& title = "Dialog", const std::string& message = "") { // 快速生成带确认取消的模态框大浮窗
    auto d = ElementBuilder<ContentDialog>();
    d->SetTitle(title);
    if (!message.empty()) d->SetMessage(message);
    return d;
}

/**
 * @brief 快速生成声明式矩形 Shape DOM 节点。
 * @param width 初始宽度，默认 100px。
 * @param height 初始高度，默认 50px。
 */
inline ElementBuilder<Rectangle> RectangleWidget(float width = 100.0f, float height = 50.0f) {
    auto r = ElementBuilder<Rectangle>();
    r->SetWidth(width);
    r->SetHeight(height);
    return r;
}

/**
 * @brief 快速生成声明式椭圆/圆形 Shape DOM 节点。
 * @param width 初始宽度，默认 50px。
 * @param height 初始高度，默认 50px。
 */
inline ElementBuilder<Ellipse> EllipseWidget(float width = 50.0f, float height = 50.0f) {
    auto e = ElementBuilder<Ellipse>();
    e->SetWidth(width);
    e->SetHeight(height);
    return e;
}

/**
 * @brief 快速生成声明式直线 Shape DOM 节点。
 * @param x1 起点 X 坐标。
 * @param y1 起点 Y 坐标。
 * @param x2 终点 X 坐标。
 * @param y2 终点 Y 坐标。
 */
inline ElementBuilder<Line> LineWidget(float x1 = 0, float y1 = 0, float x2 = 100, float y2 = 100) {
    auto l = ElementBuilder<Line>();
    l->SetX1(x1); l->SetY1(y1); l->SetX2(x2); l->SetY2(y2);
    return l;
}

/**
 * @brief 快速生成声明式 SVG Path 路径 Shape DOM 节点。
 * @param data SVG path data 描述字符串（如 "M 10 10 L 90 90 Z"）。
 */
inline ElementBuilder<Path> PathWidget(const std::string& data = "") {
    auto p = ElementBuilder<Path>();
    if (!data.empty()) p->SetData(data);
    return p;
}

/**
 * @brief 快速生成声明式 SVG DOM 控件节点 (SvgIcon)。支持事件独立绑定与 TintColor 着色。
 * @param source SVG XML 内容标记或文件路径。
 */
inline ElementBuilder<SvgIcon> SvgIconWidget(const std::string& source = "") {
    auto s = ElementBuilder<SvgIcon>();
    if (!source.empty()) s->SetSource(source);
    return s;
}

/**
 * @brief 快速生成即时绘制模式画布控件 (CanvasControl)。
 * 适用于用 Direct2D 自绘制高性能图形，在事件闭包中手写坐标碰撞检测。
 * @param width 初始宽度，默认 300px。
 * @param height 初始高度，默认 200px。
 */
inline ElementBuilder<CanvasControl> CanvasControlWidget(float width = 300.0f, float height = 200.0f) {
    auto c = ElementBuilder<CanvasControl>();
    c->SetWidth(width);
    c->SetHeight(height);
    return c;
}

struct BuildContext {
    Window* window = nullptr; // 包含当前进行构建活动的窗口宿主指针
};

// Flutter-style Component Base Class (Widget build method)
class Component {
public:
    virtual ~Component() = default;
    virtual std::shared_ptr<UIElement> Build() = 0; // 虚 build 工厂
    operator std::shared_ptr<UIElement>() { return Build(); } // 重载隐式转换方便直接当做 UIElement 使用
};

class Widget {
public:
    virtual ~Widget() = default;
    virtual std::shared_ptr<UIElement> build(BuildContext& context) = 0; // Flutter 风格构建
};

class StatelessWidget : public Component, public Widget {
public:
    StatelessWidget() = default;
    explicit StatelessWidget(BuildContext context) : m_context(context) {}
    virtual ~StatelessWidget() = default;

    void SetBuildContext(const BuildContext& context) { m_context = context; } // 注册组件上下文
    BuildContext& GetBuildContext() { return m_context; }
    const BuildContext& GetBuildContext() const { return m_context; }

    std::shared_ptr<UIElement> Build() override { return build(m_context); } // 映射 Component build 通道

private:
    BuildContext m_context; // 组件所绑定的宿主上下文信息实例
};

} // namespace DSL
} // namespace CUI
