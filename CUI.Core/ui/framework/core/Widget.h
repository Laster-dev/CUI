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

    /// 引用模式的空句柄：先声明、稍后接管控件（Ref 作为类成员时的常规形态）
    WidgetBase() requires (!kOwning) = default;

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
    Derived& Width(float v) const          { impl_->ApplyWidth(v); return self(); }
    Derived& Height(float v) const         { impl_->ApplyHeight(v); return self(); }
    Derived& MinWidth(float v) const       { impl_->ApplyMinWidth(v); return self(); }
    Derived& MinHeight(float v) const      { impl_->ApplyMinHeight(v); return self(); }
    Derived& MaxWidth(float v) const       { impl_->ApplyMaxWidth(v); return self(); }
    Derived& MaxHeight(float v) const      { impl_->ApplyMaxHeight(v); return self(); }
    Derived& Margin(float v) const         { impl_->ApplyMargin(Thickness(v)); return self(); }
    Derived& Margin(const Thickness& t) const { impl_->ApplyMargin(t); return self(); }
    Derived& Padding(float v) const        { impl_->ApplyPadding(Thickness(v)); return self(); }
    Derived& Padding(const Thickness& t) const { impl_->ApplyPadding(t); return self(); }
    /// 四向分别指定（左、上、右、下）
    Derived& Margin(float l, float t, float r, float b) const { impl_->ApplyMargin(Thickness(l, t, r, b)); return self(); }
    Derived& Padding(float l, float t, float r, float b) const { impl_->ApplyPadding(Thickness(l, t, r, b)); return self(); }
    /// 批量添加子元素，等价于依次 AddChild
    template<typename... Args>
    Derived& Children(Args&&... args) {
        (impl_->AddChild(std::forward<Args>(args)), ...);
        return self();
    }

    // ---------------- 通用外观链式方法 ----------------
    Derived& Text(const std::string& v) const { impl_->ApplyText(v); return self(); }
    Derived& FontSize(float v) const          { impl_->ApplyFontSize(v); return self(); }
    Derived& ToolTip(const std::string& v) const { impl_->ApplyToolTip(v); return self(); }
    Derived& Opacity(float v) const           { impl_->ApplyOpacity(v); return self(); }
    Derived& Enabled(bool v) const            { impl_->ApplyIsEnabled(v); return self(); }
    Derived& Align(Alignment a) const         { impl_->ApplyAlign(a); return self(); }
    Derived& AlignHorizontal(Alignment a) const { impl_->ApplyAlignHorizontal(a); return self(); }
    Derived& AlignVertical(Alignment a) const   { impl_->ApplyAlignVertical(a); return self(); }
    /// 朝向（Stack / Splitter 等按此决定横排还是竖排）
    Derived& Orientation(::CUI::Orientation o) const { impl_->ApplyOrientation(o); return self(); }
    // ---------------- 由 UIElement 补齐的通用链式方法 ----------------
    Derived& Visibility(Visibility v) const { impl_->ApplyVisibility(v); return self(); }
    Derived& IsEnabled(bool enabled) const { impl_->ApplyIsEnabled(enabled); return self(); }
    Derived& CornerRadius(float v) const { impl_->ApplyCornerRadius(v); return self(); }
    Derived& BorderThickness(float v) const { impl_->ApplyBorderThickness(v); return self(); }
    Derived& FlexGrow(float v) const { impl_->ApplyFlexGrow(v); return self(); }
    Derived& Gap(float v) const { impl_->ApplyGap(v); return self(); }
    Derived& ItemWidth(float v) const { impl_->ApplyItemWidth(v); return self(); }
    Derived& ItemHeight(float v) const { impl_->ApplyItemHeight(v); return self(); }
    Derived& LastChildFill(bool v) const { impl_->ApplyLastChildFill(v); return self(); }
    Derived& JustifyLines(bool v) const { impl_->ApplyJustifyLines(v); return self(); }
    Derived& FillLastLine(bool v) const { impl_->ApplyFillLastLine(v); return self(); }
    template<class... A> Derived& Rows(A&&... a) const { if constexpr (requires { impl_->ApplyRows(std::forward<A>(a)...); }) impl_->ApplyRows(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& Columns(A&&... a) const { if constexpr (requires { impl_->ApplyColumns(std::forward<A>(a)...); }) impl_->ApplyColumns(std::forward<A>(a)...); return self(); }
    Derived& ClipToBounds(bool v) const { impl_->ApplyClipToBounds(v); return self(); }
    Derived& CanvasLeft(float v) const { impl_->ApplyCanvasLeft(v); return self(); }
    Derived& CanvasTop(float v) const { impl_->ApplyCanvasTop(v); return self(); }
    Derived& CanvasRight(float v) const { impl_->ApplyCanvasRight(v); return self(); }
    Derived& CanvasBottom(float v) const { impl_->ApplyCanvasBottom(v); return self(); }
    Derived& ZIndex(int v) const { impl_->ApplyZIndex(v); return self(); }
    Derived& GridColumn(int v) const { impl_->ApplyGridColumn(v); return self(); }
    Derived& GridRow(int v) const { impl_->ApplyGridRow(v); return self(); }
    Derived& GridColumnSpan(int v) const { impl_->ApplyGridColumnSpan(v); return self(); }
    Derived& GridRowSpan(int v) const { impl_->ApplyGridRowSpan(v); return self(); }
    Derived& Dock(Dock d) const { impl_->ApplyDock(d); return self(); }
    Derived& ForegroundToken(ThemeTokenId id) const { if constexpr (requires { impl_->ApplyForegroundToken(id); }) impl_->ApplyForegroundToken(id); return self(); }

    // ---- 由 scripts/gen_missing_setters.py 从旧 DSL 自动补齐 ----
    Derived& Accent(const std::string& value) const { if constexpr (requires { impl_->ApplyAccent(value); }) impl_->ApplyAccent(value); return self(); }
    Derived& AcceptsReturn(bool value) const { if constexpr (requires { impl_->ApplyAcceptsReturn(value); }) impl_->ApplyAcceptsReturn(value); return self(); }
    Derived& AcceptsTab(bool value) const { if constexpr (requires { impl_->ApplyAcceptsTab(value); }) impl_->ApplyAcceptsTab(value); return self(); }
    Derived& ActionText(const std::string& value) const { if constexpr (requires { impl_->ApplyActionText(value); }) impl_->ApplyActionText(value); return self(); }
    Derived& AddItem(const std::string& value) const { if constexpr (requires { impl_->AddItem(value); }) impl_->AddItem(value); return self(); }
    Derived& AllowDrag(bool value) const { if constexpr (requires { impl_->ApplyAllowDrag(value); }) impl_->ApplyAllowDrag(value); return self(); }
    Derived& AllowDrop(bool value) const { if constexpr (requires { impl_->ApplyAllowDrop(value); }) impl_->ApplyAllowDrop(value); return self(); }
    Derived& AlwaysShowHeader(bool value) const { if constexpr (requires { impl_->ApplyAlwaysShowHeader(value); }) impl_->ApplyAlwaysShowHeader(value); return self(); }
    Derived& AutoClose(bool value) const { if constexpr (requires { impl_->ApplyAutoClose(value); }) impl_->ApplyAutoClose(value); return self(); }
    Derived& BadgeColor(D2D1_COLOR_F value) const { if constexpr (requires { impl_->ApplyBadgeColor(value); }) impl_->ApplyBadgeColor(value); return self(); }
    Derived& BadgeText(const std::string& value) const { if constexpr (requires { impl_->ApplyBadgeText(value); }) impl_->ApplyBadgeText(value); return self(); }
    Derived& Border(D2D1_COLOR_F color, float thickness = 1.0f) const { if constexpr (requires { impl_->ApplyBorderBrush(color); }) impl_->ApplyBorderBrush(color); return self(); }
    Derived& BytesPerRow(int value) const { if constexpr (requires { impl_->ApplyBytesPerRow(value); }) impl_->ApplyBytesPerRow(value); return self(); }
    Derived& CanSave(bool value) const { if constexpr (requires { impl_->ApplyCanSave(value); }) impl_->ApplyCanSave(value); return self(); }
    Derived& CaretIndex(int value) const { if constexpr (requires { impl_->ApplyCaretIndex(value); }) impl_->ApplyCaretIndex(value); return self(); }
    Derived& Checked(bool value = true) const { if constexpr (requires { impl_->ApplyChecked(value); }) impl_->ApplyChecked(value); return self(); }
    Derived& CloseButtonText(const std::string& value) const { if constexpr (requires { impl_->ApplyCloseButtonText(value); }) impl_->ApplyCloseButtonText(value); return self(); }
    Derived& Closeable(bool value) const { if constexpr (requires { impl_->ApplyCloseable(value); }) impl_->ApplyCloseable(value); return self(); }
    Derived& ColumnDefinitions(const std::string& defs) const { if constexpr (requires { impl_->ApplyColumnDefinitions(defs); }) impl_->ApplyColumnDefinitions(defs); return self(); }
    Derived& ColumnHeader(int index, const std::string& header, float width = 120.0f) const { if constexpr (requires { impl_->AddColumn(header, width); }) impl_->AddColumn(header, width); return self(); }
    Derived& ColumnVisible(int index, bool value) const { if constexpr (requires { impl_->ApplyColumnVisible(index, value); }) impl_->ApplyColumnVisible(index, value); return self(); }
    Derived& Compact(bool value) const { if constexpr (requires { impl_->ApplyCompact(value); }) impl_->ApplyCompact(value); return self(); }
    Derived& CompactModeThresholdWidth(float value) const { if constexpr (requires { impl_->ApplyCompactModeThresholdWidth(value); }) impl_->ApplyCompactModeThresholdWidth(value); return self(); }
    Derived& CompactPaneLength(float value) const { if constexpr (requires { impl_->ApplyCompactPaneLength(value); }) impl_->ApplyCompactPaneLength(value); return self(); }
    Derived& CurrentPage(int value) const { if constexpr (requires { impl_->ApplyCurrentPage(value); }) impl_->ApplyCurrentPage(value); return self(); }
    Derived& CurrentPage(const std::string& value) const { if constexpr (requires { impl_->ApplyCurrentPage(value); }) impl_->ApplyCurrentPage(value); return self(); }
    Derived& Damping(float value) const { if constexpr (requires { impl_->ApplyDamping(value); }) impl_->ApplyDamping(value); return self(); }
    Derived& Data(const std::string& value) const { if constexpr (requires { impl_->ApplyData(value); }) impl_->ApplyData(value); return self(); }
    Derived& Date(int year, int month, int day) const { if constexpr (requires { impl_->ApplyDate(year, month, day); }) impl_->ApplyDate(year, month, day); return self(); }
    Derived& DialogTitle(const std::string& value) const { if constexpr (requires { impl_->ApplyDialogTitle(value); }) impl_->ApplyDialogTitle(value); return self(); }
    Derived& DurationMs(int value) const { if constexpr (requires { impl_->ApplyDurationMs(value); }) impl_->ApplyDurationMs(value); return self(); }
    Derived& Expanded(bool value) const { if constexpr (requires { impl_->ApplyExpanded(value); }) impl_->ApplyExpanded(value); return self(); }
    Derived& ExpandedModeThresholdWidth(float value) const { if constexpr (requires { impl_->ApplyExpandedModeThresholdWidth(value); }) impl_->ApplyExpandedModeThresholdWidth(value); return self(); }
    Derived& Fill(D2D1_COLOR_F value) const { if constexpr (requires { impl_->ApplyFill(value); }) impl_->ApplyFill(value); return self(); }
    Derived& Filter(const std::string& name, const std::string& spec) const { if constexpr (requires { impl_->ApplyFilter(name, spec); }) impl_->ApplyFilter(name, spec); return self(); }
    Derived& FlowParticles(bool enabled = true) const { if constexpr (requires { impl_->ApplyFlowParticlesEnabled(enabled); }) impl_->ApplyFlowParticlesEnabled(enabled); return self(); }
    Derived& FlowParticlesEnabled(bool value) const { if constexpr (requires { impl_->ApplyFlowParticlesEnabled(value); }) impl_->ApplyFlowParticlesEnabled(value); return self(); }
    Derived& Gesture(const std::string& value) const { if constexpr (requires { impl_->ApplyGesture(value); }) impl_->ApplyGesture(value); return self(); }
    Derived& GroupName(const std::string& value) const { if constexpr (requires { impl_->ApplyGroupName(value); }) impl_->ApplyGroupName(value); return self(); }
    Derived& Header(const std::string& value) const { if constexpr (requires { impl_->ApplyHeader(value); }) impl_->ApplyHeader(value); return self(); }
    Derived& IconText(const std::string& icon) const { if constexpr (requires { impl_->ApplyIconText(icon); }) impl_->ApplyIconText(icon); return self(); }
    Derived& Id(const std::string& id) const { if constexpr (requires { impl_->ApplyId(id); }) impl_->ApplyId(id); return self(); }
    Derived& IndentWidth(float value) const { if constexpr (requires { impl_->ApplyIndentWidth(value); }) impl_->ApplyIndentWidth(value); return self(); }
    Derived& InputEnabled(bool value, bool multiline = false) const { if constexpr (requires { impl_->ApplyInputEnabled(value, multiline); }) impl_->ApplyInputEnabled(value, multiline); return self(); }
    Derived& InputText(const std::string& value) const { if constexpr (requires { impl_->ApplyInputText(value); }) impl_->ApplyInputText(value); return self(); }
    Derived& IsBackEnabled(bool value) const { if constexpr (requires { impl_->ApplyIsBackEnabled(value); }) impl_->ApplyIsBackEnabled(value); return self(); }
    Derived& IsChecked(bool value) const { if constexpr (requires { impl_->ApplyIsChecked(value); }) impl_->ApplyIsChecked(value); return self(); }
    Derived& IsChildSelected(bool value) const { if constexpr (requires { impl_->ApplyIsChildSelected(value); }) impl_->ApplyIsChildSelected(value); return self(); }
    Derived& IsClearEnabled(bool value) const { if constexpr (requires { impl_->ApplyIsClearEnabled(value); }) impl_->ApplyIsClearEnabled(value); return self(); }
    Derived& IsClosable(bool value) const { if constexpr (requires { impl_->ApplyIsClosable(value); }) impl_->ApplyIsClosable(value); return self(); }
    Derived& IsCloseButtonEnabled(bool value) const { if constexpr (requires { impl_->ApplyIsCloseButtonEnabled(value); }) impl_->ApplyIsCloseButtonEnabled(value); return self(); }
    Derived& IsCloseButtonVisible(bool value) const { if constexpr (requires { impl_->ApplyIsCloseButtonVisible(value); }) impl_->ApplyIsCloseButtonVisible(value); return self(); }
    Derived& IsCloseVisible(bool value) const { if constexpr (requires { impl_->ApplyIsCloseVisible(value); }) impl_->ApplyIsCloseVisible(value); return self(); }
    Derived& IsExpanded(bool exp) const { if constexpr (requires { impl_->ApplyIsExpanded(exp); }) impl_->ApplyIsExpanded(exp); return self(); }
    Derived& IsExpandedSilent(bool value) const { if constexpr (requires { impl_->ApplyIsExpandedSilent(value); }) impl_->ApplyIsExpandedSilent(value); return self(); }
    Derived& IsIndeterminate(bool value) const { if constexpr (requires { impl_->ApplyIsIndeterminate(value); }) impl_->ApplyIsIndeterminate(value); return self(); }
    Derived& IsMaximizeButtonEnabled(bool value) const { if constexpr (requires { impl_->ApplyIsMaximizeButtonEnabled(value); }) impl_->ApplyIsMaximizeButtonEnabled(value); return self(); }
    Derived& IsMaximizeButtonVisible(bool value) const { if constexpr (requires { impl_->ApplyIsMaximizeButtonVisible(value); }) impl_->ApplyIsMaximizeButtonVisible(value); return self(); }
    Derived& IsMinimizeButtonEnabled(bool value) const { if constexpr (requires { impl_->ApplyIsMinimizeButtonEnabled(value); }) impl_->ApplyIsMinimizeButtonEnabled(value); return self(); }
    Derived& IsMinimizeButtonVisible(bool value) const { if constexpr (requires { impl_->ApplyIsMinimizeButtonVisible(value); }) impl_->ApplyIsMinimizeButtonVisible(value); return self(); }
    Derived& IsModal(bool value) const { if constexpr (requires { impl_->ApplyIsModal(value); }) impl_->ApplyIsModal(value); return self(); }
    Derived& IsOn(bool value) const { if constexpr (requires { impl_->ApplyIsOn(value); }) impl_->ApplyIsOn(value); return self(); }
    Derived& IsOpen(bool value) const { if constexpr (requires { impl_->ApplyIsOpen(value); }) impl_->ApplyIsOpen(value); return self(); }
    Derived& IsPaneOpen(bool value) const { if constexpr (requires { impl_->ApplyIsPaneOpen(value); }) impl_->ApplyIsPaneOpen(value); return self(); }
    Derived& IsPasswordMode(bool value) const { if constexpr (requires { impl_->ApplyIsPasswordMode(value); }) impl_->ApplyIsPasswordMode(value); return self(); }
    Derived& IsReadOnly(bool ro) const { if constexpr (requires { impl_->ApplyIsReadOnly(ro); }) impl_->ApplyIsReadOnly(ro); return self(); }
    Derived& IsSelected(bool value) const { if constexpr (requires { impl_->ApplyIsSelected(value); }) impl_->ApplyIsSelected(value); return self(); }
    Derived& IsSeparator(bool value = true) const { if constexpr (requires { impl_->ApplyIsSeparator(value); }) impl_->ApplyIsSeparator(value); return self(); }
    Derived& IsSettingsVisible(bool value) const { if constexpr (requires { impl_->ApplyIsSettingsVisible(value); }) impl_->ApplyIsSettingsVisible(value); return self(); }
    Derived& IsThreeState(bool value) const { if constexpr (requires { impl_->ApplyIsThreeState(value); }) impl_->ApplyIsThreeState(value); return self(); }
    Derived& ItemIcon(int id, const std::string& value) const { if constexpr (requires { impl_->ApplyItemIcon(id, value); }) impl_->ApplyItemIcon(id, value); return self(); }
    Derived& ItemProgress(int id, float value) const { if constexpr (requires { impl_->ApplyItemProgress(id, value); }) impl_->ApplyItemProgress(id, value); return self(); }
    Derived& ItemText(int id, const std::string& value) const { if constexpr (requires { impl_->ApplyItemText(id, value); }) impl_->ApplyItemText(id, value); return self(); }
    Derived& Justified(bool enabled = true) const { if constexpr (requires { impl_->ApplyJustifyLines(enabled); }) impl_->ApplyJustifyLines(enabled); return self(); }
    Derived& Label(const std::string& value) const { if constexpr (requires { impl_->ApplyLabel(value); }) impl_->ApplyLabel(value); return self(); }
    Derived& LineSpacing(float value) const { if constexpr (requires { impl_->ApplyLineSpacing(value); }) impl_->ApplyLineSpacing(value); return self(); }
    Derived& LowPerformanceMode(bool value) const { if constexpr (requires { impl_->ApplyLowPerformanceMode(value); }) impl_->ApplyLowPerformanceMode(value); return self(); }
    Derived& Markdown(const std::string& value) const { if constexpr (requires { impl_->ApplyMarkdown(value); }) impl_->ApplyMarkdown(value); return self(); }
    Derived& MaxRating(int value) const { if constexpr (requires { impl_->ApplyMaxRating(value); }) impl_->ApplyMaxRating(value); return self(); }
    Derived& MaxTabWidth(float value) const { if constexpr (requires { impl_->ApplyMaxTabWidth(value); }) impl_->ApplyMaxTabWidth(value); return self(); }
    Derived& MaxVisibleSuggestions(int value) const { if constexpr (requires { impl_->ApplyMaxVisibleSuggestions(value); }) impl_->ApplyMaxVisibleSuggestions(value); return self(); }
    Derived& Maximum(float maxVal) const { if constexpr (requires { impl_->ApplyMaximum(maxVal); }) impl_->ApplyMaximum(maxVal); return self(); }
    Derived& Message(const std::string& m) const { if constexpr (requires { impl_->ApplyMessage(m); }) impl_->ApplyMessage(m); return self(); }
    Derived& MessageColor(const std::string& value) const { if constexpr (requires { impl_->ApplyMessageColor(value); }) impl_->ApplyMessageColor(value); return self(); }
    Derived& MinTabWidth(float value) const { if constexpr (requires { impl_->ApplyMinTabWidth(value); }) impl_->ApplyMinTabWidth(value); return self(); }
    Derived& Minimum(float minVal) const { if constexpr (requires { impl_->ApplyMinimum(minVal); }) impl_->ApplyMinimum(minVal); return self(); }
    Derived& MinimumRange(float value) const { if constexpr (requires { impl_->ApplyMinimumRange(value); }) impl_->ApplyMinimumRange(value); return self(); }
    Derived& NavigateUri(const std::string& value) const { if constexpr (requires { impl_->ApplyNavigateUri(value); }) impl_->ApplyNavigateUri(value); return self(); }
    Derived& OffsetX(float value) const { if constexpr (requires { impl_->ApplyOffsetX(value); }) impl_->ApplyOffsetX(value); return self(); }
    Derived& OffsetY(float value) const { if constexpr (requires { impl_->ApplyOffsetY(value); }) impl_->ApplyOffsetY(value); return self(); }
    Derived& OpenPaneLength(float value) const { if constexpr (requires { impl_->ApplyOpenPaneLength(value); }) impl_->ApplyOpenPaneLength(value); return self(); }
    Derived& OverlayComposed(bool value) const { if constexpr (requires { impl_->ApplyOverlayComposed(value); }) impl_->ApplyOverlayComposed(value); return self(); }
    Derived& OverlayScrollbar(bool value) const { if constexpr (requires { impl_->ApplyOverlayScrollbar(value); }) impl_->ApplyOverlayScrollbar(value); return self(); }
    Derived& PaneAutoHide(int index, bool value) const { if constexpr (requires { impl_->ApplyPaneAutoHide(index, value); }) impl_->ApplyPaneAutoHide(index, value); return self(); }
    Derived& PaneTitle(const std::string& value) const { if constexpr (requires { impl_->ApplyPaneTitle(value); }) impl_->ApplyPaneTitle(value); return self(); }
    Derived& Password(const std::string& value) const { if constexpr (requires { impl_->ApplyPassword(value); }) impl_->ApplyPassword(value); return self(); }
    template<class... A> Derived& Path(A&&... a) const { if constexpr (requires { impl_->ApplyPath(std::forward<A>(a)...); }) impl_->ApplyPath(std::forward<A>(a)...); return self(); }
    /// 路径分段可直接写花括号：.Path({ "计算机", "HKEY_LOCAL_MACHINE" })
    template<class T> Derived& Path(std::initializer_list<T> v) const {
        if constexpr (requires { impl_->ApplyPath(std::vector<T>(v)); }) impl_->ApplyPath(std::vector<T>(v));
        return self();
    }
    Derived& PersistEnabled(bool value) const { if constexpr (requires { impl_->ApplyPersistEnabled(value); }) impl_->ApplyPersistEnabled(value); return self(); }
    Derived& PrimaryButtonText(const std::string& value) const { if constexpr (requires { impl_->ApplyPrimaryButtonText(value); }) impl_->ApplyPrimaryButtonText(value); return self(); }
    Derived& Range(float lower, float upper) const { if constexpr (requires { impl_->ApplyRange(lower, upper); }) impl_->ApplyRange(lower, upper); return self(); }
    Derived& RenderStatsOverlayVisible(bool value) const { if constexpr (requires { impl_->ApplyRenderStatsOverlayVisible(value); }) impl_->ApplyRenderStatsOverlayVisible(value); return self(); }
    Derived& RowDefinitions(const std::string& defs) const { if constexpr (requires { impl_->ApplyRowDefinitions(defs); }) impl_->ApplyRowDefinitions(defs); return self(); }
    Derived& RowHeight(float value) const { if constexpr (requires { impl_->ApplyRowHeight(value); }) impl_->ApplyRowHeight(value); return self(); }
    Derived& RowSelected(int index, bool value) const { if constexpr (requires { impl_->ApplyRowSelected(index, value); }) impl_->ApplyRowSelected(index, value); return self(); }
    Derived& Running(bool value) const { if constexpr (requires { impl_->ApplyRunning(value); }) impl_->ApplyRunning(value); return self(); }
    Derived& ScrollOffsetY(float value) const { if constexpr (requires { impl_->ApplyScrollOffsetY(value); }) impl_->ApplyScrollOffsetY(value); return self(); }
    Derived& SecondaryButtonText(const std::string& value) const { if constexpr (requires { impl_->ApplySecondaryButtonText(value); }) impl_->ApplySecondaryButtonText(value); return self(); }
    Derived& SelectedIndex(int value) const { if constexpr (requires { impl_->ApplySelectedIndex(value); }) impl_->ApplySelectedIndex(value); return self(); }
    Derived& SelectsOnInvoked(bool value) const { if constexpr (requires { impl_->ApplySelectsOnInvoked(value); }) impl_->ApplySelectsOnInvoked(value); return self(); }
    Derived& ShortcutText(const std::string& value) const { if constexpr (requires { impl_->ApplyShortcutText(value); }) impl_->ApplyShortcutText(value); return self(); }
    Derived& ShowCodeLineNumbers(bool value) const { if constexpr (requires { impl_->ApplyShowCodeLineNumbers(value); }) impl_->ApplyShowCodeLineNumbers(value); return self(); }
    Derived& ShowGrid(bool value) const { if constexpr (requires { impl_->ApplyShowGrid(value); }) impl_->ApplyShowGrid(value); return self(); }
    Derived& ShowGridLines(bool value) const { if constexpr (requires { impl_->ApplyShowGridLines(value); }) impl_->ApplyShowGridLines(value); return self(); }
    Derived& ShowLegend(bool value) const { if constexpr (requires { impl_->ApplyShowLegend(value); }) impl_->ApplyShowLegend(value); return self(); }
    Derived& ShowRevealButton(bool value) const { if constexpr (requires { impl_->ApplyShowRevealButton(value); }) impl_->ApplyShowRevealButton(value); return self(); }
    Derived& ShowScrollBars(bool value) const { if constexpr (requires { impl_->ApplyShowScrollBars(value); }) impl_->ApplyShowScrollBars(value); return self(); }
    Derived& ShowTooltip(bool value) const { if constexpr (requires { impl_->ApplyShowTooltip(value); }) impl_->ApplyShowTooltip(value); return self(); }
    Derived& Size(float w, float h) const { if constexpr (requires { impl_->ApplyWidth(w); }) impl_->ApplyWidth(w); return self(); }
    Derived& Source(const std::string& value) const { if constexpr (requires { impl_->ApplySource(value); }) impl_->ApplySource(value); return self(); }
    template<class V> Derived& Stretch(V value) const { if constexpr (requires { impl_->ApplyStretch(value); }) impl_->ApplyStretch(value); return self(); }
    template<class V> Derived& Value(V value) const { if constexpr (requires { impl_->ApplyValue(value); }) impl_->ApplyValue(value); return self(); }
    template<class V> Derived& State(V value) const { if constexpr (requires { impl_->ApplyState(value); }) impl_->ApplyState(value); return self(); }
    template<class V> Derived& Content(V value) const { if constexpr (requires { impl_->ApplyContent(value); }) impl_->ApplyContent(value); return self(); }
    template<class V> Derived& Placement(V value) const { if constexpr (requires { impl_->ApplyPlacement(value); }) impl_->ApplyPlacement(value); return self(); }
    template<class V> Derived& SelectionMode(V value) const { if constexpr (requires { impl_->ApplySelectionMode(value); }) impl_->ApplySelectionMode(value); return self(); }
    template<class V> Derived& Subtitle(V value) const { if constexpr (requires { impl_->ApplySubtitle(value); }) impl_->ApplySubtitle(value); return self(); }

    // ---- 页面层仍在用的其余属性入口（同上，统一走链式，避免作者级 ->SetX 调用） ----
    template<class... A> Derived& ActiveContextMenu(A&&... a) const { if constexpr (requires { impl_->ApplyActiveContextMenu(std::forward<A>(a)...); }) impl_->ApplyActiveContextMenu(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& AntialiasMode(A&&... a) const { if constexpr (requires { impl_->ApplyAntialiasMode(std::forward<A>(a)...); }) impl_->ApplyAntialiasMode(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& BackdropType(A&&... a) const { if constexpr (requires { impl_->ApplyBackdropType(std::forward<A>(a)...); }) impl_->ApplyBackdropType(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& Bytes(A&&... a) const { if constexpr (requires { impl_->ApplyBytes(std::forward<A>(a)...); }) impl_->ApplyBytes(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& Category(A&&... a) const { if constexpr (requires { impl_->ApplyCategory(std::forward<A>(a)...); }) impl_->ApplyCategory(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& ContentFactory(A&&... a) const { if constexpr (requires { impl_->ApplyContentFactory(std::forward<A>(a)...); }) impl_->ApplyContentFactory(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& ContextMenu(A&&... a) const { if constexpr (requires { impl_->ApplyContextMenu(std::forward<A>(a)...); }) impl_->ApplyContextMenu(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& IsBackButtonVisible(A&&... a) const { if constexpr (requires { impl_->ApplyIsBackButtonVisible(std::forward<A>(a)...); }) impl_->ApplyIsBackButtonVisible(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& LazyPopulate(A&&... a) const { if constexpr (requires { impl_->ApplyLazyPopulate(std::forward<A>(a)...); }) impl_->ApplyLazyPopulate(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& Logger(A&&... a) const { if constexpr (requires { impl_->ApplyLogger(std::forward<A>(a)...); }) impl_->ApplyLogger(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& OwnerWindow(A&&... a) const { if constexpr (requires { impl_->ApplyOwnerWindow(std::forward<A>(a)...); }) impl_->ApplyOwnerWindow(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& PaneDisplayMode(A&&... a) const { if constexpr (requires { impl_->ApplyPaneDisplayMode(std::forward<A>(a)...); }) impl_->ApplyPaneDisplayMode(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& PixelFormat(A&&... a) const { if constexpr (requires { impl_->ApplyPixelFormat(std::forward<A>(a)...); }) impl_->ApplyPixelFormat(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& SelectedId(A&&... a) const { if constexpr (requires { impl_->ApplySelectedId(std::forward<A>(a)...); }) impl_->ApplySelectedId(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& SettingsItem(A&&... a) const { if constexpr (requires { impl_->ApplySettingsItem(std::forward<A>(a)...); }) impl_->ApplySettingsItem(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& StatusHandler(A&&... a) const { if constexpr (requires { impl_->ApplyStatusHandler(std::forward<A>(a)...); }) impl_->ApplyStatusHandler(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& TextAntialiasMode(A&&... a) const { if constexpr (requires { impl_->ApplyTextAntialiasMode(std::forward<A>(a)...); }) impl_->ApplyTextAntialiasMode(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& ThemeMode(A&&... a) const { if constexpr (requires { impl_->ApplyThemeMode(std::forward<A>(a)...); }) impl_->ApplyThemeMode(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& ThemeModeWithRipple(A&&... a) const { if constexpr (requires { impl_->ApplyThemeModeWithRipple(std::forward<A>(a)...); }) impl_->ApplyThemeModeWithRipple(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& LayoutType(A&&... a) const { if constexpr (requires { impl_->ApplyLayoutType(std::forward<A>(a)...); }) impl_->ApplyLayoutType(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& SideSize(A&&... a) const { if constexpr (requires { impl_->ApplySideSize(std::forward<A>(a)...); }) impl_->ApplySideSize(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& PreferredPlacement(A&&... a) const { if constexpr (requires { impl_->ApplyPreferredPlacement(std::forward<A>(a)...); }) impl_->ApplyPreferredPlacement(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& Items(A&&... a) const { if constexpr (requires { impl_->ApplyItems(std::forward<A>(a)...); }) impl_->ApplyItems(std::forward<A>(a)...); return self(); }
    /// 列表项可直接写花括号：.Items({ "A", "B" })
    template<class T> Derived& Items(std::initializer_list<T> v) const {
        if constexpr (requires { impl_->ApplyItems(std::vector<T>(v)); }) impl_->ApplyItems(std::vector<T>(v));
        return self();
    }
    template<class... A> Derived& ItemExpanded(A&&... a) const { if constexpr (requires { impl_->ApplyItemExpanded(std::forward<A>(a)...); }) impl_->ApplyItemExpanded(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& ExpandDirection(A&&... a) const { if constexpr (requires { impl_->ApplyExpandDirection(std::forward<A>(a)...); }) impl_->ApplyExpandDirection(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& LabelPosition(A&&... a) const { if constexpr (requires { impl_->ApplyLabelPosition(std::forward<A>(a)...); }) impl_->ApplyLabelPosition(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& SelectedItem(A&&... a) const { if constexpr (requires { impl_->ApplySelectedItem(std::forward<A>(a)...); }) impl_->ApplySelectedItem(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& Severity(A&&... a) const { if constexpr (requires { impl_->ApplySeverity(std::forward<A>(a)...); }) impl_->ApplySeverity(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& VirtualMode(A&&... a) const { if constexpr (requires { impl_->ApplyVirtualMode(std::forward<A>(a)...); }) impl_->ApplyVirtualMode(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& SuggestionItems(A&&... a) const { if constexpr (requires { impl_->ApplySuggestionItems(std::forward<A>(a)...); }) impl_->ApplySuggestionItems(std::forward<A>(a)...); return self(); }
    /// 候选词可直接写花括号：.SuggestionItems({ "A", "B" })
    template<class T> Derived& SuggestionItems(std::initializer_list<T> v) const {
        if constexpr (requires { impl_->ApplySuggestionItems(std::vector<T>(v)); }) impl_->ApplySuggestionItems(std::vector<T>(v));
        return self();
    }
    template<class... A> Derived& SuggestionProvider(A&&... a) const { if constexpr (requires { impl_->ApplySuggestionProvider(std::forward<A>(a)...); }) impl_->ApplySuggestionProvider(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& AutoSuggestBox(A&&... a) const { if constexpr (requires { impl_->ApplyAutoSuggestBox(std::forward<A>(a)...); }) impl_->ApplyAutoSuggestBox(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& RowIcons(A&&... a) const { if constexpr (requires { impl_->ApplyRowIcons(std::forward<A>(a)...); }) impl_->ApplyRowIcons(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& RowTags(A&&... a) const { if constexpr (requires { impl_->ApplyRowTags(std::forward<A>(a)...); }) impl_->ApplyRowTags(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& ActionCommand(A&&... a) const { if constexpr (requires { impl_->ApplyActionCommand(std::forward<A>(a)...); }) impl_->ApplyActionCommand(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& Categories(A&&... a) const { if constexpr (requires { impl_->ApplyCategories(std::forward<A>(a)...); }) impl_->ApplyCategories(std::forward<A>(a)...); return self(); }
    /// 分类轴可直接写花括号：.Categories({ "1月", "2月" })
    template<class T> Derived& Categories(std::initializer_list<T> v) const {
        if constexpr (requires { impl_->ApplyCategories(std::vector<T>(v)); }) impl_->ApplyCategories(std::vector<T>(v));
        return self();
    }
    template<class... A> Derived& Corner(A&&... a) const { if constexpr (requires { impl_->ApplyCorner(std::forward<A>(a)...); }) impl_->ApplyCorner(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& MaxEntries(A&&... a) const { if constexpr (requires { impl_->ApplyMaxEntries(std::forward<A>(a)...); }) impl_->ApplyMaxEntries(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& RightContent(A&&... a) const { if constexpr (requires { impl_->ApplyRightContent(std::forward<A>(a)...); }) impl_->ApplyRightContent(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& Series(A&&... a) const { if constexpr (requires { impl_->ApplySeries(std::forward<A>(a)...); }) impl_->ApplySeries(std::forward<A>(a)...); return self(); }
    /// 系列可直接写花括号：.Series({ s1, s2 })
    template<class T> Derived& Series(std::initializer_list<T> v) const {
        if constexpr (requires { impl_->ApplySeries(std::vector<T>(v)); }) impl_->ApplySeries(std::vector<T>(v));
        return self();
    }
    template<class... A> Derived& ImageType(A&&... a) const { if constexpr (requires { impl_->ApplyImageType(std::forward<A>(a)...); }) impl_->ApplyImageType(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& TextAlign(A&&... a) const { if constexpr (requires { impl_->ApplyTextAlign(std::forward<A>(a)...); }) impl_->ApplyTextAlign(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& VerticalAlign(A&&... a) const { if constexpr (requires { impl_->ApplyVerticalAlign(std::forward<A>(a)...); }) impl_->ApplyVerticalAlign(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& ShellContextMenuHandler(A&&... a) const { if constexpr (requires { impl_->ApplyShellContextMenuHandler(std::forward<A>(a)...); }) impl_->ApplyShellContextMenuHandler(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& VirtualRowCount(A&&... a) const { if constexpr (requires { impl_->ApplyVirtualRowCount(std::forward<A>(a)...); }) impl_->ApplyVirtualRowCount(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& ItemEnabled(A&&... a) const { if constexpr (requires { impl_->ApplyItemEnabled(std::forward<A>(a)...); }) impl_->ApplyItemEnabled(std::forward<A>(a)...); return self(); }
    template<class... A> Derived& NativeIcon(A&&... a) const { if constexpr (requires { impl_->ApplyNativeIcon(std::forward<A>(a)...); }) impl_->ApplyNativeIcon(std::forward<A>(a)...); return self(); }
    Derived& Spacing(float value) const { if constexpr (requires { impl_->ApplySpacing(value); }) impl_->ApplySpacing(value); return self(); }
    Derived& Step(float s) const { if constexpr (requires { impl_->ApplyStep(s); }) impl_->ApplyStep(s); return self(); }
    Derived& Stiffness(float value) const { if constexpr (requires { impl_->ApplyStiffness(value); }) impl_->ApplyStiffness(value); return self(); }
    Derived& Strikethrough(bool strikethrough = true) const { if constexpr (requires { impl_->ApplyIsStrikethrough(strikethrough); }) impl_->ApplyIsStrikethrough(strikethrough); return self(); }
    Derived& Stroke(D2D1_COLOR_F value) const { if constexpr (requires { impl_->ApplyStroke(value); }) impl_->ApplyStroke(value); return self(); }
    Derived& StrokeThickness(float value) const { if constexpr (requires { impl_->ApplyStrokeThickness(value); }) impl_->ApplyStrokeThickness(value); return self(); }
    Derived& Tag(const std::string& value) const { if constexpr (requires { impl_->ApplyTag(value); }) impl_->ApplyTag(value); return self(); }
    Derived& TextWrapping(bool value) const { if constexpr (requires { impl_->ApplyTextWrapping(value); }) impl_->ApplyTextWrapping(value); return self(); }
    Derived& Time(int hour, int minute) const { if constexpr (requires { impl_->ApplyTime(hour, minute); }) impl_->ApplyTime(hour, minute); return self(); }
    Derived& TintColor(D2D1_COLOR_F value) const { if constexpr (requires { impl_->ApplyTintColor(value); }) impl_->ApplyTintColor(value); return self(); }
    Derived& Title(const std::string& t) const { if constexpr (requires { impl_->ApplyTitle(t); }) impl_->ApplyTitle(t); return self(); }
    Derived& TitleColor(const std::string& value) const { if constexpr (requires { impl_->ApplyTitleColor(value); }) impl_->ApplyTitleColor(value); return self(); }
    Derived& TopMode(bool value) const { if constexpr (requires { impl_->ApplyTopMode(value); }) impl_->ApplyTopMode(value); return self(); }
    Derived& TotalPages(int value) const { if constexpr (requires { impl_->ApplyTotalPages(value); }) impl_->ApplyTotalPages(value); return self(); }
    Derived& Underline(bool underline = true) const { if constexpr (requires { impl_->ApplyIsUnderline(underline); }) impl_->ApplyIsUnderline(underline); return self(); }
    Derived& Viewport(float width, float height) const { if constexpr (requires { impl_->ApplyViewport(width, height); }) impl_->ApplyViewport(width, height); return self(); }
    Derived& VirtualCount(size_t count) const { if constexpr (requires { impl_->ApplyVirtualCount(count); }) impl_->ApplyVirtualCount(count); return self(); }
    Derived& X1(float value) const { if constexpr (requires { impl_->ApplyX1(value); }) impl_->ApplyX1(value); return self(); }
    Derived& X2(float value) const { if constexpr (requires { impl_->ApplyX2(value); }) impl_->ApplyX2(value); return self(); }
    Derived& Y1(float value) const { if constexpr (requires { impl_->ApplyY1(value); }) impl_->ApplyY1(value); return self(); }
    Derived& Y2(float value) const { if constexpr (requires { impl_->ApplyY2(value); }) impl_->ApplyY2(value); return self(); }
    Derived& BackgroundToken(ThemeTokenId id) const { impl_->ApplyBackgroundToken(id); return self(); }
    Derived& HoverBackgroundToken(ThemeTokenId id) const { impl_->ApplyHoverBackgroundToken(id); return self(); }
    Derived& PressedBackgroundToken(ThemeTokenId id) const { impl_->ApplyPressedBackgroundToken(id); return self(); }
    Derived& DisabledBackgroundToken(ThemeTokenId id) const { impl_->ApplyDisabledBackgroundToken(id); return self(); }
    Derived& BorderToken(ThemeTokenId id) const { impl_->ApplyBorderToken(id); return self(); }
    Derived& FocusedBorderToken(ThemeTokenId id) const { impl_->ApplyFocusedBorderToken(id); return self(); }
    Derived& ColorToken(ThemeTokenId id) const { impl_->ApplyColorToken(id); return self(); }
    Derived& SecondaryColorToken(ThemeTokenId id) const { impl_->ApplySecondaryColorToken(id); return self(); }
    Derived& PlaceholderColorToken(ThemeTokenId id) const { impl_->ApplyPlaceholderColorToken(id); return self(); }
    Derived& SelectedBackgroundToken(ThemeTokenId id) const { impl_->ApplySelectedBackgroundToken(id); return self(); }
    Derived& HeaderBackgroundToken(ThemeTokenId id) const { impl_->ApplyHeaderBackgroundToken(id); return self(); }
    Derived& PaneBackgroundToken(ThemeTokenId id) const { impl_->ApplyPaneBackgroundToken(id); return self(); }
    Derived& IndicatorColorToken(ThemeTokenId id) const { impl_->ApplyIndicatorColorToken(id); return self(); }
    Derived& DropdownBackgroundToken(ThemeTokenId id) const { impl_->ApplyDropdownBackgroundToken(id); return self(); }
    Derived& SelectedItemBackgroundToken(ThemeTokenId id) const { impl_->ApplySelectedItemBackgroundToken(id); return self(); }
    Derived& FillColorToken(ThemeTokenId id) const { impl_->ApplyFillColorToken(id); return self(); }
    Derived& TrackColorToken(ThemeTokenId id) const { impl_->ApplyTrackColorToken(id); return self(); }
    Derived& ActiveTrackColorToken(ThemeTokenId id) const { impl_->ApplyActiveTrackColorToken(id); return self(); }
    Derived& ThumbColorToken(ThemeTokenId id) const { impl_->ApplyThumbColorToken(id); return self(); }
    Derived& OnColorToken(ThemeTokenId id) const { impl_->ApplyOnColorToken(id); return self(); }
    Derived& OffColorToken(ThemeTokenId id) const { impl_->ApplyOffColorToken(id); return self(); }
    Derived& KnobColorToken(ThemeTokenId id) const { impl_->ApplyKnobColorToken(id); return self(); }
    Derived& CheckedBackgroundToken(ThemeTokenId id) const { impl_->ApplyCheckedBackgroundToken(id); return self(); }
    Derived& AccentColorToken(ThemeTokenId id) const { impl_->ApplyAccentColorToken(id); return self(); }
    Derived& ActiveColorToken(ThemeTokenId id) const { impl_->ApplyActiveColorToken(id); return self(); }
    Derived& UnderlineColorToken(ThemeTokenId id) const { impl_->ApplyUnderlineColorToken(id); return self(); }
    Derived& ActiveUnderlineColorToken(ThemeTokenId id) const { impl_->ApplyActiveUnderlineColorToken(id); return self(); }
    Derived& ActiveTabBackgroundToken(ThemeTokenId id) const { impl_->ApplyActiveTabBackgroundToken(id); return self(); }
    Derived& InactiveTabBackgroundToken(ThemeTokenId id) const { impl_->ApplyInactiveTabBackgroundToken(id); return self(); }
    Derived& GridLineBrushToken(ThemeTokenId id) const { impl_->ApplyGridLineBrushToken(id); return self(); }
    Derived& TitleColorToken(ThemeTokenId id) const { impl_->ApplyTitleColorToken(id); return self(); }
    Derived& MessageColorToken(ThemeTokenId id) const { impl_->ApplyMessageColorToken(id); return self(); }
    Derived& CaretColorToken(ThemeTokenId id) const { impl_->ApplyCaretColorToken(id); return self(); }
    Derived& Background(D2D1_COLOR_F c) const { impl_->ApplyBackground(c); return self(); }
    Derived& HoverBackground(D2D1_COLOR_F c) const { impl_->ApplyHoverBackground(c); return self(); }
    Derived& PressedBackground(D2D1_COLOR_F c) const { impl_->ApplyPressedBackground(c); return self(); }
    Derived& BorderBrush(D2D1_COLOR_F c) const { impl_->ApplyBorderBrush(c); return self(); }
    Derived& Color(D2D1_COLOR_F c) const { impl_->ApplyColor(c); return self(); }
    Derived& Foreground(D2D1_COLOR_F color) const { if constexpr (requires { impl_->ApplyColor(color); }) impl_->ApplyColor(color); return self(); }
    Derived& Placeholder(const std::string& placeholder) const { impl_->ApplyPlaceholder(placeholder); return self(); }
    Derived& FontFamily(const std::string& font) const { impl_->ApplyFontFamily(font); return self(); }
    Derived& FontWeight(CUI::FontWeight weight) const { impl_->ApplyFontWeight(weight); return self(); }
    Derived& FontStyle(CUI::FontStyle style) const { impl_->ApplyFontStyle(style); return self(); }
    Derived& FontStretch(CUI::FontStretch stretch) const { impl_->ApplyFontStretch(stretch); return self(); }
    Derived& IsUnderline(bool underline) const { impl_->ApplyIsUnderline(underline); return self(); }
    Derived& IsStrikethrough(bool strikethrough) const { impl_->ApplyIsStrikethrough(strikethrough); return self(); }
    Derived& ToolTipMaxWidth(float width) const { impl_->ApplyToolTipMaxWidth(width); return self(); }
    Derived& ToolTipAutoHideMs(int ms) const { impl_->ApplyToolTipAutoHideMs(ms); return self(); }
    Derived& Icon(const std::string& icon) const { impl_->ApplyIcon(icon); return self(); }
    Derived& Parent(UIElement* parent) const { impl_->ApplyParent(parent); return self(); }
    Derived& AnimationHost(UIElement* host) const { impl_->ApplyAnimationHost(host); return self(); }
    Derived& Bounds(const Rect& bounds) const { impl_->ApplyBounds(bounds); return self(); }
    Derived& Command(std::shared_ptr<Command> command) const { impl_->ApplyCommand(command); return self(); }
    Derived& ComposeOpacity(float opacity) const { impl_->ApplyComposeOpacity(opacity); return self(); }
    Derived& ComposeOffset(float x, float y) const { impl_->ApplyComposeOffset(x, y); return self(); }


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
    /// 交给以基类 shared_ptr 收口的入口（UIElement、NavigationViewItemBase 等，由下面泛型转换统一覆盖）
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
