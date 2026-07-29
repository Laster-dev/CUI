#pragma once
#include "Value.h"
#include "Event.h"
#include <unordered_map>
#include <string>
#include <memory>

namespace CUI {

class Object : public std::enable_shared_from_this<Object> {
public:
    Object() = default;
    virtual ~Object() = default;

    virtual const char* GetClassName() const { return "Object"; }

    void SetProperty(const std::string& name, const Value& val);
    Value GetProperty(const std::string& name) const;
    bool HasProperty(const std::string& name) const;

    Event<const std::string&, const Value&>& OnPropertyChanged() { return m_propertyChangedEvent; }

private:
    std::unordered_map<std::string, Value> m_properties;
    Event<const std::string&, const Value&> m_propertyChangedEvent;
};

} // namespace CUI
