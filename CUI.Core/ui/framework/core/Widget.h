#pragma once
/**
 * @file Widget.h
 * @brief 控件句柄层（Pimpl）——暴露给用户书写的唯一入口。
 *
 * 设计原则：
 *  1. 真实控件依旧是 CUI::Button / CUI::TextBox ...，继承链保持
 *     Object -> UIElement -> Control -> 具体控件（WPF 式），一律在堆上出生；
 *  2. 句柄是栈上的轻薄值对象，内部只持一个 Unique<Impl>，【没有引用计数】；
 *  3. 句柄【不转发任何接口】：通用链式方法由 WidgetBase 统一提供一次，
 *     其余全部 API 通过 operator-> 直接透传到真实控件，
 *     因此每个具体句柄只需一行声明，不存在层层包裹的转发代码。
 */
#include <memory>
#include <string>
#include <functional>
#include <type_traits>
#include "../controls/UIElement.h"

namespace CUI {

/// UI 树独占所有权的统一写法。
/// @note 句柄层内部不使用 shared_ptr；但内核 UIElement 的 m_children /
///       AddChild 目前仍是 shared_ptr，需在后续步骤中改造。
template<typename T>
using Unique = std::unique_ptr<T>;

namespace Widgets {

/**
 * @brief 句柄 CRTP 基类：提供通用链式方法 + 一切其余 API 的透传。
 * @tparam Derived 句柄自身（用于链式返回正确类型）
 * @tparam Impl    真实控件类型（继承 UIElement）
 */
template<typename Derived, typename Impl, typename Ptr = Unique<Impl>>
class WidgetBase {
public:
    using ImplType = Impl;
    using PtrType = Ptr;
    /// 拥有模式（Ptr = Unique<Impl>）：句柄独占控件；引用模式（Ptr = shared_ptr）：只观察不拥有
    static constexpr bool kOwning = std::is_same_v<Ptr, Unique<Impl>>;

    /// 无参构造：真实控件在堆上创建（构造即托管，无需用户 new）
    WidgetBase() requires kOwning : impl_(std::make_unique<Impl>()) {}

    /// 接管一个已存在的堆上控件
    explicit WidgetBase(Ptr impl) : impl_(std::move(impl)) {}

    /// 带参构造：参数原样转发给真实控件的构造函数，等价于旧 Fluent::X(args)。
    /// 仅提供构造期必填参数（如 Button("确定")），其余一律用链式 setter 表达。
    template<class A0, class... A>
        requires (kOwning && !std::is_base_of_v<WidgetBase, std::decay_t<A0>>)
    explicit WidgetBase(A0&& a0, A&&... rest)
        : impl_(std::make_unique<Impl>(std::forward<A0>(a0), std::forward<A>(rest)...)) {}

    /// 引用模式允许拷贝（共享同一控件）；拥有模式依旧独占、不可拷贝
    WidgetBase(const WidgetBase&) requires (!kOwning) = default;
    WidgetBase& operator=(const WidgetBase&) requires (!kOwning) = default;
    WidgetBase(WidgetBase&&) = default;
    WidgetBase& operator=(WidgetBase&&) = default;

    /// 链式结束：把堆上控件的所有权交出去，句柄随即置空
    Unique<Impl> Build() requires kOwning { return std::move(impl_); }

    /// 链式结束并以共享指针交出所有权：等价于 Build() 后转 shared_ptr。
    /// 便于嵌进既有的 shared_ptr / Element 体系（容器、回调捕获）而无需改写调用点。
    std::shared_ptr<Impl> Shared() requires kOwning { return std::shared_ptr<Impl>(impl_.release()); }

    /// 隐式交出所有权，可省掉 .Build()：panel->AddChild(Button().Text("x"))
    operator Unique<Impl>() requires kOwning { return std::move(impl_); }

    /// 其余全部接口一律透传——句柄不需要、也绝不去重复实现它们
    Impl* operator->() const { return impl_.get(); }

    Impl& operator*() const { return *impl_; }
    Impl* Get() const { return impl_.get(); }

    // ---------------- 通用布局链式方法 ----------------
    Derived& Width(float v) const          { impl_->SetWidth(v); return self(); }
    Derived& Height(float v) const         { impl_->SetHeight(v); return self(); }
    Derived& MinWidth(float v) const       { impl_->SetMinWidth(v); return self(); }
    Derived& MinHeight(float v) const      { impl_->SetMinHeight(v); return self(); }
    Derived& MaxWidth(float v) const       { impl_->SetMaxWidth(v); return self(); }
    Derived& MaxHeight(float v) const      { impl_->SetMaxHeight(v); return self(); }
    Derived& Margin(float v) const         { impl_->SetMargin(Thickness(v)); return self(); }
    Derived& Margin(const Thickness& t) const { impl_->SetMargin(t); return self(); }
    Derived& Padding(float v) const        { impl_->SetPadding(Thickness(v)); return self(); }
    Derived& Padding(const Thickness& t) const { impl_->SetPadding(t); return self(); }
    /// 四向分别指定（左、上、右、下）
    Derived& Margin(float l, float t, float r, float b) const { impl_->SetMargin(Thickness(l, t, r, b)); return self(); }
    Derived& Padding(float l, float t, float r, float b) const { impl_->SetPadding(Thickness(l, t, r, b)); return self(); }
    /// 批量添加子元素，等价于依次 AddChild
    template<typename... Args>
    Derived& Children(Args&&... args) {
        (impl_->AddChild(std::forward<Args>(args)), ...);
        return self();
    }

