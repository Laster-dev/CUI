#pragma once
#include "../core/Object.h"
#include "../core/PropertyDesc.h"
#include "../core/PropertyId.h"
#include "../render/GraphicsContext.h"
#include "../render/RenderNode.h"
#include "../style/ThemeTokenId.h"
#include "../layout/Layout.h"
#include "../input/RoutedEvent.h"
#include "../input/Command.h"
#include "../core/BindableProperty.h"
#include "../core/Property.h"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <chrono>
#include <cstdint>
#include <limits>

namespace CUI {

class ContextMenu;
class UIElement;
using Element = std::shared_ptr<UIElement>;

struct CanvasDrawContext {
    GraphicsContext& Context;
    Size CanvasSize{};
    CanvasDrawContext(GraphicsContext& ctx, Size sz = {}) : Context(ctx), CanvasSize(sz) {}
    operator GraphicsContext&() { return Context; }
    GraphicsContext* operator->() { return &Context; }
};

struct PointerEventArgs {
    Point Position{};
    bool Handled = false;
    int KeyCode = 0;
};

/**
 * @brief 焦点状态枚举。
 * Unfocused: 未获取焦点。
 * Pointer: 鼠标/指针点击获焦。
 * Keyboard: 键盘导航（Tab 键等）获焦。
 */
enum class FocusState {
    Unfocused, // 未获得焦点：控件当前没有输入焦点，用户操作不会直接作用在该控件上
    Pointer,   // 指针焦点：通过鼠标点击或触控操作获得焦点
    Keyboard   // 键盘焦点：通过键盘导航（如 Tab 键切换）获得焦点
};

/**
 * @brief 键盘导航时的焦点跳转切换行为模式。
 * Continue: 正常流向下一个兄弟节点。
 * Cycle: 在当前子树中循环跳转，到最后一个后绕回到第一个。
 * Contained: 限制在此节点内部，出不去。
 * None: 该节点及子树不参与键盘导航。
 */
enum class KeyboardNavigationMode {
    Continue,  // 继续：焦点按正常的深度优先逻辑流向层级树中的下一个可聚焦兄弟节点
    Cycle,     // 循环：焦点到达当前容器内最后一个子控件时，按下 Tab 将绕回到第一个子控件
    Contained, // 包含：焦点限制在当前容器内部跳转，无法通过 Tab 切换到容器外的其它控件
    None       // 无：该控件及其所有子节点均不参与键盘 Tab 导航序列
};

/**
 * @brief 字体的粗细权重枚举。
 * Thin: 极细。
 * ExtraLight: 超细。
 * Light: 细。
 * Normal: 常规。
 * Medium: 中等。
 * SemiBold: 半粗。
 * Bold: 粗体。
 * ExtraBold: 特粗。
 * Black: 浓黑。
 */
enum class FontWeight : uint16_t {
    Thin = DWRITE_FONT_WEIGHT_THIN,                 // 极细：最轻薄的字体线条粗细
    ExtraLight = DWRITE_FONT_WEIGHT_EXTRA_LIGHT,     // 超细：比极细略粗，但仍非常纤细的线条
    Light = DWRITE_FONT_WEIGHT_LIGHT,               // 细：轻量级字重，常用于副标题
    Normal = DWRITE_FONT_WEIGHT_NORMAL,             // 常规：最常用的常规字重，用于正文文本
    Medium = DWRITE_FONT_WEIGHT_MEDIUM,             // 中等：介于常规和半粗之间的字重
    SemiBold = DWRITE_FONT_WEIGHT_SEMI_BOLD,         // 半粗：中等偏粗，用于小标题以增强对比
    Bold = DWRITE_FONT_WEIGHT_BOLD,                 // 粗体：标准的粗体字重，用于突出显示标题
    ExtraBold = DWRITE_FONT_WEIGHT_EXTRA_BOLD,       // 特粗：非常粗的线条，用于高瞩目的标语
    Black = DWRITE_FONT_WEIGHT_BLACK                // 浓黑：最粗的字重，线条极为厚重
};

/**
 * @brief 字体字形倾斜样式枚举。
 * Normal: 常规直立。
 * Italic: 算法或字体内置斜体。
 * Oblique: 偏斜体。
 */
enum class FontStyle : uint8_t {
    Normal = DWRITE_FONT_STYLE_NORMAL,               // 常规：直立的正常字体样式
    Italic = DWRITE_FONT_STYLE_ITALIC,               // 斜体：由字体设计师特别设计的倾斜字形风格
    Oblique = DWRITE_FONT_STYLE_OBLIQUE              // 倾斜：通过倾斜算法强行拉斜的直立字形
};

/**
 * @brief 字体宽度横向拉伸/形变程度枚举。
 * UltraCondensed: 极窄。
 * ExtraCondensed: 特窄。
 * Condensed: 窄。
 * SemiCondensed: 偏窄。
 * Normal: 正常宽度。
 * SemiExpanded: 偏宽。
 * Expanded: 宽。
 * ExtraExpanded: 特宽。
 * UltraExpanded: 极宽。
 */
enum class FontStretch : uint8_t {
    UltraCondensed = DWRITE_FONT_STRETCH_ULTRA_CONDENSED, // 极窄：横向压缩到极致的窄体字
    ExtraCondensed = DWRITE_FONT_STRETCH_EXTRA_CONDENSED, // 特窄：非常紧凑的窄体字
    Condensed = DWRITE_FONT_STRETCH_CONDENSED,           // 窄：标准的窄体字
    SemiCondensed = DWRITE_FONT_STRETCH_SEMI_CONDENSED,   // 偏窄：略微压缩宽度的字体
    Normal = DWRITE_FONT_STRETCH_NORMAL,                 // 正常：设计师定义的标准常规宽度字体
    SemiExpanded = DWRITE_FONT_STRETCH_SEMI_EXPANDED,     // 偏宽：略微横向扩展宽度的字体
    Expanded = DWRITE_FONT_STRETCH_EXPANDED,             // 宽：宽体字
    ExtraExpanded = DWRITE_FONT_STRETCH_EXTRA_EXPANDED,   // 特宽：横向大范围扩展宽度的字体
    UltraExpanded = DWRITE_FONT_STRETCH_ULTRA_EXPANDED  // 极宽：横向拉伸至极致的字体
};

// 字体属性到字符串的序列化/反序列化映射方法
const char* FontWeightToString(FontWeight value);
FontWeight FontWeightFromString(const std::string& value);
const char* FontStyleToString(FontStyle value);
FontStyle FontStyleFromString(const std::string& value);
const char* FontStretchToString(FontStretch value);
FontStretch FontStretchFromString(const std::string& value);

/**
 * @brief 属性值特化转换萃取器。
 * 用于在 Value 异构 Variant 数据和控件属性强类型之间相互转换。
 */
template<typename T>
struct PropertyValueTraits;

template<>
struct PropertyValueTraits<bool> {
    static bool FromValue(const Value& value) { return value.AsBool(); } // 将通用值类型转换为布尔型
    static Value ToValue(bool value) { return Value(value); }            // 将布尔型转换为通用值类型
};

template<>
struct PropertyValueTraits<int> {
    static int FromValue(const Value& value) { return value.AsInt(); }   // 将通用值类型转换为整型
    static Value ToValue(int value) { return Value(value); }             // 将整型转换为通用值类型
};

template<>
struct PropertyValueTraits<float> {
    static float FromValue(const Value& value) { return value.AsFloat(); } // 将通用值类型转换为浮点型
    static Value ToValue(float value) { return Value(value); }             // 将浮点型转换为通用值类型
};

template<>
struct PropertyValueTraits<std::string> {
    static std::string FromValue(const Value& value) { return value.AsString(); } // 将通用值类型转换为字符串型
    static Value ToValue(const std::string& value) { return Value(value); }        // 将字符串型转换为通用值类型
};

template<>
struct PropertyValueTraits<Color> {
    static Color FromValue(const Value& value) {
        const D2D1_COLOR_F color = value.AsColor();
        return Color(color.r, color.g, color.b, color.a); // 将通用值转换为 Color 结构体
    }
    static Value ToValue(const Color& value) { return Value(static_cast<D2D1_COLOR_F>(value)); } // 将 Color 结构体转换为通用值
};

template<>
struct PropertyValueTraits<FontWeight> {
    static FontWeight FromValue(const Value& value) { return FontWeightFromString(value.AsString()); } // 从字符串读取并转换为字重权重
    static Value ToValue(FontWeight value) { return Value(FontWeightToString(value)); }                 // 将字重权重转换为字符串通用值
};

template<>
struct PropertyValueTraits<FontStyle> {
    static FontStyle FromValue(const Value& value) { return FontStyleFromString(value.AsString()); } // 从字符串读取并转换为字体样式
    static Value ToValue(FontStyle value) { return Value(FontStyleToString(value)); }                 // 将字体样式转换为字符串通用值
};

template<>
struct PropertyValueTraits<FontStretch> {
    static FontStretch FromValue(const Value& value) { return FontStretchFromString(value.AsString()); } // 从字符串读取并转换为字体拉伸度
    static Value ToValue(FontStretch value) { return Value(FontStretchToString(value)); }                 // 将字体拉伸度转换为字符串通用值
};

template<>
struct PropertyValueTraits<Thickness> {
    static Thickness FromValue(const Value& value) { return value.AsThickness(); }
    static Value ToValue(const Thickness& value) { return Value(value); }
};

template<>
struct PropertyValueTraits<Visibility> {
    static Visibility FromValue(const Value& value) {
        const std::string name = value.AsString("Visible");
        return name == "Hidden" ? Visibility::Hidden : name == "Collapsed" ? Visibility::Collapsed : Visibility::Visible;
    }
    static Value ToValue(Visibility value) {
        return Value(value == Visibility::Hidden ? "Hidden" : value == Visibility::Collapsed ? "Collapsed" : "Visible");
    }
};

template<>
struct PropertyValueTraits<Alignment> {
    static Alignment FromValue(const Value& value) {
        const std::string name = value.AsString("Stretch");
        return name == "Start" ? Alignment::Start : name == "Center" ? Alignment::Center : name == "End" ? Alignment::End : Alignment::Stretch;
    }
    static Value ToValue(Alignment value) {
        return Value(value == Alignment::Start ? "Start" : value == Alignment::Center ? "Center" : value == Alignment::End ? "End" : "Stretch");
    }
};

template<>
struct PropertyValueTraits<ThemeTokenId> {
    static ThemeTokenId FromValue(const Value& value) { return ThemeTokenIdFromName(value.AsString()); }
    static Value ToValue(ThemeTokenId value) { return Value(ThemeTokenIdToName(value)); }
};

template<>
struct PropertyValueTraits<Orientation> {
    static Orientation FromValue(const Value& value) {
        const std::string s = value.AsString("Vertical");
        return (s == "Horizontal") ? Orientation::Horizontal : Orientation::Vertical; // 从字符串解析布局朝向
    }
    static Value ToValue(Orientation value) {
        return Value(value == Orientation::Horizontal ? "Horizontal" : "Vertical"); // 将布局朝向序列化为字符串
    }
};

/**
 * @brief CUI 框架的终极视觉基类（所有控件与布局容器的祖先类）。
 * UIElement 统一抽象和实现了以下 UI 引擎核心流程：
 * 1. **生命周期与层级树**：管理父子节点关系（AddChild / RemoveChild）。
 * 2. **核心布局机制（双入度管道）**：
 *    - Measure()：测算大小，控件计算其所需的最适期望大小（Desired Size）。
 *    - Arrange()：编排对齐，父容器将确定区域分配给子控件。
 * 3. **绘制管线**：通过 Render() / OnRender() 触发 Direct2D 画面渲染与分层局部缓存。
 * 4. **响应式数据属性系统**：内置绑定属性定义（如 Text, Width, Background 代理），简化组件状态通信。
 * 5. **事件路由分发**：支持鼠标移入、按键焦点等底层交互回调。
 */
class UIElement : public Object {
public:
    static constexpr float kAttachedUnset = -999999.0f; // 附加布局属性（如 Canvas Left）的未设置默认哨兵值

