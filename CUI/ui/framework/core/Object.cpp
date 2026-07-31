#include "Object.h"

namespace CUI {

void Object::SetProperty(const std::string& name, const Value& val) {
    auto it = m_properties.find(name);
    bool changed = (it == m_properties.end());
    if (!changed) {
        // Compare string representation for change check
        if (it->second.AsString() != val.AsString()) {
            changed = true;
        }
    }

    m_properties[name] = val;

    if (changed) {
        m_propertyChangedEvent.Invoke(name, val);
    }
}

Value Object::GetProperty(const std::string& name) const {
    auto it = m_properties.find(name);
    if (it != m_properties.end()) {
        return it->second;
    }
    return Value();
}

bool Object::HasProperty(const std::string& name) const {
    return m_properties.find(name) != m_properties.end();
}

} // namespace CUI
