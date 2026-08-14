#pragma once
#include <string>
#include <variant>
#include <d2d1.h>

namespace CUI {

// UTF-8 与 UTF-16 字符串互转辅助函数
std::wstring Utf8ToUtf16(const std::string& str);
std::string Utf16ToUtf8(const std::wstring& wstr);

/**
 * @brief 控件外边距或内边距（Margin / Padding）定义结构体。
 * 包含 Left, Top, Right, Bottom 四个方向的宽度。
 */
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

    // 从字符串解析 Thickness (例如 "10" 或 "10,20" 或 "10,5,10,5")
    static Thickness Parse(const std::string& str);
};

/**
 * @brief 表示 RGBA 颜色的结构体。
 * 支持与 Direct2D 中的 D2D1_COLOR_F 隐式转换。
 */
struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    Color() = default;
    Color(float r_, float g_, float b_, float a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}
    Color(const D2D1_COLOR_F& c) : r(c.r), g(c.g), b(c.b), a(c.a) {}

    // 隐式转换为 D2D1_COLOR_F，方便与 Direct2D API 交互
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

    // 解析 Hex 颜色代码（支持带/不带 #、包含全角 ＃ 的格式）
    static Color Hex(const std::string& hexStr);
    static Color FromHex(const std::string& hexStr) { return Hex(hexStr); }

    // 预定义的常用行业标准颜色
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

/**
 * @brief 二维尺寸结构体（宽度与高度）
 */
struct Size {
    float width = 0.0f;
    float height = 0.0f;

    Size() = default;
    Size(float w, float h) : width(w), height(h) {}
};

/**
 * @brief 二维点坐标结构体（X 与 Y 坐标）
 */
struct Point {
    float x = 0.0f;
    float y = 0.0f;

    Point() = default;
    Point(float x_, float y_) : x(x_), y(y_) {}
};

/**
 * @brief 表示矩形区域的结构体（X, Y, Width, Height）
 */
struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    Rect() = default;
    Rect(float x_, float y_, float w, float h) : x(x_), y(y_), width(w), height(h) {}

    // 检测点是否在矩形内部（包含边界）
    bool Contains(float px, float py) const {
        return px >= x && px <= x + width && py >= y && py <= y + height;
    }

    // 检测矩形是否为空
    bool IsEmpty() const {
        return width <= 0.0f || height <= 0.0f;
    }

    // 检测两个矩形是否相交
    bool Intersects(const Rect& other) const {
        if (IsEmpty() || other.IsEmpty()) return false;
        return x < other.x + other.width
            && x + width > other.x
            && y < other.y + other.height
            && y + height > other.y;
    }

    // 计算并返回包含当前矩形与另一个矩形的并集矩形
    Rect Union(const Rect& other) const {
        if (IsEmpty()) return other;
        if (other.IsEmpty()) return *this;

        float left = (x < other.x) ? x : other.x;
        float top = (y < other.y) ? y : other.y;
        float right = (x + width > other.x + other.width) ? (x + width) : (other.x + other.width);
        float bottom = (y + height > other.y + other.height) ? (y + height) : (other.y + other.height);
        return Rect(left, top, right - left, bottom - top);
    }

    // 向外或向内等距扩充矩形大小
    Rect Inflate(float amount) const {
        return Rect(x - amount, y - amount, width + amount * 2.0f, height + amount * 2.0f);
    }

    // 转换为 Direct2D 所需的 D2D1_RECT_F
    D2D1_RECT_F ToD2D() const {
        return D2D1::RectF(x, y, x + width, y + height);
    }
};

/**
 * @brief 控件的可见性状态。
 * Visible: 可见并参与布局计算。
 * Hidden: 隐藏但不释放空间（仍参与布局）。
 * Collapsed: 彻底隐藏并释放其所占用的布局空间。
 */
enum class Visibility {
    Visible,
    Hidden,
    Collapsed
};

/**
 * @brief 布局或滑块控件的朝向。
 */
enum class Orientation {
    Horizontal,
    Vertical
};

/**
 * @brief 对齐方向枚举。
 */
enum class Alignment {
    Start,
    Center,
    End,
    Stretch
};

/**
 * @brief CUI 框架中的异构 Variant 数据类。
 * 用于属性系统中传递任意类型的通用属性值。
 */
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

    // 将属性安全类型转换为对应基本类型
    bool AsBool(bool def = false) const;
    int AsInt(int def = 0) const;
    float AsFloat(float def = 0.0f) const;
    std::string AsString(const std::string& def = "") const;
    D2D1_COLOR_F AsColor(D2D1_COLOR_F def = D2D1::ColorF(D2D1::ColorF::White)) const;
    Thickness AsThickness(Thickness def = {}) const;
    void* AsPointer(void* def = nullptr) const;

    // 解析色彩字符串（Hex/Rgba 等）
    static D2D1_COLOR_F ParseColor(const std::string& str);
    
    // 自动检测并推断类型解析字符串
    static Value ParseAuto(const std::string& str);

private:
    Type m_type = Type::None;
    std::variant<std::monostate, bool, int, float, std::string, D2D1_COLOR_F, Thickness, void*> m_val;
};

} // namespace CUI