    // ---------------- 通用外观链式方法 ----------------
    Derived& Text(const std::string& v) const { impl_->SetText(v); return self(); }
    Derived& FontSize(float v) const          { impl_->SetFontSize(v); return self(); }
    Derived& ToolTip(const std::string& v) const { impl_->SetToolTip(v); return self(); }
    Derived& Opacity(float v) const           { impl_->SetOpacity(v); return self(); }
    Derived& Enabled(bool v) const            { impl_->SetIsEnabled(v); return self(); }
    Derived& Align(Alignment a) const         { impl_->SetAlign(a); return self(); }
    Derived& AlignHorizontal(Alignment a) const { impl_->SetAlignHorizontal(a); return self(); }
    Derived& AlignVertical(Alignment a) const   { impl_->SetAlignVertical(a); return self(); }
    /// 朝向（Stack / Splitter 等按此决定横排还是竖排）
    Derived& Orientation(::CUI::Orientation o) const { impl_->SetOrientation(o); return self(); }
    // ---------------- 由 UIElement 补齐的通用链式方法 ----------------
    Derived& Visibility(Visibility v) const { impl_->SetVisibility(v); return self(); }
    Derived& IsEnabled(bool enabled) const { impl_->SetIsEnabled(enabled); return self(); }
    Derived& CornerRadius(float v) const { impl_->SetCornerRadius(v); return self(); }
    Derived& BorderThickness(float v) const { impl_->SetBorderThickness(v); return self(); }
    Derived& FlexGrow(float v) const { impl_->SetFlexGrow(v); return self(); }
    Derived& Gap(float v) const { impl_->SetGap(v); return self(); }
    Derived& ItemWidth(float v) const { impl_->SetItemWidth(v); return self(); }
    Derived& ItemHeight(float v) const { impl_->SetItemHeight(v); return self(); }
    Derived& LastChildFill(bool v) const { impl_->SetLastChildFill(v); return self(); }
    Derived& JustifyLines(bool v) const { impl_->SetJustifyLines(v); return self(); }
    Derived& FillLastLine(bool v) const { impl_->SetFillLastLine(v); return self(); }
    template<class... A> Derived& Rows(A&&... a) const { if constexpr (requires { impl_->SetRows(std::forward<A>(a)...); }) impl_->SetRows(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& Columns(A&&... a) const { if constexpr (requires { impl_->SetColumns(std::forward<A>(a)...); }) impl_->SetColumns(std::forward<A>(a)...); return self(); }
    Derived& ClipToBounds(bool v) const { impl_->SetClipToBounds(v); return self(); }
    Derived& CanvasLeft(float v) const { impl_->SetCanvasLeft(v); return self(); }
    Derived& CanvasTop(float v) const { impl_->SetCanvasTop(v); return self(); }
    Derived& CanvasRight(float v) const { impl_->SetCanvasRight(v); return self(); }
    Derived& CanvasBottom(float v) const { impl_->SetCanvasBottom(v); return self(); }
    Derived& ZIndex(int v) const { impl_->SetZIndex(v); return self(); }
    Derived& GridColumn(int v) const { impl_->SetGridColumn(v); return self(); }
    Derived& GridRow(int v) const { impl_->SetGridRow(v); return self(); }
    Derived& GridColumnSpan(int v) const { impl_->SetGridColumnSpan(v); return self(); }
    Derived& GridRowSpan(int v) const { impl_->SetGridRowSpan(v); return self(); }
    Derived& Dock(Dock d) const { impl_->SetDock(d); return self(); }
    Derived& ForegroundToken(ThemeTokenId id) const { if constexpr (requires { impl_->SetForegroundToken(id); }) impl_->SetForegroundToken(id); return self(); }

    // ---- 由 scripts/gen_missing_setters.py 从旧 DSL 自动补齐 ----
    Derived& Accent(const std::string& value) const { if constexpr (requires { impl_->SetAccent(value); }) impl_->SetAccent(value); return self(); }
    Derived& AcceptsReturn(bool value) const { if constexpr (requires { impl_->SetAcceptsReturn(value); }) impl_->SetAcceptsReturn(value); return self(); }
    Derived& AcceptsTab(bool value) const { if constexpr (requires { impl_->SetAcceptsTab(value); }) impl_->SetAcceptsTab(value); return self(); }
    Derived& ActionText(const std::string& value) const { if constexpr (requires { impl_->SetActionText(value); }) impl_->SetActionText(value); return self(); }
    Derived& AddItem(const std::string& value) const { if constexpr (requires { impl_->AddItem(value); }) impl_->AddItem(value); return self(); }
    Derived& AllowDrag(bool value) const { if constexpr (requires { impl_->SetAllowDrag(value); }) impl_->SetAllowDrag(value); return self(); }
    Derived& AllowDrop(bool value) const { if constexpr (requires { impl_->SetAllowDrop(value); }) impl_->SetAllowDrop(value); return self(); }
    Derived& AlwaysShowHeader(bool value) const { if constexpr (requires { impl_->SetAlwaysShowHeader(value); }) impl_->SetAlwaysShowHeader(value); return self(); }
    Derived& AutoClose(bool value) const { if constexpr (requires { impl_->SetAutoClose(value); }) impl_->SetAutoClose(value); return self(); }
    Derived& BadgeColor(D2D1_COLOR_F value) const { if constexpr (requires { impl_->SetBadgeColor(value); }) impl_->SetBadgeColor(value); return self(); }
    Derived& BadgeText(const std::string& value) const { if constexpr (requires { impl_->SetBadgeText(value); }) impl_->SetBadgeText(value); return self(); }
    Derived& Border(D2D1_COLOR_F color, float thickness = 1.0f) const { if constexpr (requires { impl_->SetBorderBrush(color); }) impl_->SetBorderBrush(color); return self(); }
    Derived& BytesPerRow(int value) const { if constexpr (requires { impl_->SetBytesPerRow(value); }) impl_->SetBytesPerRow(value); return self(); }
    Derived& CanSave(bool value) const { if constexpr (requires { impl_->SetCanSave(value); }) impl_->SetCanSave(value); return self(); }
    Derived& CaretIndex(int value) const { if constexpr (requires { impl_->SetCaretIndex(value); }) impl_->SetCaretIndex(value); return self(); }
    Derived& Checked(bool value = true) const { if constexpr (requires { impl_->SetChecked(value); }) impl_->SetChecked(value); return self(); }
    Derived& CloseButtonText(const std::string& value) const { if constexpr (requires { impl_->SetCloseButtonText(value); }) impl_->SetCloseButtonText(value); return self(); }
    Derived& Closeable(bool value) const { if constexpr (requires { impl_->SetCloseable(value); }) impl_->SetCloseable(value); return self(); }
    Derived& ColumnDefinitions(const std::string& defs) const { if constexpr (requires { impl_->SetColumnDefinitions(defs); }) impl_->SetColumnDefinitions(defs); return self(); }
    Derived& ColumnHeader(int index, const std::string& header, float width = 120.0f) const { if constexpr (requires { impl_->AddColumn(header, width); }) impl_->AddColumn(header, width); return self(); }
    Derived& ColumnVisible(int index, bool value) const { if constexpr (requires { impl_->SetColumnVisible(index, value); }) impl_->SetColumnVisible(index, value); return self(); }
    Derived& Compact(bool value) const { if constexpr (requires { impl_->SetCompact(value); }) impl_->SetCompact(value); return self(); }
    Derived& CompactModeThresholdWidth(float value) const { if constexpr (requires { impl_->SetCompactModeThresholdWidth(value); }) impl_->SetCompactModeThresholdWidth(value); return self(); }
    Derived& CompactPaneLength(float value) const { if constexpr (requires { impl_->SetCompactPaneLength(value); }) impl_->SetCompactPaneLength(value); return self(); }
    Derived& CurrentPage(int value) const { if constexpr (requires { impl_->SetCurrentPage(value); }) impl_->SetCurrentPage(value); return self(); }
    Derived& CurrentPage(const std::string& value) const { if constexpr (requires { impl_->SetCurrentPage(value); }) impl_->SetCurrentPage(value); return self(); }
    Derived& Damping(float value) const { if constexpr (requires { impl_->SetDamping(value); }) impl_->SetDamping(value); return self(); }
    Derived& Data(const std::string& value) const { if constexpr (requires { impl_->SetData(value); }) impl_->SetData(value); return self(); }
    Derived& Date(int year, int month, int day) const { if constexpr (requires { impl_->SetDate(year, month, day); }) impl_->SetDate(year, month, day); return self(); }
    Derived& DialogTitle(const std::string& value) const { if constexpr (requires { impl_->SetDialogTitle(value); }) impl_->SetDialogTitle(value); return self(); }
    Derived& DurationMs(int value) const { if constexpr (requires { impl_->SetDurationMs(value); }) impl_->SetDurationMs(value); return self(); }
    Derived& Expanded(bool value) const { if constexpr (requires { impl_->SetExpanded(value); }) impl_->SetExpanded(value); return self(); }
    Derived& ExpandedModeThresholdWidth(float value) const { if constexpr (requires { impl_->SetExpandedModeThresholdWidth(value); }) impl_->SetExpandedModeThresholdWidth(value); return self(); }
    Derived& Fill(D2D1_COLOR_F value) const { if constexpr (requires { impl_->SetFill(value); }) impl_->SetFill(value); return self(); }
    Derived& Filter(const std::string& name, const std::string& spec) const { if constexpr (requires { impl_->SetFilter(name, spec); }) impl_->SetFilter(name, spec); return self(); }
    Derived& FlowParticles(bool enabled = true) const { if constexpr (requires { impl_->SetFlowParticlesEnabled(enabled); }) impl_->SetFlowParticlesEnabled(enabled); return self(); }
    Derived& FlowParticlesEnabled(bool value) const { if constexpr (requires { impl_->SetFlowParticlesEnabled(value); }) impl_->SetFlowParticlesEnabled(value); return self(); }
    Derived& Gesture(const std::string& value) const { if constexpr (requires { impl_->SetGesture(value); }) impl_->SetGesture(value); return self(); }
    Derived& GroupName(const std::string& value) const { if constexpr (requires { impl_->SetGroupName(value); }) impl_->SetGroupName(value); return self(); }
    Derived& Header(const std::string& value) const { if constexpr (requires { impl_->SetHeader(value); }) impl_->SetHeader(value); return self(); }
    Derived& IconText(const std::string& icon) const { if constexpr (requires { impl_->SetIconText(icon); }) impl_->SetIconText(icon); return self(); }
    Derived& Id(const std::string& id) const { if constexpr (requires { impl_->SetId(id); }) impl_->SetId(id); return self(); }
    Derived& IndentWidth(float value) const { if constexpr (requires { impl_->SetIndentWidth(value); }) impl_->SetIndentWidth(value); return self(); }
    Derived& InputEnabled(bool value, bool multiline = false) const { if constexpr (requires { impl_->SetInputEnabled(value, multiline); }) impl_->SetInputEnabled(value, multiline); return self(); }
    Derived& InputText(const std::string& value) const { if constexpr (requires { impl_->SetInputText(value); }) impl_->SetInputText(value); return self(); }
    Derived& IsBackEnabled(bool value) const { if constexpr (requires { impl_->SetIsBackEnabled(value); }) impl_->SetIsBackEnabled(value); return self(); }
    Derived& IsChecked(bool value) const { if constexpr (requires { impl_->SetIsChecked(value); }) impl_->SetIsChecked(value); return self(); }
    Derived& IsChildSelected(bool value) const { if constexpr (requires { impl_->SetIsChildSelected(value); }) impl_->SetIsChildSelected(value); return self(); }
    Derived& IsClearEnabled(bool value) const { if constexpr (requires { impl_->SetIsClearEnabled(value); }) impl_->SetIsClearEnabled(value); return self(); }
    Derived& IsClosable(bool value) const { if constexpr (requires { impl_->SetIsClosable(value); }) impl_->SetIsClosable(value); return self(); }
    Derived& IsCloseButtonEnabled(bool value) const { if constexpr (requires { impl_->SetIsCloseButtonEnabled(value); }) impl_->SetIsCloseButtonEnabled(value); return self(); }
    Derived& IsCloseButtonVisible(bool value) const { if constexpr (requires { impl_->SetIsCloseButtonVisible(value); }) impl_->SetIsCloseButtonVisible(value); return self(); }
    Derived& IsCloseVisible(bool value) const { if constexpr (requires { impl_->SetIsCloseVisible(value); }) impl_->SetIsCloseVisible(value); return self(); }
    Derived& IsExpanded(bool exp) const { if constexpr (requires { impl_->SetIsExpanded(exp); }) impl_->SetIsExpanded(exp); return self(); }
    Derived& IsExpandedSilent(bool value) const { if constexpr (requires { impl_->SetIsExpandedSilent(value); }) impl_->SetIsExpandedSilent(value); return self(); }
    Derived& IsIndeterminate(bool value) const { if constexpr (requires { impl_->SetIsIndeterminate(value); }) impl_->SetIsIndeterminate(value); return self(); }
    Derived& IsMaximizeButtonEnabled(bool value) const { if constexpr (requires { impl_->SetIsMaximizeButtonEnabled(value); }) impl_->SetIsMaximizeButtonEnabled(value); return self(); }
    Derived& IsMaximizeButtonVisible(bool value) const { if constexpr (requires { impl_->SetIsMaximizeButtonVisible(value); }) impl_->SetIsMaximizeButtonVisible(value); return self(); }
    Derived& IsMinimizeButtonEnabled(bool value) const { if constexpr (requires { impl_->SetIsMinimizeButtonEnabled(value); }) impl_->SetIsMinimizeButtonEnabled(value); return self(); }
    Derived& IsMinimizeButtonVisible(bool value) const { if constexpr (requires { impl_->SetIsMinimizeButtonVisible(value); }) impl_->SetIsMinimizeButtonVisible(value); return self(); }
    Derived& IsModal(bool value) const { if constexpr (requires { impl_->SetIsModal(value); }) impl_->SetIsModal(value); return self(); }
    Derived& IsOn(bool value) const { if constexpr (requires { impl_->SetIsOn(value); }) impl_->SetIsOn(value); return self(); }
    Derived& IsOpen(bool value) const { if constexpr (requires { impl_->SetIsOpen(value); }) impl_->SetIsOpen(value); return self(); }
    Derived& IsPaneOpen(bool value) const { if constexpr (requires { impl_->SetIsPaneOpen(value); }) impl_->SetIsPaneOpen(value); return self(); }
    Derived& IsPasswordMode(bool value) const { if constexpr (requires { impl_->SetIsPasswordMode(value); }) impl_->SetIsPasswordMode(value); return self(); }
    Derived& IsReadOnly(bool ro) const { if constexpr (requires { impl_->SetIsReadOnly(ro); }) impl_->SetIsReadOnly(ro); return self(); }
    Derived& IsSelected(bool value) const { if constexpr (requires { impl_->SetIsSelected(value); }) impl_->SetIsSelected(value); return self(); }
    Derived& IsSeparator(bool value = true) const { if constexpr (requires { impl_->SetIsSeparator(value); }) impl_->SetIsSeparator(value); return self(); }
    Derived& IsSettingsVisible(bool value) const { if constexpr (requires { impl_->SetIsSettingsVisible(value); }) impl_->SetIsSettingsVisible(value); return self(); }
    Derived& IsThreeState(bool value) const { if constexpr (requires { impl_->SetIsThreeState(value); }) impl_->SetIsThreeState(value); return self(); }
    Derived& ItemIcon(int id, const std::string& value) const { if constexpr (requires { impl_->SetItemIcon(id, value); }) impl_->SetItemIcon(id, value); return self(); }
    Derived& ItemProgress(int id, float value) const { if constexpr (requires { impl_->SetItemProgress(id, value); }) impl_->SetItemProgress(id, value); return self(); }
    Derived& ItemText(int id, const std::string& value) const { if constexpr (requires { impl_->SetItemText(id, value); }) impl_->SetItemText(id, value); return self(); }
    Derived& Justified(bool enabled = true) const { if constexpr (requires { impl_->SetJustifyLines(enabled); }) impl_->SetJustifyLines(enabled); return self(); }
    Derived& Label(const std::string& value) const { if constexpr (requires { impl_->SetLabel(value); }) impl_->SetLabel(value); return self(); }
    Derived& LineSpacing(float value) const { if constexpr (requires { impl_->SetLineSpacing(value); }) impl_->SetLineSpacing(value); return self(); }
    Derived& LowPerformanceMode(bool value) const { if constexpr (requires { impl_->SetLowPerformanceMode(value); }) impl_->SetLowPerformanceMode(value); return self(); }
    Derived& Markdown(const std::string& value) const { if constexpr (requires { impl_->SetMarkdown(value); }) impl_->SetMarkdown(value); return self(); }
    Derived& MaxRating(int value) const { if constexpr (requires { impl_->SetMaxRating(value); }) impl_->SetMaxRating(value); return self(); }
    Derived& MaxTabWidth(float value) const { if constexpr (requires { impl_->SetMaxTabWidth(value); }) impl_->SetMaxTabWidth(value); return self(); }
    Derived& MaxVisibleSuggestions(int value) const { if constexpr (requires { impl_->SetMaxVisibleSuggestions(value); }) impl_->SetMaxVisibleSuggestions(value); return self(); }
    Derived& Maximum(float maxVal) const { if constexpr (requires { impl_->SetMaximum(maxVal); }) impl_->SetMaximum(maxVal); return self(); }
    Derived& Message(const std::string& m) const { if constexpr (requires { impl_->SetMessage(m); }) impl_->SetMessage(m); return self(); }
    Derived& MessageColor(const std::string& value) const { if constexpr (requires { impl_->SetMessageColor(value); }) impl_->SetMessageColor(value); return self(); }
    Derived& MinTabWidth(float value) const { if constexpr (requires { impl_->SetMinTabWidth(value); }) impl_->SetMinTabWidth(value); return self(); }
    Derived& Minimum(float minVal) const { if constexpr (requires { impl_->SetMinimum(minVal); }) impl_->SetMinimum(minVal); return self(); }
    Derived& MinimumRange(float value) const { if constexpr (requires { impl_->SetMinimumRange(value); }) impl_->SetMinimumRange(value); return self(); }
    Derived& NavigateUri(const std::string& value) const { if constexpr (requires { impl_->SetNavigateUri(value); }) impl_->SetNavigateUri(value); return self(); }
    Derived& OffsetX(float value) const { if constexpr (requires { impl_->SetOffsetX(value); }) impl_->SetOffsetX(value); return self(); }
    Derived& OffsetY(float value) const { if constexpr (requires { impl_->SetOffsetY(value); }) impl_->SetOffsetY(value); return self(); }
    Derived& OpenPaneLength(float value) const { if constexpr (requires { impl_->SetOpenPaneLength(value); }) impl_->SetOpenPaneLength(value); return self(); }
    Derived& OverlayComposed(bool value) const { if constexpr (requires { impl_->SetOverlayComposed(value); }) impl_->SetOverlayComposed(value); return self(); }
    Derived& OverlayScrollbar(bool value) const { if constexpr (requires { impl_->SetOverlayScrollbar(value); }) impl_->SetOverlayScrollbar(value); return self(); }
    Derived& PaneAutoHide(int index, bool value) const { if constexpr (requires { impl_->SetPaneAutoHide(index, value); }) impl_->SetPaneAutoHide(index, value); return self(); }
    Derived& PaneTitle(const std::string& value) const { if constexpr (requires { impl_->SetPaneTitle(value); }) impl_->SetPaneTitle(value); return self(); }
    Derived& Password(const std::string& value) const { if constexpr (requires { impl_->SetPassword(value); }) impl_->SetPassword(value); return self(); }
    Derived& Path(const std::string& value) const { if constexpr (requires { impl_->SetPath(value); }) impl_->SetPath(value); return self(); }
    Derived& PersistEnabled(bool value) const { if constexpr (requires { impl_->SetPersistEnabled(value); }) impl_->SetPersistEnabled(value); return self(); }
    Derived& PrimaryButtonText(const std::string& value) const { if constexpr (requires { impl_->SetPrimaryButtonText(value); }) impl_->SetPrimaryButtonText(value); return self(); }
    Derived& Range(float lower, float upper) const { if constexpr (requires { impl_->SetRange(lower, upper); }) impl_->SetRange(lower, upper); return self(); }
    Derived& RenderStatsOverlayVisible(bool value) const { if constexpr (requires { impl_->SetRenderStatsOverlayVisible(value); }) impl_->SetRenderStatsOverlayVisible(value); return self(); }
    Derived& RowDefinitions(const std::string& defs) const { if constexpr (requires { impl_->SetRowDefinitions(defs); }) impl_->SetRowDefinitions(defs); return self(); }
    Derived& RowHeight(float value) const { if constexpr (requires { impl_->SetRowHeight(value); }) impl_->SetRowHeight(value); return self(); }
    Derived& RowSelected(int index, bool value) const { if constexpr (requires { impl_->SetRowSelected(index, value); }) impl_->SetRowSelected(index, value); return self(); }
    Derived& Running(bool value) const { if constexpr (requires { impl_->SetRunning(value); }) impl_->SetRunning(value); return self(); }
    Derived& ScrollOffsetY(float value) const { if constexpr (requires { impl_->SetScrollOffsetY(value); }) impl_->SetScrollOffsetY(value); return self(); }
    Derived& SecondaryButtonText(const std::string& value) const { if constexpr (requires { impl_->SetSecondaryButtonText(value); }) impl_->SetSecondaryButtonText(value); return self(); }
    Derived& SelectedIndex(int value) const { if constexpr (requires { impl_->SetSelectedIndex(value); }) impl_->SetSelectedIndex(value); return self(); }
    Derived& SelectsOnInvoked(bool value) const { if constexpr (requires { impl_->SetSelectsOnInvoked(value); }) impl_->SetSelectsOnInvoked(value); return self(); }
    Derived& ShortcutText(const std::string& value) const { if constexpr (requires { impl_->SetShortcutText(value); }) impl_->SetShortcutText(value); return self(); }
    Derived& ShowCodeLineNumbers(bool value) const { if constexpr (requires { impl_->SetShowCodeLineNumbers(value); }) impl_->SetShowCodeLineNumbers(value); return self(); }
    Derived& ShowGrid(bool value) const { if constexpr (requires { impl_->SetShowGrid(value); }) impl_->SetShowGrid(value); return self(); }
    Derived& ShowGridLines(bool value) const { if constexpr (requires { impl_->SetShowGridLines(value); }) impl_->SetShowGridLines(value); return self(); }
    Derived& ShowLegend(bool value) const { if constexpr (requires { impl_->SetShowLegend(value); }) impl_->SetShowLegend(value); return self(); }
    Derived& ShowRevealButton(bool value) const { if constexpr (requires { impl_->SetShowRevealButton(value); }) impl_->SetShowRevealButton(value); return self(); }
    Derived& ShowScrollBars(bool value) const { if constexpr (requires { impl_->SetShowScrollBars(value); }) impl_->SetShowScrollBars(value); return self(); }
    Derived& ShowTooltip(bool value) const { if constexpr (requires { impl_->SetShowTooltip(value); }) impl_->SetShowTooltip(value); return self(); }
    Derived& Size(float w, float h) const { if constexpr (requires { impl_->SetWidth(w); }) impl_->SetWidth(w); return self(); }
    Derived& Source(const std::string& value) const { if constexpr (requires { impl_->SetSource(value); }) impl_->SetSource(value); return self(); }
    template<class V> Derived& Stretch(V value) const { if constexpr (requires { impl_->SetStretch(value); }) impl_->SetStretch(value); return self(); }
    template<class V> Derived& Value(V value) const { if constexpr (requires { impl_->SetValue(value); }) impl_->SetValue(value); return self(); }
    template<class V> Derived& State(V value) const { if constexpr (requires { impl_->SetState(value); }) impl_->SetState(value); return self(); }
    template<class V> Derived& Content(V value) const { if constexpr (requires { impl_->SetContent(value); }) impl_->SetContent(value); return self(); }
    template<class V> Derived& Placement(V value) const { if constexpr (requires { impl_->SetPlacement(value); }) impl_->SetPlacement(value); return self(); }
    template<class V> Derived& SelectionMode(V value) const { if constexpr (requires { impl_->SetSelectionMode(value); }) impl_->SetSelectionMode(value); return self(); }
    template<class V> Derived& Subtitle(V value) const { if constexpr (requires { impl_->SetSubtitle(value); }) impl_->SetSubtitle(value); return self(); }

    // ---- 页面层仍在用的其余属性入口（同上，统一走链式，避免作者级 ->SetX 调用） ----
    template<class... A> Derived& ActiveContextMenu(A&&... a) const { if constexpr (requires { impl_->SetActiveContextMenu(std::forward<A>(a)...); }) impl_->SetActiveContextMenu(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& AntialiasMode(A&&... a) const { if constexpr (requires { impl_->SetAntialiasMode(std::forward<A>(a)...); }) impl_->SetAntialiasMode(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& BackdropType(A&&... a) const { if constexpr (requires { impl_->SetBackdropType(std::forward<A>(a)...); }) impl_->SetBackdropType(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& Bytes(A&&... a) const { if constexpr (requires { impl_->SetBytes(std::forward<A>(a)...); }) impl_->SetBytes(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& Category(A&&... a) const { if constexpr (requires { impl_->SetCategory(std::forward<A>(a)...); }) impl_->SetCategory(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& ContentFactory(A&&... a) const { if constexpr (requires { impl_->SetContentFactory(std::forward<A>(a)...); }) impl_->SetContentFactory(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& ContextMenu(A&&... a) const { if constexpr (requires { impl_->SetContextMenu(std::forward<A>(a)...); }) impl_->SetContextMenu(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& IsBackButtonVisible(A&&... a) const { if constexpr (requires { impl_->SetIsBackButtonVisible(std::forward<A>(a)...); }) impl_->SetIsBackButtonVisible(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& LazyPopulate(A&&... a) const { if constexpr (requires { impl_->SetLazyPopulate(std::forward<A>(a)...); }) impl_->SetLazyPopulate(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& Logger(A&&... a) const { if constexpr (requires { impl_->SetLogger(std::forward<A>(a)...); }) impl_->SetLogger(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& OwnerWindow(A&&... a) const { if constexpr (requires { impl_->SetOwnerWindow(std::forward<A>(a)...); }) impl_->SetOwnerWindow(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& PaneDisplayMode(A&&... a) const { if constexpr (requires { impl_->SetPaneDisplayMode(std::forward<A>(a)...); }) impl_->SetPaneDisplayMode(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& PixelFormat(A&&... a) const { if constexpr (requires { impl_->SetPixelFormat(std::forward<A>(a)...); }) impl_->SetPixelFormat(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& SelectedId(A&&... a) const { if constexpr (requires { impl_->SetSelectedId(std::forward<A>(a)...); }) impl_->SetSelectedId(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& SettingsItem(A&&... a) const { if constexpr (requires { impl_->SetSettingsItem(std::forward<A>(a)...); }) impl_->SetSettingsItem(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& StatusHandler(A&&... a) const { if constexpr (requires { impl_->SetStatusHandler(std::forward<A>(a)...); }) impl_->SetStatusHandler(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& TextAntialiasMode(A&&... a) const { if constexpr (requires { impl_->SetTextAntialiasMode(std::forward<A>(a)...); }) impl_->SetTextAntialiasMode(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& ThemeMode(A&&... a) const { if constexpr (requires { impl_->SetThemeMode(std::forward<A>(a)...); }) impl_->SetThemeMode(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& ThemeModeWithRipple(A&&... a) const { if constexpr (requires { impl_->SetThemeModeWithRipple(std::forward<A>(a)...); }) impl_->SetThemeModeWithRipple(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& LayoutType(A&&... a) const { if constexpr (requires { impl_->SetLayoutType(std::forward<A>(a)...); }) impl_->SetLayoutType(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& SideSize(A&&... a) const { if constexpr (requires { impl_->SetSideSize(std::forward<A>(a)...); }) impl_->SetSideSize(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& PreferredPlacement(A&&... a) const { if constexpr (requires { impl_->SetPreferredPlacement(std::forward<A>(a)...); }) impl_->SetPreferredPlacement(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& Items(A&&... a) const { if constexpr (requires { impl_->SetItems(std::forward<A>(a)...); }) impl_->SetItems(std::forward<A>(a)...); return self(); }
    /// 列表项可直接写花括号：.Items({ "A", "B" })
    template<class T> Derived& Items(std::initializer_list<T> v) const {
        if constexpr (requires { impl_->SetItems(std::vector<T>(v)); }) impl_->SetItems(std::vector<T>(v));
        return self();
    }
    template<class... A> Derived& ItemExpanded(A&&... a) const { if constexpr (requires { impl_->SetItemExpanded(std::forward<A>(a)...); }) impl_->SetItemExpanded(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& ExpandDirection(A&&... a) const { if constexpr (requires { impl_->SetExpandDirection(std::forward<A>(a)...); }) impl_->SetExpandDirection(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& LabelPosition(A&&... a) const { if constexpr (requires { impl_->SetLabelPosition(std::forward<A>(a)...); }) impl_->SetLabelPosition(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& SelectedItem(A&&... a) const { if constexpr (requires { impl_->SetSelectedItem(std::forward<A>(a)...); }) impl_->SetSelectedItem(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& Severity(A&&... a) const { if constexpr (requires { impl_->SetSeverity(std::forward<A>(a)...); }) impl_->SetSeverity(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& VirtualMode(A&&... a) const { if constexpr (requires { impl_->SetVirtualMode(std::forward<A>(a)...); }) impl_->SetVirtualMode(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& SuggestionItems(A&&... a) const { if constexpr (requires { impl_->SetSuggestionItems(std::forward<A>(a)...); }) impl_->SetSuggestionItems(std::forward<A>(a)...); return self(); }
    /// 候选词可直接写花括号：.SuggestionItems({ "A", "B" })
    template<class T> Derived& SuggestionItems(std::initializer_list<T> v) const {
        if constexpr (requires { impl_->SetSuggestionItems(std::vector<T>(v)); }) impl_->SetSuggestionItems(std::vector<T>(v));
        return self();
    }
    template<class... A> Derived& SuggestionProvider(A&&... a) const { if constexpr (requires { impl_->SetSuggestionProvider(std::forward<A>(a)...); }) impl_->SetSuggestionProvider(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& AutoSuggestBox(A&&... a) const { if constexpr (requires { impl_->SetAutoSuggestBox(std::forward<A>(a)...); }) impl_->SetAutoSuggestBox(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& RowIcons(A&&... a) const { if constexpr (requires { impl_->SetRowIcons(std::forward<A>(a)...); }) impl_->SetRowIcons(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& RowTags(A&&... a) const { if constexpr (requires { impl_->SetRowTags(std::forward<A>(a)...); }) impl_->SetRowTags(std::forward<A>(a)...); return self(); }
    Derived& Spacing(float value) const { if constexpr (requires { impl_->SetSpacing(value); }) impl_->SetSpacing(value); return self(); }
    Derived& Step(float s) const { if constexpr (requires { impl_->SetStep(s); }) impl_->SetStep(s); return self(); }
    Derived& Stiffness(float value) const { if constexpr (requires { impl_->SetStiffness(value); }) impl_->SetStiffness(value); return self(); }
    Derived& Strikethrough(bool strikethrough = true) const { if constexpr (requires { impl_->SetIsStrikethrough(strikethrough); }) impl_->SetIsStrikethrough(strikethrough); return self(); }
    Derived& Stroke(D2D1_COLOR_F value) const { if constexpr (requires { impl_->SetStroke(value); }) impl_->SetStroke(value); return self(); }
    Derived& StrokeThickness(float value) const { if constexpr (requires { impl_->SetStrokeThickness(value); }) impl_->SetStrokeThickness(value); return self(); }
    Derived& Tag(const std::string& value) const { if constexpr (requires { impl_->SetTag(value); }) impl_->SetTag(value); return self(); }
    Derived& TextWrapping(bool value) const { if constexpr (requires { impl_->SetTextWrapping(value); }) impl_->SetTextWrapping(value); return self(); }
    Derived& Time(int hour, int minute) const { if constexpr (requires { impl_->SetTime(hour, minute); }) impl_->SetTime(hour, minute); return self(); }
    Derived& TintColor(D2D1_COLOR_F value) const { if constexpr (requires { impl_->SetTintColor(value); }) impl_->SetTintColor(value); return self(); }
    Derived& Title(const std::string& t) const { if constexpr (requires { impl_->SetTitle(t); }) impl_->SetTitle(t); return self(); }
    Derived& TitleColor(const std::string& value) const { if constexpr (requires { impl_->SetTitleColor(value); }) impl_->SetTitleColor(value); return self(); }
    Derived& TopMode(bool value) const { if constexpr (requires { impl_->SetTopMode(value); }) impl_->SetTopMode(value); return self(); }
    Derived& TotalPages(int value) const { if constexpr (requires { impl_->SetTotalPages(value); }) impl_->SetTotalPages(value); return self(); }
    Derived& Underline(bool underline = true) const { if constexpr (requires { impl_->SetIsUnderline(underline); }) impl_->SetIsUnderline(underline); return self(); }
    Derived& Viewport(float width, float height) const { if constexpr (requires { impl_->SetViewport(width, height); }) impl_->SetViewport(width, height); return self(); }
    Derived& VirtualCount(size_t count) const { if constexpr (requires { impl_->SetVirtualCount(count); }) impl_->SetVirtualCount(count); return self(); }
    Derived& X1(float value) const { if constexpr (requires { impl_->SetX1(value); }) impl_->SetX1(value); return self(); }
    Derived& X2(float value) const { if constexpr (requires { impl_->SetX2(value); }) impl_->SetX2(value); return self(); }
    Derived& Y1(float value) const { if constexpr (requires { impl_->SetY1(value); }) impl_->SetY1(value); return self(); }
    Derived& Y2(float value) const { if constexpr (requires { impl_->SetY2(value); }) impl_->SetY2(value); return self(); }
    Derived& BackgroundToken(ThemeTokenId id) const { impl_->SetBackgroundToken(id); return self(); }
    Derived& HoverBackgroundToken(ThemeTokenId id) const { impl_->SetHoverBackgroundToken(id); return self(); }
    Derived& PressedBackgroundToken(ThemeTokenId id) const { impl_->SetPressedBackgroundToken(id); return self(); }
    Derived& DisabledBackgroundToken(ThemeTokenId id) const { impl_->SetDisabledBackgroundToken(id); return self(); }
    Derived& BorderToken(ThemeTokenId id) const { impl_->SetBorderToken(id); return self(); }
    Derived& FocusedBorderToken(ThemeTokenId id) const { impl_->SetFocusedBorderToken(id); return self(); }
    Derived& ColorToken(ThemeTokenId id) const { impl_->SetColorToken(id); return self(); }
    Derived& SecondaryColorToken(ThemeTokenId id) const { impl_->SetSecondaryColorToken(id); return self(); }
    Derived& PlaceholderColorToken(ThemeTokenId id) const { impl_->SetPlaceholderColorToken(id); return self(); }
    Derived& SelectedBackgroundToken(ThemeTokenId id) const { impl_->SetSelectedBackgroundToken(id); return self(); }
    Derived& HeaderBackgroundToken(ThemeTokenId id) const { impl_->SetHeaderBackgroundToken(id); return self(); }
    Derived& PaneBackgroundToken(ThemeTokenId id) const { impl_->SetPaneBackgroundToken(id); return self(); }
    Derived& IndicatorColorToken(ThemeTokenId id) const { impl_->SetIndicatorColorToken(id); return self(); }
    Derived& DropdownBackgroundToken(ThemeTokenId id) const { impl_->SetDropdownBackgroundToken(id); return self(); }
    Derived& SelectedItemBackgroundToken(ThemeTokenId id) const { impl_->SetSelectedItemBackgroundToken(id); return self(); }
    Derived& FillColorToken(ThemeTokenId id) const { impl_->SetFillColorToken(id); return self(); }
    Derived& TrackColorToken(ThemeTokenId id) const { impl_->SetTrackColorToken(id); return self(); }
    Derived& ActiveTrackColorToken(ThemeTokenId id) const { impl_->SetActiveTrackColorToken(id); return self(); }
    Derived& ThumbColorToken(ThemeTokenId id) const { impl_->SetThumbColorToken(id); return self(); }
    Derived& OnColorToken(ThemeTokenId id) const { impl_->SetOnColorToken(id); return self(); }
    Derived& OffColorToken(ThemeTokenId id) const { impl_->SetOffColorToken(id); return self(); }
    Derived& KnobColorToken(ThemeTokenId id) const { impl_->SetKnobColorToken(id); return self(); }
    Derived& CheckedBackgroundToken(ThemeTokenId id) const { impl_->SetCheckedBackgroundToken(id); return self(); }
    Derived& AccentColorToken(ThemeTokenId id) const { impl_->SetAccentColorToken(id); return self(); }
    Derived& ActiveColorToken(ThemeTokenId id) const { impl_->SetActiveColorToken(id); return self(); }
    Derived& UnderlineColorToken(ThemeTokenId id) const { impl_->SetUnderlineColorToken(id); return self(); }
    Derived& ActiveUnderlineColorToken(ThemeTokenId id) const { impl_->SetActiveUnderlineColorToken(id); return self(); }
    Derived& ActiveTabBackgroundToken(ThemeTokenId id) const { impl_->SetActiveTabBackgroundToken(id); return self(); }
    Derived& InactiveTabBackgroundToken(ThemeTokenId id) const { impl_->SetInactiveTabBackgroundToken(id); return self(); }
    Derived& GridLineBrushToken(ThemeTokenId id) const { impl_->SetGridLineBrushToken(id); return self(); }
    Derived& TitleColorToken(ThemeTokenId id) const { impl_->SetTitleColorToken(id); return self(); }
    Derived& MessageColorToken(ThemeTokenId id) const { impl_->SetMessageColorToken(id); return self(); }
    Derived& CaretColorToken(ThemeTokenId id) const { impl_->SetCaretColorToken(id); return self(); }
    Derived& Background(D2D1_COLOR_F c) const { impl_->SetBackground(c); return self(); }
    Derived& HoverBackground(D2D1_COLOR_F c) const { impl_->SetHoverBackground(c); return self(); }
    Derived& PressedBackground(D2D1_COLOR_F c) const { impl_->SetPressedBackground(c); return self(); }
    Derived& BorderBrush(D2D1_COLOR_F c) const { impl_->SetBorderBrush(c); return self(); }
    Derived& Color(D2D1_COLOR_F c) const { impl_->SetColor(c); return self(); }
    Derived& Foreground(D2D1_COLOR_F color) const { if constexpr (requires { impl_->SetColor(color); }) impl_->SetColor(color); return self(); }
    Derived& Placeholder(const std::string& placeholder) const { impl_->SetPlaceholder(placeholder); return self(); }
    Derived& FontFamily(const std::string& font) const { impl_->SetFontFamily(font); return self(); }
    Derived& FontWeight(CUI::FontWeight weight) const { impl_->SetFontWeight(weight); return self(); }
    Derived& FontStyle(CUI::FontStyle style) const { impl_->SetFontStyle(style); return self(); }
    Derived& FontStretch(CUI::FontStretch stretch) const { impl_->SetFontStretch(stretch); return self(); }
    Derived& IsUnderline(bool underline) const { impl_->SetIsUnderline(underline); return self(); }
    Derived& IsStrikethrough(bool strikethrough) const { impl_->SetIsStrikethrough(strikethrough); return self(); }
    Derived& ToolTipMaxWidth(float width) const { impl_->SetToolTipMaxWidth(width); return self(); }
    Derived& ToolTipAutoHideMs(int ms) const { impl_->SetToolTipAutoHideMs(ms); return self(); }
    Derived& Icon(const std::string& icon) const { impl_->SetIcon(icon); return self(); }
    Derived& Parent(UIElement* parent) const { impl_->SetParent(parent); return self(); }
    Derived& AnimationHost(UIElement* host) const { impl_->SetAnimationHost(host); return self(); }
    Derived& Bounds(const Rect& bounds) const { impl_->SetBounds(bounds); return self(); }
    Derived& Command(std::shared_ptr<Command> command) const { impl_->SetCommand(command); return self(); }
    Derived& ComposeOpacity(float opacity) const { impl_->SetComposeOpacity(opacity); return self(); }
    Derived& ComposeOffset(float x, float y) const { impl_->SetComposeOffset(x, y); return self(); }


    /**
     * @brief 注册点击回调（链式）。
     * @note 与 Impl 上的【数据成员】CallbackProperty OnClick 同名，但不冲突：
     *       句柄用 `.OnClick(fn)` 走本成员函数；要访问控件上的 OnClick 成员本身
     *       （如 OnClick.Connect / OnClick().Invoke）请用箭头 `->OnClick`。
     */
    Derived& OnClick(std::function<void(UIElement*)> handler) {
        impl_->OnClick.Connect(std::move(handler));
        return self();
    }
    /// Click 为 OnClick 的同义别名，两种写法均可
    Derived& Click(std::function<void(UIElement*)> handler) {
        impl_->OnClick.Connect(std::move(handler));
        return self();
    }

protected:
    Ptr impl_;

protected:
    /// 链式方法在 const 句柄上同样可用（改的是控件而非句柄），故统一走 const 版本。
    Derived& self() const { return const_cast<Derived&>(static_cast<const Derived&>(*this)); }
};

/**
 * @brief 非拥有观察句柄：包装已交给 UI 树的控件，运行期继续用链式方法改属性。
 *
 * 取代页面里对裸 shared_ptr 调用作者级 `->SetXxx()` 的写法：
 * @code
 *   Widgets::Ref<Image> img = Widgets::Image().Shared(); // 交给 UI 树
 *   img.Stretch(Stretch::Fill).Width(120);               // 运行期依旧链式
 * @endcode
 */
template<typename Impl>
class Ref : public WidgetBase<Ref<Impl>, Impl, std::shared_ptr<Impl>> {
public:
    using Base = WidgetBase<Ref<Impl>, Impl, std::shared_ptr<Impl>>;

    Ref() = default;
    /// 非显式：允许 Widgets::Ref<Foo> f = Widgets::Foo().Shared();
    Ref(std::shared_ptr<Impl> p) : Base(std::move(p)) {}
    Ref& operator=(std::shared_ptr<Impl> p) { this->impl_ = std::move(p); return *this; }

    /// 取出底层共享指针，用于 AddChild 等仍以 shared_ptr 交接入口的场合
    std::shared_ptr<Impl> Ptr() const { return this->impl_; }
    /// 裸指针访问，保持与 shared_ptr 一致的手感
    Impl* get() const { return this->impl_.get(); }
    operator std::shared_ptr<Impl>() const { return this->impl_; }
    /// 直接交给以 shared_ptr<UIElement> 收口的容器（Column/Row/Children 等）
    operator std::shared_ptr<UIElement>() const { return this->impl_; }
    /// 交给以基类 shared_ptr 收口的入口（如 AddMenuItem(shared_ptr<NavigationViewItemBase>)）
    template<class U> requires std::is_convertible_v<Impl*, U*>
    operator std::shared_ptr<U>() const { return this->impl_; }
    bool Valid() const { return static_cast<bool>(this->impl_); }
    explicit operator bool() const { return static_cast<bool>(this->impl_); }
};

/// 推导指引：Widgets::Ref x = Widgets::Foo().Shared(); 无需手写控件类型名
template<typename Impl>
Ref(std::shared_ptr<Impl>) -> Ref<Impl>;

} // namespace Widgets
} // namespace CUI
