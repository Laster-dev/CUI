#pragma once
#include "Object.h"
#include "PropertyId.h"
#include <memory>

namespace CUI {

enum class BindingMode {
    OneWay,
    TwoWay,
    OneTime
};

class Binding {
public:
    Binding(std::shared_ptr<Object> target, PropertyId targetProperty,
            std::shared_ptr<Object> source, PropertyId sourceProperty,
            BindingMode mode = BindingMode::OneWay);
    ~Binding();

    void UpdateTarget();
    void UpdateSource();

private:
    std::weak_ptr<Object> m_target;
    PropertyId m_targetProperty = PropertyId::None;
    std::weak_ptr<Object> m_source;
    PropertyId m_sourceProperty = PropertyId::None;
    BindingMode m_mode;

    EventId m_sourceConnId = 0;
    EventId m_targetConnId = 0;
    bool m_isUpdating = false;
};

} // namespace CUI
