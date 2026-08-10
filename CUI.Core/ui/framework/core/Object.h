#pragma once
#include "Value.h"
#include "Event.h"
#include "PropertyId.h"
#include <memory>
#include <utility>
#include <vector>

namespace CUI {

class Object : public std::enable_shared_from_this<Object> {
public:
    Object() = default;
    virtual ~Object() = default;

    virtual const char* GetClassName() const { return "Object"; }

    virtual void SetProperty(PropertyId id, const Value& val);
    virtual Value GetProperty(PropertyId id) const;
    virtual bool HasProperty(PropertyId id) const;
    virtual std::vector<std::pair<PropertyId, Value>> SnapshotProperties() const { return {}; }

    Event<PropertyId, const Value&>& OnPropertyIdChanged() { return m_propertyIdChangedEvent; }

protected:
    void NotifyPropertyIdChanged(PropertyId id, const Value& val) {
        m_propertyIdChangedEvent.Invoke(id, val);
    }

private:
    Event<PropertyId, const Value&> m_propertyIdChangedEvent;
};

} // namespace CUI
