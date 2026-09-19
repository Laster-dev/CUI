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
#include "../controls/Toast.h"
#include "../controls/PagingControl.h"
#include "../controls/Splitter.h"
#include "../controls/Expander.h"
#include "../controls/Flyout.h"
#include "../controls/WindowTitleBar.h"
#include "../controls/NavigationView.h"
#include "../controls/MessageBox.h"
#include "../controls/shapes/Shapes.h"
#include "../controls/CanvasControl.h"
#include "../controls/topology/TopologyView.h"
#include "../controls/docking/DockManager.h"
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

template<typename T> struct IsSharedPtr : std::false_type {};
template<typename U> struct IsSharedPtr<std::shared_ptr<U>> : std::true_type {};

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

    /**
     * 默认构造函数
     * 创建一个新的 T 类型实例并持有其共享指针
     */
    ElementBuilder() : ElementRef<T>(std::make_shared<T>()) {}

    /**
     * 显式构造函数
     * @param elem 已存在的共享指针，用于初始化 ElementBuilder
     */
    explicit ElementBuilder(std::shared_ptr<T> elem) : ElementRef<T>(std::move(elem)) {}

    /**
     * 模板构造函数
     * 允许从其他类型的 ElementRef<U> 转换为 ElementBuilder<T>
     * 前提是 U 可转换为 T
     * @param ref 其他类型的 ElementRef
     */
    template<typename U, typename = std::enable_if_t<!std::is_same_v<U, T>&& std::is_convertible_v<U*, T*>>>
    ElementBuilder(const ElementRef<U>& ref) : ElementRef<T>(ref.Shared()) {}/**
     * 构建函数
     * 链式调用的终点，返回最终构造完成的共享指针
     * @return std::shared_ptr<T>
     */
    template<typename U = T, typename = std::enable_if_t<!std::is_same_v<U, UIElement>>>
    operator std::shared_ptr<UIElement>() const { return m_ptr; }

    std::shared_ptr<T> Build() const { return m_ptr; }/**
     * 获取底层原始指针
     * @return T* 原始指针
     */
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

    /**
     * 设置元素的最大宽度
     * @param w 最大宽度值 (单位: 像素或逻辑单位)
     * @return 返回当前 ElementBuilder 引用，支持链式调用
     */
    ElementBuilder& MaxWidth(float w) {
        m_ptr->SetMaxWidth(w);
        return *this;
    }

    /**
     * 设置元素的最大高度
     * @param h 最大高度值 (单位: 像素或逻辑单位)
     * @return 返回当前 ElementBuilder 引用，支持链式调用
     */
    ElementBuilder& MaxHeight(float h) {
        m_ptr->SetMaxHeight(h);
        return *this;
    }

    /**
     * 设置元素的固定宽高尺寸
     * @param w 宽度值 (单位: 像素或逻辑单位)
     * @param h 高度值 (单位: 像素或逻辑单位)
     * @return 返回当前 ElementBuilder 引用，支持链式调用
     */
    ElementBuilder& Size(float w, float h) {
        m_ptr->SetWidth(w);
        m_ptr->SetHeight(h);
        return *this;
    }


    /**
     * 设置元素整体对齐方式
     * @param a 对齐枚举值 (如 Left, Center, Right)
     * @return 返回当前 ElementBuilder 引用，支持链式调用
     */
    ElementBuilder& Align(CUI::Alignment a) {
        m_ptr->SetAlign(a);
        return *this;
    }

    /**
     * 设置元素的水平对齐方式
     * @param a 水平对齐枚举值 (如 Left, Center, Right)
     * @return 返回当前 ElementBuilder 引用，支持链式调用
     */
    ElementBuilder& AlignHorizontal(CUI::Alignment a) {
        m_ptr->SetAlignHorizontal(a);
        return *this;
    }

    /**
     * 设置元素的垂直对齐方式
     * @param a 垂直对齐枚举值 (如 Top, Middle, Bottom)
     * @return 返回当前 ElementBuilder 引用，支持链式调用
     */
    ElementBuilder& AlignVertical(CUI::Alignment a) {
        m_ptr->SetAlignVertical(a);
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

    ElementBuilder& Margin(const Thickness& margin) {
        m_ptr->SetMargin(margin);
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

    ElementBuilder& Padding(const Thickness& padding) {
        m_ptr->SetPadding(padding);
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

    ElementBuilder& TitleColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetTitleColorToken(id); }) m_ptr->SetTitleColorToken(id); return *this; }
    ElementBuilder& MessageColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetMessageColorToken(id); }) m_ptr->SetMessageColorToken(id); return *this; }
    ElementBuilder& AccentColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetAccentColorToken(id); }) m_ptr->SetAccentColorToken(id); return *this; }
    ElementBuilder& PaneBackgroundToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetPaneBackgroundToken(id); }) m_ptr->SetPaneBackgroundToken(id); return *this; }
    ElementBuilder& IndicatorColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetIndicatorColorToken(id); }) m_ptr->SetIndicatorColorToken(id); return *this; }
    ElementBuilder& SecondaryColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetSecondaryColorToken(id); }) m_ptr->SetSecondaryColorToken(id); return *this; }
    ElementBuilder& DisabledBackgroundToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetDisabledBackgroundToken(id); }) m_ptr->SetDisabledBackgroundToken(id); return *this; }
    ElementBuilder& ColorToken(ThemeTokenId id) { return ForegroundToken(id); }
    ElementBuilder& ClipToBounds(bool value) { if constexpr (requires { m_ptr->SetClipToBounds(value); }) m_ptr->SetClipToBounds(value); return *this; }
    ElementBuilder& KeyboardNavigationMode(CUI::KeyboardNavigationMode mode) { if constexpr (requires { m_ptr->SetKeyboardNavigationMode(mode); }) m_ptr->SetKeyboardNavigationMode(mode); return *this; }
    ElementBuilder& OverlayScrollbar(bool value) { if constexpr (requires { m_ptr->SetOverlayScrollbar(value); }) m_ptr->SetOverlayScrollbar(value); return *this; }
    ElementBuilder& FillColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetFillColorToken(id); }) m_ptr->SetFillColorToken(id); return *this; }
    ElementBuilder& TrackColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetTrackColorToken(id); }) m_ptr->SetTrackColorToken(id); return *this; }
    ElementBuilder& ActiveTrackColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetActiveTrackColorToken(id); }) m_ptr->SetActiveTrackColorToken(id); return *this; }
    ElementBuilder& RowHeight(float value) { if constexpr (requires { m_ptr->SetRowHeight(value); }) m_ptr->SetRowHeight(value); return *this; }
    ElementBuilder& BytesPerRow(int value) { if constexpr (requires { m_ptr->SetBytesPerRow(value); }) m_ptr->SetBytesPerRow(value); return *this; }
    ElementBuilder& GridLineBrushToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetGridLineBrushToken(id); }) m_ptr->SetGridLineBrushToken(id); return *this; }
    ElementBuilder& ThumbColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetThumbColorToken(id); }) m_ptr->SetThumbColorToken(id); return *this; }
    ElementBuilder& PlaceholderColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetPlaceholderColorToken(id); }) m_ptr->SetPlaceholderColorToken(id); return *this; }
    ElementBuilder& DropdownBackgroundToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetDropdownBackgroundToken(id); }) m_ptr->SetDropdownBackgroundToken(id); return *this; }
    ElementBuilder& SelectedItemBackgroundToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetSelectedItemBackgroundToken(id); }) m_ptr->SetSelectedItemBackgroundToken(id); return *this; }
    ElementBuilder& UnderlineColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetUnderlineColorToken(id); }) m_ptr->SetUnderlineColorToken(id); return *this; }
    ElementBuilder& ActiveUnderlineColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetActiveUnderlineColorToken(id); }) m_ptr->SetActiveUnderlineColorToken(id); return *this; }
    ElementBuilder& CaretColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetCaretColorToken(id); }) m_ptr->SetCaretColorToken(id); return *this; }
    ElementBuilder& OnColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetOnColorToken(id); }) m_ptr->SetOnColorToken(id); return *this; }
    ElementBuilder& OffColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetOffColorToken(id); }) m_ptr->SetOffColorToken(id); return *this; }
    ElementBuilder& KnobColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetKnobColorToken(id); }) m_ptr->SetKnobColorToken(id); return *this; }
    ElementBuilder& HeaderBackgroundToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetHeaderBackgroundToken(id); }) m_ptr->SetHeaderBackgroundToken(id); return *this; }
    ElementBuilder& ActiveTabBackgroundToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetActiveTabBackgroundToken(id); }) m_ptr->SetActiveTabBackgroundToken(id); return *this; }
    ElementBuilder& InactiveTabBackgroundToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetInactiveTabBackgroundToken(id); }) m_ptr->SetInactiveTabBackgroundToken(id); return *this; }
    ElementBuilder& LastChildFill(bool value) { if constexpr (requires { m_ptr->SetLastChildFill(value); }) m_ptr->SetLastChildFill(value); return *this; }
    ElementBuilder& ItemHeight(float value) { if constexpr (requires { m_ptr->SetItemHeight(value); }) m_ptr->SetItemHeight(value); return *this; }
    ElementBuilder& ActiveColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetActiveColorToken(id); }) m_ptr->SetActiveColorToken(id); return *this; }
    ElementBuilder& CheckedBackgroundToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetCheckedBackgroundToken(id); }) m_ptr->SetCheckedBackgroundToken(id); return *this; }
    ElementBuilder& BackgroundToken(ThemeTokenId id) { // 绑定背景色主题 Token
        m_ptr->SetBackgroundToken(id);
        return *this;
    }

    ElementBuilder& Background(D2D1_COLOR_F color) { // 设定硬编码背景颜色
        if constexpr (requires { m_ptr->SetBackground(color); }) m_ptr->SetBackground(color);
        return *this;
    }

    ElementBuilder& Background(const std::string& color) {
        return Background(Color::Hex(color));
    }

    ElementBuilder& HoverBackgroundToken(ThemeTokenId id) { // 绑定悬浮背景色主题 Token
        m_ptr->SetHoverBackgroundToken(id);
        return *this;
    }

    ElementBuilder& HoverBackground(D2D1_COLOR_F color) { // 设定硬编码悬浮背景颜色
        if constexpr (requires { m_ptr->SetHoverBackground(color); }) m_ptr->SetHoverBackground(color);
        return *this;
    }
    ElementBuilder& HoverBackground(const std::string& color) {
        return HoverBackground(Color::Hex(color));
    }


    ElementBuilder& PressedBackgroundToken(ThemeTokenId id) { // 绑定按下背景色主题 Token
        m_ptr->SetPressedBackgroundToken(id);
        return *this;
    }

    ElementBuilder& PressedBackground(D2D1_COLOR_F color) { // 设定硬编码按下背景颜色
        if constexpr (requires { m_ptr->SetPressedBackground(color); }) m_ptr->SetPressedBackground(color);
        return *this;
    }
    ElementBuilder& PressedBackground(const std::string& color) {
        return PressedBackground(Color::Hex(color));
    }


    ElementBuilder& FocusedBorderToken(ThemeTokenId id) {
        if constexpr (requires { m_ptr->SetFocusedBorderToken(id); }) m_ptr->SetFocusedBorderToken(id);
        return *this;
    }
    ElementBuilder& ForegroundToken(ThemeTokenId id) { // 绑定字元前景主题 Token
        m_ptr->SetColorToken(id);
        return *this;
    }

    ElementBuilder& Color(D2D1_COLOR_F color) { return Foreground(color); }

    ElementBuilder& Foreground(D2D1_COLOR_F color) {
        if constexpr (requires { m_ptr->SetColor(color); }) m_ptr->SetColor(color);
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

    ElementBuilder& LineSpacing(float value) { if constexpr (requires { m_ptr->SetLineSpacing(value); }) m_ptr->SetLineSpacing(value); return *this; }
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

    ElementBuilder& BorderBrush(D2D1_COLOR_F color) { if constexpr (requires { m_ptr->SetBorderBrush(color); }) m_ptr->SetBorderBrush(color); return *this; }
    ElementBuilder& Border(D2D1_COLOR_F color, float thickness = 1.0f) { // 设定硬编码边框颜色和粗细
        m_ptr->SetBorderBrush(color);
        m_ptr->SetBorderThickness(thickness);
        return *this;
    }

    ElementBuilder& IsUnderline(bool value) { if constexpr (requires { m_ptr->SetIsUnderline(value); }) m_ptr->SetIsUnderline(value); return *this; }
    ElementBuilder& IsStrikethrough(bool value) { if constexpr (requires { m_ptr->SetIsStrikethrough(value); }) m_ptr->SetIsStrikethrough(value); return *this; }
    ElementBuilder& ItemWidth(float value) { if constexpr (requires { m_ptr->SetItemWidth(value); }) m_ptr->SetItemWidth(value); return *this; }
    ElementBuilder& JustifyLines(bool value) { if constexpr (requires { m_ptr->SetJustifyLines(value); }) m_ptr->SetJustifyLines(value); return *this; }
    ElementBuilder& CanvasLeft(float value) { if constexpr (requires { m_ptr->SetCanvasLeft(value); }) m_ptr->SetCanvasLeft(value); return *this; }
    ElementBuilder& CanvasTop(float value) { if constexpr (requires { m_ptr->SetCanvasTop(value); }) m_ptr->SetCanvasTop(value); return *this; }
    ElementBuilder& CanvasRight(float value) { if constexpr (requires { m_ptr->SetCanvasRight(value); }) m_ptr->SetCanvasRight(value); return *this; }
    ElementBuilder& CanvasBottom(float value) { if constexpr (requires { m_ptr->SetCanvasBottom(value); }) m_ptr->SetCanvasBottom(value); return *this; }
    ElementBuilder& Dock(CUI::Dock value) { if constexpr (requires { m_ptr->SetDock(value); }) m_ptr->SetDock(value); return *this; }
    ElementBuilder& IsEnabled(bool enabled) { // 设定控件交互可用状态
        m_ptr->SetIsEnabled(enabled);
        return *this;
    }

    ElementBuilder& Visibility(CUI::Visibility value) { m_ptr->SetVisibility(value); return *this; }
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

    ElementBuilder& OverlayComposed(bool value) { if constexpr (requires { m_ptr->SetOverlayComposed(value); }) m_ptr->SetOverlayComposed(value); return *this; }
    ElementBuilder& ClearChildren() { if constexpr (requires { m_ptr->ClearChildren(); }) m_ptr->ClearChildren(); return *this; }
    template<typename ChildT>
    ElementBuilder& AddChild(const std::shared_ptr<ChildT>& child) {
        if (child) {
            if constexpr (requires { m_ptr->AddChild(child); }) m_ptr->AddChild(child);
            else m_ptr->AddChild(std::static_pointer_cast<UIElement>(child));
        }
        return *this;
    }

    template<typename ChildT>
    ElementBuilder& AddChild(const ElementBuilder<ChildT>& child) {
        return AddChild(child.Shared());
    }

    template<typename ChildT>
    ElementBuilder& RemoveChild(const std::shared_ptr<ChildT>& child) {
        if (child) {
            if constexpr (requires { m_ptr->RemoveChild(child); }) m_ptr->RemoveChild(child);
            else m_ptr->RemoveChild(std::static_pointer_cast<UIElement>(child));
        }
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
    ElementBuilder& IconText(const std::string& icon) {
        if constexpr (requires { m_ptr->SetIconText(icon); }) m_ptr->SetIconText(icon);
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
    ElementBuilder& GridRow(int value) { m_ptr->SetGridRow(value); return *this; }
    ElementBuilder& GridColumn(int value) { m_ptr->SetGridColumn(value); return *this; }
    ElementBuilder& GridColumnSpan(int value) { m_ptr->SetGridColumnSpan(value); return *this; }
    ElementBuilder& GridRowSpan(int value) { m_ptr->SetGridRowSpan(value); return *this; }

    ElementBuilder& TextAlign(TextAlignment value) {
        if constexpr (requires { m_ptr->SetTextAlign(value); }) m_ptr->SetTextAlign(value);
        return *this;
    }

    ElementBuilder& VerticalAlign(TextVerticalAlignment value) {
        if constexpr (requires { m_ptr->SetVerticalAlign(value); }) m_ptr->SetVerticalAlign(value);
        return *this;
    }

    ElementBuilder& Justified(bool enabled = true) { m_ptr->SetJustifyLines(enabled); return *this; }
    ElementBuilder& FillLastLine(bool enabled = true) { m_ptr->SetFillLastLine(enabled); return *this; }

    ElementBuilder& ClosedCallback(std::function<void()> handler) { if constexpr (requires { m_ptr->SetClosedCallback(std::move(handler)); }) m_ptr->SetClosedCallback(std::move(handler)); return *this; }
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

    ElementBuilder& OnCheckStateChanged(std::function<void(CheckBox*, CheckState)> handler) {
        if constexpr (std::is_same_v<CheckBox, T>) m_ptr->OnCheckStateChanged().Connect(std::move(handler));
        return *this;
    }    ElementBuilder& OnCheckChanged(std::function<void(CheckBox*, CheckState)> handler) { // 连接复选框/单选钮选中状态更改回调
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

    ElementBuilder& SelectedBackgroundToken(ThemeTokenId id) { if constexpr (requires { m_ptr->SetSelectedBackgroundToken(id); }) m_ptr->SetSelectedBackgroundToken(id); return *this; }

    ElementBuilder& IndentWidth(float value) { if constexpr (requires { m_ptr->SetIndentWidth(value); }) m_ptr->SetIndentWidth(value); return *this; }
    ElementBuilder& OnSelectionChanged(std::function<void(TreeView*, std::shared_ptr<TreeViewItem>)> handler) { if constexpr (requires { m_ptr->OnSelectionChanged(); }) m_ptr->OnSelectionChanged().Connect(std::move(handler)); return *this; }
    ElementBuilder& OnItemToggled(std::function<void(TreeView*, std::shared_ptr<TreeViewItem>)> handler) { if constexpr (requires { m_ptr->OnItemToggled(); }) m_ptr->OnItemToggled().Connect(std::move(handler)); return *this; }
    ElementBuilder& OnItemDoubleClicked(std::function<void(TreeView*, std::shared_ptr<TreeViewItem>)> handler) { if constexpr (requires { m_ptr->OnItemDoubleClicked(); }) m_ptr->OnItemDoubleClicked().Connect(std::move(handler)); return *this; }
    ElementBuilder& OnBreadcrumbItemClicked(std::function<void(BreadcrumbBar*, int, const std::string&)> handler) { if constexpr (requires { m_ptr->OnItemClicked(); }) m_ptr->OnItemClicked().Connect(std::move(handler)); return *this; }
    ElementBuilder& ClearItems() { if constexpr (requires { m_ptr->ClearItems(); }) m_ptr->ClearItems(); return *this; }
    ElementBuilder& AddTreeItem(std::shared_ptr<TreeViewItem> item) { if constexpr (requires { m_ptr->AddItem(item); }) m_ptr->AddItem(std::move(item)); return *this; }
    ElementBuilder& SelectedTreeItem(std::shared_ptr<TreeViewItem> item) { if constexpr (requires { m_ptr->SetSelectedItem(item); }) m_ptr->SetSelectedItem(std::move(item)); return *this; }
    ElementBuilder& PathNodes(const std::vector<std::string>& value) { if constexpr (requires { m_ptr->SetPath(value); }) m_ptr->SetPath(value); return *this; }    ElementBuilder& OnSelectionChanged(std::function<void(ComboBox*, int, const std::string&)> handler) {
        if constexpr (std::is_same_v<ComboBox, T>) m_ptr->OnSelectionChanged().Connect(std::move(handler));
        return *this;
    }    ElementBuilder& OnSelectionChanged(std::function<void(SegmentedControl*, int, const std::string&)> handler) {
        if constexpr (std::is_same_v<SegmentedControl, T>) {
            m_ptr->OnSelectionChanged().Connect(handler);
        }
        return *this;
    }
    ElementBuilder& OnToggled(std::function<void(ToggleButton*, bool)> handler) {
        if constexpr (std::is_same_v<ToggleButton, T>) m_ptr->OnToggled().Connect(handler);
        return *this;
    }

    ElementBuilder& OnItemChosen(std::function<void(DropDownButton*, int, const std::string&)> handler) {
        if constexpr (std::is_same_v<DropDownButton, T>) m_ptr->OnItemChosen().Connect(handler);
        return *this;
    }

    ElementBuilder<CUI::ContextMenu> Menu(const std::string& title) {
        if constexpr (std::is_same_v<MenuBar, T>) return ElementBuilder<CUI::ContextMenu>(m_ptr->AddMenu(title));
        return ElementBuilder<CUI::ContextMenu>();
    }
    ElementBuilder<MenuItem> Item(const std::string& text, std::function<void()> handler = nullptr) {
        if constexpr (std::is_same_v<CUI::ContextMenu, T>) return ElementBuilder<MenuItem>(m_ptr->AddItem(text, std::move(handler)));
        return ElementBuilder<MenuItem>();
    }    ElementBuilder<CUI::ContextMenu> SubMenu(const std::string& title) {
        if constexpr (std::is_same_v<CUI::ContextMenu, T>) return ElementBuilder<CUI::ContextMenu>(m_ptr->AddSubMenu(title));
        return ElementBuilder<CUI::ContextMenu>();
    }
    ElementBuilder& AddItem(const std::string& item, const std::string& shortcut, std::function<void()> handler) {
        if constexpr (requires { m_ptr->AddItem(item, shortcut, handler); }) m_ptr->AddItem(item, shortcut, std::move(handler));
        return *this;
    }
    ElementBuilder& ParentContextMenu(ContextMenu* value) { if constexpr (requires { m_ptr->SetParentContextMenu(value); }) m_ptr->SetParentContextMenu(value); return *this; }
    ElementBuilder& SubMenu(std::shared_ptr<ContextMenu> value) { if constexpr (requires { m_ptr->SetSubMenu(value); }) m_ptr->SetSubMenu(std::move(value)); return *this; }
    ElementBuilder& ShortcutText(const std::string& value) { if constexpr (requires { m_ptr->SetShortcutText(value); }) m_ptr->SetShortcutText(value); return *this; }
    ElementBuilder& IsSeparator(bool value = true) { if constexpr (requires { m_ptr->SetIsSeparator(value); }) m_ptr->SetIsSeparator(value); return *this; }    ElementBuilder& Checked(bool value = true) { if constexpr (requires { m_ptr->SetChecked(value); }) m_ptr->SetChecked(value); return *this; }    ElementBuilder& AddItem(const std::string& item, std::function<void()> handler) {
        if constexpr (requires { m_ptr->AddItem(item, handler); }) m_ptr->AddItem(item, std::move(handler));
        return *this;
    }

    ElementBuilder& AddSeparator() {
        if constexpr (requires { m_ptr->AddSeparator(); }) m_ptr->AddSeparator();
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

    ElementBuilder& Items(const std::vector<std::shared_ptr<CUI::TreeViewItem>>& items) { if constexpr (requires { m_ptr->SetItems(items); }) m_ptr->SetItems(items); return *this; }

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

    ElementBuilder& SelectedIndex(int value) { if constexpr (requires { m_ptr->SetSelectedIndex(value); }) m_ptr->SetSelectedIndex(value); return *this; }
    ElementBuilder& SelectionMode(ListBoxSelectionMode value) { if constexpr (requires { m_ptr->SetSelectionMode(value); }) m_ptr->SetSelectionMode(value); return *this; }
    ElementBuilder& SelectionMode(ListViewSelectionMode value) { if constexpr (requires { m_ptr->SetSelectionMode(value); }) m_ptr->SetSelectionMode(value); return *this; }
    ElementBuilder& ShowScrollBars(bool value) { if constexpr (requires { m_ptr->SetShowScrollBars(value); }) m_ptr->SetShowScrollBars(value); return *this; }
    ElementBuilder& VirtualCount(size_t count) { if constexpr (requires { m_ptr->SetVirtualCount(count); }) m_ptr->SetVirtualCount(count); return *this; }
    template<typename DataSourceT> ElementBuilder& VirtualMode(int count, DataSourceT* source) { if constexpr (requires { m_ptr->SetVirtualMode(count, source); }) m_ptr->SetVirtualMode(count, source); return *this; }
    ElementBuilder& ExpandDirection(ExpandDirection value) { if constexpr (requires { m_ptr->SetExpandDirection(value); }) m_ptr->SetExpandDirection(value); return *this; }
    ElementBuilder& Placement(FlyoutPlacement value) { if constexpr (requires { m_ptr->SetPlacement(value); }) m_ptr->SetPlacement(value); return *this; }
    ElementBuilder& IsCloseVisible(bool value) { if constexpr (requires { m_ptr->SetIsCloseVisible(value); }) m_ptr->SetIsCloseVisible(value); return *this; }
    ElementBuilder& IsModal(bool value) { if constexpr (requires { m_ptr->SetIsModal(value); }) m_ptr->SetIsModal(value); return *this; }
    ElementBuilder& PreferredPlacement(BubblePlacement value) { if constexpr (requires { m_ptr->SetPreferredPlacement(value); }) m_ptr->SetPreferredPlacement(value); return *this; }
    ElementBuilder& TintColor(D2D1_COLOR_F value) { if constexpr (requires { m_ptr->SetTintColor(value); }) m_ptr->SetTintColor(value); return *this; }
    template<typename ItemT> ElementBuilder& ItemExpanded(const std::shared_ptr<ItemT>& item, bool value) { if constexpr (requires { m_ptr->SetItemExpanded(item, value); }) m_ptr->SetItemExpanded(item, value); return *this; }
    ElementBuilder& State(CheckState value) { if constexpr (requires { m_ptr->SetState(value); }) m_ptr->SetState(value); return *this; }    ElementBuilder& IsPasswordMode(bool value) { if constexpr (requires { m_ptr->SetIsPasswordMode(value); }) m_ptr->SetIsPasswordMode(value); return *this; }
    ElementBuilder& ShowRevealButton(bool value) { if constexpr (requires { m_ptr->SetShowRevealButton(value); }) m_ptr->SetShowRevealButton(value); return *this; }
    ElementBuilder& AcceptsReturn(bool value) { if constexpr (requires { m_ptr->SetAcceptsReturn(value); }) m_ptr->SetAcceptsReturn(value); return *this; }
    ElementBuilder& TextWrapping(bool value) { if constexpr (requires { m_ptr->SetTextWrapping(value); }) m_ptr->SetTextWrapping(value); return *this; }
    ElementBuilder& AllowDrag(bool value) { if constexpr (requires { m_ptr->SetAllowDrag(value); }) m_ptr->SetAllowDrag(value); return *this; }
    ElementBuilder& AllowDrop(bool value) { if constexpr (requires { m_ptr->SetAllowDrop(value); }) m_ptr->SetAllowDrop(value); return *this; }
    ElementBuilder& AcceptsTab(bool value) { if constexpr (requires { m_ptr->SetAcceptsTab(value); }) m_ptr->SetAcceptsTab(value); return *this; }
    ElementBuilder& ToolTipMaxWidth(float value) { if constexpr (requires { m_ptr->SetToolTipMaxWidth(value); }) m_ptr->SetToolTipMaxWidth(value); return *this; }
    ElementBuilder& ToolTipAutoHideMs(int value) { if constexpr (requires { m_ptr->SetToolTipAutoHideMs(value); }) m_ptr->SetToolTipAutoHideMs(value); return *this; }
    ElementBuilder& SelectAll() { if constexpr (requires { m_ptr->SelectAll(); }) m_ptr->SelectAll(); return *this; }    ElementBuilder& Placeholder(const std::string& text) {
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
    template<typename RowsT>
    ElementBuilder& Rows(RowsT&& value) {
        if constexpr (requires { m_ptr->Rows = std::forward<RowsT>(value); }) m_ptr->Rows = std::forward<RowsT>(value);
        else if constexpr (requires { m_ptr->SetRows(std::forward<RowsT>(value)); }) m_ptr->SetRows(std::forward<RowsT>(value));
        return *this;
    }

    ElementBuilder& Columns(int value) {
        if constexpr (requires { m_ptr->SetColumns(value); }) m_ptr->SetColumns(value);
        return *this;
    }

    ElementBuilder& ColumnHeader(int index, const std::string& header, float width = 120.0f) {
        if constexpr (requires { m_ptr->AddColumn(header, width); }) {
            if (index == static_cast<int>(m_ptr->GetColumns().size())) m_ptr->AddColumn(header, width);
        }
        return *this;
    }

    ElementBuilder& AddItem(const std::string& value) {
        if constexpr (requires { m_ptr->AddItem(value); }) m_ptr->AddItem(value);
        return *this;
    }

    ElementBuilder& Range(float lower, float upper) {
        if constexpr (requires { m_ptr->SetRange(lower, upper); }) m_ptr->SetRange(lower, upper);
        return *this;
    }

    ElementBuilder& IsIndeterminate(bool value) {
        if constexpr (requires { m_ptr->SetIsIndeterminate(value); }) m_ptr->SetIsIndeterminate(value);
        return *this;
    }

    ElementBuilder& MaxRating(int value) {
        if constexpr (requires { m_ptr->SetMaxRating(value); }) m_ptr->SetMaxRating(value);
        return *this;
    }

    ElementBuilder& GroupName(const std::string& value) {
        if constexpr (requires { m_ptr->SetGroupName(value); }) m_ptr->SetGroupName(value);
        return *this;
    }

    ElementBuilder& IsChecked(bool value) { if constexpr (requires { m_ptr->SetIsChecked(value); }) m_ptr->SetIsChecked(value); return *this; }
    ElementBuilder& IsOn(bool value) {
        if constexpr (requires { m_ptr->SetIsOn(value); }) m_ptr->SetIsOn(value);
        return *this;
    }

    ElementBuilder& TotalPages(int value) {
        if constexpr (requires { m_ptr->SetTotalPages(value); }) m_ptr->SetTotalPages(value);
        return *this;
    }

    ElementBuilder& CurrentPage(int value) {
        if constexpr (requires { m_ptr->SetCurrentPage(value); }) m_ptr->SetCurrentPage(value);
        return *this;
    }
    ElementBuilder& CurrentPage(const std::string& value) { if constexpr (requires { m_ptr->SetCurrentPage(value); }) m_ptr->SetCurrentPage(value); return *this; }


    ElementBuilder& Header(const std::string& value) {
        if constexpr (requires { m_ptr->SetHeader(value); }) m_ptr->SetHeader(value);
        return *this;
    }

    ElementBuilder& NavigateUri(const std::string& value) {
        if constexpr (requires { m_ptr->SetNavigateUri(value); }) m_ptr->SetNavigateUri(value);
        return *this;
    }

    ElementBuilder& Path(const std::vector<std::string>& value) { if constexpr (requires { m_ptr->SetPath(value); }) m_ptr->SetPath(value); return *this; }
    ElementBuilder& Path(const std::string& value) {
        if constexpr (requires { m_ptr->SetPath(value); }) m_ptr->SetPath(value);
        return *this;
    }

    ElementBuilder& Data(const std::string& value) {
        if constexpr (requires { m_ptr->SetData(value); }) m_ptr->SetData(value);
        return *this;
    }

    ElementBuilder& Stretch(CUI::Stretch value) { if constexpr (requires { m_ptr->SetStretch(value); }) m_ptr->SetStretch(value); return *this; }
    ElementBuilder& StretchMode(CUI::Stretch value) { if constexpr (requires { m_ptr->SetStretch(value); }) m_ptr->SetStretch(value); return *this; }
    ElementBuilder& BadgeText(const std::string& value) { if constexpr (requires { m_ptr->SetBadgeText(value); }) m_ptr->SetBadgeText(value); return *this; }
    ElementBuilder& BadgeColor(D2D1_COLOR_F value) { if constexpr (requires { m_ptr->SetBadgeColor(value); }) m_ptr->SetBadgeColor(value); return *this; }    ElementBuilder& Source(const std::string& value) {
        if constexpr (requires { m_ptr->SetSource(value); }) m_ptr->SetSource(value);
        return *this;
    }

    ElementBuilder& X1(float value) { if constexpr (requires { m_ptr->SetX1(value); }) m_ptr->SetX1(value); return *this; }
    ElementBuilder& Y1(float value) { if constexpr (requires { m_ptr->SetY1(value); }) m_ptr->SetY1(value); return *this; }
    ElementBuilder& X2(float value) { if constexpr (requires { m_ptr->SetX2(value); }) m_ptr->SetX2(value); return *this; }
    ElementBuilder& Y2(float value) { if constexpr (requires { m_ptr->SetY2(value); }) m_ptr->SetY2(value); return *this; }

    ElementBuilder& PaneTitle(const std::string& value) { if constexpr (requires { m_ptr->SetPaneTitle(value); }) m_ptr->SetPaneTitle(value); return *this; }
    ElementBuilder& PaneDisplayMode(NavigationViewPaneDisplayMode value) { if constexpr (requires { m_ptr->SetPaneDisplayMode(value); }) m_ptr->SetPaneDisplayMode(value); return *this; }
    ElementBuilder& IsPaneOpen(bool value) { if constexpr (requires { m_ptr->SetIsPaneOpen(value); }) m_ptr->SetIsPaneOpen(value); return *this; }
    ElementBuilder& OpenPaneLength(float value) { if constexpr (requires { m_ptr->SetOpenPaneLength(value); }) m_ptr->SetOpenPaneLength(value); return *this; }
    ElementBuilder& CompactPaneLength(float value) { if constexpr (requires { m_ptr->SetCompactPaneLength(value); }) m_ptr->SetCompactPaneLength(value); return *this; }
    ElementBuilder& CompactModeThresholdWidth(float value) { if constexpr (requires { m_ptr->SetCompactModeThresholdWidth(value); }) m_ptr->SetCompactModeThresholdWidth(value); return *this; }
    ElementBuilder& ExpandedModeThresholdWidth(float value) { if constexpr (requires { m_ptr->SetExpandedModeThresholdWidth(value); }) m_ptr->SetExpandedModeThresholdWidth(value); return *this; }
    ElementBuilder& AlwaysShowHeader(bool value) { if constexpr (requires { m_ptr->SetAlwaysShowHeader(value); }) m_ptr->SetAlwaysShowHeader(value); return *this; }
    ElementBuilder& IsSettingsVisible(bool value) { if constexpr (requires { m_ptr->SetIsSettingsVisible(value); }) m_ptr->SetIsSettingsVisible(value); return *this; }
    ElementBuilder& Content(std::shared_ptr<UIElement> value) { if constexpr (requires { m_ptr->SetContent(value); }) m_ptr->SetContent(std::move(value)); return *this; }
    ElementBuilder& ContentFactory(std::function<std::shared_ptr<UIElement>()> value) { if constexpr (requires { m_ptr->SetContentFactory(std::move(value)); }) m_ptr->SetContentFactory(std::move(value)); return *this; }
    ElementBuilder& AutoSuggestBox(std::shared_ptr<UIElement> value) { if constexpr (requires { m_ptr->SetAutoSuggestBox(value); }) m_ptr->SetAutoSuggestBox(std::move(value)); return *this; }
    ElementBuilder& IsBackButtonVisible(NavigationViewBackButtonVisible value) { if constexpr (requires { m_ptr->SetIsBackButtonVisible(value); }) m_ptr->SetIsBackButtonVisible(value); return *this; }
    ElementBuilder& IsBackEnabled(bool value) { if constexpr (requires { m_ptr->SetIsBackEnabled(value); }) m_ptr->SetIsBackEnabled(value); return *this; }
    ElementBuilder& SelectedItem(NavigationViewItem* value) { if constexpr (requires { m_ptr->SetSelectedItem(value); }) m_ptr->SetSelectedItem(value); return *this; }
    template<typename ItemT> ElementBuilder& SelectedItem(const std::shared_ptr<ItemT>& value) { if constexpr (requires { m_ptr->SetSelectedItem(value); }) m_ptr->SetSelectedItem(value); return *this; }
    ElementBuilder& Severity(InfoBarSeverity value) { if constexpr (requires { m_ptr->SetSeverity(value); }) m_ptr->SetSeverity(value); return *this; }
    ElementBuilder& IsClosable(bool value) { if constexpr (requires { m_ptr->SetIsClosable(value); }) m_ptr->SetIsClosable(value); return *this; }
    ElementBuilder& ActionText(const std::string& value) { if constexpr (requires { m_ptr->SetActionText(value); }) m_ptr->SetActionText(value); return *this; }
    ElementBuilder& IsOpen(bool value) { if constexpr (requires { m_ptr->SetIsOpen(value); }) m_ptr->SetIsOpen(value); return *this; }
    ElementBuilder& AddMenuItem(std::shared_ptr<NavigationViewItemBase> value) { if constexpr (requires { m_ptr->AddMenuItem(value); }) m_ptr->AddMenuItem(std::move(value)); return *this; }
    ElementBuilder& AddFooterMenuItem(std::shared_ptr<NavigationViewItemBase> value) { if constexpr (requires { m_ptr->AddFooterMenuItem(value); }) m_ptr->AddFooterMenuItem(std::move(value)); return *this; }
    ElementBuilder& Tag(const std::string& value) { if constexpr (requires { m_ptr->SetTag(value); }) m_ptr->SetTag(value); return *this; }
    ElementBuilder& SelectsOnInvoked(bool value) { if constexpr (requires { m_ptr->SetSelectsOnInvoked(value); }) m_ptr->SetSelectsOnInvoked(value); return *this; }
    ElementBuilder& AddNestedItem(std::shared_ptr<NavigationViewItemBase> value) { if constexpr (requires { m_ptr->AddMenuItem(value); }) m_ptr->AddMenuItem(std::move(value)); return *this; }
    ElementBuilder& Owner(NavigationView* value) { if constexpr (requires { m_ptr->SetOwner(value); }) m_ptr->SetOwner(value); return *this; }
    ElementBuilder& Compact(bool value) { if constexpr (requires { m_ptr->SetCompact(value); }) m_ptr->SetCompact(value); return *this; }
    ElementBuilder& TopMode(bool value) { if constexpr (requires { m_ptr->SetTopMode(value); }) m_ptr->SetTopMode(value); return *this; }
    ElementBuilder& IsSelected(bool value) { if constexpr (requires { m_ptr->SetIsSelected(value); }) m_ptr->SetIsSelected(value); return *this; }
    ElementBuilder& IsChildSelected(bool value) { if constexpr (requires { m_ptr->SetIsChildSelected(value); }) m_ptr->SetIsChildSelected(value); return *this; }
    ElementBuilder& IsExpandedSilent(bool value) { if constexpr (requires { m_ptr->SetIsExpandedSilent(value); }) m_ptr->SetIsExpandedSilent(value); return *this; }
    ElementBuilder& Opacity(float value) { if constexpr (requires { m_ptr->SetOpacity(value); }) m_ptr->SetOpacity(value); return *this; }
    ElementBuilder& ComposeOpacity(float value) { if constexpr (requires { m_ptr->SetComposeOpacity(value); }) m_ptr->SetComposeOpacity(value); return *this; }
    ElementBuilder& OnInvoked(std::function<void(NavigationViewItem*)> handler) { if constexpr (requires { m_ptr->OnInvoked(); }) m_ptr->OnInvoked().Connect(std::move(handler)); return *this; }
    ElementBuilder& OnExpandChanged(std::function<void(NavigationViewItem*)> handler) { if constexpr (requires { m_ptr->OnExpandChanged(); }) m_ptr->OnExpandChanged().Connect(std::move(handler)); return *this; }    ElementBuilder& OnNavigationItemInvoked(std::function<void(NavigationView*, const NavigationViewItemInvokedEventArgs&)> handler) { if constexpr (requires { m_ptr->OnItemInvoked(); }) m_ptr->OnItemInvoked().Connect(std::move(handler)); return *this; }
    ElementBuilder& OnNavigationBackRequested(std::function<void(NavigationView*)> handler) { if constexpr (requires { m_ptr->OnBackRequested(); }) m_ptr->OnBackRequested().Connect(std::move(handler)); return *this; }
    ElementBuilder& Filter(const std::string& name, const std::string& spec) { if constexpr (requires { m_ptr->SetFilter(name, spec); }) m_ptr->SetFilter(name, spec); return *this; }
    ElementBuilder& ToastTypeValue(ToastType value) { if constexpr (requires { m_ptr->SetType(value); }) m_ptr->SetType(value); return *this; }
    ElementBuilder& Corner(ToastCorner value) { if constexpr (requires { m_ptr->SetCorner(value); }) m_ptr->SetCorner(value); return *this; }
    ElementBuilder& DurationMs(int value) { if constexpr (requires { m_ptr->SetDurationMs(value); }) m_ptr->SetDurationMs(value); return *this; }
    ElementBuilder& AutoClose(bool value) { if constexpr (requires { m_ptr->SetAutoClose(value); }) m_ptr->SetAutoClose(value); return *this; }
    ElementBuilder& Closeable(bool value) { if constexpr (requires { m_ptr->SetCloseable(value); }) m_ptr->SetCloseable(value); return *this; }
    ElementBuilder& Accent(const std::string& value) { if constexpr (requires { m_ptr->SetAccent(value); }) m_ptr->SetAccent(value); return *this; }
    ElementBuilder& TitleColor(const std::string& value) { if constexpr (requires { m_ptr->SetTitleColor(value); }) m_ptr->SetTitleColor(value); return *this; }
    ElementBuilder& MessageColor(const std::string& value) { if constexpr (requires { m_ptr->SetMessageColor(value); }) m_ptr->SetMessageColor(value); return *this; }
    ElementBuilder& OffsetX(float value) { if constexpr (requires { m_ptr->SetOffsetX(value); }) m_ptr->SetOffsetX(value); return *this; }
    ElementBuilder& OffsetY(float value) { if constexpr (requires { m_ptr->SetOffsetY(value); }) m_ptr->SetOffsetY(value); return *this; }
    ElementBuilder& Spacing(float value) { if constexpr (requires { m_ptr->SetSpacing(value); }) m_ptr->SetSpacing(value); return *this; }    ElementBuilder& PrimaryButtonText(const std::string& value) { if constexpr (requires { m_ptr->SetPrimaryButtonText(value); }) m_ptr->SetPrimaryButtonText(value); return *this; }
    ElementBuilder& SecondaryButtonText(const std::string& value) { if constexpr (requires { m_ptr->SetSecondaryButtonText(value); }) m_ptr->SetSecondaryButtonText(value); return *this; }
    ElementBuilder& CloseButtonText(const std::string& value) { if constexpr (requires { m_ptr->SetCloseButtonText(value); }) m_ptr->SetCloseButtonText(value); return *this; }
    ElementBuilder& InputEnabled(bool value, bool multiline = false) { if constexpr (requires { m_ptr->SetInputEnabled(value, multiline); }) m_ptr->SetInputEnabled(value, multiline); return *this; }
    ElementBuilder& InputText(const std::string& value) { if constexpr (requires { m_ptr->SetInputText(value); }) m_ptr->SetInputText(value); return *this; }    ElementBuilder& DialogTitle(const std::string& value) { if constexpr (requires { m_ptr->SetDialogTitle(value); }) m_ptr->SetDialogTitle(value); return *this; }
    ElementBuilder& OnPathChanged(std::function<void(FilePicker*, const std::string&)> handler) { if constexpr (requires { m_ptr->OnPathChanged(); }) m_ptr->OnPathChanged().Connect(std::move(handler)); return *this; }    ElementBuilder& OnNavigationDisplayModeChanged(std::function<void(NavigationView*, const NavigationViewDisplayModeChangedEventArgs&)> handler) { if constexpr (requires { m_ptr->OnDisplayModeChanged(); }) m_ptr->OnDisplayModeChanged().Connect(std::move(handler)); return *this; }

    ElementBuilder& ActiveContextMenu(std::shared_ptr<CUI::ContextMenu> value) { if constexpr (requires { m_ptr->SetActiveContextMenu(value); }) m_ptr->SetActiveContextMenu(std::move(value)); return *this; }
    ElementBuilder& BackdropType(CUI::BackdropType value) { if constexpr (requires { m_ptr->SetBackdropType(value); }) m_ptr->SetBackdropType(value); return *this; }
    ElementBuilder& RenderStatsOverlayVisible(bool value) { if constexpr (requires { m_ptr->SetRenderStatsOverlayVisible(value); }) m_ptr->SetRenderStatsOverlayVisible(value); return *this; }
    ElementBuilder& ThemeMode(CUI::ThemeMode value) { if constexpr (requires { m_ptr->SetThemeMode(value); }) m_ptr->SetThemeMode(value); return *this; }
    ElementBuilder& ThemeModeWithRipple(CUI::ThemeMode value, Point origin) { if constexpr (requires { m_ptr->SetThemeModeWithRipple(value, origin); }) m_ptr->SetThemeModeWithRipple(value, origin); return *this; }
    ElementBuilder& ContextMenu(std::shared_ptr<CUI::ContextMenu> value) { if constexpr (requires { m_ptr->SetContextMenu(value); }) m_ptr->SetContextMenu(std::move(value)); return *this; }
    ElementBuilder& RightContent(std::shared_ptr<UIElement> value) { if constexpr (requires { m_ptr->SetRightContent(value); }) m_ptr->SetRightContent(std::move(value)); return *this; }
    ElementBuilder& CaretIndex(int value) { if constexpr (requires { m_ptr->SetCaretIndex(value); }) m_ptr->SetCaretIndex(value); return *this; }
    ElementBuilder& ColumnVisible(int index, bool value) { if constexpr (requires { m_ptr->SetColumnVisible(index, value); }) m_ptr->SetColumnVisible(index, value); return *this; }
    ElementBuilder& RowSelected(int index, bool value) { if constexpr (requires { m_ptr->SetRowSelected(index, value); }) m_ptr->SetRowSelected(index, value); return *this; }
    ElementBuilder& Date(int year, int month, int day) { if constexpr (requires { m_ptr->SetDate(year, month, day); }) m_ptr->SetDate(year, month, day); return *this; }
    ElementBuilder& Time(int hour, int minute) { if constexpr (requires { m_ptr->SetTime(hour, minute); }) m_ptr->SetTime(hour, minute); return *this; }
    ElementBuilder& Fill(D2D1_COLOR_F value) { if constexpr (requires { m_ptr->SetFill(value); }) m_ptr->SetFill(value); return *this; }
    ElementBuilder& Stroke(D2D1_COLOR_F value) { if constexpr (requires { m_ptr->SetStroke(value); }) m_ptr->SetStroke(value); return *this; }
    ElementBuilder& StrokeThickness(float value) { if constexpr (requires { m_ptr->SetStrokeThickness(value); }) m_ptr->SetStrokeThickness(value); return *this; }
    ElementBuilder& Damping(float value) { if constexpr (requires { m_ptr->SetDamping(value); }) m_ptr->SetDamping(value); return *this; }
    ElementBuilder& Stiffness(float value) { if constexpr (requires { m_ptr->SetStiffness(value); }) m_ptr->SetStiffness(value); return *this; }
    ElementBuilder& Running(bool value) { if constexpr (requires { m_ptr->SetRunning(value); }) m_ptr->SetRunning(value); return *this; }
    ElementBuilder& Viewport(float width, float height) { if constexpr (requires { m_ptr->SetViewport(width, height); }) m_ptr->SetViewport(width, height); return *this; }
    ElementBuilder& ScrollOffsetY(float value) { if constexpr (requires { m_ptr->SetScrollOffsetY(value); }) m_ptr->SetScrollOffsetY(value); return *this; }
    ElementBuilder& PaneAutoHide(int index, bool value) { if constexpr (requires { m_ptr->SetPaneAutoHide(index, value); }) m_ptr->SetPaneAutoHide(index, value); return *this; }
    ElementBuilder& SideSize(CUI::DockSide side, float value) { if constexpr (requires { m_ptr->SetSideSize(side, value); }) m_ptr->SetSideSize(side, value); return *this; }
    ElementBuilder& Label(const std::string& value) { if constexpr (requires { m_ptr->SetLabel(value); }) m_ptr->SetLabel(value); return *this; }
    ElementBuilder& LabelPosition(CUI::CommandBarLabelPosition value) { if constexpr (requires { m_ptr->SetLabelPosition(value); }) m_ptr->SetLabelPosition(value); return *this; }
    ElementBuilder& Markdown(const std::string& value) { if constexpr (requires { m_ptr->SetMarkdown(value); }) m_ptr->SetMarkdown(value); return *this; }
    ElementBuilder& Password(const std::string& value) { if constexpr (requires { m_ptr->SetPassword(value); }) m_ptr->SetPassword(value); return *this; }
    ElementBuilder& IsThreeState(bool value) { if constexpr (requires { m_ptr->SetIsThreeState(value); }) m_ptr->SetIsThreeState(value); return *this; }
    ElementBuilder& MaxTabWidth(float value) { if constexpr (requires { m_ptr->SetMaxTabWidth(value); }) m_ptr->SetMaxTabWidth(value); return *this; }
    ElementBuilder& MinTabWidth(float value) { if constexpr (requires { m_ptr->SetMinTabWidth(value); }) m_ptr->SetMinTabWidth(value); return *this; }
    ElementBuilder& ShowGrid(bool value) { if constexpr (requires { m_ptr->SetShowGrid(value); }) m_ptr->SetShowGrid(value); return *this; }
    ElementBuilder& ShowGridLines(bool value) { if constexpr (requires { m_ptr->SetShowGridLines(value); }) m_ptr->SetShowGridLines(value); return *this; }
    ElementBuilder& ShowLegend(bool value) { if constexpr (requires { m_ptr->SetShowLegend(value); }) m_ptr->SetShowLegend(value); return *this; }
    ElementBuilder& ShowTooltip(bool value) { if constexpr (requires { m_ptr->SetShowTooltip(value); }) m_ptr->SetShowTooltip(value); return *this; }
    ElementBuilder& Categories(std::vector<std::string> value) { if constexpr (requires { m_ptr->SetCategories(std::move(value)); }) m_ptr->SetCategories(std::move(value)); return *this; }
    ElementBuilder& Series(std::vector<CUI::ChartSeries> value) { if constexpr (requires { m_ptr->SetSeries(std::move(value)); }) m_ptr->SetSeries(std::move(value)); return *this; }
    ElementBuilder& LiveData(std::vector<std::string> categories, std::vector<CUI::ChartSeries> series, bool replay = false) { if constexpr (requires { m_ptr->SetLiveData(std::move(categories), std::move(series), replay); }) m_ptr->SetLiveData(std::move(categories), std::move(series), replay); return *this; }
    ElementBuilder& ItemText(int id, const std::string& value) { if constexpr (requires { m_ptr->SetItemText(id, value); }) m_ptr->SetItemText(id, value); return *this; }
    ElementBuilder& ItemProgress(int id, float value) { if constexpr (requires { m_ptr->SetItemProgress(id, value); }) m_ptr->SetItemProgress(id, value); return *this; }
    ElementBuilder& SuggestionItems(std::vector<std::string> value) { if constexpr (requires { m_ptr->SetSuggestionItems(value); }) m_ptr->SetSuggestionItems(value); return *this; }
    template<typename F> ElementBuilder& SuggestionProvider(F&& value) { if constexpr (requires { m_ptr->SetSuggestionProvider(std::forward<F>(value)); }) m_ptr->SetSuggestionProvider(std::forward<F>(value)); return *this; }
    template<typename F> ElementBuilder& OnDraw(F&& value) { if constexpr (requires { m_ptr->SetOnDraw(std::forward<F>(value)); }) m_ptr->SetOnDraw(std::forward<F>(value)); return *this; }
    template<typename F> ElementBuilder& OnCanvasMouseDown(F&& value) { if constexpr (requires { m_ptr->SetOnCanvasMouseDown(std::forward<F>(value)); }) m_ptr->SetOnCanvasMouseDown(std::forward<F>(value)); return *this; }
    template<typename F> ElementBuilder& OnCanvasMouseMove(F&& value) { if constexpr (requires { m_ptr->SetOnCanvasMouseMove(std::forward<F>(value)); }) m_ptr->SetOnCanvasMouseMove(std::forward<F>(value)); return *this; }
    template<typename F> ElementBuilder& OnCanvasMouseUp(F&& value) { if constexpr (requires { m_ptr->SetOnCanvasMouseUp(std::forward<F>(value)); }) m_ptr->SetOnCanvasMouseUp(std::forward<F>(value)); return *this; }
    template<typename F> ElementBuilder& OnTick(F&& value) { if constexpr (requires { m_ptr->SetOnTick(std::forward<F>(value)); }) m_ptr->SetOnTick(std::forward<F>(value)); return *this; }
    template<typename F> ElementBuilder& StatusHandler(F&& value) { if constexpr (requires { m_ptr->SetStatusHandler(std::forward<F>(value)); }) m_ptr->SetStatusHandler(std::forward<F>(value)); return *this; }
    ElementBuilder& LowPerformanceMode(bool value) { if constexpr (requires { m_ptr->SetLowPerformanceMode(value); }) m_ptr->SetLowPerformanceMode(value); return *this; }
    ElementBuilder& ImageType(CUI::ImageType value) { if constexpr (requires { m_ptr->SetImageType(value); }) m_ptr->SetImageType(value); return *this; }
    ElementBuilder& OwnerWindow(CUI::Window* value) { if constexpr (requires { m_ptr->SetOwnerWindow(value); }) m_ptr->SetOwnerWindow(value); return *this; }
    ElementBuilder& ActionCommand(std::shared_ptr<CUI::Command> value) { if constexpr (requires { m_ptr->SetActionCommand(value); }) m_ptr->SetActionCommand(std::move(value)); return *this; }
    ElementBuilder& MaxEntries(uint32_t value) { if constexpr (requires { m_ptr->SetMaxEntries(value); }) m_ptr->SetMaxEntries(value); return *this; }
    ElementBuilder& Expanded(bool value) { if constexpr (requires { m_ptr->SetExpanded(value); }) m_ptr->SetExpanded(value); return *this; }
    ElementBuilder& PersistEnabled(bool value) { if constexpr (requires { m_ptr->SetPersistEnabled(value); }) m_ptr->SetPersistEnabled(value); return *this; }
    ElementBuilder& ShowCodeLineNumbers(bool value) { if constexpr (requires { m_ptr->SetShowCodeLineNumbers(value); }) m_ptr->SetShowCodeLineNumbers(value); return *this; }
    ElementBuilder& MinimumRange(float value) { if constexpr (requires { m_ptr->SetMinimumRange(value); }) m_ptr->SetMinimumRange(value); return *this; }
    ElementBuilder& IsClearEnabled(bool value) { if constexpr (requires { m_ptr->SetIsClearEnabled(value); }) m_ptr->SetIsClearEnabled(value); return *this; }
    ElementBuilder& ItemIcon(int id, const std::string& value) { if constexpr (requires { m_ptr->SetItemIcon(id, value); }) m_ptr->SetItemIcon(id, value); return *this; }
    ElementBuilder& CanSave(bool value) { if constexpr (requires { m_ptr->SetCanSave(value); }) m_ptr->SetCanSave(value); return *this; }
    ElementBuilder& MaxVisibleSuggestions(int value) { if constexpr (requires { m_ptr->SetMaxVisibleSuggestions(value); }) m_ptr->SetMaxVisibleSuggestions(value); return *this; }
    ElementBuilder& FlowParticlesEnabled(bool value) { if constexpr (requires { m_ptr->SetFlowParticlesEnabled(value); }) m_ptr->SetFlowParticlesEnabled(value); return *this; }
    ElementBuilder& Gesture(const std::string& value) { if constexpr (requires { m_ptr->SetGesture(value); }) m_ptr->SetGesture(value); return *this; }
    ElementBuilder& IsMinimizeButtonVisible(bool value) { if constexpr (requires { m_ptr->SetIsMinimizeButtonVisible(value); }) m_ptr->SetIsMinimizeButtonVisible(value); return *this; }
    ElementBuilder& IsMaximizeButtonVisible(bool value) { if constexpr (requires { m_ptr->SetIsMaximizeButtonVisible(value); }) m_ptr->SetIsMaximizeButtonVisible(value); return *this; }
    ElementBuilder& IsCloseButtonVisible(bool value) { if constexpr (requires { m_ptr->SetIsCloseButtonVisible(value); }) m_ptr->SetIsCloseButtonVisible(value); return *this; }
    ElementBuilder& IsMinimizeButtonEnabled(bool value) { if constexpr (requires { m_ptr->SetIsMinimizeButtonEnabled(value); }) m_ptr->SetIsMinimizeButtonEnabled(value); return *this; }
    ElementBuilder& IsMaximizeButtonEnabled(bool value) { if constexpr (requires { m_ptr->SetIsMaximizeButtonEnabled(value); }) m_ptr->SetIsMaximizeButtonEnabled(value); return *this; }
    ElementBuilder& IsCloseButtonEnabled(bool value) { if constexpr (requires { m_ptr->SetIsCloseButtonEnabled(value); }) m_ptr->SetIsCloseButtonEnabled(value); return *this; }

};

template<typename T>
ElementBuilder<T>& Borrow(ElementBuilder<T>& builder) {
    return builder;
}

template<typename T>
ElementBuilder<T> Borrow(const ElementBuilder<T>& builder) {
    return builder;
}

template<typename T>
ElementBuilder<T> Borrow(const std::shared_ptr<T>& element) {
    return ElementBuilder<T>(element);
}

template<typename T>
ElementBuilder<T> Borrow(T* element) {
    return ElementBuilder<T>(std::shared_ptr<T>(element, [](T*) {}));
}

template<typename T, typename = std::enable_if_t<!IsSharedPtr<T>::value>>
ElementBuilder<T> Borrow(T& element) {
    return ElementBuilder<T>(std::shared_ptr<T>(&element, [](T*) {}));
}


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
    u.Rows(rows);
    u.Columns(cols);
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

inline ElementBuilder<ScrollViewer> ScrollViewerWidget() { // 快速生成通用滚动视图
    return ElementBuilder<ScrollViewer>();
}

inline ElementBuilder<Panel> Expanded(std::shared_ptr<UIElement> child, float flex = 1.0f) { // 快速生成弹性延伸填充块
    auto p = ElementBuilder<Panel>();
    p.FlexGrow(flex);
    if (child) {
        child->SetFlexGrow(1.0f);
        p.AddChild(child);
    }
    return p;
}

inline ElementBuilder<Slider> SliderWidget(float val = 0.0f, float min = 0.0f, float max = 100.0f, std::function<void(Slider*, float)> onChanged = nullptr) { // 快速生成游标滑动条
    auto s = ElementBuilder<Slider>();
    s.Minimum(min);
    s.Maximum(max);
    s.Value(val);
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
    s.Minimum(min);
    s.Maximum(max);
    s.Range(lower, upper);
    if (onChanged) s.OnValueChanged(onChanged);
    return s;
}

inline ElementBuilder<ProgressBar> ProgressBarWidget(float val = 0.0f, bool isIndeterminate = false) { // 快速生成水平条形进度显示表
    auto p = ElementBuilder<ProgressBar>();
    p.Value(val);
    p.IsIndeterminate(isIndeterminate);
    return p;
}

inline ElementBuilder<ProgressRing> ProgressRingWidget(float val = 0.0f, bool isIndeterminate = true) { // 快速生成圆形旋转进度加载环
    auto p = ElementBuilder<ProgressRing>();
    p.Value(val);
    p.IsIndeterminate(isIndeterminate);
    return p;
}

inline ElementBuilder<AutoSuggestBox> AutoSuggestBoxWidget(const std::string& placeholder = "搜索…") { // 快速生成带模糊关联建议匹配的输入框
    auto a = ElementBuilder<AutoSuggestBox>();
    a.Placeholder(placeholder);
    return a;
}

inline ElementBuilder<StatusBar> StatusBarWidget() { // 快速生成底部状态控制条
    return ElementBuilder<StatusBar>();
}

inline ElementBuilder<RatingControl> RatingWidget(float value = 3.5f, int maxRating = 5) { // 快速生成五星级评分控件
    auto r = ElementBuilder<RatingControl>();
    r.MaxRating(maxRating);
    r.Value(value);
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

inline ElementBuilder<Toast> ToastWidget() { // 快速生成全局应用内通知浮层
    return ElementBuilder<Toast>();
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

inline ElementBuilder<DockManager> DockManagerWidget() { // 快速生成高级停靠窗体管理器
    return ElementBuilder<DockManager>();
}

inline ElementBuilder<WindowTitleBar> TitleBarWidget(const std::string& title = "CUI Application") { // 快速生成窗口顶部自定义标题栏
    auto t = ElementBuilder<WindowTitleBar>();
    t.Title(title);
    return t;
}

inline ElementBuilder<Image> ImageWidget() { // 快速生成图片加载盒
    return ElementBuilder<Image>();
}

inline ElementBuilder<FilePicker> FilePickerWidget(const std::string& path = "") { // 快速生成文件路径拾取器
    auto f = ElementBuilder<FilePicker>();
    if (!path.empty()) {
        f.Path(path);
    }
    return f;
}

inline ElementBuilder<FolderPicker> FolderPickerWidget(const std::string& path = "") { // 快速生成文件夹目录拾取器
    auto f = ElementBuilder<FolderPicker>();
    if (!path.empty()) {
        f.Path(path);
    }
    return f;
}

inline ElementBuilder<SegmentedControl> SegmentedWidget(std::initializer_list<const char*> items = {}) { // 快速生成 iOS 风格的左右滑动分段选择单选组
    auto s = ElementBuilder<SegmentedControl>();
    for (const char* item : items) {
        if (item && *item) {
            s.AddItem(item);
        }
    }
    return s;
}

inline ElementBuilder<NumberBox> NumberBoxWidget(double val = 0.0) { // 快速生成带上下微调箭头数值框
    auto n = ElementBuilder<NumberBox>();
    n.Value(static_cast<float>(val));
    return n;
}

inline ElementBuilder<PasswordBox> PasswordBoxWidget(const std::string& placeholder = "请输入密码") { // 快速生成遮罩密码安全输入框
    auto p = ElementBuilder<PasswordBox>();
    p.Placeholder(placeholder);
    return p;
}

inline ElementBuilder<RadioButton> RadioButtonTile(const std::string& text = "", const std::string& group = "DefaultGroup") { // 快速生成单选按钮卡片项
    auto r = ElementBuilder<RadioButton>();
    if (!text.empty()) r.Text(text);
    r.GroupName(group);
    return r;
}

inline ElementBuilder<ToggleSwitch> ToggleSwitchTile(const std::string& header = "", bool isOn = false) { // 快速生成滑道式物理开关
    auto t = ElementBuilder<ToggleSwitch>();
    if (!header.empty()) t.Header(header);
    t.IsOn(isOn);
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
    p.TotalPages(total);
    p.CurrentPage(current);
    return p;
}

inline ElementBuilder<Splitter> SplitterWidget(Orientation orientation = Orientation::Horizontal) { // 快速生成拖拽式布局调整分割条
    auto s = ElementBuilder<Splitter>();
    if (orientation == Orientation::Horizontal) {
        s.Orientation(Orientation::Horizontal);
        s.Width(-1.0f);
        s.Height(10.0f);
    } else {
        s.Orientation(Orientation::Vertical);
        s.Width(10.0f);
        s.Height(-1.0f);
    }
    s.Align(Alignment::Stretch);
    return s;
}

inline ElementBuilder<Expander> ExpanderWidget(const std::string& title = "Expander") { // 快速生成可拉伸折拢的内容卡片 Expander
    auto c = ElementBuilder<Expander>();
    c.Header(title);
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
    if (!uri.empty()) h.NavigateUri(uri);
    return h;
}

inline ElementBuilder<ContentDialog> ContentDialogWidget(const std::string& title = "Dialog", const std::string& message = "") { // 快速生成带确认取消的模态框大浮窗
    auto d = ElementBuilder<ContentDialog>();
    d.Title(title);
    if (!message.empty()) d.Message(message);
    return d;
}

/**
 * @brief 快速生成声明式矩形 Shape DOM 节点。
 * @param width 初始宽度，默认 100px。
 * @param height 初始高度，默认 50px。
 */
inline ElementBuilder<Rectangle> RectangleWidget(float width = 100.0f, float height = 50.0f) {
    auto r = ElementBuilder<Rectangle>();
    r.Width(width);
    r.Height(height);
    return r;
}

/**
 * @brief 快速生成声明式椭圆/圆形 Shape DOM 节点。
 * @param width 初始宽度，默认 50px。
 * @param height 初始高度，默认 50px。
 */
inline ElementBuilder<Ellipse> EllipseWidget(float width = 50.0f, float height = 50.0f) {
    auto e = ElementBuilder<Ellipse>();
    e.Width(width);
    e.Height(height);
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
    l.X1(x1).Y1(y1).X2(x2).Y2(y2);
    return l;
}

/**
 * @brief 快速生成声明式 SVG Path 路径 Shape DOM 节点。
 * @param data SVG path data 描述字符串（如 "M 10 10 L 90 90 Z"）。
 */
inline ElementBuilder<Path> PathWidget(const std::string& data = "") {
    auto p = ElementBuilder<Path>();
    if (!data.empty()) p.Data(data);
    return p;
}

/**
 * @brief 快速生成声明式 SVG DOM 控件节点 (SvgIcon)。支持事件独立绑定与 TintColor 着色。
 * @param source SVG XML 内容标记或文件路径。
 */
inline ElementBuilder<SvgIcon> SvgIconWidget(const std::string& source = "") {
    auto s = ElementBuilder<SvgIcon>();
    if (!source.empty()) s.Source(source);
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
    c.Width(width);
    c.Height(height);
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


namespace Fluent {

template<typename T, typename... Args>
ElementBuilder<T> Control(Args&&... args) {
    return ElementBuilder<T>(std::make_shared<T>(std::forward<Args>(args)...));
}

inline ElementBuilder<CUI::Button> Button(const std::string& text = "") {
    return Control<CUI::Button>(text);
}

inline ElementBuilder<CUI::TextBlock> TextBlock(const std::string& text = "") {
    return Control<CUI::TextBlock>(text);
}

inline ElementBuilder<CUI::TextBox> TextBox(const std::string& text = "") {
    return Control<CUI::TextBox>(text);
}

inline ElementBuilder<CUI::StackPanel> StackPanel() {
    return Control<CUI::StackPanel>();
}

inline ElementBuilder<CUI::Panel> Panel() {
    return Control<CUI::Panel>();
}

inline ElementBuilder<CUI::Grid> Grid() {
    return Control<CUI::Grid>();
}

inline ElementBuilder<CUI::ComboBox> ComboBox() {
    return Control<CUI::ComboBox>();
}

inline ElementBuilder<CUI::ListBox> ListBox() {
    return Control<CUI::ListBox>();
}

inline ElementBuilder<CUI::ListView> ListView() {
    return Control<CUI::ListView>();
}

inline ElementBuilder<CUI::CheckBox> CheckBox(const std::string& text = "") {
    return Control<CUI::CheckBox>(text);
}

inline ElementBuilder<CUI::Slider> Slider() {
    return Control<CUI::Slider>();
}
inline ElementBuilder<CUI::ToggleButton> ToggleButton(const std::string& text = "") {
    return Control<CUI::ToggleButton>(text);
}

inline ElementBuilder<CUI::DropDownButton> DropDownButton(const std::string& text = "") {
    return Control<CUI::DropDownButton>(text);
}

inline ElementBuilder<CUI::SplitButton> SplitButton(const std::string& text = "") {
    return Control<CUI::SplitButton>(text);
}

// ---------------------------------------------------------------------------
// 以下工厂保持「工厂名 == 控件类名」，统一放在 CUI::DSL::Fluent 子命名空间中，
// 既不污染 CUI 命名空间，也不会与 CUI::<ClassName> 类型名产生二义性冲突。
// 写法：Fluent::PasswordBox("请输入密码").Width(240).Build()
// 若想省略 Fluent:: 前缀，请 #include "CUI.h"（内含可选的同名快捷宏）。
// ---------------------------------------------------------------------------

inline ElementBuilder<CUI::PasswordBox> PasswordBox(const std::string& placeholder = "请输入密码") {
    return PasswordBoxWidget(placeholder);
}
inline ElementBuilder<CUI::NumberBox> NumberBox(double value = 0.0) {
    return NumberBoxWidget(value);
}
inline ElementBuilder<CUI::RadioButton> RadioButton(const std::string& text = "", const std::string& group = "DefaultGroup") {
    return RadioButtonTile(text, group);
}
inline ElementBuilder<CUI::ToggleSwitch> ToggleSwitch() {
    return ToggleSwitchWidget();
}
inline ElementBuilder<CUI::HyperlinkButton> HyperlinkButton(const std::string& text = "", const std::string& uri = "") {
    return HyperlinkButtonWidget(text, uri);
}
inline ElementBuilder<CUI::SegmentedControl> SegmentedControl(std::initializer_list<const char*> items = {}) {
    return SegmentedWidget(items);
}
inline ElementBuilder<CUI::DatePicker> DatePicker() { return DatePickerWidget(); }
inline ElementBuilder<CUI::TimePicker> TimePicker() { return TimePickerWidget(); }
inline ElementBuilder<CUI::ColorPicker> ColorPicker() { return ColorPickerWidget(); }
inline ElementBuilder<CUI::BreadcrumbBar> BreadcrumbBar() { return BreadcrumbBarWidget(); }
inline ElementBuilder<CUI::PagingControl> PagingControl(int current = 1, int total = 10) {
    return PagingControlWidget(current, total);
}
inline ElementBuilder<CUI::Splitter> Splitter(CUI::Orientation orientation = CUI::Orientation::Horizontal) {
    return SplitterWidget(orientation);
}
inline ElementBuilder<CUI::Expander> Expander(const std::string& title = "Expander") {
    return ExpanderWidget(title);
}
inline ElementBuilder<CUI::TreeView> TreeView() { return TreeViewWidget(); }
inline ElementBuilder<CUI::Image> Image() { return ImageWidget(); }
inline ElementBuilder<CUI::FilePicker> FilePicker(const std::string& path = "") { return FilePickerWidget(path); }
inline ElementBuilder<CUI::FolderPicker> FolderPicker(const std::string& path = "") { return FolderPickerWidget(path); }
inline ElementBuilder<CUI::Canvas> Canvas() { return CanvasWidget(); }
inline ElementBuilder<CUI::WrapPanel> WrapPanel(const std::string& orient = "Horizontal") { return WrapPanelWidget(orient); }
inline ElementBuilder<CUI::DockPanel> DockPanel() { return DockPanelWidget(); }
inline ElementBuilder<CUI::UniformGrid> UniformGrid(int rows = 2, int cols = 2) { return UniformGridWidget(rows, cols); }
inline ElementBuilder<CUI::ScrollViewer> ScrollViewer() { return ScrollViewerWidget(); }
inline ElementBuilder<CUI::Flyout> Flyout() { return FlyoutWidget(); }
inline ElementBuilder<CUI::MenuBar> MenuBar() { return MenuBarWidget(); }
inline ElementBuilder<CUI::CommandBar> CommandBar() { return CommandBarWidget(); }
inline ElementBuilder<CUI::InfoBar> InfoBar() { return InfoBarWidget(); }
inline ElementBuilder<CUI::Toast> Toast() { return ToastWidget(); }
inline ElementBuilder<CUI::LogView> LogView() { return LogViewWidget(); }
inline ElementBuilder<CUI::MarkdownView> MarkdownView() { return MarkdownViewWidget(); }
inline ElementBuilder<CUI::LineChart> LineChart() { return LineChartWidget(); }
inline ElementBuilder<CUI::BarChart> BarChart() { return BarChartWidget(); }
inline ElementBuilder<CUI::PieChart> PieChart() { return PieChartWidget(); }
inline ElementBuilder<CUI::TopologyView> TopologyView() { return TopologyWidget(); }
inline ElementBuilder<CUI::SvgIcon> SvgIcon(const std::string& source = "") { return SvgIconWidget(source); }
inline ElementBuilder<CUI::CanvasControl> CanvasControl(float width = 300.0f, float height = 200.0f) {
    return CanvasControlWidget(width, height);
}
inline ElementBuilder<CUI::ContentDialog> ContentDialog(const std::string& title = "Dialog", const std::string& message = "") {
    return ContentDialogWidget(title, message);
}
inline ElementBuilder<CUI::StatusBar> StatusBar() { return StatusBarWidget(); }
inline ElementBuilder<CUI::RatingControl> RatingControl(float value = 3.5f, int maxRating = 5) {
    return RatingWidget(value, maxRating);
}
inline ElementBuilder<CUI::TeachingTip> TeachingTip() { return TeachingTipWidget(); }
inline ElementBuilder<CUI::AutoSuggestBox> AutoSuggestBox(const std::string& placeholder = "搜索…") {
    return AutoSuggestBoxWidget(placeholder);
}
inline ElementBuilder<CUI::ProgressBar> ProgressBar(float value = 0.0f, bool isIndeterminate = false) {
    return ProgressBarWidget(value, isIndeterminate);
}
inline ElementBuilder<CUI::ProgressRing> ProgressRing(float value = 0.0f, bool isIndeterminate = true) {
    return ProgressRingWidget(value, isIndeterminate);
}
inline ElementBuilder<CUI::RangeSlider> RangeSlider(float lower = 20.0f, float upper = 80.0f,
                                                    float min = 0.0f, float max = 100.0f) {
    return RangeSliderWidget(lower, upper, min, max);
}
inline ElementBuilder<CUI::DockManager> DockManager() { return DockManagerWidget(); }
inline ElementBuilder<CUI::WindowTitleBar> WindowTitleBar(const std::string& title = "CUI Application") {
    return TitleBarWidget(title);
}
inline ElementBuilder<CUI::Rectangle> Rectangle(float width = 100.0f, float height = 50.0f) {
    return RectangleWidget(width, height);
}
inline ElementBuilder<CUI::Ellipse> Ellipse(float width = 50.0f, float height = 50.0f) {
    return EllipseWidget(width, height);
}
inline ElementBuilder<CUI::Line> Line(float x1 = 0, float y1 = 0, float x2 = 100, float y2 = 100) {
    return LineWidget(x1, y1, x2, y2);
}
inline ElementBuilder<CUI::Path> Path(const std::string& data = "") { return PathWidget(data); }

} // namespace Fluent
} // namespace DSL
} // namespace CUI