    UIElement();
    virtual ~UIElement();
    virtual const char* GetClassName() const override { return "UIElement"; } // 获取当前类的类名字符串

    // 以下为内置的响应式属性代理，支持通过 Bind() 直接绑定到数据源 State 实例
    PropertyRef<std::string, PropertyId::Text> Text;                    // 文本属性代理：控制控件内展示的文字信息
    PropertyRef<std::string, PropertyId::FontFamily> FontFamily;        // 字体族属性代理：指定渲染文本所采用的字体族名称
    PropertyRef<float, PropertyId::FontSize> FontSize;                  // 字体大小属性代理：设置文字的排版大小 (px)
    PropertyRef<CUI::FontWeight, PropertyId::FontWeight> FontWeight;    // 字体粗细属性代理：控制文字粗细权重数值
    PropertyRef<CUI::FontStyle, PropertyId::FontStyle> FontStyle;        // 字体样式属性代理：控制文字直立或倾斜
    PropertyRef<CUI::FontStretch, PropertyId::FontStretch> FontStretch;  // 字体拉伸属性代理：控制文本字元的水平宽度形变
    PropertyRef<bool, PropertyId::IsUnderline> Underline;               // 下划线属性代理：设置文本下方是否显示水平底线
    PropertyRef<bool, PropertyId::IsStrikethrough> Strikethrough;       // 删除线属性代理：设置文本中央是否显示贯穿横线
    PropertyRef<Color, PropertyId::Color> TextColor;                    // 文字颜色属性代理：指定渲染文本所需的前景色彩
    PropertyRef<Color, PropertyId::Background> Background;              // 背景颜色属性代理：指定控件正常状态下的背景填充色
    PropertyRef<Color, PropertyId::HoverBackground> HoverBackground;    // 悬停背景颜色代理：指定鼠标悬停在控件上方时的背景色
    PropertyRef<Color, PropertyId::PressedBackground> PressedBackground;
    PropertyRef<Color, PropertyId::BorderBrush> BorderBrush;
    PropertyRef<CUI::Color, PropertyId::Color> Foreground;
    PropertyRef<ThemeTokenId, PropertyId::BackgroundToken> BackgroundToken;
    PropertyRef<ThemeTokenId, PropertyId::HoverBackgroundToken> HoverBackgroundToken;
    PropertyRef<ThemeTokenId, PropertyId::PressedBackgroundToken> PressedBackgroundToken;
    PropertyRef<ThemeTokenId, PropertyId::DisabledBackgroundToken> DisabledBackgroundToken;
    PropertyRef<ThemeTokenId, PropertyId::BorderToken> BorderToken;
    PropertyRef<ThemeTokenId, PropertyId::FocusedBorderToken> FocusedBorderToken;
    PropertyRef<ThemeTokenId, PropertyId::ColorToken> ColorToken;
    PropertyRef<ThemeTokenId, PropertyId::SecondaryColorToken> SecondaryColorToken;
    PropertyRef<ThemeTokenId, PropertyId::PlaceholderColorToken> PlaceholderColorToken;
    PropertyRef<ThemeTokenId, PropertyId::SelectedBackgroundToken> SelectedBackgroundToken;
    PropertyRef<ThemeTokenId, PropertyId::HeaderBackgroundToken> HeaderBackgroundToken;
    PropertyRef<ThemeTokenId, PropertyId::PaneBackgroundToken> PaneBackgroundToken;
    PropertyRef<ThemeTokenId, PropertyId::IndicatorColorToken> IndicatorColorToken;
    PropertyRef<ThemeTokenId, PropertyId::DropdownBackgroundToken> DropdownBackgroundToken;
    PropertyRef<ThemeTokenId, PropertyId::SelectedItemBackgroundToken> SelectedItemBackgroundToken;
    PropertyRef<ThemeTokenId, PropertyId::FillColorToken> FillColorToken;
    PropertyRef<ThemeTokenId, PropertyId::TrackColorToken> TrackColorToken;
    PropertyRef<ThemeTokenId, PropertyId::ActiveTrackColorToken> ActiveTrackColorToken;
    PropertyRef<ThemeTokenId, PropertyId::ThumbColorToken> ThumbColorToken;
    PropertyRef<ThemeTokenId, PropertyId::OnColorToken> OnColorToken;
    PropertyRef<ThemeTokenId, PropertyId::OffColorToken> OffColorToken;
    PropertyRef<ThemeTokenId, PropertyId::KnobColorToken> KnobColorToken;
    PropertyRef<ThemeTokenId, PropertyId::CheckedBackgroundToken> CheckedBackgroundToken;
    PropertyRef<ThemeTokenId, PropertyId::AccentColorToken> AccentColorToken;
    PropertyRef<ThemeTokenId, PropertyId::ActiveColorToken> ActiveColorToken;
    PropertyRef<ThemeTokenId, PropertyId::UnderlineColorToken> UnderlineColorToken;
    PropertyRef<ThemeTokenId, PropertyId::ActiveUnderlineColorToken> ActiveUnderlineColorToken;
    PropertyRef<ThemeTokenId, PropertyId::ActiveTabBackgroundToken> ActiveTabBackgroundToken;
    PropertyRef<ThemeTokenId, PropertyId::InactiveTabBackgroundToken> InactiveTabBackgroundToken;
    PropertyRef<ThemeTokenId, PropertyId::GridLineBrushToken> GridLineBrushToken;
    PropertyRef<ThemeTokenId, PropertyId::TitleColorToken> TitleColorToken;
    PropertyRef<ThemeTokenId, PropertyId::MessageColorToken> MessageColorToken;
    PropertyRef<ThemeTokenId, PropertyId::CaretColorToken> CaretColorToken;
    PropertyRef<std::string, PropertyId::Placeholder> Placeholder;
    PropertyRef<std::string, PropertyId::ToolTip> ToolTip;
    PropertyRef<std::string, PropertyId::Icon> Icon;
    PropertyRef<float, PropertyId::Width> Width;                        // 显式宽度代理：如果显式指定，将强制覆盖测算布局的宽度
    PropertyRef<float, PropertyId::Height> Height;                      // 显式高度代理：如果显式指定，将强制覆盖测算布局的高度
    PropertyRef<Thickness, PropertyId::Margin> Margin;
    PropertyRef<Thickness, PropertyId::Padding> Padding;
    PropertyRef<CUI::Visibility, PropertyId::Visibility> VisibilityProperty;
    PropertyRef<Alignment, PropertyId::Align> Align;
    PropertyRef<Alignment, PropertyId::AlignHorizontal> AlignHorizontal;
    PropertyRef<Alignment, PropertyId::AlignVertical> AlignVertical;
    PropertyRef<bool, PropertyId::IsEnabled> IsEnabledProperty;          // 交互启用状态代理：控制控件是否可接收物理输入响应
    PropertyRef<float, PropertyId::Opacity> Opacity;                    // 不透明度代理：控制控件的渲染透明度等级 (0.0f - 1.0f)
    PropertyRef<float, PropertyId::MinWidth> MinWidth;
    PropertyRef<float, PropertyId::MinHeight> MinHeight;
    PropertyRef<float, PropertyId::MaxWidth> MaxWidth;
    PropertyRef<float, PropertyId::MaxHeight> MaxHeight;
    PropertyRef<float, PropertyId::CornerRadius> CornerRadius;
    PropertyRef<float, PropertyId::BorderThickness> BorderThickness;
    PropertyRef<float, PropertyId::FlexGrow> FlexGrow;
    PropertyRef<CUI::Orientation, PropertyId::Orientation> Orientation; // 布局方向属性代理：控制 Stack/Wrap 等容器的排列朝向
    PropertyRef<float, PropertyId::Gap> Gap;
    PropertyRef<float, PropertyId::ItemWidth> ItemWidth;
    PropertyRef<float, PropertyId::ItemHeight> ItemHeight;
    PropertyRef<bool, PropertyId::LastChildFill> LastChildFill;
    PropertyRef<bool, PropertyId::JustifyLines> JustifyLines;
    PropertyRef<bool, PropertyId::FillLastLine> FillLastLine;
    PropertyRef<int, PropertyId::Rows> Rows;
    PropertyRef<int, PropertyId::Columns> Columns;
    PropertyRef<bool, PropertyId::ClipToBounds> ClipToBounds;
    PropertyRef<float, PropertyId::CanvasLeft> CanvasLeft;
    PropertyRef<float, PropertyId::CanvasTop> CanvasTop;
    PropertyRef<float, PropertyId::CanvasRight> CanvasRight;
    PropertyRef<float, PropertyId::CanvasBottom> CanvasBottom;
    PropertyRef<int, PropertyId::ZIndex> ZIndex;
    PropertyRef<int, PropertyId::GridColumn> GridColumn;
    PropertyRef<int, PropertyId::GridRow> GridRow;
    PropertyRef<int, PropertyId::GridColumnSpan> GridColumnSpan;
    PropertyRef<int, PropertyId::GridRowSpan> GridRowSpan;
    PropertyRef<std::string, PropertyId::Id> Id;
    PropertyRef<std::string, PropertyId::Tag> Tag;
    PropertyRef<std::string, PropertyId::NavigateUri> NavigateUri;
    PropertyRef<bool, PropertyId::AcceptsReturn> AcceptsReturn;
    PropertyRef<bool, PropertyId::TextWrapping> TextWrapping;
    PropertyRef<bool, PropertyId::IsReadOnly> IsReadOnly;
    PropertyRef<bool, PropertyId::IsPasswordRevealed> IsPasswordRevealed;
    PropertyRef<bool, PropertyId::ShowRevealButton> ShowRevealButton;
    PropertyRef<bool, PropertyId::IsOn> IsOn;
    PropertyRef<bool, PropertyId::IsExpanded> IsExpanded;
    PropertyRef<bool, PropertyId::IsOpen> IsOpen;
    PropertyRef<bool, PropertyId::IsCloseVisible> IsCloseVisible;
    PropertyRef<bool, PropertyId::IsPaneOpen> IsPaneOpen;
    PropertyRef<bool, PropertyId::IsSettingsVisible> IsSettingsVisible;
    PropertyRef<std::string, PropertyId::Title> Title;
    PropertyRef<std::string, PropertyId::Message> Message;
    PropertyRef<std::string, PropertyId::Subtitle> Subtitle;
    PropertyRef<std::string, PropertyId::Header> Header;
    PropertyRef<std::string, PropertyId::PaneTitle> PaneTitle;
    PropertyRef<std::string, PropertyId::GroupName> GroupName;
    PropertyRef<std::string, PropertyId::ActionText> ActionText;

