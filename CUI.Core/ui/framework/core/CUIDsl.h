#pragma once

#include "Widgets.h"
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
        m_ptr->ApplyId(id);
        return *this;
    }

    ElementBuilder& Width(float w) { // 设定显式排版宽度
        m_ptr->ApplyWidth(w);
        return *this;
    }

    ElementBuilder& Height(float h) { // 设定显式排版高度
        m_ptr->ApplyHeight(h);
        return *this;
    }

    ElementBuilder& MinWidth(float w) { // 设定布局最小限制宽度
        m_ptr->ApplyMinWidth(w);
        return *this;
    }

    ElementBuilder& MinHeight(float h) { // 设定布局最小限制高度
        m_ptr->ApplyMinHeight(h);
        return *this;
    }

    /**
     * 设置元素的最大宽度
     * @param w 最大宽度值 (单位: 像素或逻辑单位)
     * @return 返回当前 ElementBuilder 引用，支持链式调用
     */
    ElementBuilder& MaxWidth(float w) {
        m_ptr->ApplyMaxWidth(w);
        return *this;
    }

    /**
     * 设置元素的最大高度
     * @param h 最大高度值 (单位: 像素或逻辑单位)
     * @return 返回当前 ElementBuilder 引用，支持链式调用
     */
    ElementBuilder& MaxHeight(float h) {
        m_ptr->ApplyMaxHeight(h);
        return *this;
    }

    /**
     * 设置元素的固定宽高尺寸
     * @param w 宽度值 (单位: 像素或逻辑单位)
     * @param h 高度值 (单位: 像素或逻辑单位)
     * @return 返回当前 ElementBuilder 引用，支持链式调用
     */
    ElementBuilder& Size(float w, float h) {
        m_ptr->ApplyWidth(w);
        m_ptr->ApplyHeight(h);
        return *this;
    }


    /**
     * 设置元素整体对齐方式
     * @param a 对齐枚举值 (如 Left, Center, Right)
     * @return 返回当前 ElementBuilder 引用，支持链式调用
     */
    ElementBuilder& Align(CUI::Alignment a) {
        m_ptr->ApplyAlign(a);
        return *this;
    }

    /**
     * 设置元素的水平对齐方式
     * @param a 水平对齐枚举值 (如 Left, Center, Right)
     * @return 返回当前 ElementBuilder 引用，支持链式调用
     */
    ElementBuilder& AlignHorizontal(CUI::Alignment a) {
        m_ptr->ApplyAlignHorizontal(a);
        return *this;
    }

    /**
     * 设置元素的垂直对齐方式
     * @param a 垂直对齐枚举值 (如 Top, Middle, Bottom)
     * @return 返回当前 ElementBuilder 引用，支持链式调用
     */
    ElementBuilder& AlignVertical(CUI::Alignment a) {
        m_ptr->ApplyAlignVertical(a);
        return *this;
    }


    ElementBuilder& Margin(float all) { // 设定四向均匀外边距
        m_ptr->ApplyMargin(Thickness(all));
        return *this;
    }

    ElementBuilder& Margin(float l, float t, float r, float b) { // 设定具体外边距数值
        m_ptr->ApplyMargin(Thickness(l, t, r, b));
        return *this;
    }

    ElementBuilder& Margin(const Thickness& margin) {
        m_ptr->ApplyMargin(margin);
        return *this;
    }

    ElementBuilder& Padding(float all) { // 设定四向均匀内边距
        m_ptr->ApplyPadding(Thickness(all));
        return *this;
    }

    ElementBuilder& Padding(float l, float t, float r, float b) { // 设定具体内边距数值
        m_ptr->ApplyPadding(Thickness(l, t, r, b));
        return *this;
    }

    ElementBuilder& Padding(const Thickness& padding) {
        m_ptr->ApplyPadding(padding);
        return *this;
    }

    ElementBuilder& FlexGrow(float flex) { // 设定弹性伸展权重
        m_ptr->ApplyFlexGrow(flex);
        return *this;
    }

    ElementBuilder& ZIndex(int zIndex) { // 设定 Canvas 子项的绘制与命中层级
        m_ptr->ApplyZIndex(zIndex);
        return *this;
    }

    ElementBuilder& TitleColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplyTitleColorToken(id); }) m_ptr->ApplyTitleColorToken(id); return *this; }
    ElementBuilder& MessageColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplyMessageColorToken(id); }) m_ptr->ApplyMessageColorToken(id); return *this; }
    ElementBuilder& AccentColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplyAccentColorToken(id); }) m_ptr->ApplyAccentColorToken(id); return *this; }
    ElementBuilder& PaneBackgroundToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplyPaneBackgroundToken(id); }) m_ptr->ApplyPaneBackgroundToken(id); return *this; }
    ElementBuilder& IndicatorColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplyIndicatorColorToken(id); }) m_ptr->ApplyIndicatorColorToken(id); return *this; }
    ElementBuilder& SecondaryColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplySecondaryColorToken(id); }) m_ptr->ApplySecondaryColorToken(id); return *this; }
    ElementBuilder& DisabledBackgroundToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplyDisabledBackgroundToken(id); }) m_ptr->ApplyDisabledBackgroundToken(id); return *this; }
    ElementBuilder& ColorToken(ThemeTokenId id) { return ForegroundToken(id); }
    ElementBuilder& ClipToBounds(bool value) { if constexpr (requires { m_ptr->ApplyClipToBounds(value); }) m_ptr->ApplyClipToBounds(value); return *this; }
    ElementBuilder& KeyboardNavigationMode(CUI::KeyboardNavigationMode mode) { if constexpr (requires { m_ptr->ApplyKeyboardNavigationMode(mode); }) m_ptr->ApplyKeyboardNavigationMode(mode); return *this; }
    ElementBuilder& OverlayScrollbar(bool value) { if constexpr (requires { m_ptr->ApplyOverlayScrollbar(value); }) m_ptr->ApplyOverlayScrollbar(value); return *this; }
    ElementBuilder& FillColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplyFillColorToken(id); }) m_ptr->ApplyFillColorToken(id); return *this; }
    ElementBuilder& TrackColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplyTrackColorToken(id); }) m_ptr->ApplyTrackColorToken(id); return *this; }
    ElementBuilder& ActiveTrackColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplyActiveTrackColorToken(id); }) m_ptr->ApplyActiveTrackColorToken(id); return *this; }
    ElementBuilder& RowHeight(float value) { if constexpr (requires { m_ptr->ApplyRowHeight(value); }) m_ptr->ApplyRowHeight(value); return *this; }
    ElementBuilder& BytesPerRow(int value) { if constexpr (requires { m_ptr->ApplyBytesPerRow(value); }) m_ptr->ApplyBytesPerRow(value); return *this; }
    ElementBuilder& GridLineBrushToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplyGridLineBrushToken(id); }) m_ptr->ApplyGridLineBrushToken(id); return *this; }
    ElementBuilder& ThumbColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplyThumbColorToken(id); }) m_ptr->ApplyThumbColorToken(id); return *this; }
    ElementBuilder& PlaceholderColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplyPlaceholderColorToken(id); }) m_ptr->ApplyPlaceholderColorToken(id); return *this; }
    ElementBuilder& DropdownBackgroundToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplyDropdownBackgroundToken(id); }) m_ptr->ApplyDropdownBackgroundToken(id); return *this; }
    ElementBuilder& SelectedItemBackgroundToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplySelectedItemBackgroundToken(id); }) m_ptr->ApplySelectedItemBackgroundToken(id); return *this; }
    ElementBuilder& UnderlineColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplyUnderlineColorToken(id); }) m_ptr->ApplyUnderlineColorToken(id); return *this; }
    ElementBuilder& ActiveUnderlineColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplyActiveUnderlineColorToken(id); }) m_ptr->ApplyActiveUnderlineColorToken(id); return *this; }
    ElementBuilder& CaretColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplyCaretColorToken(id); }) m_ptr->ApplyCaretColorToken(id); return *this; }
    ElementBuilder& OnColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplyOnColorToken(id); }) m_ptr->ApplyOnColorToken(id); return *this; }
    ElementBuilder& OffColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplyOffColorToken(id); }) m_ptr->ApplyOffColorToken(id); return *this; }
    ElementBuilder& KnobColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplyKnobColorToken(id); }) m_ptr->ApplyKnobColorToken(id); return *this; }
    ElementBuilder& HeaderBackgroundToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplyHeaderBackgroundToken(id); }) m_ptr->ApplyHeaderBackgroundToken(id); return *this; }
    ElementBuilder& ActiveTabBackgroundToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplyActiveTabBackgroundToken(id); }) m_ptr->ApplyActiveTabBackgroundToken(id); return *this; }
    ElementBuilder& InactiveTabBackgroundToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplyInactiveTabBackgroundToken(id); }) m_ptr->ApplyInactiveTabBackgroundToken(id); return *this; }
    ElementBuilder& LastChildFill(bool value) { if constexpr (requires { m_ptr->ApplyLastChildFill(value); }) m_ptr->ApplyLastChildFill(value); return *this; }
    ElementBuilder& ItemHeight(float value) { if constexpr (requires { m_ptr->ApplyItemHeight(value); }) m_ptr->ApplyItemHeight(value); return *this; }
    ElementBuilder& ActiveColorToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplyActiveColorToken(id); }) m_ptr->ApplyActiveColorToken(id); return *this; }
    ElementBuilder& CheckedBackgroundToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplyCheckedBackgroundToken(id); }) m_ptr->ApplyCheckedBackgroundToken(id); return *this; }
    ElementBuilder& BackgroundToken(ThemeTokenId id) { // 绑定背景色主题 Token
        m_ptr->ApplyBackgroundToken(id);
        return *this;
    }

    ElementBuilder& Background(D2D1_COLOR_F color) { // 设定硬编码背景颜色
        if constexpr (requires { m_ptr->ApplyBackground(color); }) m_ptr->ApplyBackground(color);
        return *this;
    }

    ElementBuilder& Background(const std::string& color) {
        return Background(Color::Hex(color));
    }

    ElementBuilder& HoverBackgroundToken(ThemeTokenId id) { // 绑定悬浮背景色主题 Token
        m_ptr->ApplyHoverBackgroundToken(id);
        return *this;
    }

    ElementBuilder& HoverBackground(D2D1_COLOR_F color) { // 设定硬编码悬浮背景颜色
        if constexpr (requires { m_ptr->ApplyHoverBackground(color); }) m_ptr->ApplyHoverBackground(color);
        return *this;
    }
    ElementBuilder& HoverBackground(const std::string& color) {
        return HoverBackground(Color::Hex(color));
    }


    ElementBuilder& PressedBackgroundToken(ThemeTokenId id) { // 绑定按下背景色主题 Token
        m_ptr->ApplyPressedBackgroundToken(id);
        return *this;
    }

    ElementBuilder& PressedBackground(D2D1_COLOR_F color) { // 设定硬编码按下背景颜色
        if constexpr (requires { m_ptr->ApplyPressedBackground(color); }) m_ptr->ApplyPressedBackground(color);
        return *this;
    }
    ElementBuilder& PressedBackground(const std::string& color) {
        return PressedBackground(Color::Hex(color));
    }


    ElementBuilder& FocusedBorderToken(ThemeTokenId id) {
        if constexpr (requires { m_ptr->ApplyFocusedBorderToken(id); }) m_ptr->ApplyFocusedBorderToken(id);
        return *this;
    }
    ElementBuilder& ForegroundToken(ThemeTokenId id) { // 绑定字元前景主题 Token
        m_ptr->ApplyColorToken(id);
        return *this;
    }

    ElementBuilder& Color(D2D1_COLOR_F color) { return Foreground(color); }

    ElementBuilder& Foreground(D2D1_COLOR_F color) {
        if constexpr (requires { m_ptr->ApplyColor(color); }) m_ptr->ApplyColor(color);
        return *this;
    }

    ElementBuilder& FontSize(float size) { // 设定字体大小 (px)
        m_ptr->ApplyFontSize(size);
        return *this;
    }

    ElementBuilder& FontFamily(const std::string& family) { // 指定渲染字体族名称
        m_ptr->ApplyFontFamily(family);
        return *this;
    }

    ElementBuilder& FontWeight(CUI::FontWeight weight) { // 设置文本字重粗细
        m_ptr->ApplyFontWeight(weight);
        return *this;
    }

    ElementBuilder& FontStyle(CUI::FontStyle style) { // 设置文本字形直立/倾斜
        m_ptr->ApplyFontStyle(style);
        return *this;
    }

    ElementBuilder& LineSpacing(float value) { if constexpr (requires { m_ptr->ApplyLineSpacing(value); }) m_ptr->ApplyLineSpacing(value); return *this; }
    ElementBuilder& FontStretch(CUI::FontStretch stretch) { // 设置字体拉伸方向
        m_ptr->ApplyFontStretch(stretch);
        return *this;
    }

    ElementBuilder& Underline(bool underline = true) { // 设定是否增加下划线修饰
        m_ptr->ApplyIsUnderline(underline);
        return *this;
    }

    ElementBuilder& Strikethrough(bool strikethrough = true) { // 设定是否增加删除线修饰
        m_ptr->ApplyIsStrikethrough(strikethrough);
        return *this;
    }

    ElementBuilder& CornerRadius(float r) { // 设定矩形边角圆角像素半径
        m_ptr->ApplyCornerRadius(r);
        return *this;
    }

    ElementBuilder& BorderToken(ThemeTokenId id, float thickness = 1.0f) { // 设定边框主题颜色和粗细
        m_ptr->ApplyBorderToken(id);
        m_ptr->ApplyBorderThickness(thickness);
        return *this;
    }

    ElementBuilder& BorderBrush(D2D1_COLOR_F color) { if constexpr (requires { m_ptr->ApplyBorderBrush(color); }) m_ptr->ApplyBorderBrush(color); return *this; }
    ElementBuilder& Border(D2D1_COLOR_F color, float thickness = 1.0f) { // 设定硬编码边框颜色和粗细
        m_ptr->ApplyBorderBrush(color);
        m_ptr->ApplyBorderThickness(thickness);
        return *this;
    }

    ElementBuilder& IsUnderline(bool value) { if constexpr (requires { m_ptr->ApplyIsUnderline(value); }) m_ptr->ApplyIsUnderline(value); return *this; }
    ElementBuilder& IsStrikethrough(bool value) { if constexpr (requires { m_ptr->ApplyIsStrikethrough(value); }) m_ptr->ApplyIsStrikethrough(value); return *this; }
    ElementBuilder& ItemWidth(float value) { if constexpr (requires { m_ptr->ApplyItemWidth(value); }) m_ptr->ApplyItemWidth(value); return *this; }
    ElementBuilder& JustifyLines(bool value) { if constexpr (requires { m_ptr->ApplyJustifyLines(value); }) m_ptr->ApplyJustifyLines(value); return *this; }
    ElementBuilder& CanvasLeft(float value) { if constexpr (requires { m_ptr->ApplyCanvasLeft(value); }) m_ptr->ApplyCanvasLeft(value); return *this; }
    ElementBuilder& CanvasTop(float value) { if constexpr (requires { m_ptr->ApplyCanvasTop(value); }) m_ptr->ApplyCanvasTop(value); return *this; }
    ElementBuilder& CanvasRight(float value) { if constexpr (requires { m_ptr->ApplyCanvasRight(value); }) m_ptr->ApplyCanvasRight(value); return *this; }
    ElementBuilder& CanvasBottom(float value) { if constexpr (requires { m_ptr->ApplyCanvasBottom(value); }) m_ptr->ApplyCanvasBottom(value); return *this; }
    ElementBuilder& Dock(CUI::Dock value) { if constexpr (requires { m_ptr->ApplyDock(value); }) m_ptr->ApplyDock(value); return *this; }
    ElementBuilder& IsEnabled(bool enabled) { // 设定控件交互可用状态
        m_ptr->ApplyIsEnabled(enabled);
        return *this;
    }

    ElementBuilder& Visibility(CUI::Visibility value) { m_ptr->ApplyVisibility(value); return *this; }
    ElementBuilder& Visibility(const std::string& vis) { // 设定控件的可见性模式
        if (vis == "Hidden") m_ptr->ApplyVisibility(CUI::Visibility::Hidden);
        else if (vis == "Collapsed") m_ptr->ApplyVisibility(CUI::Visibility::Collapsed);
        else m_ptr->ApplyVisibility(CUI::Visibility::Visible);
        return *this;
    }

    // 容器元素添加子元素
    ElementBuilder& Children(std::initializer_list<std::shared_ptr<UIElement>> list) { // 批量导入添加子控件集合
        for (auto& child : list) {
            if (child) m_ptr->AddChild(child);
        }
        return *this;
    }

    ElementBuilder& OverlayComposed(bool value) { if constexpr (requires { m_ptr->ApplyOverlayComposed(value); }) m_ptr->ApplyOverlayComposed(value); return *this; }
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
        m_ptr->ApplyText(text);
        return *this;
    }

    ElementBuilder& ToolTip(const std::string& tip) { // 设定鼠标停留信息气泡内容
        m_ptr->ApplyToolTip(tip);
        return *this;
    }

    ElementBuilder& Icon(const std::string& icon) { // 赋予图标特征
        m_ptr->ApplyIcon(icon);
        return *this;
    }
    ElementBuilder& IconText(const std::string& icon) {
        if constexpr (requires { m_ptr->ApplyIconText(icon); }) m_ptr->ApplyIconText(icon);
        return *this;
    }

    ElementBuilder& Subtitle(const std::string& subtitle) { // 设定 Expander 副标题文本
        auto expander = std::dynamic_pointer_cast<Expander>(m_ptr);
        if (expander) {
            expander->ApplySubtitle(subtitle);
        }
        return *this;
    }

    ElementBuilder& Orientation(const std::string& orient) { // 设定布局的分布朝向
        if (orient == "Horizontal" || orient == "Row") {
            m_ptr->ApplyOrientation(CUI::Orientation::Horizontal);
        } else {
            m_ptr->ApplyOrientation(CUI::Orientation::Vertical);
        }
        return *this;
    }

    ElementBuilder& Gap(float gap) { // 设定子控件之间分隔的像素间距
        m_ptr->ApplyGap(gap);
        return *this;
    }
    ElementBuilder& GridRow(int value) { m_ptr->ApplyGridRow(value); return *this; }
    ElementBuilder& GridColumn(int value) { m_ptr->ApplyGridColumn(value); return *this; }
    ElementBuilder& GridColumnSpan(int value) { m_ptr->ApplyGridColumnSpan(value); return *this; }
    ElementBuilder& GridRowSpan(int value) { m_ptr->ApplyGridRowSpan(value); return *this; }

    ElementBuilder& TextAlign(TextAlignment value) {
        if constexpr (requires { m_ptr->ApplyTextAlign(value); }) m_ptr->ApplyTextAlign(value);
        return *this;
    }

    ElementBuilder& VerticalAlign(TextVerticalAlignment value) {
        if constexpr (requires { m_ptr->ApplyVerticalAlign(value); }) m_ptr->ApplyVerticalAlign(value);
        return *this;
    }

    ElementBuilder& Justified(bool enabled = true) { m_ptr->ApplyJustifyLines(enabled); return *this; }
    ElementBuilder& FillLastLine(bool enabled = true) { m_ptr->ApplyFillLastLine(enabled); return *this; }

    ElementBuilder& ClosedCallback(std::function<void()> handler) { if constexpr (requires { m_ptr->ApplyClosedCallback(std::move(handler)); }) m_ptr->ApplyClosedCallback(std::move(handler)); return *this; }
    ElementBuilder& OnClick(std::function<void(UIElement*)> handler) { // 连接 Click 单击事件回调
        if constexpr (std::is_base_of_v<Control, T> || std::is_same_v<CUI::Button, T> || std::is_same_v<HyperlinkButton, T>) {
            m_ptr->OnClick.Connect(handler);
        }
        return *this;
    }

    ElementBuilder& Command(std::shared_ptr<CUI::Command> command) { // 绑定触发执行的 Action 命令
        m_ptr->ApplyCommand(std::move(command));
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

    ElementBuilder& SelectedBackgroundToken(ThemeTokenId id) { if constexpr (requires { m_ptr->ApplySelectedBackgroundToken(id); }) m_ptr->ApplySelectedBackgroundToken(id); return *this; }

    ElementBuilder& IndentWidth(float value) { if constexpr (requires { m_ptr->ApplyIndentWidth(value); }) m_ptr->ApplyIndentWidth(value); return *this; }
    ElementBuilder& OnSelectionChanged(std::function<void(TreeView*, std::shared_ptr<TreeViewItem>)> handler) { if constexpr (requires { m_ptr->OnSelectionChanged(); }) m_ptr->OnSelectionChanged().Connect(std::move(handler)); return *this; }
    ElementBuilder& OnItemToggled(std::function<void(TreeView*, std::shared_ptr<TreeViewItem>)> handler) { if constexpr (requires { m_ptr->OnItemToggled(); }) m_ptr->OnItemToggled().Connect(std::move(handler)); return *this; }
    ElementBuilder& OnItemDoubleClicked(std::function<void(TreeView*, std::shared_ptr<TreeViewItem>)> handler) { if constexpr (requires { m_ptr->OnItemDoubleClicked(); }) m_ptr->OnItemDoubleClicked().Connect(std::move(handler)); return *this; }
    ElementBuilder& OnBreadcrumbItemClicked(std::function<void(BreadcrumbBar*, int, const std::string&)> handler) { if constexpr (requires { m_ptr->OnItemClicked(); }) m_ptr->OnItemClicked().Connect(std::move(handler)); return *this; }
    ElementBuilder& ClearItems() { if constexpr (requires { m_ptr->ClearItems(); }) m_ptr->ClearItems(); return *this; }
    ElementBuilder& AddTreeItem(std::shared_ptr<TreeViewItem> item) { if constexpr (requires { m_ptr->AddItem(item); }) m_ptr->AddItem(std::move(item)); return *this; }
    ElementBuilder& SelectedTreeItem(std::shared_ptr<TreeViewItem> item) { if constexpr (requires { m_ptr->ApplySelectedItem(item); }) m_ptr->ApplySelectedItem(std::move(item)); return *this; }
    ElementBuilder& PathNodes(const std::vector<std::string>& value) { if constexpr (requires { m_ptr->ApplyPath(value); }) m_ptr->ApplyPath(value); return *this; }    ElementBuilder& OnSelectionChanged(std::function<void(ComboBox*, int, const std::string&)> handler) {
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
    ElementBuilder& ParentContextMenu(ContextMenu* value) { if constexpr (requires { m_ptr->ApplyParentContextMenu(value); }) m_ptr->ApplyParentContextMenu(value); return *this; }
    ElementBuilder& SubMenu(std::shared_ptr<ContextMenu> value) { if constexpr (requires { m_ptr->ApplySubMenu(value); }) m_ptr->ApplySubMenu(std::move(value)); return *this; }
    ElementBuilder& ShortcutText(const std::string& value) { if constexpr (requires { m_ptr->ApplyShortcutText(value); }) m_ptr->ApplyShortcutText(value); return *this; }
    ElementBuilder& IsSeparator(bool value = true) { if constexpr (requires { m_ptr->ApplyIsSeparator(value); }) m_ptr->ApplyIsSeparator(value); return *this; }    ElementBuilder& Checked(bool value = true) { if constexpr (requires { m_ptr->ApplyChecked(value); }) m_ptr->ApplyChecked(value); return *this; }    ElementBuilder& AddItem(const std::string& item, std::function<void()> handler) {
        if constexpr (requires { m_ptr->AddItem(item, handler); }) m_ptr->AddItem(item, std::move(handler));
        return *this;
    }

    ElementBuilder& AddSeparator() {
        if constexpr (requires { m_ptr->AddSeparator(); }) m_ptr->AddSeparator();
        return *this;
    }


    ElementBuilder& BorderThickness(float thickness) {
        m_ptr->ApplyBorderThickness(thickness);
        return *this;
    }

    ElementBuilder& Orientation(CUI::Orientation o) {
        m_ptr->ApplyOrientation(o);
        return *this;
    }

    ElementBuilder& Items(const std::vector<std::shared_ptr<CUI::TreeViewItem>>& items) { if constexpr (requires { m_ptr->ApplyItems(items); }) m_ptr->ApplyItems(items); return *this; }

    template<typename ItemsT>
    ElementBuilder& Items(ItemsT&& items) {
        if constexpr (requires { m_ptr->Items = std::forward<ItemsT>(items); }) {
            m_ptr->Items = std::forward<ItemsT>(items);
        } else if constexpr (requires { m_ptr->ApplyItems(std::forward<ItemsT>(items)); }) {
            m_ptr->ApplyItems(std::forward<ItemsT>(items));
        }
        return *this;
    }

    ElementBuilder& Items(std::initializer_list<std::string> items) {
        if constexpr (requires { m_ptr->Items = items; }) {
            m_ptr->Items = items;
        } else if constexpr (requires { m_ptr->ApplyItems(items); }) {
            m_ptr->ApplyItems(items);
        }
        return *this;
    }

    template<typename V>
    ElementBuilder& Value(V&& v) {
        if constexpr (requires { m_ptr->Value = std::forward<V>(v); }) {
            m_ptr->Value = std::forward<V>(v);
        } else if constexpr (requires { m_ptr->ApplyValue(std::forward<V>(v)); }) {
            m_ptr->ApplyValue(std::forward<V>(v));
        }
        return *this;
    }

    ElementBuilder& Minimum(float minVal) {
        if constexpr (requires { m_ptr->Minimum = minVal; }) {
            m_ptr->Minimum = minVal;
        } else if constexpr (requires { m_ptr->ApplyMinimum(minVal); }) {
            m_ptr->ApplyMinimum(minVal);
        }
        return *this;
    }

    ElementBuilder& Maximum(float maxVal) {
        if constexpr (requires { m_ptr->Maximum = maxVal; }) {
            m_ptr->Maximum = maxVal;
        } else if constexpr (requires { m_ptr->ApplyMaximum(maxVal); }) {
            m_ptr->ApplyMaximum(maxVal);
        }
        return *this;
    }

    ElementBuilder& Step(float s) {
        if constexpr (requires { m_ptr->Step = s; }) {
            m_ptr->Step = s;
        } else if constexpr (requires { m_ptr->ApplyStep(s); }) {
            m_ptr->ApplyStep(s);
        }
        return *this;
    }

    ElementBuilder& IsReadOnly(bool ro) {
        if constexpr (requires { m_ptr->IsReadOnly = ro; }) {
            m_ptr->IsReadOnly = ro;
        } else if constexpr (requires { m_ptr->ApplyIsReadOnly(ro); }) {
            m_ptr->ApplyIsReadOnly(ro);
        }
        return *this;
    }

    ElementBuilder& IsExpanded(bool exp) {
        if constexpr (requires { m_ptr->IsExpanded = exp; }) {
            m_ptr->IsExpanded = exp;
        } else if constexpr (requires { m_ptr->ApplyIsExpanded(exp); }) {
            m_ptr->ApplyIsExpanded(exp);
        }
        return *this;
    }

    ElementBuilder& ColumnDefinitions(const std::string& defs) {
        if constexpr (requires { m_ptr->ColumnDefinitions = defs; }) {
            m_ptr->ColumnDefinitions = defs;
        } else if constexpr (requires { m_ptr->ApplyColumnDefinitions(defs); }) {
            m_ptr->ApplyColumnDefinitions(defs);
        }
        return *this;
    }

    ElementBuilder& RowDefinitions(const std::string& defs) {
        if constexpr (requires { m_ptr->RowDefinitions = defs; }) {
            m_ptr->RowDefinitions = defs;
        } else if constexpr (requires { m_ptr->ApplyRowDefinitions(defs); }) {
            m_ptr->ApplyRowDefinitions(defs);
        }
        return *this;
    }

    ElementBuilder& SelectedIndex(int value) { if constexpr (requires { m_ptr->ApplySelectedIndex(value); }) m_ptr->ApplySelectedIndex(value); return *this; }
    ElementBuilder& SelectionMode(ListBoxSelectionMode value) { if constexpr (requires { m_ptr->ApplySelectionMode(value); }) m_ptr->ApplySelectionMode(value); return *this; }
    ElementBuilder& SelectionMode(ListViewSelectionMode value) { if constexpr (requires { m_ptr->ApplySelectionMode(value); }) m_ptr->ApplySelectionMode(value); return *this; }
    ElementBuilder& ShowScrollBars(bool value) { if constexpr (requires { m_ptr->ApplyShowScrollBars(value); }) m_ptr->ApplyShowScrollBars(value); return *this; }
    ElementBuilder& VirtualCount(size_t count) { if constexpr (requires { m_ptr->ApplyVirtualCount(count); }) m_ptr->ApplyVirtualCount(count); return *this; }
    template<typename DataSourceT> ElementBuilder& VirtualMode(int count, DataSourceT* source) { if constexpr (requires { m_ptr->ApplyVirtualMode(count, source); }) m_ptr->ApplyVirtualMode(count, source); return *this; }
    ElementBuilder& ExpandDirection(ExpandDirection value) { if constexpr (requires { m_ptr->ApplyExpandDirection(value); }) m_ptr->ApplyExpandDirection(value); return *this; }
    ElementBuilder& Placement(FlyoutPlacement value) { if constexpr (requires { m_ptr->ApplyPlacement(value); }) m_ptr->ApplyPlacement(value); return *this; }
    ElementBuilder& IsCloseVisible(bool value) { if constexpr (requires { m_ptr->ApplyIsCloseVisible(value); }) m_ptr->ApplyIsCloseVisible(value); return *this; }
    ElementBuilder& IsModal(bool value) { if constexpr (requires { m_ptr->ApplyIsModal(value); }) m_ptr->ApplyIsModal(value); return *this; }
    ElementBuilder& PreferredPlacement(BubblePlacement value) { if constexpr (requires { m_ptr->ApplyPreferredPlacement(value); }) m_ptr->ApplyPreferredPlacement(value); return *this; }
    ElementBuilder& TintColor(D2D1_COLOR_F value) { if constexpr (requires { m_ptr->ApplyTintColor(value); }) m_ptr->ApplyTintColor(value); return *this; }
    template<typename ItemT> ElementBuilder& ItemExpanded(const std::shared_ptr<ItemT>& item, bool value) { if constexpr (requires { m_ptr->ApplyItemExpanded(item, value); }) m_ptr->ApplyItemExpanded(item, value); return *this; }
    ElementBuilder& State(CheckState value) { if constexpr (requires { m_ptr->ApplyState(value); }) m_ptr->ApplyState(value); return *this; }    ElementBuilder& IsPasswordMode(bool value) { if constexpr (requires { m_ptr->ApplyIsPasswordMode(value); }) m_ptr->ApplyIsPasswordMode(value); return *this; }
    ElementBuilder& ShowRevealButton(bool value) { if constexpr (requires { m_ptr->ApplyShowRevealButton(value); }) m_ptr->ApplyShowRevealButton(value); return *this; }
    ElementBuilder& AcceptsReturn(bool value) { if constexpr (requires { m_ptr->ApplyAcceptsReturn(value); }) m_ptr->ApplyAcceptsReturn(value); return *this; }
    ElementBuilder& TextWrapping(bool value) { if constexpr (requires { m_ptr->ApplyTextWrapping(value); }) m_ptr->ApplyTextWrapping(value); return *this; }
    ElementBuilder& AllowDrag(bool value) { if constexpr (requires { m_ptr->ApplyAllowDrag(value); }) m_ptr->ApplyAllowDrag(value); return *this; }
    ElementBuilder& AllowDrop(bool value) { if constexpr (requires { m_ptr->ApplyAllowDrop(value); }) m_ptr->ApplyAllowDrop(value); return *this; }
    ElementBuilder& AcceptsTab(bool value) { if constexpr (requires { m_ptr->ApplyAcceptsTab(value); }) m_ptr->ApplyAcceptsTab(value); return *this; }
    ElementBuilder& ToolTipMaxWidth(float value) { if constexpr (requires { m_ptr->ApplyToolTipMaxWidth(value); }) m_ptr->ApplyToolTipMaxWidth(value); return *this; }
    ElementBuilder& ToolTipAutoHideMs(int value) { if constexpr (requires { m_ptr->ApplyToolTipAutoHideMs(value); }) m_ptr->ApplyToolTipAutoHideMs(value); return *this; }
    ElementBuilder& SelectAll() { if constexpr (requires { m_ptr->SelectAll(); }) m_ptr->SelectAll(); return *this; }    ElementBuilder& Placeholder(const std::string& text) {
        if constexpr (requires { m_ptr->Placeholder = text; }) {
            m_ptr->Placeholder = text;
        } else if constexpr (requires { m_ptr->ApplyPlaceholder(text); }) {
            m_ptr->ApplyPlaceholder(text);
        }
        return *this;
    }

    ElementBuilder& Title(const std::string& t) {
        if constexpr (requires { m_ptr->Title = t; }) {
            m_ptr->Title = t;
        } else if constexpr (requires { m_ptr->ApplyTitle(t); }) {
            m_ptr->ApplyTitle(t);
        }
        return *this;
    }

    ElementBuilder& Message(const std::string& m) {
        if constexpr (requires { m_ptr->Message = m; }) {
            m_ptr->Message = m;
        } else if constexpr (requires { m_ptr->ApplyMessage(m); }) {
            m_ptr->ApplyMessage(m);
        }
        return *this;
    }

    ElementBuilder& Nodes(const std::vector<std::shared_ptr<TopologyNode>>& nodes) {
        if constexpr (requires { m_ptr->Nodes = nodes; }) {
            m_ptr->Nodes = nodes;
        } else if constexpr (requires { m_ptr->ApplyNodes(nodes); }) {
            m_ptr->ApplyNodes(nodes);
        }
        return *this;
    }

    ElementBuilder& Edges(const std::vector<TopologyEdge>& edges) {
        if constexpr (requires { m_ptr->Edges = edges; }) {
            m_ptr->Edges = edges;
        } else if constexpr (requires { m_ptr->ApplyEdges(edges); }) {
            m_ptr->ApplyEdges(edges);
        }
        return *this;
    }

    ElementBuilder& LayoutType(TopologyLayoutType t) {
        if constexpr (requires { m_ptr->LayoutType = t; }) {
            m_ptr->LayoutType = t;
        } else if constexpr (requires { m_ptr->ApplyLayoutType(t); }) {
            m_ptr->ApplyLayoutType(t);
        }
        return *this;
    }

    ElementBuilder& FlowParticles(bool enabled = true) {
        if constexpr (requires { m_ptr->FlowParticles = enabled; }) {
            m_ptr->FlowParticles = enabled;
        } else if constexpr (requires { m_ptr->ApplyFlowParticlesEnabled(enabled); }) {
            m_ptr->ApplyFlowParticlesEnabled(enabled);
        }
        return *this;
    }
    template<typename RowsT>
    ElementBuilder& Rows(RowsT&& value) {
        if constexpr (requires { m_ptr->Rows = std::forward<RowsT>(value); }) m_ptr->Rows = std::forward<RowsT>(value);
        else if constexpr (requires { m_ptr->ApplyRows(std::forward<RowsT>(value)); }) m_ptr->ApplyRows(std::forward<RowsT>(value));
        return *this;
    }

    ElementBuilder& Columns(int value) {
        if constexpr (requires { m_ptr->ApplyColumns(value); }) m_ptr->ApplyColumns(value);
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
        if constexpr (requires { m_ptr->ApplyRange(lower, upper); }) m_ptr->ApplyRange(lower, upper);
        return *this;
    }

    ElementBuilder& IsIndeterminate(bool value) {
        if constexpr (requires { m_ptr->ApplyIsIndeterminate(value); }) m_ptr->ApplyIsIndeterminate(value);
        return *this;
    }

    ElementBuilder& MaxRating(int value) {
        if constexpr (requires { m_ptr->ApplyMaxRating(value); }) m_ptr->ApplyMaxRating(value);
        return *this;
    }

    ElementBuilder& GroupName(const std::string& value) {
        if constexpr (requires { m_ptr->ApplyGroupName(value); }) m_ptr->ApplyGroupName(value);
        return *this;
    }

    ElementBuilder& IsChecked(bool value) { if constexpr (requires { m_ptr->ApplyIsChecked(value); }) m_ptr->ApplyIsChecked(value); return *this; }
    ElementBuilder& IsOn(bool value) {
        if constexpr (requires { m_ptr->ApplyIsOn(value); }) m_ptr->ApplyIsOn(value);
        return *this;
    }

    ElementBuilder& TotalPages(int value) {
        if constexpr (requires { m_ptr->ApplyTotalPages(value); }) m_ptr->ApplyTotalPages(value);
        return *this;
    }

    ElementBuilder& CurrentPage(int value) {
        if constexpr (requires { m_ptr->ApplyCurrentPage(value); }) m_ptr->ApplyCurrentPage(value);
        return *this;
    }
    ElementBuilder& CurrentPage(const std::string& value) { if constexpr (requires { m_ptr->ApplyCurrentPage(value); }) m_ptr->ApplyCurrentPage(value); return *this; }


    ElementBuilder& Header(const std::string& value) {
        if constexpr (requires { m_ptr->ApplyHeader(value); }) m_ptr->ApplyHeader(value);
        return *this;
    }

    ElementBuilder& NavigateUri(const std::string& value) {
        if constexpr (requires { m_ptr->ApplyNavigateUri(value); }) m_ptr->ApplyNavigateUri(value);
        return *this;
    }

    ElementBuilder& Path(const std::vector<std::string>& value) { if constexpr (requires { m_ptr->ApplyPath(value); }) m_ptr->ApplyPath(value); return *this; }
    ElementBuilder& Path(const std::string& value) {
        if constexpr (requires { m_ptr->ApplyPath(value); }) m_ptr->ApplyPath(value);
        return *this;
    }

    ElementBuilder& Data(const std::string& value) {
        if constexpr (requires { m_ptr->ApplyData(value); }) m_ptr->ApplyData(value);
        return *this;
    }

    ElementBuilder& Stretch(CUI::Stretch value) { if constexpr (requires { m_ptr->ApplyStretch(value); }) m_ptr->ApplyStretch(value); return *this; }
    ElementBuilder& StretchMode(CUI::Stretch value) { if constexpr (requires { m_ptr->ApplyStretch(value); }) m_ptr->ApplyStretch(value); return *this; }
    ElementBuilder& BadgeText(const std::string& value) { if constexpr (requires { m_ptr->ApplyBadgeText(value); }) m_ptr->ApplyBadgeText(value); return *this; }
    ElementBuilder& BadgeColor(D2D1_COLOR_F value) { if constexpr (requires { m_ptr->ApplyBadgeColor(value); }) m_ptr->ApplyBadgeColor(value); return *this; }    ElementBuilder& Source(const std::string& value) {
        if constexpr (requires { m_ptr->ApplySource(value); }) m_ptr->ApplySource(value);
        return *this;
    }

    ElementBuilder& X1(float value) { if constexpr (requires { m_ptr->ApplyX1(value); }) m_ptr->ApplyX1(value); return *this; }
    ElementBuilder& Y1(float value) { if constexpr (requires { m_ptr->ApplyY1(value); }) m_ptr->ApplyY1(value); return *this; }
    ElementBuilder& X2(float value) { if constexpr (requires { m_ptr->ApplyX2(value); }) m_ptr->ApplyX2(value); return *this; }
    ElementBuilder& Y2(float value) { if constexpr (requires { m_ptr->ApplyY2(value); }) m_ptr->ApplyY2(value); return *this; }

    ElementBuilder& PaneTitle(const std::string& value) { if constexpr (requires { m_ptr->ApplyPaneTitle(value); }) m_ptr->ApplyPaneTitle(value); return *this; }
    ElementBuilder& PaneDisplayMode(NavigationViewPaneDisplayMode value) { if constexpr (requires { m_ptr->ApplyPaneDisplayMode(value); }) m_ptr->ApplyPaneDisplayMode(value); return *this; }
    ElementBuilder& IsPaneOpen(bool value) { if constexpr (requires { m_ptr->ApplyIsPaneOpen(value); }) m_ptr->ApplyIsPaneOpen(value); return *this; }
    ElementBuilder& OpenPaneLength(float value) { if constexpr (requires { m_ptr->ApplyOpenPaneLength(value); }) m_ptr->ApplyOpenPaneLength(value); return *this; }
    ElementBuilder& CompactPaneLength(float value) { if constexpr (requires { m_ptr->ApplyCompactPaneLength(value); }) m_ptr->ApplyCompactPaneLength(value); return *this; }
    ElementBuilder& CompactModeThresholdWidth(float value) { if constexpr (requires { m_ptr->ApplyCompactModeThresholdWidth(value); }) m_ptr->ApplyCompactModeThresholdWidth(value); return *this; }
    ElementBuilder& ExpandedModeThresholdWidth(float value) { if constexpr (requires { m_ptr->ApplyExpandedModeThresholdWidth(value); }) m_ptr->ApplyExpandedModeThresholdWidth(value); return *this; }
    ElementBuilder& AlwaysShowHeader(bool value) { if constexpr (requires { m_ptr->ApplyAlwaysShowHeader(value); }) m_ptr->ApplyAlwaysShowHeader(value); return *this; }
    ElementBuilder& IsSettingsVisible(bool value) { if constexpr (requires { m_ptr->ApplyIsSettingsVisible(value); }) m_ptr->ApplyIsSettingsVisible(value); return *this; }
    ElementBuilder& Content(std::shared_ptr<UIElement> value) { if constexpr (requires { m_ptr->ApplyContent(value); }) m_ptr->ApplyContent(std::move(value)); return *this; }
    ElementBuilder& ContentFactory(std::function<std::shared_ptr<UIElement>()> value) { if constexpr (requires { m_ptr->ApplyContentFactory(std::move(value)); }) m_ptr->ApplyContentFactory(std::move(value)); return *this; }
    ElementBuilder& AutoSuggestBox(std::shared_ptr<UIElement> value) { if constexpr (requires { m_ptr->ApplyAutoSuggestBox(value); }) m_ptr->ApplyAutoSuggestBox(std::move(value)); return *this; }
    ElementBuilder& IsBackButtonVisible(NavigationViewBackButtonVisible value) { if constexpr (requires { m_ptr->ApplyIsBackButtonVisible(value); }) m_ptr->ApplyIsBackButtonVisible(value); return *this; }
    ElementBuilder& IsBackEnabled(bool value) { if constexpr (requires { m_ptr->ApplyIsBackEnabled(value); }) m_ptr->ApplyIsBackEnabled(value); return *this; }
    ElementBuilder& SelectedItem(NavigationViewItem* value) { if constexpr (requires { m_ptr->ApplySelectedItem(value); }) m_ptr->ApplySelectedItem(value); return *this; }
    template<typename ItemT> ElementBuilder& SelectedItem(const std::shared_ptr<ItemT>& value) { if constexpr (requires { m_ptr->ApplySelectedItem(value); }) m_ptr->ApplySelectedItem(value); return *this; }
    ElementBuilder& Severity(InfoBarSeverity value) { if constexpr (requires { m_ptr->ApplySeverity(value); }) m_ptr->ApplySeverity(value); return *this; }
    ElementBuilder& IsClosable(bool value) { if constexpr (requires { m_ptr->ApplyIsClosable(value); }) m_ptr->ApplyIsClosable(value); return *this; }
    ElementBuilder& ActionText(const std::string& value) { if constexpr (requires { m_ptr->ApplyActionText(value); }) m_ptr->ApplyActionText(value); return *this; }
    ElementBuilder& IsOpen(bool value) { if constexpr (requires { m_ptr->ApplyIsOpen(value); }) m_ptr->ApplyIsOpen(value); return *this; }
    ElementBuilder& AddMenuItem(std::shared_ptr<NavigationViewItemBase> value) { if constexpr (requires { m_ptr->AddMenuItem(value); }) m_ptr->AddMenuItem(std::move(value)); return *this; }
    ElementBuilder& AddFooterMenuItem(std::shared_ptr<NavigationViewItemBase> value) { if constexpr (requires { m_ptr->AddFooterMenuItem(value); }) m_ptr->AddFooterMenuItem(std::move(value)); return *this; }
    ElementBuilder& Tag(const std::string& value) { if constexpr (requires { m_ptr->ApplyTag(value); }) m_ptr->ApplyTag(value); return *this; }
    ElementBuilder& SelectsOnInvoked(bool value) { if constexpr (requires { m_ptr->ApplySelectsOnInvoked(value); }) m_ptr->ApplySelectsOnInvoked(value); return *this; }
    ElementBuilder& AddNestedItem(std::shared_ptr<NavigationViewItemBase> value) { if constexpr (requires { m_ptr->AddMenuItem(value); }) m_ptr->AddMenuItem(std::move(value)); return *this; }
    ElementBuilder& Owner(NavigationView* value) { if constexpr (requires { m_ptr->ApplyOwner(value); }) m_ptr->ApplyOwner(value); return *this; }
    ElementBuilder& Compact(bool value) { if constexpr (requires { m_ptr->ApplyCompact(value); }) m_ptr->ApplyCompact(value); return *this; }
    ElementBuilder& TopMode(bool value) { if constexpr (requires { m_ptr->ApplyTopMode(value); }) m_ptr->ApplyTopMode(value); return *this; }
    ElementBuilder& IsSelected(bool value) { if constexpr (requires { m_ptr->ApplyIsSelected(value); }) m_ptr->ApplyIsSelected(value); return *this; }
    ElementBuilder& IsChildSelected(bool value) { if constexpr (requires { m_ptr->ApplyIsChildSelected(value); }) m_ptr->ApplyIsChildSelected(value); return *this; }
    ElementBuilder& IsExpandedSilent(bool value) { if constexpr (requires { m_ptr->ApplyIsExpandedSilent(value); }) m_ptr->ApplyIsExpandedSilent(value); return *this; }
    ElementBuilder& Opacity(float value) { if constexpr (requires { m_ptr->ApplyOpacity(value); }) m_ptr->ApplyOpacity(value); return *this; }
    ElementBuilder& ComposeOpacity(float value) { if constexpr (requires { m_ptr->ApplyComposeOpacity(value); }) m_ptr->ApplyComposeOpacity(value); return *this; }
    ElementBuilder& OnInvoked(std::function<void(NavigationViewItem*)> handler) { if constexpr (requires { m_ptr->OnInvoked(); }) m_ptr->OnInvoked().Connect(std::move(handler)); return *this; }
    ElementBuilder& OnExpandChanged(std::function<void(NavigationViewItem*)> handler) { if constexpr (requires { m_ptr->OnExpandChanged(); }) m_ptr->OnExpandChanged().Connect(std::move(handler)); return *this; }    ElementBuilder& OnNavigationItemInvoked(std::function<void(NavigationView*, const NavigationViewItemInvokedEventArgs&)> handler) { if constexpr (requires { m_ptr->OnItemInvoked(); }) m_ptr->OnItemInvoked().Connect(std::move(handler)); return *this; }
    ElementBuilder& OnNavigationBackRequested(std::function<void(NavigationView*)> handler) { if constexpr (requires { m_ptr->OnBackRequested(); }) m_ptr->OnBackRequested().Connect(std::move(handler)); return *this; }
    ElementBuilder& Filter(const std::string& name, const std::string& spec) { if constexpr (requires { m_ptr->ApplyFilter(name, spec); }) m_ptr->ApplyFilter(name, spec); return *this; }
    ElementBuilder& ToastTypeValue(ToastType value) { if constexpr (requires { m_ptr->ApplyType(value); }) m_ptr->ApplyType(value); return *this; }
    ElementBuilder& Corner(ToastCorner value) { if constexpr (requires { m_ptr->ApplyCorner(value); }) m_ptr->ApplyCorner(value); return *this; }
    ElementBuilder& DurationMs(int value) { if constexpr (requires { m_ptr->ApplyDurationMs(value); }) m_ptr->ApplyDurationMs(value); return *this; }
    ElementBuilder& AutoClose(bool value) { if constexpr (requires { m_ptr->ApplyAutoClose(value); }) m_ptr->ApplyAutoClose(value); return *this; }
    ElementBuilder& Closeable(bool value) { if constexpr (requires { m_ptr->ApplyCloseable(value); }) m_ptr->ApplyCloseable(value); return *this; }
    ElementBuilder& Accent(const std::string& value) { if constexpr (requires { m_ptr->ApplyAccent(value); }) m_ptr->ApplyAccent(value); return *this; }
    ElementBuilder& TitleColor(const std::string& value) { if constexpr (requires { m_ptr->ApplyTitleColor(value); }) m_ptr->ApplyTitleColor(value); return *this; }
    ElementBuilder& MessageColor(const std::string& value) { if constexpr (requires { m_ptr->ApplyMessageColor(value); }) m_ptr->ApplyMessageColor(value); return *this; }
    ElementBuilder& OffsetX(float value) { if constexpr (requires { m_ptr->ApplyOffsetX(value); }) m_ptr->ApplyOffsetX(value); return *this; }
    ElementBuilder& OffsetY(float value) { if constexpr (requires { m_ptr->ApplyOffsetY(value); }) m_ptr->ApplyOffsetY(value); return *this; }
    ElementBuilder& Spacing(float value) { if constexpr (requires { m_ptr->ApplySpacing(value); }) m_ptr->ApplySpacing(value); return *this; }    ElementBuilder& PrimaryButtonText(const std::string& value) { if constexpr (requires { m_ptr->ApplyPrimaryButtonText(value); }) m_ptr->ApplyPrimaryButtonText(value); return *this; }
    ElementBuilder& SecondaryButtonText(const std::string& value) { if constexpr (requires { m_ptr->ApplySecondaryButtonText(value); }) m_ptr->ApplySecondaryButtonText(value); return *this; }
    ElementBuilder& CloseButtonText(const std::string& value) { if constexpr (requires { m_ptr->ApplyCloseButtonText(value); }) m_ptr->ApplyCloseButtonText(value); return *this; }
    ElementBuilder& InputEnabled(bool value, bool multiline = false) { if constexpr (requires { m_ptr->ApplyInputEnabled(value, multiline); }) m_ptr->ApplyInputEnabled(value, multiline); return *this; }
    ElementBuilder& InputText(const std::string& value) { if constexpr (requires { m_ptr->ApplyInputText(value); }) m_ptr->ApplyInputText(value); return *this; }    ElementBuilder& DialogTitle(const std::string& value) { if constexpr (requires { m_ptr->ApplyDialogTitle(value); }) m_ptr->ApplyDialogTitle(value); return *this; }
    ElementBuilder& OnPathChanged(std::function<void(FilePicker*, const std::string&)> handler) { if constexpr (requires { m_ptr->OnPathChanged(); }) m_ptr->OnPathChanged().Connect(std::move(handler)); return *this; }    ElementBuilder& OnNavigationDisplayModeChanged(std::function<void(NavigationView*, const NavigationViewDisplayModeChangedEventArgs&)> handler) { if constexpr (requires { m_ptr->OnDisplayModeChanged(); }) m_ptr->OnDisplayModeChanged().Connect(std::move(handler)); return *this; }

    ElementBuilder& ActiveContextMenu(std::shared_ptr<CUI::ContextMenu> value) { if constexpr (requires { m_ptr->ApplyActiveContextMenu(value); }) m_ptr->ApplyActiveContextMenu(std::move(value)); return *this; }
    ElementBuilder& BackdropType(CUI::BackdropType value) { if constexpr (requires { m_ptr->ApplyBackdropType(value); }) m_ptr->ApplyBackdropType(value); return *this; }
    ElementBuilder& RenderStatsOverlayVisible(bool value) { if constexpr (requires { m_ptr->ApplyRenderStatsOverlayVisible(value); }) m_ptr->ApplyRenderStatsOverlayVisible(value); return *this; }
    ElementBuilder& ThemeMode(CUI::ThemeMode value) { if constexpr (requires { m_ptr->ApplyThemeMode(value); }) m_ptr->ApplyThemeMode(value); return *this; }
    ElementBuilder& ThemeModeWithRipple(CUI::ThemeMode value, Point origin) { if constexpr (requires { m_ptr->ApplyThemeModeWithRipple(value, origin); }) m_ptr->ApplyThemeModeWithRipple(value, origin); return *this; }
    ElementBuilder& ContextMenu(std::shared_ptr<CUI::ContextMenu> value) { if constexpr (requires { m_ptr->ApplyContextMenu(value); }) m_ptr->ApplyContextMenu(std::move(value)); return *this; }
    ElementBuilder& RightContent(std::shared_ptr<UIElement> value) { if constexpr (requires { m_ptr->ApplyRightContent(value); }) m_ptr->ApplyRightContent(std::move(value)); return *this; }
    ElementBuilder& CaretIndex(int value) { if constexpr (requires { m_ptr->ApplyCaretIndex(value); }) m_ptr->ApplyCaretIndex(value); return *this; }
    ElementBuilder& ColumnVisible(int index, bool value) { if constexpr (requires { m_ptr->ApplyColumnVisible(index, value); }) m_ptr->ApplyColumnVisible(index, value); return *this; }
    ElementBuilder& RowSelected(int index, bool value) { if constexpr (requires { m_ptr->ApplyRowSelected(index, value); }) m_ptr->ApplyRowSelected(index, value); return *this; }
    ElementBuilder& Date(int year, int month, int day) { if constexpr (requires { m_ptr->ApplyDate(year, month, day); }) m_ptr->ApplyDate(year, month, day); return *this; }
    ElementBuilder& Time(int hour, int minute) { if constexpr (requires { m_ptr->ApplyTime(hour, minute); }) m_ptr->ApplyTime(hour, minute); return *this; }
    ElementBuilder& Fill(D2D1_COLOR_F value) { if constexpr (requires { m_ptr->ApplyFill(value); }) m_ptr->ApplyFill(value); return *this; }
    ElementBuilder& Stroke(D2D1_COLOR_F value) { if constexpr (requires { m_ptr->ApplyStroke(value); }) m_ptr->ApplyStroke(value); return *this; }
    ElementBuilder& StrokeThickness(float value) { if constexpr (requires { m_ptr->ApplyStrokeThickness(value); }) m_ptr->ApplyStrokeThickness(value); return *this; }
    ElementBuilder& Damping(float value) { if constexpr (requires { m_ptr->ApplyDamping(value); }) m_ptr->ApplyDamping(value); return *this; }
    ElementBuilder& Stiffness(float value) { if constexpr (requires { m_ptr->ApplyStiffness(value); }) m_ptr->ApplyStiffness(value); return *this; }
    ElementBuilder& Running(bool value) { if constexpr (requires { m_ptr->ApplyRunning(value); }) m_ptr->ApplyRunning(value); return *this; }
    ElementBuilder& Viewport(float width, float height) { if constexpr (requires { m_ptr->ApplyViewport(width, height); }) m_ptr->ApplyViewport(width, height); return *this; }
    ElementBuilder& ScrollOffsetY(float value) { if constexpr (requires { m_ptr->ApplyScrollOffsetY(value); }) m_ptr->ApplyScrollOffsetY(value); return *this; }
    ElementBuilder& PaneAutoHide(int index, bool value) { if constexpr (requires { m_ptr->ApplyPaneAutoHide(index, value); }) m_ptr->ApplyPaneAutoHide(index, value); return *this; }
    ElementBuilder& SideSize(CUI::DockSide side, float value) { if constexpr (requires { m_ptr->ApplySideSize(side, value); }) m_ptr->ApplySideSize(side, value); return *this; }
    ElementBuilder& Label(const std::string& value) { if constexpr (requires { m_ptr->ApplyLabel(value); }) m_ptr->ApplyLabel(value); return *this; }
    ElementBuilder& LabelPosition(CUI::CommandBarLabelPosition value) { if constexpr (requires { m_ptr->ApplyLabelPosition(value); }) m_ptr->ApplyLabelPosition(value); return *this; }
    ElementBuilder& Markdown(const std::string& value) { if constexpr (requires { m_ptr->ApplyMarkdown(value); }) m_ptr->ApplyMarkdown(value); return *this; }
    ElementBuilder& Password(const std::string& value) { if constexpr (requires { m_ptr->ApplyPassword(value); }) m_ptr->ApplyPassword(value); return *this; }
    ElementBuilder& IsThreeState(bool value) { if constexpr (requires { m_ptr->ApplyIsThreeState(value); }) m_ptr->ApplyIsThreeState(value); return *this; }
    ElementBuilder& MaxTabWidth(float value) { if constexpr (requires { m_ptr->ApplyMaxTabWidth(value); }) m_ptr->ApplyMaxTabWidth(value); return *this; }
    ElementBuilder& MinTabWidth(float value) { if constexpr (requires { m_ptr->ApplyMinTabWidth(value); }) m_ptr->ApplyMinTabWidth(value); return *this; }
    ElementBuilder& ShowGrid(bool value) { if constexpr (requires { m_ptr->ApplyShowGrid(value); }) m_ptr->ApplyShowGrid(value); return *this; }
    ElementBuilder& ShowGridLines(bool value) { if constexpr (requires { m_ptr->ApplyShowGridLines(value); }) m_ptr->ApplyShowGridLines(value); return *this; }
    ElementBuilder& ShowLegend(bool value) { if constexpr (requires { m_ptr->ApplyShowLegend(value); }) m_ptr->ApplyShowLegend(value); return *this; }
    ElementBuilder& ShowTooltip(bool value) { if constexpr (requires { m_ptr->ApplyShowTooltip(value); }) m_ptr->ApplyShowTooltip(value); return *this; }
    ElementBuilder& Categories(std::vector<std::string> value) { if constexpr (requires { m_ptr->ApplyCategories(std::move(value)); }) m_ptr->ApplyCategories(std::move(value)); return *this; }
    ElementBuilder& Series(std::vector<CUI::ChartSeries> value) { if constexpr (requires { m_ptr->ApplySeries(std::move(value)); }) m_ptr->ApplySeries(std::move(value)); return *this; }
    ElementBuilder& LiveData(std::vector<std::string> categories, std::vector<CUI::ChartSeries> series, bool replay = false) { if constexpr (requires { m_ptr->ApplyLiveData(std::move(categories), std::move(series), replay); }) m_ptr->ApplyLiveData(std::move(categories), std::move(series), replay); return *this; }
    ElementBuilder& ItemText(int id, const std::string& value) { if constexpr (requires { m_ptr->ApplyItemText(id, value); }) m_ptr->ApplyItemText(id, value); return *this; }
    ElementBuilder& ItemProgress(int id, float value) { if constexpr (requires { m_ptr->ApplyItemProgress(id, value); }) m_ptr->ApplyItemProgress(id, value); return *this; }
    ElementBuilder& SuggestionItems(std::vector<std::string> value) { if constexpr (requires { m_ptr->ApplySuggestionItems(value); }) m_ptr->ApplySuggestionItems(value); return *this; }
    template<typename F> ElementBuilder& SuggestionProvider(F&& value) { if constexpr (requires { m_ptr->ApplySuggestionProvider(std::forward<F>(value)); }) m_ptr->ApplySuggestionProvider(std::forward<F>(value)); return *this; }
    template<typename F> ElementBuilder& OnDraw(F&& value) { if constexpr (requires { m_ptr->ApplyOnDraw(std::forward<F>(value)); }) m_ptr->ApplyOnDraw(std::forward<F>(value)); return *this; }
    template<typename F> ElementBuilder& OnCanvasMouseDown(F&& value) { if constexpr (requires { m_ptr->ApplyOnCanvasMouseDown(std::forward<F>(value)); }) m_ptr->ApplyOnCanvasMouseDown(std::forward<F>(value)); return *this; }
    template<typename F> ElementBuilder& OnCanvasMouseMove(F&& value) { if constexpr (requires { m_ptr->ApplyOnCanvasMouseMove(std::forward<F>(value)); }) m_ptr->ApplyOnCanvasMouseMove(std::forward<F>(value)); return *this; }
    template<typename F> ElementBuilder& OnCanvasMouseUp(F&& value) { if constexpr (requires { m_ptr->ApplyOnCanvasMouseUp(std::forward<F>(value)); }) m_ptr->ApplyOnCanvasMouseUp(std::forward<F>(value)); return *this; }
    template<typename F> ElementBuilder& OnTick(F&& value) { if constexpr (requires { m_ptr->ApplyOnTick(std::forward<F>(value)); }) m_ptr->ApplyOnTick(std::forward<F>(value)); return *this; }
    template<typename F> ElementBuilder& StatusHandler(F&& value) { if constexpr (requires { m_ptr->ApplyStatusHandler(std::forward<F>(value)); }) m_ptr->ApplyStatusHandler(std::forward<F>(value)); return *this; }
    ElementBuilder& LowPerformanceMode(bool value) { if constexpr (requires { m_ptr->ApplyLowPerformanceMode(value); }) m_ptr->ApplyLowPerformanceMode(value); return *this; }
    ElementBuilder& ImageType(CUI::ImageType value) { if constexpr (requires { m_ptr->ApplyImageType(value); }) m_ptr->ApplyImageType(value); return *this; }
    ElementBuilder& OwnerWindow(CUI::Window* value) { if constexpr (requires { m_ptr->ApplyOwnerWindow(value); }) m_ptr->ApplyOwnerWindow(value); return *this; }
    ElementBuilder& ActionCommand(std::shared_ptr<CUI::Command> value) { if constexpr (requires { m_ptr->ApplyActionCommand(value); }) m_ptr->ApplyActionCommand(std::move(value)); return *this; }
    ElementBuilder& MaxEntries(uint32_t value) { if constexpr (requires { m_ptr->ApplyMaxEntries(value); }) m_ptr->ApplyMaxEntries(value); return *this; }
    ElementBuilder& Expanded(bool value) { if constexpr (requires { m_ptr->ApplyExpanded(value); }) m_ptr->ApplyExpanded(value); return *this; }
    ElementBuilder& PersistEnabled(bool value) { if constexpr (requires { m_ptr->ApplyPersistEnabled(value); }) m_ptr->ApplyPersistEnabled(value); return *this; }
    ElementBuilder& ShowCodeLineNumbers(bool value) { if constexpr (requires { m_ptr->ApplyShowCodeLineNumbers(value); }) m_ptr->ApplyShowCodeLineNumbers(value); return *this; }
    ElementBuilder& MinimumRange(float value) { if constexpr (requires { m_ptr->ApplyMinimumRange(value); }) m_ptr->ApplyMinimumRange(value); return *this; }
    ElementBuilder& IsClearEnabled(bool value) { if constexpr (requires { m_ptr->ApplyIsClearEnabled(value); }) m_ptr->ApplyIsClearEnabled(value); return *this; }
    ElementBuilder& ItemIcon(int id, const std::string& value) { if constexpr (requires { m_ptr->ApplyItemIcon(id, value); }) m_ptr->ApplyItemIcon(id, value); return *this; }
    ElementBuilder& CanSave(bool value) { if constexpr (requires { m_ptr->ApplyCanSave(value); }) m_ptr->ApplyCanSave(value); return *this; }
    ElementBuilder& MaxVisibleSuggestions(int value) { if constexpr (requires { m_ptr->ApplyMaxVisibleSuggestions(value); }) m_ptr->ApplyMaxVisibleSuggestions(value); return *this; }
    ElementBuilder& FlowParticlesEnabled(bool value) { if constexpr (requires { m_ptr->ApplyFlowParticlesEnabled(value); }) m_ptr->ApplyFlowParticlesEnabled(value); return *this; }
    ElementBuilder& Gesture(const std::string& value) { if constexpr (requires { m_ptr->ApplyGesture(value); }) m_ptr->ApplyGesture(value); return *this; }
    ElementBuilder& IsMinimizeButtonVisible(bool value) { if constexpr (requires { m_ptr->ApplyIsMinimizeButtonVisible(value); }) m_ptr->ApplyIsMinimizeButtonVisible(value); return *this; }
    ElementBuilder& IsMaximizeButtonVisible(bool value) { if constexpr (requires { m_ptr->ApplyIsMaximizeButtonVisible(value); }) m_ptr->ApplyIsMaximizeButtonVisible(value); return *this; }
    ElementBuilder& IsCloseButtonVisible(bool value) { if constexpr (requires { m_ptr->ApplyIsCloseButtonVisible(value); }) m_ptr->ApplyIsCloseButtonVisible(value); return *this; }
    ElementBuilder& IsMinimizeButtonEnabled(bool value) { if constexpr (requires { m_ptr->ApplyIsMinimizeButtonEnabled(value); }) m_ptr->ApplyIsMinimizeButtonEnabled(value); return *this; }
    ElementBuilder& IsMaximizeButtonEnabled(bool value) { if constexpr (requires { m_ptr->ApplyIsMaximizeButtonEnabled(value); }) m_ptr->ApplyIsMaximizeButtonEnabled(value); return *this; }
    ElementBuilder& IsCloseButtonEnabled(bool value) { if constexpr (requires { m_ptr->ApplyIsCloseButtonEnabled(value); }) m_ptr->ApplyIsCloseButtonEnabled(value); return *this; }

};




struct ChildArgument {
    Element element;
    ChildArgument() = default;
    ChildArgument(Element value) : element(std::move(value)) {}
    template<typename T> ChildArgument(std::shared_ptr<T> value) : element(std::move(value)) {}
    template<typename T> ChildArgument(const ElementRef<T>& ref) : element(ref.Shared()) {}
    /// 非拥有观察句柄（Widgets::Ref）：共享同一控件，可直接写进 Row/Column 的初始化列表
    template<typename T> ChildArgument(const Widgets::Ref<T>& ref) : element(ref.Ptr()) {}
    template<typename T> ChildArgument(const ElementBuilder<T>& builder) : element(builder.Shared()) {}
    /// 新句柄层（Widgets::Xxx）右值：自动 Build 交出所有权，可直接写进 Row/Column 的初始化列表
    template<typename H> requires requires(H& h) { h.Build(); }
    ChildArgument(H&& h) : element(Element(h.Build())) {}
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



inline ElementBuilder<Panel> Container() { // 快速生成空泛的排版盒模型容器
    return ElementBuilder<Panel>();
}



inline ElementBuilder<ScrollViewer> SingleChildScrollView() { // 快速生成单子控件滚动查看器
    return ElementBuilder<ScrollViewer>();
}



inline ElementBuilder<Panel> Expanded(std::shared_ptr<UIElement> child, float flex = 1.0f) { // 快速生成弹性延伸填充块
    auto p = ElementBuilder<Panel>();
    p.FlexGrow(flex);
    if (child) {
        child->ApplyFlexGrow(1.0f);
        p.AddChild(child);
    }
    return p;
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

    void ApplyBuildContext(const BuildContext& context) { m_context = context; } // 注册组件上下文
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
    auto p = Control<CUI::PasswordBox>();
    p.Placeholder(placeholder);
    return p;
}
inline ElementBuilder<CUI::NumberBox> NumberBox(double value = 0.0) {
    auto n = Control<CUI::NumberBox>();
    n.Value(static_cast<float>(value));
    return n;
}
inline ElementBuilder<CUI::RadioButton> RadioButton(const std::string& text = "", const std::string& group = "DefaultGroup") {
    auto r = Control<CUI::RadioButton>();
    if (!text.empty()) r.Text(text);
    r.GroupName(group);
    return r;
}
inline ElementBuilder<CUI::ToggleSwitch> ToggleSwitch() {
    return Control<CUI::ToggleSwitch>();
}
inline ElementBuilder<CUI::HyperlinkButton> HyperlinkButton(const std::string& text = "", const std::string& uri = "") {
    auto h = Control<CUI::HyperlinkButton>();
    if (!text.empty()) h.Text(text);
    if (!uri.empty()) h.NavigateUri(uri);
    return h;
}
inline ElementBuilder<CUI::SegmentedControl> SegmentedControl(std::initializer_list<const char*> items = {}) {
    auto s = Control<CUI::SegmentedControl>();
    for (const char* item : items) {
        if (item && *item) s.AddItem(item);
    }
    return s;
}
inline ElementBuilder<CUI::DatePicker> DatePicker() { return Control<CUI::DatePicker>(); }
inline ElementBuilder<CUI::TimePicker> TimePicker() { return Control<CUI::TimePicker>(); }
inline ElementBuilder<CUI::ColorPicker> ColorPicker() { return Control<CUI::ColorPicker>(); }
inline ElementBuilder<CUI::PagingControl> PagingControl(int current = 1, int total = 10) {
    auto p = Control<CUI::PagingControl>();
    p.TotalPages(total);
    p.CurrentPage(current);
    return p;
}
inline ElementBuilder<CUI::Splitter> Splitter(CUI::Orientation orientation = CUI::Orientation::Horizontal) {
    auto s = Control<CUI::Splitter>();
    if (orientation == CUI::Orientation::Horizontal) {
        s.Orientation(CUI::Orientation::Horizontal);
        s.Width(-1.0f);
        s.Height(10.0f);
    } else {
        s.Orientation(CUI::Orientation::Vertical);
        s.Width(10.0f);
        s.Height(-1.0f);
    }
    s.Align(Alignment::Stretch);
    return s;
}
inline ElementBuilder<CUI::Expander> Expander(const std::string& title = "Expander") {
    auto e = Control<CUI::Expander>();
    e.Header(title);
    return e;
}
inline ElementBuilder<CUI::TreeView> TreeView() { return Control<CUI::TreeView>(); }
inline ElementBuilder<CUI::FilePicker> FilePicker(const std::string& path = "") {
    auto f = Control<CUI::FilePicker>();
    if (!path.empty()) f.Path(path);
    return f;
}
inline ElementBuilder<CUI::FolderPicker> FolderPicker(const std::string& path = "") {
    auto f = Control<CUI::FolderPicker>();
    if (!path.empty()) f.Path(path);
    return f;
}
inline ElementBuilder<CUI::Canvas> Canvas() { return Control<CUI::Canvas>(); }
inline ElementBuilder<CUI::WrapPanel> WrapPanel(const std::string& orient = "Horizontal") {
    return Control<CUI::WrapPanel>().Orientation(orient);
}
inline ElementBuilder<CUI::DockPanel> DockPanel() { return Control<CUI::DockPanel>(); }
inline ElementBuilder<CUI::UniformGrid> UniformGrid(int rows = 2, int cols = 2) {
    auto u = Control<CUI::UniformGrid>();
    u.Rows(rows);
    u.Columns(cols);
    return u;
}
inline ElementBuilder<CUI::ScrollViewer> ScrollViewer() { return Control<CUI::ScrollViewer>(); }
inline ElementBuilder<CUI::Flyout> Flyout() { return Control<CUI::Flyout>(); }
inline ElementBuilder<CUI::MenuBar> MenuBar() { return Control<CUI::MenuBar>(); }
inline ElementBuilder<CUI::CommandBar> CommandBar() { return Control<CUI::CommandBar>(); }
inline ElementBuilder<CUI::InfoBar> InfoBar() { return Control<CUI::InfoBar>(); }
inline ElementBuilder<CUI::LogView> LogView() { return Control<CUI::LogView>(); }
inline ElementBuilder<CUI::TopologyView> TopologyView() { return Control<CUI::TopologyView>(); }
inline ElementBuilder<CUI::SvgIcon> SvgIcon(const std::string& source = "") {
    auto s = Control<CUI::SvgIcon>();
    if (!source.empty()) s.Source(source);
    return s;
}
inline ElementBuilder<CUI::CanvasControl> CanvasControl(float width = 300.0f, float height = 200.0f) {
    auto c = Control<CUI::CanvasControl>();
    c.Width(width);
    c.Height(height);
    return c;
}
inline ElementBuilder<CUI::ContentDialog> ContentDialog(const std::string& title = "Dialog", const std::string& message = "") {
    auto d = Control<CUI::ContentDialog>();
    d.Title(title);
    if (!message.empty()) d.Message(message);
    return d;
}
inline ElementBuilder<CUI::StatusBar> StatusBar() { return Control<CUI::StatusBar>(); }
inline ElementBuilder<CUI::RatingControl> RatingControl(float value = 3.5f, int maxRating = 5) {
    auto r = Control<CUI::RatingControl>();
    r.MaxRating(maxRating);
    r.Value(value);
    return r;
}
inline ElementBuilder<CUI::TeachingTip> TeachingTip() { return Control<CUI::TeachingTip>(); }
inline ElementBuilder<CUI::AutoSuggestBox> AutoSuggestBox(const std::string& placeholder = "搜索…") {
    auto a = Control<CUI::AutoSuggestBox>();
    a.Placeholder(placeholder);
    return a;
}
inline ElementBuilder<CUI::ProgressBar> ProgressBar(float value = 0.0f, bool isIndeterminate = false) {
    auto p = Control<CUI::ProgressBar>();
    p.Value(value);
    p.IsIndeterminate(isIndeterminate);
    return p;
}
inline ElementBuilder<CUI::ProgressRing> ProgressRing(float value = 0.0f, bool isIndeterminate = true) {
    auto p = Control<CUI::ProgressRing>();
    p.Value(value);
    p.IsIndeterminate(isIndeterminate);
    return p;
}
inline ElementBuilder<CUI::RangeSlider> RangeSlider(float lower = 20.0f, float upper = 80.0f,
                                                    float min = 0.0f, float max = 100.0f) {
    auto s = Control<CUI::RangeSlider>();
    s.Minimum(min);
    s.Maximum(max);
    s.Range(lower, upper);
    return s;
}
inline ElementBuilder<CUI::DockManager> DockManager() { return Control<CUI::DockManager>(); }
inline ElementBuilder<CUI::WindowTitleBar> WindowTitleBar(const std::string& title = "CUI Application") {
    auto t = Control<CUI::WindowTitleBar>();
    t.Title(title);
    return t;
}
inline ElementBuilder<CUI::Rectangle> Rectangle(float width = 100.0f, float height = 50.0f) {
    auto r = Control<CUI::Rectangle>();
    r.Width(width);
    r.Height(height);
    return r;
}
inline ElementBuilder<CUI::Ellipse> Ellipse(float width = 50.0f, float height = 50.0f) {
    auto e = Control<CUI::Ellipse>();
    e.Width(width);
    e.Height(height);
    return e;
}
inline ElementBuilder<CUI::Line> Line(float x1 = 0, float y1 = 0, float x2 = 100, float y2 = 100) {
    auto l = Control<CUI::Line>();
    l.X1(x1).Y1(y1).X2(x2).Y2(y2);
    return l;
}
inline ElementBuilder<CUI::Path> Path(const std::string& data = "") {
    auto p = Control<CUI::Path>();
    if (!data.empty()) p.Data(data);
    return p;
}

} // namespace Fluent
} // namespace DSL
} // namespace CUI





