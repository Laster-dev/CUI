#pragma once

#include "Binding.h"
#include "Observable.h"
#include "State.h"
#include "ValueConverter.h"

#include <functional>
#include <memory>
#include <utility>

namespace CUI {

class UIElement;

/**
 * @brief 绑定属性的具体承载容器。
 * 它持有控件的 Getter/Setter 映射，并管理数据绑定（Data Binding）的连接、解绑以及多向数据流。
 * @tparam T 属性所代表的值类型。
 */
template<typename T>
class BindableProperty final {
public:
    using Getter = std::function<T()>;
    using Setter = std::function<void(const T&)>;

    BindableProperty(Object& owner, PropertyId propertyId, Getter getter, Setter setter)
        : m_owner(owner), m_propertyId(propertyId), m_getter(std::move(getter)), m_setter(std::move(setter)) {}

    ~BindableProperty() { Unbind(); }
    BindableProperty(const BindableProperty&) = delete;
    BindableProperty& operator=(const BindableProperty&) = delete;

    /**
     * @brief 获取当前属性的值。
     */
    T Get() const { return m_getter(); }

    /**
     * @brief 设置属性的新值（若为单向绑定且已被绑定则拦截写入，以防污染源数据）。
     */
    bool Set(const T& value) {
        if (m_isBound && m_mode == BindingMode::OneWay) return false;
        m_setter(value);
        return true;
    }

    /**
     * @brief 绑定到一个 Observable 可观察数据源。
     * @param value 共享指针包装的 Observable 实例。
     * @param mode 绑定模式（OneWay 单向, TwoWay 双向, OneTime 一次性）。
     */
    void Bind(const std::shared_ptr<Observable<T>>& value, BindingMode mode = BindingMode::TwoWay) {
        Unbind();
        if (!value) return;

        m_isBound = true;
        m_mode = mode;
        ApplySourceValue(value->Get());
        if (mode == BindingMode::OneTime) return;

        // 连接数据源的值改变事件
        const EventId sourceConnection = value->OnChanged().Connect([this](const T& updated) {
            ApplySourceValue(updated);
        });
        m_disconnectSource = [value, sourceConnection]() {
            value->OnChanged().Disconnect(sourceConnection);
        };
        
        // 如果是双向绑定，还要监听控件自身属性变更事件，并将其反写回数据源
        if (mode == BindingMode::TwoWay) {
            m_writeBack = [value](const T& updated) { value->Set(updated); };
            ConnectTarget();
        }
    }

    /**
     * @brief 绑定到一个 State 状态（State 为包装了 Observable 的轻量级容器）。
     */
    void Bind(const CUI::State<T>& value, BindingMode mode = BindingMode::TwoWay) {
        Bind(value.Share(), mode);
    }

    /**
     * @brief 带值转换器（ValueConverter）的属性绑定。
     * 允许数据源类型 TSource 与控件属性类型 T 发生类型转换（例如：bool 转 Visibility）。
     */
    template<typename TSource>
    void Bind(const std::shared_ptr<Observable<TSource>>& source,
              const std::shared_ptr<IValueConverter<TSource, T>>& converter,
              BindingMode mode = BindingMode::OneWay) {
        Unbind();
        if (!source || !converter) return;

        m_isBound = true;
        m_mode = mode;
        ApplySourceValue(converter->Convert(source->Get()));
        if (mode == BindingMode::OneTime) return;

        const EventId sourceConnection = source->OnChanged().Connect([this, converter](const TSource& updated) {
            ApplySourceValue(converter->Convert(updated));
        });
        m_disconnectSource = [source, sourceConnection]() {
            source->OnChanged().Disconnect(sourceConnection);
        };
        if (mode == BindingMode::TwoWay) {
            m_writeBack = [source, converter](const T& updated) {
                if (const auto converted = converter->ConvertBack(updated)) source->Set(*converted);
            };
            ConnectTarget();
        }
    }

    template<typename TSource>
    void Bind(const CUI::State<TSource>& source,
              const std::shared_ptr<IValueConverter<TSource, T>>& converter,
              BindingMode mode = BindingMode::OneWay) {
        Bind(source.Share(), converter, mode);
    }

    /**
     * @brief 断开并释放当前的所有绑定连接。
     */
    void Unbind() {
        if (m_disconnectSource) m_disconnectSource();
        if (m_targetConnection != 0) m_owner.OnPropertyIdChanged().Disconnect(m_targetConnection);
        m_disconnectSource = {};
        m_writeBack = {};
        m_targetConnection = 0;
        m_isBound = false;
        m_mode = BindingMode::OneWay;
        m_updating = false;
    }

    bool IsBound() const { return m_isBound; }
    bool IsUpdating() const { return m_updating; }
    BindingMode GetBindingMode() const { return m_mode; }

private:
    // 监听控件对应的 OnPropertyIdChanged 信号，建立反向同步信道
    void ConnectTarget() {
        m_targetConnection = m_owner.OnPropertyIdChanged().Connect([this](PropertyId changed, const Value&) {
            if (changed == m_propertyId && !m_updating && m_writeBack) {
                m_updating = true;
                m_writeBack(m_getter());
                m_updating = false;
            }
        });
    }

    // 更新数据，带有更新锁保护（m_updating），避免双向循环同步导致死锁
    void ApplySourceValue(const T& value) {
        m_updating = true;
        m_setter(value);
        m_updating = false;
    }

    Object& m_owner;
    PropertyId m_propertyId = PropertyId::None;
    Getter m_getter;
    Setter m_setter;
    std::function<void()> m_disconnectSource;
    std::function<void(const T&)> m_writeBack;
    EventId m_targetConnection = 0;
    BindingMode m_mode = BindingMode::OneWay;
    bool m_isBound = false;
    bool m_updating = false;
};

/**
 * @brief 暴露在控件接口中的绑定属性代理（Dsl 指针）。
 * 它是轻量级智能句柄，重载了 -> 运算符以访问底层的 BindableProperty，并确保懒加载初始化。
 * @tparam T 属性值类型
 * @tparam Id 对应的 PropertyId 枚举值
 */
template<typename T, PropertyId Id>
class PropertyRef final {
public:
    PropertyRef() = default;

    // 绑定该属性到控件的所有者所有权中
    void Initialize(UIElement& owner) { m_owner = &owner; }
    
    // 隐式重载，在第一次点出属性名时在 UIElement 内部自动懒加载构造 BindableProperty 并返回指针
    BindableProperty<T>* operator->();
    const BindableProperty<T>* operator->() const;

    PropertyRef& operator=(const T& value) { Set(value); return *this; }
    operator T() const { return Get(); }

    void Bind(const std::shared_ptr<Observable<T>>& value, BindingMode mode = BindingMode::TwoWay);
    void Bind(const CUI::State<T>& value, BindingMode mode = BindingMode::TwoWay);

    template<typename TSource>
    void Bind(const std::shared_ptr<Observable<TSource>>& source,
              const std::shared_ptr<IValueConverter<TSource, T>>& converter,
              BindingMode mode = BindingMode::OneWay);

    template<typename TSource>
    void Bind(const CUI::State<TSource>& source,
              const std::shared_ptr<IValueConverter<TSource, T>>& converter,
              BindingMode mode = BindingMode::OneWay);

    void Unbind();
    bool Set(const T& value);
    T Get() const;
    bool IsBound() const;
    bool IsUpdating() const;

private:
    UIElement* m_owner = nullptr;
};

} // namespace CUI
