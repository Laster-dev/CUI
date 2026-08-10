#include "Object.h"

namespace CUI {

void Object::SetProperty(PropertyId /*id*/, const Value& /*val*/) {}

Value Object::GetProperty(PropertyId /*id*/) const {
    return Value();
}

bool Object::HasProperty(PropertyId /*id*/) const {
    return false;
}

} // namespace CUI
