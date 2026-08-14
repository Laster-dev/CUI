#pragma once

#include "Binding.h"
#include "Observable.h"

#include <functional>
#include <memory>
#include <utility>

namespace CUI {

template<typename T>
class BindableProperty final {
public:
    using Getter = std::function<T()>;
    using Setter = std::function<void(const T&)>;

    BindableProperty(Object& owner, PropertyId propertyId, Getter getter, Setter setter)
        : m_owner(owner),
          m_propertyId(propertyId),
          m_getter(std::move(getter)),
          m_setter(std::move(setter)) {}

    ~BindableProperty() {
        Unbind();
    }

    BindableProperty(const BindableProperty&) = delete;
    BindableProperty& operator=(const BindableProperty&) = delete;

    const T Get() const {
        return m_getter();
    }

    void Set(const T& value) {
        m_setter(value);
    }

    void Bind(const std::shared_ptr<Observable<T>>& value,
              BindingMode mode = BindingMode::TwoWay) {
        Unbind();
        if (!value) {
            return;
        }

        m_source = value;
        m_mode = mode;
        ApplySourceValue(value->Get());

        if (mode == BindingMode::OneTime) {
            return;
        }

        m_sourceConnection = value->OnChanged().Connect([this](const T& updated) {
            ApplySourceValue(updated);
        });

        if (mode == BindingMode::TwoWay) {
            m_targetConnection = m_owner.OnPropertyIdChanged().Connect(
                [this](PropertyId changed, const Value&) {
                    if (changed == m_propertyId && !m_updating && m_source) {
                        m_updating = true;
                        m_source->Set(m_getter());
                        m_updating = false;
                    }
                });
        }
    }

    void Unbind() {
        if (m_source && m_sourceConnection != 0) {
            m_source->OnChanged().Disconnect(m_sourceConnection);
        }
        if (m_targetConnection != 0) {
            m_owner.OnPropertyIdChanged().Disconnect(m_targetConnection);
        }
        m_source.reset();
        m_sourceConnection = 0;
        m_targetConnection = 0;
        m_mode = BindingMode::OneWay;
        m_updating = false;
    }

    bool IsBound() const {
        return static_cast<bool>(m_source);
    }

    bool IsUpdating() const {
        return m_updating;
    }

private:
    void ApplySourceValue(const T& value) {
        m_updating = true;
        m_setter(value);
        m_updating = false;
    }

    Object& m_owner;
    PropertyId m_propertyId = PropertyId::None;
    Getter m_getter;
    Setter m_setter;
    std::shared_ptr<Observable<T>> m_source;
    EventId m_sourceConnection = 0;
    EventId m_targetConnection = 0;
    BindingMode m_mode = BindingMode::OneWay;
    bool m_updating = false;
};


} // namespace CUI
