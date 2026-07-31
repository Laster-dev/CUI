#pragma once
#include "Object.h"
#include <memory>
#include <string>

namespace CUI {

enum class BindingMode {
    OneWay,
    TwoWay,
    OneTime
};

class Binding {
public:
    Binding(std::shared_ptr<Object> target, std::string targetProperty,
            std::shared_ptr<Object> source, std::string sourceProperty,
            BindingMode mode = BindingMode::OneWay);
    ~Binding();

    void UpdateTarget();
    void UpdateSource();

private:
    std::weak_ptr<Object> m_target;
    std::string m_targetProperty;
    std::weak_ptr<Object> m_source;
    std::string m_sourceProperty;
    BindingMode m_mode;

    EventId m_sourceConnId = 0;
    EventId m_targetConnId = 0;
    bool m_isUpdating = false;
};

} // namespace CUI
