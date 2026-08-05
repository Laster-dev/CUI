#pragma once
#include "PropertyId.h"
#include "../core/Value.h"
#include <cstddef>
#include <cstdint>

namespace CUI {

class UIElement;

enum class PropertyKind : uint8_t {
    Bool,
    Int,
    Float,
    String,
    Color,
    Thickness,
    Enum,
    ThemeToken
};

struct PropertyDesc {
    PropertyId id = PropertyId::None;
    const char* displayName = nullptr;
    const char* category = nullptr;
    PropertyKind kind = PropertyKind::String;
    const char* const* enumOptions = nullptr;
    void (*get)(const UIElement* self, Value& out) = nullptr;
    void (*set)(UIElement* self, const Value& in) = nullptr;
};

struct PropertyDescSpan {
    const PropertyDesc* data = nullptr;
    size_t count = 0;
};

const PropertyDesc* FindPropertyDescById(PropertyId id);
const PropertyDesc* FindPropertyDescForElement(const UIElement* element, PropertyId id);

} // namespace CUI