    // 只读属性
    ReadOnlyProperty<Rect> Bounds;
    ReadOnlyProperty<::HWND> HWND;
    ReadOnlyProperty<Size> DesiredSize;
    ReadOnlyProperty<float> ActualWidth;
    ReadOnlyProperty<float> ActualHeight;

    // 集合属性
    CollectionProperty<Element> Children;

    // 回调属性
    CallbackProperty<void(UIElement*)> OnClick;
    CallbackProperty<void(CanvasDrawContext&)> OnDraw;
    CallbackProperty<void(UIElement*, PointerEventArgs&)> OnCanvasMouseDown;
    CallbackProperty<void(UIElement*, PointerEventArgs&)> OnCanvasMouseMove;
    CallbackProperty<void(UIElement*, PointerEventArgs&)> OnCanvasMouseUp;
    CallbackProperty<void(float)> OnTick;

    template<typename T, PropertyId Id>
    PropertyRef<T, Id> Property() {
        PropertyRef<T, Id> property;
        property.Initialize(*this);
        return property;
    }

    virtual PropertyDescSpan GetPropertyDescs() const; // 获取当前控件类型定义的所有属性元数据描述符信息

    /**
     * @brief 属性绑定工厂。获取或创建对应属性 ID 的具体绑定容器实例（懒加载）。
     */
    template<typename T, PropertyId PropId>
    BindableProperty<T>* GetOrCreatePropertyBinding();

    /**
     * @brief 属性查找辅助方法。查找指定属性 ID 是否已有绑定实例。
     */
    template<typename T, PropertyId PropId>
    BindableProperty<T>* FindPropertyBinding() const;

    // 常规属性的属性封装层与手动 Getter / Setter
    float GetWidth() const { return m_width; }          // 获取显式设置的布局宽度数值
    void SetWidth(float v);                             // 显式设置布局宽度，并触发 InvalidateMeasure 重新测算
    float GetHeight() const { return m_height; }        // 获取显式设置的布局高度数值
    void SetHeight(float v);                            // 显式设置布局高度，并触发 InvalidateMeasure 重新测算
    float GetMinWidth() const { return m_minWidth; }    // 获取布局约束的最小宽度值
    void SetMinWidth(float v);                          // 设置布局约束的最小宽度值，并使测量变脏
    float GetMinHeight() const { return m_minHeight; }  // 获取布局约束的最小高度值
    void SetMinHeight(float v);                         // 设置布局约束的最小高度值，并使测量变脏
    float GetMaxWidth() const { return m_maxWidth; }
    void SetMaxWidth(float v);
    float GetMaxHeight() const { return m_maxHeight; }
    void SetMaxHeight(float v);
    Thickness GetMargin() const { return m_margin; }    // 获取控件外边距边缘厚度
    void SetMargin(const Thickness& margin);            // 设置控件外边距边缘厚度，并触发父容器重新排列
    Thickness GetPadding() const { return m_padding; }  // 获取控件内边距填充厚度
    void SetPadding(const Thickness& padding);          // 设置控件内边距填充厚度，并触发内部元素重新测算

    Visibility GetVisibility() const { return m_visibility; } // 获取当前控件的可见性状态
    void SetVisibility(Visibility v);                          // 设置控件的可见性状态，控制隐藏或彻底折叠排版占位
    
    /**
     * @brief 用于测量探针的快速可见性赋值（不会触发 InvalidateMeasure 重新布局与属性通知）。
     * 例如在 Expander 测量折叠部分折拢时的高度。
     */
    void SetVisibilityForMeasureProbe(Visibility v) { m_visibility = v; } // 静默式设定控件内部排版所用的临时可见性
    
    /**
     * @brief 检查节点自身及其所有父辈树是否全被启用。
     */
    bool IsEnabled() const {
        for (const UIElement* walk = this; walk; walk = walk->m_parent) {
            if (!walk->m_isEnabled) {
                return false; // 如果有任何一级父容器被禁用，自身亦处于隐式禁用不可交互状态
            }
        }
        return true;
    }
    void SetIsEnabled(bool enabled); // 设置控件的交互启用状态，控制激活与灰色不可用样式切换

    float GetOpacity() const { return m_opacity; }       // 获取渲染透明度数值
    void SetOpacity(float v);                            // 设置渲染透明度，范围 0.0f - 1.0f
    float GetCornerRadius() const { return m_cornerRadius; } // 获取矩形圆角半径像素值
    void SetCornerRadius(float v);                       // 设置矩形圆角半径，触发重绘
    float GetBorderThickness() const { return m_borderThickness; } // 获取外包围边框线条粗细
    void SetBorderThickness(float v);                    // 设置外包围边框线条粗细，触发重新测算
    float GetFlexGrow() const { return m_flexGrow; }    // 获取在弹性容器中拉伸拉伸填充所占用的空间权重比例
    void SetFlexGrow(float v);                           // 设置在弹性容器中拉伸拉伸填充所占用的空间权重比例

    // 对齐属性管理
    Alignment GetAlign() const { return m_align; }                          // 获取常规排版对齐模式
    void SetAlign(Alignment a);                                             // 设置常规排版对齐模式
    Alignment GetAlignHorizontal() const { return m_alignHorizontal; }      // 获取水平排列对齐模式
    void SetAlignHorizontal(Alignment a);                                    // 设置水平排列对齐模式
    Alignment GetAlignVertical() const { return m_alignVertical; }          // 获取垂直排列对齐模式
    void SetAlignVertical(Alignment a);                                      // 设置垂直排列对齐模式

    // 布局特定属性
    CUI::Orientation GetOrientation() const { return m_orientation; }            // 获取 Stack 等布局的方向朝向
    void SetOrientation(CUI::Orientation o);                                     // 设置 Stack 等布局的方向朝向
    float GetGap() const { return m_gap; }                                  // 获取子元素排列分布的间距像素值
    void SetGap(float v);                                                   // 设置子元素排列分布的间距像素值
    float GetItemWidth() const { return m_itemWidth; }                      // 获取网格或容器内部子项单元的最大限定宽度
    void SetItemWidth(float v);                                             // 设置网格或容器内部子项单元的最大限定宽度
    float GetItemHeight() const { return m_itemHeight; }                    // 获取网格或容器内部子项单元的最大限定高度
    void SetItemHeight(float v);                                             // 设置网格或容器内部子项单元的最大限定高度
    bool GetLastChildFill() const { return m_lastChildFill; }              // 停靠或弹性容器中是否让最后一个子控件强制拉伸填满剩余区域
    void SetLastChildFill(bool v);                                          // 设定停靠或弹性容器中是否让最后一个子控件强制拉伸填满剩余区域
    bool GetJustifyLines() const { return m_justifyLines; }
    void SetJustifyLines(bool v);
    bool GetFillLastLine() const { return m_fillLastLine; }
    void SetFillLastLine(bool v);
    int GetRows() const { return m_rows; }                                  // 获取网格容器预设的行数
    void SetRows(int v);                                                    // 设置网格容器预设的行数
    int GetColumns() const { return m_columns; }                            // 获取网格容器预设的列数
    void SetColumns(int v);                                                 // 设置网格容器预设的列数

    bool GetClipToBounds() const { return m_clipToBounds; }                // 判定是否裁剪超出控件几何范围的内容
    void SetClipToBounds(bool v);                                           // 设定是否裁剪超出控件几何范围的内容

    // 附加定位依赖属性 (Canvas 面板坐标定位)
    float GetCanvasLeft() const { return m_canvasLeft; }                    // 获取画布绝对坐标系中的 X 左边缘像素值
    void SetCanvasLeft(float v);                                            // 设置画布绝对坐标系中的 X 左边缘像素值
    float GetCanvasTop() const { return m_canvasTop; }                      // 获取画布绝对坐标系中的 Y 顶边缘像素值
    void SetCanvasTop(float v);                                             // 设置画布绝对坐标系中的 Y 顶边缘像素值
    float GetCanvasRight() const { return m_canvasRight; }                  // 获取画布绝对坐标系中的 X 右边缘参考像素值
    void SetCanvasRight(float v);                                           // 设置画布绝对坐标系中的 X 右边缘参考像素值
    float GetCanvasBottom() const { return m_canvasBottom; }                // 获取画布绝对坐标系中的 Y 底边缘参考像素值
    void SetCanvasBottom(float v);                                          // 设置画布绝对坐标系中的 Y 底边缘参考像素值
    int GetZIndex() const { return m_zIndex; }                                // 获取 Canvas 中的绘制与命中层级
    void SetZIndex(int v);                                                    // 设置 Canvas 中的绘制与命中层级
    
    // Grid 布局定位参数
    int GetGridColumn() const { return m_gridColumn; }                      // 获取被编排在 Grid 布局中的目标列索引号
    void SetGridColumn(int v);                                              // 设置被编排在 Grid 布局中的目标列索引号
    int GetGridRow() const { return m_gridRow; }                            // 获取被编排在 Grid 布局中的目标行索引号
    void SetGridRow(int v);                                                 // 设置被编排在 Grid 布局中的目标行索引号
    int GetGridColumnSpan() const { return m_gridColumnSpan; }              // 获取在网格中跨越合并的列数
    void SetGridColumnSpan(int v);                                          // 设置在网格中跨越合并的列数
    int GetGridRowSpan() const { return m_gridRowSpan; }                    // 获取在网格中跨越合并的行数
    void SetGridRowSpan(int v);                                             // 设置在网格中跨越合并的行数
    
    // DockPanel 停靠位置
    Dock GetDock() const { return m_dock; }                                  // 获取子元素在 DockPanel 中的停靠方位边沿
    void SetDock(Dock d);                                                   // 设定子元素在 DockPanel 中的停靠方位边沿

