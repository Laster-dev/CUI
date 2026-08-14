#pragma once
#include "Object.h"
#include "PropertyId.h"
#include <memory>

namespace CUI {

enum class BindingMode {
    OneWay,//单向绑定：数据源更新时，目标属性会随之更新；但目标属性的修改不会反向影响数据源。
    TwoWay,//双向绑定：数据源和目标属性互相同步。修改任意一方，另一方都会更新。
    OneTime//一次性绑定：在绑定建立时把数据源的值赋给目标属性，但之后不再跟随更新。
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
