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
template<typename Derived, typename Impl>
class WidgetBase {
public:
    using ImplType = Impl;

    /// 无参构造：真实控件在堆上创建（构造即托管，无需用户 new）
    WidgetBase() : impl_(std::make_unique<Impl>()) {}

    /// 接管一个已存在的堆上控件
    explicit WidgetBase(Unique<Impl> impl) : impl_(std::move(impl)) {}

    WidgetBase(const WidgetBase&) = delete;
    WidgetBase& operator=(const WidgetBase&) = delete;
    WidgetBase(WidgetBase&&) = default;
    WidgetBase& operator=(WidgetBase&&) = default;

    /// 链式结束：把堆上控件的所有权交出去，句柄随即置空
    Unique<Impl> Build() { return std::move(impl_); }

    /// 隐式交出所有权，可省掉 .Build()：panel->AddChild(Button().Text("x"))
    operator Unique<Impl>() { return std::move(impl_); }

    /// 其余全部接口一律透传——句柄不需要、也绝不去重复实现它们
    Impl* operator->() const { return impl_.get(); }
    Impl& operator*() const { return *impl_; }
    Impl* Get() const { return impl_.get(); }

    // ---------------- 通用布局链式方法 ----------------
    Derived& Width(float v)          { impl_->SetWidth(v); return self(); }
    Derived& Height(float v)         { impl_->SetHeight(v); return self(); }
    Derived& MinWidth(float v)       { impl_->SetMinWidth(v); return self(); }
    Derived& MinHeight(float v)      { impl_->SetMinHeight(v); return self(); }
    Derived& MaxWidth(float v)       { impl_->SetMaxWidth(v); return self(); }
    Derived& MaxHeight(float v)      { impl_->SetMaxHeight(v); return self(); }
    Derived& Margin(float v)         { impl_->SetMargin(Thickness(v)); return self(); }
    Derived& Margin(const Thickness& t) { impl_->SetMargin(t); return self(); }
    Derived& Padding(float v)        { impl_->SetPadding(Thickness(v)); return self(); }
    Derived& Padding(const Thickness& t) { impl_->SetPadding(t); return self(); }

    // ---------------- 通用外观链式方法 ----------------
    Derived& Text(const std::string& v) { impl_->SetText(v); return self(); }
    Derived& FontSize(float v)          { impl_->SetFontSize(v); return self(); }
    Derived& ToolTip(const std::string& v) { impl_->SetToolTip(v); return self(); }
    Derived& Opacity(float v)           { impl_->SetOpacity(v); return self(); }
    Derived& Enabled(bool v)            { impl_->SetIsEnabled(v); return self(); }
    Derived& Align(Alignment a)         { impl_->SetAlign(a); return self(); }
    Derived& AlignHorizontal(Alignment a) { impl_->SetAlignHorizontal(a); return self(); }
    Derived& AlignVertical(Alignment a)   { impl_->SetAlignVertical(a); return self(); }

    /**
     * @brief 注册点击回调。
     * @note 不可命名为 OnClick：Impl 上有同名【数据成员】CallbackProperty OnClick，
     *       成员函数会遮蔽它，使内核既有 OnClick.Connect / OnClick().Invoke 失效。
     */
    Derived& Click(std::function<void(UIElement*)> handler) {
        impl_->OnClick.Connect(std::move(handler));
        return self();
    }

protected:
    Unique<Impl> impl_;

private:
    Derived& self() { return static_cast<Derived&>(*this); }
};

} // namespace Widgets
} // namespace CUI