    // 主题色彩 Token 手动分配与解析接口
    ThemeTokenId GetBackgroundToken() const { return m_backgroundToken; }   // 获取背景颜色所关联的主题样式 Token
    void SetBackgroundToken(ThemeTokenId id);                               // 设置背景颜色所关联的主题样式 Token
    ThemeTokenId GetHoverBackgroundToken() const { return m_hoverBackgroundToken; } // 获取鼠标悬浮时的背景主题 Token
    void SetHoverBackgroundToken(ThemeTokenId id);                          // 设置鼠标悬浮时的背景主题 Token
    ThemeTokenId GetPressedBackgroundToken() const { return m_pressedBackgroundToken; } // 获取按下状态的背景主题 Token
    void SetPressedBackgroundToken(ThemeTokenId id);                        // 设置按下状态的背景主题 Token
    ThemeTokenId GetDisabledBackgroundToken() const { return m_disabledBackgroundToken; } // 获取不可交互时的灰色背景主题 Token
    void SetDisabledBackgroundToken(ThemeTokenId id);                       // 设置不可交互时的灰色背景主题 Token
    ThemeTokenId GetBorderToken() const { return m_borderToken; }           // 获取静态边框的主题色彩 Token
    void SetBorderToken(ThemeTokenId id);                                   // 设置静态边框的主题色彩 Token
    ThemeTokenId GetFocusedBorderToken() const { return m_focusedBorderToken; } // 获取选中聚焦时的高亮边框主题 Token
    void SetFocusedBorderToken(ThemeTokenId id);                            // 设置选中聚焦时的高亮边框主题 Token
    ThemeTokenId GetColorToken() const { return m_colorToken; }              // 获取文字前景字元的主题 Token
    void SetColorToken(ThemeTokenId id);                                    // 设置文字前景字元的主题 Token
    ThemeTokenId GetSecondaryColorToken() const { return m_secondaryColorToken; } // 获取次要辅助信息文本主题 Token
    void SetSecondaryColorToken(ThemeTokenId id);                           // 设置次要辅助信息文本主题 Token
    ThemeTokenId GetPlaceholderColorToken() const { return m_placeholderColorToken; } // 获取水印占位文本主题 Token
    void SetPlaceholderColorToken(ThemeTokenId id);                         // 设置水印占位文本主题 Token
    ThemeTokenId GetSelectedBackgroundToken() const { return m_selectedBackgroundToken; } // 获取被选择项的背景主题 Token
    void SetSelectedBackgroundToken(ThemeTokenId id);                       // 设置被选择项的背景主题 Token
    ThemeTokenId GetHeaderBackgroundToken() const { return m_headerBackgroundToken; } // 获取标题头部装饰的主题 Token
    void SetHeaderBackgroundToken(ThemeTokenId id);                         // 设置标题头部装饰的主题 Token
    ThemeTokenId GetPaneBackgroundToken() const { return m_paneBackgroundToken; } // 获取抽屉、侧边栏专用的底色主题 Token
    void SetPaneBackgroundToken(ThemeTokenId id);                           // 设置抽屉、侧边栏专用的底色主题 Token
    ThemeTokenId GetIndicatorColorToken() const { return m_indicatorColorToken; } // 获取焦点浮动游标、滚动条指示点主题 Token
    void SetIndicatorColorToken(ThemeTokenId id);                           // 设置焦点浮动游标、滚动条指示点主题 Token
    ThemeTokenId GetDropdownBackgroundToken() const { return m_dropdownBackgroundToken; } // 获取下拉弹出菜单背景板主题 Token
    void SetDropdownBackgroundToken(ThemeTokenId id);                       // 设置下拉弹出菜单背景板主题 Token
    ThemeTokenId GetSelectedItemBackgroundToken() const { return m_selectedItemBackgroundToken; } // 获取列表中已被选项的底盘主题 Token
    void SetSelectedItemBackgroundToken(ThemeTokenId id);                   // 设置列表中已被选项的底盘主题 Token
    ThemeTokenId GetFillColorToken() const { return m_fillColorToken; }      // 获取一般性几何实心填充色主题 Token
    void SetFillColorToken(ThemeTokenId id);                                // 设置一般性几何实心填充色主题 Token
    ThemeTokenId GetTrackColorToken() const { return m_trackColorToken; }    // 获取滑道静止底槽的颜色主题 Token
    void SetTrackColorToken(ThemeTokenId id);                               // 设置滑道静止底槽的颜色主题 Token
    ThemeTokenId GetActiveTrackColorToken() const { return m_activeTrackColorToken; } // 获取滑块左侧填充的高亮进度色主题 Token
    void SetActiveTrackColorToken(ThemeTokenId id);                         // 设置滑块左侧填充的高亮进度色主题 Token
    ThemeTokenId GetThumbColorToken() const { return m_thumbColorToken; }    // 获取拖动小纽扣外饰颜色主题 Token
    void SetThumbColorToken(ThemeTokenId id);                               // 设置拖动小纽扣外饰颜色主题 Token
    ThemeTokenId GetOnColorToken() const { return m_onColorToken; }          // 获取 ToggleSwitch 开启状态下的滑槽主题 Token
    void SetOnColorToken(ThemeTokenId id);                                  // 设置 ToggleSwitch 开启状态下的滑槽主题 Token
    ThemeTokenId GetOffColorToken() const { return m_offColorToken; }        // 获取 ToggleSwitch 关闭状态下的滑槽主题 Token
    void SetOffColorToken(ThemeTokenId id);                                 // 设置 ToggleSwitch 关闭状态下的滑槽主题 Token
    ThemeTokenId GetKnobColorToken() const { return m_knobColorToken; }      // 获取 ToggleSwitch 纽扣触点填充主题 Token
    void SetKnobColorToken(ThemeTokenId id);                                // 设置 ToggleSwitch 纽扣触点填充主题 Token
    ThemeTokenId GetCheckedBackgroundToken() const { return m_checkedBackgroundToken; } // 获取CheckBox复选框勾选底盘主题 Token
    void SetCheckedBackgroundToken(ThemeTokenId id);                        // 设置CheckBox复选框勾选底盘主题 Token
    ThemeTokenId GetAccentColorToken() const { return m_accentColorToken; }  // 获取系统全局强调色/品牌特征高亮色 Token
    void SetAccentColorToken(ThemeTokenId id);                              // 设置系统全局强调色/品牌特征高亮色 Token
    ThemeTokenId GetActiveColorToken() const { return m_activeColorToken; }  // 获取选中激活状态下主要元件颜色 Token
    void SetActiveColorToken(ThemeTokenId id);                              // 设置选中激活状态下主要元件颜色 Token
    ThemeTokenId GetUnderlineColorToken() const { return m_underlineColorToken; } // 获取静止下划线线条色彩 Token
    void SetUnderlineColorToken(ThemeTokenId id);                           // 设置静止下划线线条色彩 Token
    ThemeTokenId GetActiveUnderlineColorToken() const { return m_activeUnderlineColorToken; } // 获取激活/Hover下划线线条色彩 Token
    void SetActiveUnderlineColorToken(ThemeTokenId id);                     // 设置激活/Hover下划线线条色彩 Token
    ThemeTokenId GetActiveTabBackgroundToken() const { return m_activeTabBackgroundToken; } // 获取活动中 Tab 标签头背景色 Token
    void SetActiveTabBackgroundToken(ThemeTokenId id);                      // 设置活动中 Tab 标签头背景色 Token
    ThemeTokenId GetInactiveTabBackgroundToken() const { return m_inactiveTabBackgroundToken; } // 获取未激活 Tab 标签头背景色 Token
    void SetInactiveTabBackgroundToken(ThemeTokenId id);                    // 设置未激活 Tab 标签头背景色 Token
    ThemeTokenId GetGridLineBrushToken() const { return m_gridLineBrushToken; } // 获取网格分割线画笔颜色 Token
    void SetGridLineBrushToken(ThemeTokenId id);                            // 设置网格分割线画笔颜色 Token
    ThemeTokenId GetTitleColorToken() const { return m_titleColorToken; }    // 获取消息框标题大文本颜色 Token
    void SetTitleColorToken(ThemeTokenId id);                               // 设置消息框标题大文本颜色 Token
    ThemeTokenId GetMessageColorToken() const { return m_messageColorToken; } // 获取消息框内容文本的颜色 Token
    void SetMessageColorToken(ThemeTokenId id);                             // 设置消息框内容文本的颜色 Token
    ThemeTokenId GetCaretColorToken() const { return m_caretColorToken; }    // 获取 TextBox 文本光标闪烁主题 Token
    void SetCaretColorToken(ThemeTokenId id);                               // 设置 TextBox 文本光标闪烁主题 Token

    /**
     * @brief 解析对应的主题色彩 Token。若该 Token 未指定，则回退解析指定的 fallback 色彩 Token。
     */
    D2D1_COLOR_F ResolveThemeColor(ThemeTokenId token, ThemeTokenId fallback) const;
    D2D1_COLOR_F ResolveThemeColor(PropertyId tokenId, ThemeTokenId fallback) const;

    // 自定义具体硬编码配色重写接口
    void SetBackground(D2D1_COLOR_F c);                 // 硬编码设置背景色（将覆盖对应的 Token 主题配置）
    void SetBackgroundColor(D2D1_COLOR_F c) { SetBackground(c); } // 设置硬编码背景色彩值别名
    D2D1_COLOR_F GetBackgroundColor() const { return m_backgroundColor; } // 获取重写的硬编码背景色
    bool HasBackgroundColor() const { return m_hasBackgroundColor; } // 判断当前是否启用了硬编码背景重写
    void SetHoverBackground(D2D1_COLOR_F c);            // 硬编码设置鼠标悬浮背景色
    D2D1_COLOR_F GetHoverBackgroundColor() const { return m_hoverBackgroundColor; } // 获取硬编码悬浮背景色
    bool HasHoverBackgroundColor() const { return m_hasHoverBackgroundColor; } // 是否设置了悬停背景重写
    void SetPressedBackground(D2D1_COLOR_F c);          // 硬编码设置鼠标按下背景色
    D2D1_COLOR_F GetPressedBackgroundColor() const { return m_pressedBackgroundColor; } // 获取硬编码按下背景色
    bool HasPressedBackgroundColor() const { return m_hasPressedBackgroundColor; } // 是否设置了按压背景重写
    
    void SetBorderBrush(D2D1_COLOR_F c);                // 硬编码设置边框笔刷色彩值
    D2D1_COLOR_F GetBorderBrushColor() const { return m_borderBrushColor; } // 获取硬编码设置的边框色彩值
    bool HasBorderBrushColor() const { return m_hasBorderBrushColor; } // 是否设置了边框重写色彩值
    void SetColor(D2D1_COLOR_F c);                      // 硬编码设置前景/文本色彩值
    void SetTextColor(D2D1_COLOR_F c) { SetColor(c); } // 设置硬编码前景/文本色彩值别名
    D2D1_COLOR_F GetColorValue() const { return m_colorValue; } // 获取硬编码前景色彩值
    D2D1_COLOR_F GetTextColor() const { return m_colorValue; } // 获取硬编码文本色彩值别名
    bool HasColorValue() const { return m_hasColorValue; } // 是否存在硬编码前景色彩值
    bool HasTextColor() const { return m_hasColorValue; } // 是否存在硬编码文本色彩值别名

