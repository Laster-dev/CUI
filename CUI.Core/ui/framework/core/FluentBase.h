#pragma once
// ============================================================================
//  FluentBase.h —— WPF 式控件链式配置基类（CRTP，编译期零开销）
//
//  设计要点：
//  1. 【不是包装层】链式方法通过 CRTP 直接作用于控件自身并返回 Derived&，
//     运行期不产生任何额外对象——没有 Builder、没有 shared_ptr 句柄、没有套娃。
//  2. 【仍是真实子类】class Button : public FluentBase<Button, Control>
//     => Button 依然 is-a Control，既有代码与继承关系零破坏。
//  3. 【无参构造 + 链式配置】控件一律无参构造，所有参数走链式方法。
//  4. 【独占所有权】Build() 把配置完成的控件搬上堆，返回 Unique<Derived>
//     （即 std::unique_ptr），树内父子由 Panel 独占持有，父指针为裸指针。
//
//  命名说明：之所以叫 FluentBase 而非 Fluent，是因为 CUI::DSL::Fluent
//  已作为工厂命名空间存在，避免 using namespace 后同名二义。
// ============================================================================

#include "../controls/UIElement.h"

#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace CUI {

/// 独占所有权别名。控件树的父子关系用它表达：父独占子，子裸指针回指父。
template<typename T>
using Unique = std::unique_ptr<T>;

/**
 * @brief 为控件注入链式配置能力的 CRTP 基类。
 * @tparam Derived 具体控件类型（CRTP 参数），链式方法一律返回 Derived& 以保持类型不断链。
 * @tparam Base    该控件原本的基类（如 Control / Panel / UIElement）。
 */
template<typename Derived, typename Base = UIElement>
class FluentBase : public Base {
public:
    using Base::Base; // 继承基类构造函数（保持无参构造可用）

    /// 从基类引用安全回落到具体派生类型引用。
    Derived& self() { return static_cast<Derived&>(*this); }
    const Derived& self() const { return static_cast<const Derived&>(*this); }

    // ------------------------------------------------------------------ 尺寸
    Derived& Width(float v) { this->SetWidth(v); return self(); }
    Derived& Height(float v) { this->SetHeight(v); return self(); }
    Derived& MinWidth(float v) { this->SetMinWidth(v); return self(); }
    Derived& MinHeight(float v) { this->SetMinHeight(v); return self(); }
    Derived& MaxWidth(float v) { this->SetMaxWidth(v); return self(); }
    Derived& MaxHeight(float v) { this->SetMaxHeight(v); return self(); }
    /// 注意：不可命名为 Size —— Size 是布局结构体类型名，同名方法会遮蔽它，
    /// 导致 virtual Size Measure(Size) 等声明无法解析。
    Derived& SetSize(float w, float h) { this->SetWidth(w); this->SetHeight(h); return self(); }

    // -------------------------------------------------------------- 边距与填充
    Derived& Margin(const Thickness& v) { this->SetMargin(v); return self(); }
    Derived& Padding(const Thickness& v) { this->SetPadding(v); return self(); }

    // ------------------------------------------------------------------ 内容
    Derived& Text(const std::string& v) { this->SetText(v); return self(); }

    // ------------------------------------------------------------------ 事件
    /**
     * @brief 注册点击回调（链式）：btn.Click([](UIElement*){ ... })
     *
     * @note 本方法【不可】命名为 OnClick。UIElement 已有同名数据成员
     *       `CallbackProperty<void(UIElement*)> OnClick`，而 C++ 名字查找不区分
     *       数据成员与成员函数——一旦派生类声明同名成员函数，就会遮蔽该数据成员，
     *       导致内核既有写法 OnClick.Connect(h) 与 OnClick().Invoke(this) 全线编译失败。
     *       因此链式注册点取名为 Click（与 WPF 的 Button.Click 事件名一致）。
     */
    Derived& Click(std::function<void(UIElement*)> handler) {
        this->OnClick.Connect(std::move(handler));
        return self();
    }

    /// 访问点击事件对象本体（Connect / Invoke / 判空）。
    auto& ClickEvent() { return this->OnClick; }

    // ------------------------------------------------------------------ 终结
    /**
     * @brief 链式终点：把已配置好的控件搬上堆，交出独占所有权。
     * @return Unique<Derived> —— 可直接 std::move 进父容器。
     * @note 调用后原栈上对象已被移走，不可再使用（语义同 std::move）。
     */
    Unique<Derived> Build() { return std::make_unique<Derived>(std::move(self())); }
};

} // namespace CUI
