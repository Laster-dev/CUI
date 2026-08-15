#pragma once

#include "BindableProperty.h"
#include "Binding.h"
#include "Event.h"
#include "Observable.h"
#include "PropertyId.h"
#include "State.h"
#include "ValueConverter.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <initializer_list>
#include <type_traits>
#include <utility>

namespace CUI {

class UIElement;

// 属性元数据影响标记
namespace PropertyAffects {
    inline constexpr uint32_t None           = 0;
    inline constexpr uint32_t AffectsMeasure  = 1 << 0;
    inline constexpr uint32_t AffectsArrange  = 1 << 1;
    inline constexpr uint32_t AffectsRender   = 1 << 2;
    inline constexpr uint32_t AffectsHitTest  = 1 << 3;
    inline constexpr uint32_t AffectsDataView = 1 << 4;
}

/**
 * @brief 统一可读写、可绑定属性模型 (Property<T>)
 * 适用于控件状态（如 Width, IsEnabled, Foreground, SelectedIndex 等）
 */
template<typename T, PropertyId Id = PropertyId::None>
class Property final {
public:
    Property() = default;
    void Initialize(UIElement& owner) { m_ref.Initialize(owner); }

    Property& operator=(const T& value) {
        m_ref.Set(value);
        return *this;
    }

    operator T() const { return m_ref.Get(); }
    T Get() const { return m_ref.Get(); }
    bool Set(const T& value) { return m_ref.Set(value); }

    void Bind(const std::shared_ptr<Observable<T>>& value, BindingMode mode = BindingMode::TwoWay) {
        m_ref.Bind(value, mode);
    }
    void Bind(const CUI::State<T>& value, BindingMode mode = BindingMode::TwoWay) {
        m_ref.Bind(value, mode);
    }

    template<typename TSource>
    void Bind(const std::shared_ptr<Observable<TSource>>& source,
              const std::shared_ptr<IValueConverter<TSource, T>>& converter,
              BindingMode mode = BindingMode::OneWay) {
        m_ref.Bind(source, converter, mode);
    }

    template<typename TSource>
    void Bind(const CUI::State<TSource>& source,
              const std::shared_ptr<IValueConverter<TSource, T>>& converter,
              BindingMode mode = BindingMode::OneWay) {
        m_ref.Bind(source, converter, mode);
    }

    void Unbind() { m_ref.Unbind(); }
    bool IsBound() const { return m_ref.IsBound(); }
    bool IsUpdating() const { return m_ref.IsUpdating(); }

    BindableProperty<T>* operator->() { return m_ref.operator->(); }
    const BindableProperty<T>* operator->() const { return m_ref.operator->(); }

private:
    PropertyRef<T, Id> m_ref;
};

/**
 * @brief 只读属性模型 (ReadOnlyProperty<T>)
 * 适用于布局、平台或控件计算值（如 Bounds, DesiredSize, HWND, RowCount 等）
 */
template<typename T>
class ReadOnlyProperty final {
public:
    using Getter = std::function<T()>;

    ReadOnlyProperty() = default;
    explicit ReadOnlyProperty(Getter getter) : m_getter(std::move(getter)) {}

    void Initialize(Getter getter) { m_getter = std::move(getter); }

    ReadOnlyProperty(const ReadOnlyProperty&) = delete;
    ReadOnlyProperty& operator=(const ReadOnlyProperty&) = delete;

    operator T() const { return Get(); }
    T Get() const { return m_getter ? m_getter() : T{}; }

    struct ArrowProxy {
        T value;
        const T* operator->() const { return &value; }
    };
    ArrowProxy operator->() const { return ArrowProxy{ Get() }; }

private:
    Getter m_getter;
};

/**
 * @brief 回调属性模型 (CallbackProperty<Signature>)
 * 适用于作者级事件回调槽位（如 OnClick, OnDraw, OnCanvasMouseDown 等）
 */
template<typename Signature>
class CallbackProperty;

template<typename ReturnType, typename... Args>
class CallbackProperty<ReturnType(Args...)> final {
public:
    using Handler = std::function<ReturnType(Args...)>;

    CallbackProperty() = default;

    CallbackProperty& operator=(Handler handler) {
        m_handler = std::move(handler);
        return *this;
    }

    CallbackProperty& operator=(std::nullptr_t) {
        m_handler = nullptr;
        return *this;
    }

    ReturnType operator()(Args... args) const {
        return Invoke(std::forward<Args>(args)...);
    }