    const std::string& GetText() const { return m_text; } // 获取当前缓存的展示文字字符串
    void SetText(const std::string& text);              // 修改展示文本，触发重新测量及重绘
    void BindText(const std::shared_ptr<Observable<std::string>>& value); // 将文本绑定至一个 Observable 数据源上
    void UnbindText();                                  // 断开当前绑定的文本数据源链接
    const std::string& GetPlaceholder() const { return m_placeholder; } // 获取 TextBox 内部的水印文本
    void SetPlaceholder(const std::string& placeholder); // 设置 TextBox 内部的水印提示文本
    
    // 字体相关手动接口
    const std::string& GetFontFamily() const { return m_fontFamily; } // 获取当前指定的字体族名称
    void SetFontFamily(const std::string& font);        // 修改所采用的字体族，触发重新测算
    float GetFontSize() const { return m_fontSize; }    // 获取当前文字渲染大小 (px)
    void SetFontSize(float size);                       // 修改渲染文字大小，触发重新测算
    CUI::FontWeight GetFontWeight() const { return m_fontWeight; } // 获取字重粗细
    void SetFontWeight(CUI::FontWeight weight);         // 设定字重粗细，触发重新测算
    DWRITE_FONT_WEIGHT ResolveFontWeight() const;       // 解析成 DWrite API 可直接认知的数值
    CUI::FontStyle GetFontStyle() const { return m_fontStyle; } // 获取字体样式（斜体/直立）
    void SetFontStyle(CUI::FontStyle style);            // 设定字体样式，触发重新排版
    DWRITE_FONT_STYLE ResolveFontStyle() const;         // 解析成 DWrite API 直接认知的斜体数值
    CUI::FontStretch GetFontStretch() const { return m_fontStretch; } // 获取字元水平拉伸变形度
    void SetFontStretch(CUI::FontStretch stretch);      // 设定字元水平拉伸变形度
    DWRITE_FONT_STRETCH ResolveFontStretch() const;     // 解析成 DWrite 可认知的水平变形拉伸值
    
    bool IsUnderline() const { return m_isUnderline; }  // 判断文字是否拥有下划线修饰
    void SetIsUnderline(bool underline);                // 控制文字是否有下划线修饰
    bool IsStrikethrough() const { return m_isStrikethrough; } // 判断文字是否拥有中线删除线修饰
    void SetIsStrikethrough(bool strikethrough);        // 控制文字是否有中线删除线修饰
    
    // 工具提示 (ToolTip) 设置
    const std::string& GetToolTip() const { return m_toolTip; } // 获取当前设置的鼠标悬停信息提示字符串
    void SetToolTip(const std::string& tip);            // 设置鼠标悬停信息提示字符串
    void SetToolTipMaxWidth(float width);               // 指定悬停提示框内部文字折行的最长像素宽度上限
    float GetToolTipMaxWidth() const { return m_toolTipMaxWidth; } // 获取悬停提示框内部文字折行的最长像素宽度上限
    void SetToolTipAutoHideMs(int ms);                  // 设置显示几毫秒后将自动淡出关闭悬停框
    int GetToolTipAutoHideMs() const { return m_toolTipAutoHideMs; } // 获取自动隐藏的时限 (ms)

    // 全局 ToolTip 静态时间参数管理
    static void SetToolTipShowDelayMs(int ms);          // 静态设置鼠标需要在此控件悬停几毫秒后触发显示提示框
    static void SetToolTipHideDelayMs(int ms);          // 静态设置鼠标离去后需要延迟几毫秒后再彻底折拢提示框
    static void SetDefaultToolTipMaxWidth(float width); // 静态设置全局提示框默认换行像素宽
    static void SetDefaultToolTipAutoHideMs(int ms);    // 静态设置全局默认多久后淡出提示框
    static int GetToolTipShowDelayMs();                 // 获取全局弹出延迟时限
    static int GetToolTipHideDelayMs();                 // 获取全局关弹延迟时限
    static float GetDefaultToolTipMaxWidth();           // 获取全局默认换行像素宽
    static int GetDefaultToolTipAutoHideMs();           // 获取全局默认淡出限时值
    
    // 图标设置
    const std::string& GetIcon() const { return m_icon; } // 获取控件关联的矢量/图片图标名称
    void SetIcon(const std::string& icon);              // 设定控件关联的图标，并使界面局部刷新

    // 反射式动态属性存取接口
    void SetProperty(PropertyId id, const Value& val) override; // 通过运行时反射 PropertyId 强行注入通用属性值
    Value GetProperty(PropertyId id) const override;           // 根据 PropertyId 反射读取属性的通用 Variant 格式值
    bool HasProperty(PropertyId id) const override;            // 判断该控件内部属性列表是否存在某个反射标识属性
    std::vector<std::pair<PropertyId, Value>> SnapshotProperties() const override; // 生成该控件所有动态属性的内存快照

    // 控件调试与标记标识
    ::HWND GetHWND() const;
    std::string GetId() const { return m_id; }          // 获取用户手动分配的控件唯一字符串 ID
    void SetId(const std::string& id) { m_id = id; }
    std::string GetTag() const { return m_tag; }
    void SetTag(const std::string& tag) { m_tag = tag; }    // 分配唯一的字符串 ID 以便在层次树中检索
    std::string GetStyleClass() const { return m_styleClass; } // 获取样式表类名
    void SetStyleClass(const std::string& styleClass) { m_styleClass = styleClass; } // 赋予样式表类名

    // 树结构层级关系存取
    UIElement* GetParent() const { return m_parent; }   // 获取指向父节点控件的原始指针
    void SetParent(UIElement* parent);                  // 设置所属的父节点，维护树结构

    /**
     * @brief 注册动画宿主。
     * 对于悬浮泡泡、弹出对话框等未真实加入排版树 (Measure/Arrange) 的视觉元素，
     * 需要绑定到活动状态的树节点以参与动画时钟 tick 步进并标记区域重绘。
     */
    void SetAnimationHost(UIElement* host);
    UIElement* GetAnimationHost() const { return m_animationHost; } // 获取该 Popup 解离节点的动画挂载宿主

    // 子节点操作
    const std::vector<std::shared_ptr<UIElement>>& GetChildren() const { return m_children; } // 获取旗下所有的直系子控件集合
    std::vector<std::shared_ptr<UIElement>> GetVisualChildren() const; // 获取按当前容器绘制规则排序的子节点
    virtual bool UsesZIndexOrdering() const { return false; } // Canvas 覆写后按 ZIndex 进行稳定排序
    void AddChild(std::shared_ptr<UIElement> child);    // 加入子节点，并将整个容器链 Measure 标记为脏
    void AddChildQuiet(std::shared_ptr<UIElement> child); // 静默加入子节点，不会触发 Invalidate 重新测算
    void RemoveChild(std::shared_ptr<UIElement> child); // 移除直系子控件，标记画面和测量均变脏
    void RemoveChildQuiet(std::shared_ptr<UIElement> child); // 静默移除直系子控件
    void RemoveChildRaw(UIElement* child);              // 针对弱引用的子控件原始指针进行删除操作
    void ClearChildren();                               // 清空名下的所有子控件

    // 层次树搜索查找
    std::shared_ptr<UIElement> FindElementById(const std::string& id); // 在当前子树中通过 ID 深度优先检索子控件

    // 控件外包络盒与 desiredSize 存取
    Rect GetBounds() const { return m_bounds; }         // 获取排版后的局部门口包络边界
    void SetBounds(const Rect& bounds);                 // 强行指定控件的位置和包络盒
    Size GetDesiredSize() const { return m_desiredSize; } // 获取测量测算得出的期望占位宽与高

    /**
     * @brief 布局第一阶段：测量阶段。
     * 每个控件根据父级分配的 availableSize 递归调用子控件的 Measure，并计算自己所需的 desiredSize 并保存。
     * 派生类重写该方法以实现自定义的测量计算。
     */
    virtual Size Measure(Size availableSize);

    /**
     * @brief 布局第二阶段：排列阶段。
     * 父控件在确定自己大小后，将最终的位置和分配矩形 finalRect 分配给子控件，子控件在内部进行对齐排版。
     */
    virtual void Arrange(Rect finalRect);

    virtual bool ShouldClipToBounds() const; // 判断当前是否应当在边界截断，防止溢出绘制

    /**
     * @brief 控件的总体绘制入口。负责处理分层栅格化缓存、Clip 边界截断，并触发具体的 OnRender。
     */
    virtual void Render(GraphicsContext& ctx);

    /**
     * @brief 控件的具体自绘逻辑。派生控件应重写该方法，利用 ctx 进行绘制（如绘制文本、圆形、边框等）。
     */
    virtual void OnRender(GraphicsContext& ctx);

    /**
     * @brief 绘制覆盖在普通渲染层之上的遮罩/悬浮弹出层（如 DropDown 列表、Tooltip 等）。
     */
    virtual void RenderOverlay(GraphicsContext& ctx);
    virtual void OnRenderOverlay(GraphicsContext& ctx) {}

    // 点击碰撞测试
    virtual UIElement* HitTest(float x, float y);       // 在排版树内递归进行点检测，定位鼠标落入的最里层激活节点
    virtual UIElement* HitTestOverlay(float x, float y); // 对覆盖图层区域进行点碰撞测试
    virtual UIElement* OnHitTestOverlay(float x, float y) { return nullptr; } // 允许具体Popup控件重写定位它专属的悬浮层点命中
    
    /**
     * @brief 若节点在正常的排版边界外渲染了弹出/悬浮层，必须返回 true 允许穿透命中测试，否则这些事件会在外层被吞掉。
     */
    virtual bool NeedsOverlayHitTest() const { return false; } // 指明当前控件的悬浮层是否有接受命中测试请求的诉求

    // 交互状态判断
    bool IsHovered() const { return m_isHovered; }      // 鼠标当前是否悬浮在该控件的正上方
    bool IsPressed() const { return m_isPressed; }      // 用户鼠标左键是否在此控件上处于按下的未释放状态
    bool IsFocused() const { return m_isFocused; }      // 控件当前是否持有窗口焦点

    virtual HCURSOR GetCursor() const { return nullptr; } // 虚函数以允许控件自定义在 Hover 时的 Windows 鼠标样式类型

