#pragma once
#include <string>
#include <variant>
#include <d2d1.h>

namespace CUI {

std::wstring Utf8ToUtf16(const std::string& str);
std::string Utf16ToUtf8(const std::wstring& wstr);

struct Thickness {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;

    Thickness() = default;
    Thickness(float uniform) : left(uniform), top(uniform), right(uniform), bottom(uniform) {}
    Thickness(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b) {}

    static Thickness Parse(const std::string& str);
};

struct Size {
    float width = 0.0f;
    float height = 0.0f;

    Size() = default;
    Size(float w, float h) : width(w), height(h) {}
};

struct Point {
    float x = 0.0f;
    float y = 0.0f;

    Point() = default;
    Point(float x_, float y_) : x(x_), y(y_) {}
};

struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    Rect() = default;
    Rect(float x_, float y_, float w, float h) : x(x_), y(y_), width(w), height(h) {}

    bool Contains(float px, float py) const {
        return px >= x && px <= x + width && py >= y && py <= y + height;
    }

    D2D1_RECT_F ToD2D() const {
        return D2D1::RectF(x, y, x + width, y + height);
    }
};

enum class Visibility {
    Visible,
    Hidden,
    Collapsed
};

enum class Orientation {
    Horizontal,
    Vertical
};

enum class Alignment {
    Start,
    Center,
    End,
    Stretch
};

class Value {
public:
    enum class Type {
        None,
        Bool,
        Int,
        Float,
        String,
        Color,
        Thickness,
        Pointer
    };

    Value() : m_type(Type::None) {}
    Value(bool v) : m_type(Type::Bool), m_val(v) {}
    Value(int v) : m_type(Type::Int), m_val(v) {}
    Value(float v) : m_type(Type::Float), m_val(v) {}
    Value(const std::string& v) : m_type(Type::String), m_val(v) {}
    Value(const char* v) : m_type(Type::String), m_val(std::string(v)) {}
    Value(D2D1_COLOR_F v) : m_type(Type::Color), m_val(v) {}
    Value(Thickness v) : m_type(Type::Thickness), m_val(v) {}
    Value(void* v) : m_type(Type::Pointer), m_val(v) {}

    Type GetType() const { return m_type; }
    bool IsEmpty() const { return m_type == Type::None; }

    bool AsBool(bool def = false) const;
    int AsInt(int def = 0) const;
    float AsFloat(float def = 0.0f) const;
    std::string AsString(const std::string& def = "") const;
    D2D1_COLOR_F AsColor(D2D1_COLOR_F def = D2D1::ColorF(D2D1::ColorF::White)) const;
    Thickness AsThickness(Thickness def = {}) const;
    void* AsPointer(void* def = nullptr) const;

    static D2D1_COLOR_F ParseColor(const std::string& str);
    static Value ParseAuto(const std::string& str);

private:
    Type m_type = Type::None;
    std::variant<std::monostate, bool, int, float, std::string, D2D1_COLOR_F, Thickness, void*> m_val;
};

} // namespace CUI
