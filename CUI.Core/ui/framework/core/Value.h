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

    bool operator==(const Thickness& o) const {
        return left == o.left && top == o.top && right == o.right && bottom == o.bottom;
    }
    bool operator!=(const Thickness& o) const { return !(*this == o); }

    static Thickness Parse(const std::string& str);
};

struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    Color() = default;
    Color(float r_, float g_, float b_, float a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}

    // Implicit conversion to D2D1_COLOR_F
    operator D2D1_COLOR_F() const {
        D2D1_COLOR_F c;
        c.r = r;
        c.g = g;
        c.b = b;
        c.a = a;
        return c;
    }

    static Color RGBA(float r, float g, float b, float a = 1.0f) {
        return Color(r, g, b, a);
    }

    static Color FromRGBA(float r, float g, float b, float a = 1.0f) {
        return Color(r, g, b, a);
    }

    static Color FromRGB(float r, float g, float b) {
        return Color(r, g, b, 1.0f);
    }

    static Color Hex(const std::string& hexStr);
    static Color FromHex(const std::string& hexStr) { return Hex(hexStr); }

    // Predefined industry standard colors
    static const Color Transparent;
    static const Color Black;
    static const Color White;
    static const Color Gray;
    static const Color LightGray;
    static const Color Red;
    static const Color Green;
    static const Color Blue;
    static const Color Yellow;
    static const Color Cyan;
    static const Color Magenta;
    static const Color Orange;
    static const Color Purple;
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

    bool IsEmpty() const {
        return width <= 0.0f || height <= 0.0f;
    }

    bool Intersects(const Rect& other) const {
        if (IsEmpty() || other.IsEmpty()) return false;
        return x < other.x + other.width
            && x + width > other.x
            && y < other.y + other.height
            && y + height > other.y;
    }

    Rect Union(const Rect& other) const {
        if (IsEmpty()) return other;
        if (other.IsEmpty()) return *this;

        float left = (x < other.x) ? x : other.x;
        float top = (y < other.y) ? y : other.y;
        float right = (x + width > other.x + other.width) ? (x + width) : (other.x + other.width);
        float bottom = (y + height > other.y + other.height) ? (y + height) : (other.y + other.height);
        return Rect(left, top, right - left, bottom - top);
    }

    Rect Inflate(float amount) const {
        return Rect(x - amount, y - amount, width + amount * 2.0f, height + amount * 2.0f);
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