    // 常规底层事件路由虚函数，派生控件可自行重写
    virtual void OnMouseEnter();                        // 鼠标首次进入控件物理几何边界时的触发入口
    virtual void OnMouseLeave();                        // 鼠标滑出控件物理几何边界时的触发入口
    virtual void OnMouseDown(Point pt);                  // 鼠标任意键在控件区域内按下的回调入口
    virtual void OnMouseDblClick(Point pt) {}           // 鼠标双击事件
    virtual void OnMouseRightClick(Point pt) {}          // 鼠标右键单击释放事件
    virtual bool OnContextMenuRelease(Point pt) { (void)pt; return false; } // 响应系统弹出右键菜单，返回 true 代表拦截事件
    virtual void OnMouseUp(Point pt);                    // 鼠标松开释放事件
    virtual void OnMouseMove(Point pt);                  // 鼠标在控件上方移动的事件入口
    virtual void OnMouseWheel(float delta);             // 鼠标滚轮发生位移的事件入口
    
    // 浏览器风格的鼠标中键自动滚屏 (Capturing)
    virtual bool OnMiddleButtonDown(Point pt) { (void)pt; return false; } // 鼠标中键按下回调，返回 true 代表捕获滚屏
    virtual void OnMiddleButtonUp(Point pt) { (void)pt; }                  // 鼠标中键释放回调
    virtual bool IsMiddleScrollActive() const { return false; }            // 验证当前是否正处于中键自动滚动运行模式
    
    virtual bool OnKeyDown(int vkCode);                 // 键盘物理扫描按键压下时的回调，返回 true 代表完全消耗此事件
    virtual void OnCharInput(wchar_t ch) {}             // 键盘经过 IME 翻译后转出的 Unicode 字元字符输入回调
    
    // 键盘焦点属性与控制
    virtual bool AcceptsTabFocus() const { return false; } // 表明该控件是否能够允许通过 Tab 按键获焦（默认不可聚焦）
    KeyboardNavigationMode GetKeyboardNavigationMode() const { return m_keyboardNavigationMode; } // 获取 Tab 跳转循环规则
    void SetKeyboardNavigationMode(KeyboardNavigationMode mode) { m_keyboardNavigationMode = mode; } // 设定 Tab 跳转循环规则
    FocusState GetFocusState() const { return m_focusState; } // 获取获焦的激活状态类型
    void SetFocusState(FocusState state) { m_focusState = state; } // 设置获焦类型
    bool Focus(FocusState state = FocusState::Keyboard); // 主动申请将窗口焦点聚焦到此控件
    void Blur();                                        // 主动剥夺当前控件的窗口聚焦，释放焦点
    bool ShowsKeyboardFocusRing() const {
        return m_isFocused && m_focusState == FocusState::Keyboard; // 指明只有在键盘获焦时才渲染高亮聚焦虚线边框环
    }

    // 命令绑定系统
    void SetCommand(std::shared_ptr<Command> command);   // 为控件（如按钮）分配绑定的 Action 命令逻辑
    std::shared_ptr<Command> GetCommand() const { return m_command; } // 读取已绑定的 Action 命令
    bool ExecuteBoundCommand();                         // 触发执行已绑定的 Command 命令
    
    // 自动滚轮时钟 tick
    virtual void OnAutoScrollTick() {}                  // 自动动画滚屏周期性 Tick 调度回调
    virtual bool NeedsAutoScrollTick() const { return false; } // 表明当前控件是否急需自动滚动 Tick 事件
    
    // 动画驱动机制
    virtual bool OnAnimationTick();                     // 被 AnimationManager 泵出的全局时钟物理动画帧驱动入口
    virtual bool HasSelfAnimation() const { return false; } // 宣告此节点当前是否包含正在活动中的过渡动画
    virtual bool IsModalOverlayOpen() const { return false; } // 宣告当前控件是否开启了模态弹出层（会冻结主界面的消息及渲染）
    virtual bool IsComposeOnlyAnimation() const { return false; } // 标识当前是否属于无需无效化场景的独立合成层（DComp）动画
    virtual bool ComposePresent(GraphicsContext& ctx) { (void)ctx; return false; } // 硬件加速合成呈现入口
    
    // 计算并收集当前动画对画面造成的污损矩形范围 (Animation Bounds)
    virtual void CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const; // 仅收集自身变动范围
    virtual void CollectAnimationBounds(Rect& dirtyRect, bool& hasDirty) const;   // 递归收集包括子树在内的动画变动脏矩形
    virtual void CollectRenderDirtyRegion(DirtyRegion& dirtyRegion, bool consume = true); // 提取重绘的失效脏区域
    
    // 动画管理器注册
    void RequestAnimationTicks();                       // 登记动画更新需求，把自身送入全局动画 Tick 循环中
    void CancelAnimationTicks();                        // 注销动画，从 Tick 队列中移除以节约 CPU 开销
    bool IsAnimationTicksRegistered() const { return m_animationTicksRegistered; } // 验证是否已在动画驱动注册列表中
    
    // 跨 HWND 多窗口停靠交互属性
    void SetPresentsOnOwnerWindow(bool enabled) { m_presentsOnOwnerWindow = enabled; } // 设定是否在主窗口完成绘制呈现
    bool PresentsOnOwnerWindow() const { return m_presentsOnOwnerWindow; } // 获取是否在主窗口完成绘制呈现
    void SetOverlayComposed(bool enabled) { m_overlayComposed = enabled; } // 标识该控件已被强制转移绘制于悬浮图层
    bool IsOverlayComposed() const { return m_overlayComposed; } // 获取是否在悬浮层被合成
    
    // 全局静态动画开关控制
    static void SetAnimationsEnabled(bool enabled);     // 静态配置全局过渡动画的使能总开关（关闭可显著降低资源负载）
    static bool AreAnimationsEnabled();                 // 查询全局动画是否正常开启
    static void SetAnimationDeltaSeconds(float dtSeconds); // 设置模拟时钟物理帧间隔秒数
    static float GetAnimationDeltaSeconds();            // 读取模拟时钟物理帧间隔秒数
    
    // 渲染污损版本计数 (以区分无效呈现优化)
    static uint64_t GetRenderDirtySerial() { return s_renderDirtySerial; } // 读取全局失效重绘日志的累计计数号
    
    RenderNode& GetRenderNode() { return m_renderNode; } // 提取节点自身直接映射的底层组合场景图渲染树节点
    const RenderNode& GetRenderNode() const { return m_renderNode; }
    virtual void SyncRenderState();                     // 布局/状态发生突变后，向底层 RenderNode 渲染状态进行物理同步
    virtual void MarkRenderContentDirty();              // 强制将当前控件设为变脏需要重绘
    virtual void MarkRenderRectDirty(const Rect& rect);  // 失效当前控件包络矩形中的特定子区域
    virtual void OnThemeChanged();                      // 响应程序主题全局切换（例如由浅色变为深色模式）

    // 触发局部失效重新测量排版
    void InvalidateMeasure();                           // 使测算结果失效，重新排队进行 Measure 测量
    void InvalidateArrange();                           // 使对齐结果失效，重新排队进行 Arrange 排列
    bool IsMeasureDirty() const { return m_measureDirty; } // 是否为测量待办脏节点
    bool IsArrangeDirty() const { return m_arrangeDirty; } // 是否为排列对齐待办脏节点
    bool HasLayoutDirtyInSubtree() const;                 // 当前节点或任一子节点是否待重新布局
    void FlushLayout(Size availableSize, const Rect& arrangeRect); // 执行局部局限内的布局快速刷新

    // 高端合成图层属性提升相关 (Promoted Layer)
    void PromoteLayer(bool promote);                    // 将该控件的渲染分支直接提权成为一块离屏 Direct2D 图块纹理层
    bool IsLayerPromoted() const { return m_layerPromoted; } // 查询当前是否是高阶离屏合成纹理图层
    void SetComposeOpacity(float opacity);              // 动画直接操控离屏合成层透明度（不需要重新光栅化栅格渲染）
    float GetComposeOpacity() const { return m_composeOpacity; } // 获取合成透明度
    void SetComposeOffset(float x, float y);            // 动画直接平移合成图块
    float GetComposeOffsetX() const { return m_composeOffsetX; } // 获取合成层 X 轴平移距离
    float GetComposeOffsetY() const { return m_composeOffsetY; } // 获取合成层 Y 轴平移距离
    bool HasComposeDirty() const { return m_composeDirty; } // 合成属性是否脏，需要重新提交 Composition 层级树
    void ClearComposeDirty() { m_composeDirty = false; } // 擦除合成属性脏标记

    // 页面生命周期阶段回调
    virtual void OnNavigatedTo();                       // NavigationView 的内容子页面完成挂载展示后的调用入口
    virtual void OnNavigatedFrom();                     //  NavigationView 卸载子页面或其离屏时的生命周期结束入口
    void PauseAnimationSubtree();                       // 递归休眠当前节点和所有子树动画，暂停重排
    void ResumeAnimationSubtree();                      // 唤醒当前节点和所有子树动画重新工作

    // 路由事件处理入口
    virtual void OnRoutedEvent(RoutedEventArgs& args);  // 具有穿透和冒泡特性的 Routed 事件系统派发主通道

    // 上下文右键菜单管理
    void SetContextMenu(std::shared_ptr<ContextMenu> menu) { m_contextMenu = menu; } // 为该控件关联分配一套快捷右键上下文菜单
    std::shared_ptr<ContextMenu> GetContextMenu() const { return m_contextMenu; } // 提取当前关联的右键上下文菜单

    virtual void OnFocus();                             // 聚焦生命周期的具体调用入口
    virtual void OnBlur();                              // 失去焦点的具体调用入口

    Event<UIElement*, Point>& OnMouseDownEvent() { return m_onMouseDownEvent; } // 获得鼠标按下事件委托的连接点

protected:
    // 响应式数据绑定系统的底层承载结构槽定义
    struct PropertyBindingSlotBase {
        virtual ~PropertyBindingSlotBase() = default;
    };

    template<typename T>
    struct PropertyBindingSlot final : PropertyBindingSlotBase {
        PropertyBindingSlot(
            Object& owner,
            PropertyId propertyId,
            typename BindableProperty<T>::Getter getter,
            typename BindableProperty<T>::Setter setter)
            : value(owner, propertyId, std::move(getter), std::move(setter)) {}

        BindableProperty<T> value;
    };

    mutable std::unique_ptr<std::unordered_map<PropertyId, std::unique_ptr<PropertyBindingSlotBase>>> m_propertyBindings; // 内部缓存的属性反射绑定双向同步结构映射图表

    void NotifyFieldChanged(PropertyId id, const Value& val); // 主动发送属性修改通知，分发给绑定器和订阅者

    template<typename T, PropertyId Id>
    friend class PropertyRef;
    bool DescHasOptionalProperty(const PropertyDesc& desc) const; // 检查元数据描述符是否附带特定的非必要附加参数