    ReturnType Invoke(Args... args) const {
        if constexpr (std::is_void_v<ReturnType>) {
            if (m_handler) m_handler(std::forward<Args>(args)...);
            for (const auto& kv : m_subscribers) {
                if (kv.second) kv.second(std::forward<Args>(args)...);
            }
        } else {
            if (m_handler) return m_handler(std::forward<Args>(args)...);
            return ReturnType{};
        }
    }

    EventId Connect(Handler handler) {
        if (!handler) return 0;
        EventId id = ++m_nextId;
        m_subscribers.push_back({id, std::move(handler)});
        return id;
    }

    EventId Subscribe(Handler handler) {
        return Connect(std::move(handler));
    }

    void Disconnect(EventId id) {
        m_subscribers.erase(
            std::remove_if(m_subscribers.begin(), m_subscribers.end(),
                           [id](const auto& pair) { return pair.first == id; }),
            m_subscribers.end());
    }

    CallbackProperty& operator()() { return *this; }
    const CallbackProperty& operator()() const { return *this; }

    explicit operator bool() const { return static_cast<bool>(m_handler) || !m_subscribers.empty(); }
    bool HasHandler() const { return static_cast<bool>(m_handler) || !m_subscribers.empty(); }
    const Handler& GetHandler() const { return m_handler; }
    operator Handler() const { return m_handler; }

private:
    Handler m_handler;
    std::vector<std::pair<EventId, Handler>> m_subscribers;
    EventId m_nextId = 0;
};

/**
 * @brief 集合属性模型 (CollectionProperty<T>)
 * 适用于结构、数据或选择集合（如 Children, Items, Rows, Columns, SelectedIndices 等）
 */
template<typename T>
class CollectionProperty final {
public:
    using Getter = std::function<const std::vector<T>&()>;
    using Setter = std::function<void(const std::vector<T>&)>;

    CollectionProperty() = default;
    CollectionProperty(Getter getter, Setter setter)
        : m_getter(std::move(getter)), m_setter(std::move(setter)) {}

    void Initialize(Getter getter, Setter setter) {
        m_getter = std::move(getter);
        m_setter = std::move(setter);
    }

    CollectionProperty& operator=(const std::vector<T>& items) {
        if (m_setter) m_setter(items);
        return *this;
    }

    CollectionProperty& operator=(std::initializer_list<T> items) {
        if (m_setter) m_setter(std::vector<T>(items));
        return *this;
    }

    const std::vector<T>& Get() const {
        static const std::vector<T> s_empty;
        return m_getter ? m_getter() : s_empty;
    }

    operator const std::vector<T>&() const { return Get(); }

    auto begin() const { return Get().begin(); }
    auto end() const { return Get().end(); }
    size_t size() const { return Get().size(); }
    bool empty() const { return Get().empty(); }
    const T& operator[](size_t index) const { return Get()[index]; }

private:
    Getter m_getter;
    Setter m_setter;
};

/**
 * @brief 值语义作者句柄 (ElementRef<T>)
 * 内部持有 std::shared_ptr<T>，提供自然值语义与框架互操作
 */
template <typename T>
class ElementRef {
public:
    ElementRef() : m_ptr(std::make_shared<T>()) {}
    ElementRef(std::nullptr_t) : m_ptr(nullptr) {}
    ElementRef(std::shared_ptr<T> ptr) : m_ptr(std::move(ptr)) {}
    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    ElementRef(std::shared_ptr<U> ptr) : m_ptr(std::move(ptr)) {}
    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    ElementRef(const ElementRef<U>& other) : m_ptr(other.Shared()) {}

    // 指针操作
    T* operator->() const { return m_ptr.get(); }
    T& operator*() const { return *m_ptr; }
    T* get() const { return m_ptr.get(); }
    T* Native() const { return m_ptr.get(); }
    std::shared_ptr<T> Shared() const { return m_ptr; }

    // 逻辑判空
    explicit operator bool() const { return static_cast<bool>(m_ptr); }
    bool operator==(const ElementRef& other) const { return m_ptr == other.m_ptr; }
    bool operator!=(const ElementRef& other) const { return m_ptr != other.m_ptr; }
    bool operator==(std::nullptr_t) const { return m_ptr == nullptr; }
    bool operator!=(std::nullptr_t) const { return m_ptr != nullptr; }

    // 隐式互操作转换
    operator std::shared_ptr<T>() const { return m_ptr; }
    operator std::shared_ptr<UIElement>() const { return m_ptr; }

    std::shared_ptr<T> Build() const { return m_ptr; }

protected:
    std::shared_ptr<T> m_ptr;
};

} // namespace CUI
