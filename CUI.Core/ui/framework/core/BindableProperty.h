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

    T Get() const { return m_getter(); }

    bool Set(const T& value) {
        if (m_isBound && m_mode == BindingMode::OneWay) return false;
        m_setter(value);
        return true;
    }

    void Bind(const std::shared_ptr<Observable<T>>& value, BindingMode mode = BindingMode::TwoWay) {
        Unbind();
        if (!value) return;

        m_isBound = true;
        m_mode = mode;
        ApplySourceValue(value->Get());
        if (mode == BindingMode::OneTime) return;

        const EventId sourceConnection = value->OnChanged().Connect([this](const T& updated) {
            ApplySourceValue(updated);
        });
        m_disconnectSource = [value, sourceConnection]() {
            value->OnChanged().Disconnect(sourceConnection);
        };
        if (mode == BindingMode::TwoWay) {
            m_writeBack = [value](const T& updated) { value->Set(updated); };
            ConnectTarget();
        }
    }

    void Bind(const CUI::State<T>& value, BindingMode mode = BindingMode::TwoWay) {
        Bind(value.Share(), mode);
    }

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
    void ConnectTarget() {
        m_targetConnection = m_owner.OnPropertyIdChanged().Connect([this](PropertyId changed, const Value&) {
            if (changed == m_propertyId && !m_updating && m_writeBack) {
                m_updating = true;
                m_writeBack(m_getter());
                m_updating = false;
            }
        });
    }

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

template<typename T, PropertyId Id>
class PropertyRef final {
public:
    PropertyRef() = default;

    void Initialize(UIElement& owner) { m_owner = &owner; }
    BindableProperty<T>* operator->();
    const BindableProperty<T>* operator->() const;

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