    // 控件核心变量定义
    std::string m_id;
    std::string m_tag;                                                 // 控件的全局字符串唯一检索标识
    std::string m_styleClass;                                         // 关联样式类以映射不同的 CSS/Qss 风格渲染分支
    std::string m_text;                                               // 内置核心字符串文本
    std::string m_placeholder;                                        // 输入框等专用的淡灰色占位提示符字符串
    std::string m_fontFamily{ "微软雅黑" };                            // 指定渲染所用字库名称，默认采用"微软雅黑"
    CUI::FontWeight m_fontWeight = CUI::FontWeight::Normal;            // 字体加粗权级变量，默认为正常字重
    CUI::FontStyle m_fontStyle = CUI::FontStyle::Normal;              // 字体倾斜样式，默认为直立
    CUI::FontStretch m_fontStretch = CUI::FontStretch::Normal;        // 字元横向拉伸宽度，默认为标准比例
    std::string m_toolTip;                                            // 指针悬浮气泡框内的纯文本内容
    std::string m_icon;                                               // 图标库名称或指向图标文件的路径
    float m_fontSize = 12.0f;                                         // 文字像素高度大小 (px)，默认 12.0f
    bool m_isUnderline = false;                                       // 文本底端划线状态使能，默认为 false
    bool m_isStrikethrough = false;                                   // 文本删除线状态使能，默认为 false

    float m_width = -1.0f;                                            // 用户强行指派的宽度。若小于 0 则代表使用布局自适应测算
    float m_height = -1.0f;                                           // 用户强行指派的高度。若小于 0 则代表使用布局自适应测算
    float m_minWidth = 0.0f;
    float m_maxWidth = -1.0f;                                          // 布局所强制遵循的最小像素宽
    float m_minHeight = 0.0f;
    float m_maxHeight = -1.0f;                                         // 布局所强制遵循的最小像素高
    Thickness m_margin{};                                             // 外边距厚度，负责在盒模型中撑开与外部同级控件的间距
    Thickness m_padding{};                                            // 内边距厚度，负责在盒模型中撑开与自身内部子控件的空白
    Visibility m_visibility = Visibility::Visible;                    // 可见性，定义在测算时是否保留所占有的尺寸
    bool m_isEnabled = true;                                          // 交互标记，为 false 时将拦截一切鼠标和键盘导航响应，样式渲染置灰
    float m_opacity = 1.0f;                                           // 渲染混合图层不透明度，0.0f 为全透，1.0f 为全实
    float m_cornerRadius = 0.0f;                                      // 绘制圆角矩形区域的角半径值
    float m_borderThickness = 0.0f;                                   // 边框描边粗细像素宽
    float m_flexGrow = 0.0f;                                          // 当在 FlexPanel 排版时，弹性撑开所吃掉多余空间的权重
    Alignment m_align = Alignment::Stretch;                           // 定义常规排版对齐模式，默认进行强制拉伸 Stretch
    Alignment m_alignHorizontal = Alignment::Stretch;                 // 定义水平排版对齐模式
    Alignment m_alignVertical = Alignment::Stretch;                   // 定义垂直排版对齐模式
    CUI::Orientation m_orientation = CUI::Orientation::Vertical;           // 描述容器中多子项排版的空间朝向，默认垂直向下排列
    float m_gap = 0.0f;                                               // 子控件多列/多行排列时相互隔开的缝隙大小
    float m_itemWidth = -1.0f;                                        // 指派内部每个项单元分配的固定宽度
    float m_itemHeight = -1.0f;                                       // 指派内部每个项单元分配的固定高度
    bool m_lastChildFill = false;
    bool m_justifyLines = false;
    bool m_fillLastLine = true;                                     // 在 Dock 布局中是否强行放大最后一个子控件塞满所有剩余死角
    int m_rows = 1;                                                   // 给 GridPanel 这种网格容器指定的绝对行行数
    int m_columns = 1;                                                // 给 GridPanel 这种网格容器指定的绝对列列数
    bool m_clipToBounds = false;                                      // 为 true 时，会设置几何裁切区域，拒绝让子节点越界绘制
    bool m_subtreeRenderDirty = false;                                // 标记该节点以下至少有一级子孙控件请求了重新绘制
    bool m_measureDirty = true;                                       // 测量变脏标记。一旦子控件或尺寸改变，该值置为 true 重算 desiredSize
    bool m_arrangeDirty = true;                                       // 对齐变脏标记。一旦被强制重绘或父级要求，该值置为 true 重算 bounds
    Size m_lastMeasureAvailable{ -1.0f, -1.0f };                      // 记录上一次进行 Measure 调用时，父控件所放出的可用边界
    
    // 合成层渲染参数
    bool m_layerPromoted = false;                                     // 是否升级成了独立分层图块缓存，以大幅提升复杂组合下的重绘效率
    bool m_composeDirty = false;                                      // 合成属性发生改变脏标记，指示宿主窗口下一次刷新图层结构
    float m_composeOpacity = 1.0f;                                    // 离屏纹理合成时的不透明度参数
    float m_composeOffsetX = 0.0f;                                    // 离屏纹理合成时的视口水平位移量
    float m_composeOffsetY = 0.0f;                                    // 离屏纹理合成时的视口垂直位移量
    Rect m_lastComposeScreenBounds{};                                 // 缓存该合成纹理最后在屏幕设备上的物理绝对坐标

    // 附加布局参数变量
    float m_canvasLeft = kAttachedUnset;                              // Canvas 自绘定位 X 左坐标绝对像素值
    float m_canvasTop = kAttachedUnset;                               // Canvas 自绘定位 Y 顶坐标绝对像素值
    float m_canvasRight = kAttachedUnset;                             // Canvas 自绘定位右边界像素参考值
    float m_canvasBottom = kAttachedUnset;                            // Canvas 自绘定位底边界像素参考值
    int m_zIndex = 0;                                                   // Canvas 子项的绘制与命中层级
    int m_gridColumn = 0;                                             // 该控件放置在网格组件的第几列
    int m_gridRow = 0;                                                // 该控件放置在网格组件的第几行
    int m_gridColumnSpan = 1;                                         // 该网格子项在水平方向要拉伸合并几列网格
    int m_gridRowSpan = 1;                                            // 该网格子项在垂直方向要拉伸合并几行网格
    Dock m_dock = Dock::Left;                                         // 该子项挂载停靠组件时的方位（左、上、右、下、填充）

    // 主题样式管理 Token ID 列表
    ThemeTokenId m_backgroundToken = ThemeTokenId::Unset;             // 从资源字典映射该控件背景色的 Token 标签
    ThemeTokenId m_hoverBackgroundToken = ThemeTokenId::Unset;        // 鼠标进入上方后重绘背景色的 Token 标签
    ThemeTokenId m_pressedBackgroundToken = ThemeTokenId::Unset;      // 鼠标按下时背景色过渡的主题 Token 标签
    ThemeTokenId m_disabledBackgroundToken = ThemeTokenId::Unset;      // 被禁用以后呈现灰色底盘的主题 Token 标签
    ThemeTokenId m_borderToken = ThemeTokenId::Unset;                 // 静态边框四周所用的主题 Token 标签
    ThemeTokenId m_focusedBorderToken = ThemeTokenId::Unset;          // 获取键盘焦点后，外发光或高亮边框的主题 Token 标签
    ThemeTokenId m_colorToken = ThemeTokenId::Unset;                  // 主前景文字的色彩主题 Token 标签
    ThemeTokenId m_secondaryColorToken = ThemeTokenId::Unset;          // 副标题、修饰性文本的主题 Token 标签
    ThemeTokenId m_placeholderColorToken = ThemeTokenId::Unset;        // 占位文字的淡灰色主题 Token 标签
    ThemeTokenId m_selectedBackgroundToken = ThemeTokenId::Unset;      // 选中项被填充高亮背景的主题 Token 标签
    ThemeTokenId m_headerBackgroundToken = ThemeTokenId::Unset;        // 面板头部横条装饰底色的主题 Token 标签
    ThemeTokenId m_paneBackgroundToken = ThemeTokenId::Unset;          // 导航侧面抽屉大区域底盘主题 Token 标签
    ThemeTokenId m_indicatorColorToken = ThemeTokenId::Unset;          // 页签划线滑块、滚动滑块的高亮指示主题 Token 标签
    ThemeTokenId m_dropdownBackgroundToken = ThemeTokenId::Unset;      // ComboBox 下拉弹窗大底色板主题 Token 标签
    ThemeTokenId m_selectedItemBackgroundToken = ThemeTokenId::Unset;  // 弹窗里被选定特定行底色的主题 Token 标签
    ThemeTokenId m_fillColorToken = ThemeTokenId::Unset;               // 控制内部复杂几何自绘时实心填充色 Token 标签
    ThemeTokenId m_trackColorToken = ThemeTokenId::Unset;              // 滑动槽静止凹槽色彩主题 Token 标签
    ThemeTokenId m_activeTrackColorToken = ThemeTokenId::Unset;        // 滑动条高亮部分进度色彩主题 Token 标签
    ThemeTokenId m_thumbColorToken = ThemeTokenId::Unset;              // 滑动手柄小纽扣的背景主题 Token 标签
    ThemeTokenId m_onColorToken = ThemeTokenId::Unset;                 // 物理开关处于 On 激活时滑槽的背景主题 Token 标签
    ThemeTokenId m_offColorToken = ThemeTokenId::Unset;                // 物理开关处于 Off 关闭时滑槽的背景主题 Token 标签
    ThemeTokenId m_knobColorToken = ThemeTokenId::Unset;               // 开关物理圆触点的背景主题 Token 标签
    ThemeTokenId m_checkedBackgroundToken = ThemeTokenId::Unset;        // CheckBox 复选框在被勾选时的背景 Token 标签
    ThemeTokenId m_accentColorToken = ThemeTokenId::Unset;             // 系统主题强调色（默认亮蓝色）Token 标签
    ThemeTokenId m_activeColorToken = ThemeTokenId::Unset;             // 处于活跃点击交互状态下控件前景 Token 标签
    ThemeTokenId m_underlineColorToken = ThemeTokenId::Unset;          // 超链接等静态下划线笔刷 Token 标签
    ThemeTokenId m_activeUnderlineColorToken = ThemeTokenId::Unset;    // 超链接等 Hover 时高亮下划线笔刷 Token 标签
    ThemeTokenId m_activeTabBackgroundToken = ThemeTokenId::Unset;     // 选中 Tab 选项夹卡片顶色 Token 标签
    ThemeTokenId m_inactiveTabBackgroundToken = ThemeTokenId::Unset;   // 闲置未选 Tab 选项夹卡片顶色 Token 标签
    ThemeTokenId m_gridLineBrushToken = ThemeTokenId::Unset;           // 分隔表格或大板块间隙界线的颜色画笔 Token 标签
    ThemeTokenId m_titleColorToken = ThemeTokenId::Unset;              // MessageBox 粗体大标题色彩 Token 标签
    ThemeTokenId m_messageColorToken = ThemeTokenId::Unset;            // MessageBox 常规正文消息色彩 Token 标签
    ThemeTokenId m_caretColorToken = ThemeTokenId::Unset;              // 文本输入框中闪烁的竖线光标色彩 Token 标签

    // 硬编码颜色值 (若存在，则直接覆盖上述 Token 解析出的色彩)
    D2D1_COLOR_F m_backgroundColor{};                                 // 手动赋值的背景 Direct2D RGBA 颜色结构体
    D2D1_COLOR_F m_borderBrushColor{};                                // 手动赋值的边框笔刷 Direct2D RGBA 颜色结构体
    D2D1_COLOR_F m_hoverBackgroundColor{};                             // 手动赋值的悬浮状态 Direct2D RGBA 背景颜色结构体
    D2D1_COLOR_F m_pressedBackgroundColor{};                           // 手动赋值的按压状态 Direct2D RGBA 背景颜色结构体
    D2D1_COLOR_F m_colorValue{ 1, 1, 1, 1 };                          // 手动赋值的前景/文字 Direct2D RGBA 颜色结构体
    bool m_hasBackgroundColor = false;                                // 标记当前是否生效了硬编码背景色彩重写
    bool m_hasBorderBrushColor = false;                               // 标记当前是否生效了硬编码边框色彩重写
    bool m_hasHoverBackgroundColor = false;                           // 标记当前是否生效了硬编码悬浮色彩重写
    bool m_hasPressedBackgroundColor = false;                         // 标记当前是否生效了硬编码按压色彩重写
    bool m_hasColorValue = false;                                     // 标记当前是否生效了硬编码字元前景色彩重写

    // 控件层级关联与包络参数
    UIElement* m_parent = nullptr;                                    // 弱引用父类指针。根节点的父类通常为 nullptr
    UIElement* m_animationHost = nullptr;                             // 非拥有性Popup/解离挂载节点的时钟宿主原始指针
    std::vector<std::shared_ptr<UIElement>> m_children;               // 共享所有权的子节点强指针向量，控制渲染层级及布局树
    Rect m_bounds{};                                                  // 排版计算决定的包络盒矩形（X, Y, W, H 为局部坐标）
    Size m_desiredSize{};                                             // 测量测算时自己期望的最佳大小，供父控件 Arrange 参考使用
    
    // 鼠标/按键物理状态
    bool m_isHovered = false;                                         // 缓存当前鼠标指针是否落在此控件边界盒内
    bool m_isPressed = false;                                         // 缓存鼠标左键是否在此控件上按压未起
    bool m_isFocused = false;                                         // 缓存此控件当前是否得到了窗口按键焦点
    FocusState m_focusState = FocusState::Unfocused;                  // 说明该焦点是通过鼠标(Pointer)还是键盘(Keyboard)切换而来
    KeyboardNavigationMode m_keyboardNavigationMode = KeyboardNavigationMode::Continue; // 控制 Tab 在当前节点子树中的导航流动规则
    std::shared_ptr<Command> m_command;                               // 绑定的通用动作 Command，供 OnClick 触发执行

    bool m_animationTicksRegistered = false;                          // 指示自身当前是否已经挂号进入了全局的时钟帧 Tick 回调链
    bool m_presentsOnOwnerWindow = true;                              // 控制当前渲染节点是否要在原生的 Win32 宿主窗口显示绘制
    bool m_overlayComposed = false;                                   // 说明此节点当前是否已被解耦挂在 Overlay 悬浮视图层中
    bool m_subtreeNeedsOverlayHit = false;                            // 子树中是否存在有悬浮命中需要的 Popup 节点
    void MarkSubtreeNeedsOverlayHitTest();                            // 往上溯源标记子树中有悬浮穿透命中需求，避免 move 遍历整棵树

    // ToolTip 参数与定时管理
    Point m_lastMousePos{ 0.0f, 0.0f };                               // 上次触发MouseMove时记录的鼠标局点坐标
    Point m_tooltipAnchorPos{ 0.0f, 0.0f };                           // 悬浮气泡框在视口定位时锁定的锚定参考点
    std::chrono::steady_clock::time_point m_lastMouseMoveTime{};      // 记录鼠标最后发生移动的时间戳，用于悬停几秒后唤醒气泡
    std::chrono::steady_clock::time_point m_tooltipShownAt{};         // 悬浮气泡框成功呈现在屏幕上的开始时间戳
    std::chrono::steady_clock::time_point m_tooltipHideAt{};          // 悬浮气泡框计划的自动消退结束时间戳
    Rect m_tooltipPaintRect{};                                        // 该 ToolTip 的自绘制版几何外边界
    float m_toolTipMaxWidth = -1.0f;                                  // 当前 ToolTip 的特定换行行宽
    int m_toolTipAutoHideMs = -1;                                     // 当前 ToolTip 独占的自动淡出毫秒数
    bool m_tooltipVisible = false;                                    // 工具气泡框当前物理上是否是完全可见状态
    bool m_tooltipHideArmed = false;                                  // 自动关闭计时器是否已经被安装好并正处于静默计时中
    
    RenderNode m_renderNode;                                          // 深度绑定当前控件的渲染指令及合成效果对象 (RenderNode)
    std::shared_ptr<ContextMenu> m_contextMenu;                       // 注册挂载的鼠标右键上下文弹出菜单实例

    // 常规 UI 事件对象
    Event<UIElement*, Point> m_onMouseDownEvent;                      // 鼠标按下事件注册中心

    // 全局静态属性状态
    static bool s_animationsEnabled;                                  // 静态总闸，允许一次性全局冻结整个 UI 系统的全部过渡动画
    static float s_animationDeltaSeconds;                              // 全局动画Tick帧率步进间隔比例（默认 0.016s 对应 60FPS）
    static uint64_t s_renderDirtySerial;                              // 重绘脏标志计数值，每一次失效重绘该大号全局自增，防过度 Present
    static int s_toolTipShowDelayMs;                                  // 静态全局默认鼠标静止悬停多少 ms 后弹出 ToolTip（默认 500ms）
    static int s_toolTipHideDelayMs;                                  // 静态全局默认鼠标离去几毫秒后再淡出 ToolTip
    static int s_toolTipAutoHideMs;                                   // 静态全局默认 ToolTip 展开多久后强制自行消退（默认 5000ms）
    static float s_toolTipMaxWidth;                                   // 静态全局默认 ToolTip 在没有定制行宽时的自适应折行限制宽度

    // 内部 ToolTip 生命周期函数
    void ShowToolTipNow();                                            // 立即在当前悬停坐标处初始化并呈气泡浮窗
    void HideToolTipNow();                                            // 立即隐藏并回收气泡浮窗，设其 visible 为 false
    void ArmToolTipHide();                                            // 启动自动淡出定时计时
    void DirtyToolTipRect();                                          // 使 ToolTip 覆盖区域失效以申请重绘擦除或刷新
    bool ToolTipTick(std::chrono::steady_clock::time_point now);      // 工具气泡计时轮询时钟步进，处理延迟唤醒及自动消退逻辑
};


template<typename T, PropertyId PropId>
BindableProperty<T>* UIElement::GetOrCreatePropertyBinding() {
    if (!m_propertyBindings) {
        m_propertyBindings = std::make_unique<std::unordered_map<PropertyId, std::unique_ptr<PropertyBindingSlotBase>>>();
    }
    auto existing = m_propertyBindings->find(PropId);
    if (existing != m_propertyBindings->end()) {
        auto* slot = dynamic_cast<PropertyBindingSlot<T>*>(existing->second.get());
        return slot ? &slot->value : nullptr;
    }

    auto slot = std::make_unique<PropertyBindingSlot<T>>(
        *this,
        PropId,
        [this] { return PropertyValueTraits<T>::FromValue(GetProperty(PropId)); },
        [this](const T& value) { SetProperty(PropId, PropertyValueTraits<T>::ToValue(value)); });
    auto* result = &slot->value;
    m_propertyBindings->emplace(PropId, std::move(slot));
    return result;
}

template<typename T, PropertyId PropId>
BindableProperty<T>* UIElement::FindPropertyBinding() const {
    if (!m_propertyBindings) return nullptr;
    const auto existing = m_propertyBindings->find(PropId);
    if (existing == m_propertyBindings->end()) return nullptr;
    auto* slot = dynamic_cast<PropertyBindingSlot<T>*>(existing->second.get());
    return slot ? &slot->value : nullptr;
}

template<typename T, PropertyId Id>
BindableProperty<T>* PropertyRef<T, Id>::operator->() {
    return m_owner ? m_owner->template GetOrCreatePropertyBinding<T, Id>() : nullptr;
}

template<typename T, PropertyId Id>
const BindableProperty<T>* PropertyRef<T, Id>::operator->() const {
    return m_owner ? m_owner->template GetOrCreatePropertyBinding<T, Id>() : nullptr;
}

template<typename T, PropertyId Id>
void PropertyRef<T, Id>::Bind(const std::shared_ptr<Observable<T>>& value, BindingMode mode) {
    if (auto* binding = operator->()) binding->Bind(value, mode);
}

template<typename T, PropertyId Id>
void PropertyRef<T, Id>::Bind(const CUI::State<T>& value, BindingMode mode) {
    if (auto* binding = operator->()) binding->Bind(value, mode);
}

template<typename T, PropertyId Id>
template<typename TSource>
void PropertyRef<T, Id>::Bind(const std::shared_ptr<Observable<TSource>>& source,
                              const std::shared_ptr<IValueConverter<TSource, T>>& converter,
                              BindingMode mode) {
    if (auto* binding = operator->()) binding->Bind(source, converter, mode);
}

template<typename T, PropertyId Id>
template<typename TSource>
void PropertyRef<T, Id>::Bind(const CUI::State<TSource>& source,
                              const std::shared_ptr<IValueConverter<TSource, T>>& converter,
                              BindingMode mode) {
    if (auto* binding = operator->()) binding->Bind(source, converter, mode);
}

template<typename T, PropertyId Id>
void PropertyRef<T, Id>::Unbind() {
    if (m_owner) {
        if (auto* binding = m_owner->template FindPropertyBinding<T, Id>()) binding->Unbind();
    }
}

template<typename T, PropertyId Id>
bool PropertyRef<T, Id>::Set(const T& value) {
    if (auto* binding = operator->()) return binding->Set(value);
    return false;
}

template<typename T, PropertyId Id>
T PropertyRef<T, Id>::Get() const {
    if (m_owner) return PropertyValueTraits<T>::FromValue(m_owner->GetProperty(Id));
    return {};
}

template<typename T, PropertyId Id>
bool PropertyRef<T, Id>::IsUpdating() const {
    return m_owner && m_owner->template FindPropertyBinding<T, Id>()
        && m_owner->template FindPropertyBinding<T, Id>()->IsUpdating();
}

template<typename T, PropertyId Id>
bool PropertyRef<T, Id>::IsBound() const {
    return m_owner && m_owner->template FindPropertyBinding<T, Id>()
        && m_owner->template FindPropertyBinding<T, Id>()->IsBound();
}

} // namespace CUI
