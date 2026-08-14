#pragma once
#include <string>
#include <variant>
#include <d2d1.h>

namespace CUI {

// UTF-8 与 UTF-16 字符串互转辅助函数
std::wstring Utf8ToUtf16(const std::string& str); // 将 UTF-8 转换为宽字符 UTF-16 字符串
std::string Utf16ToUtf8(const std::wstring& wstr); // 将 UTF-16 宽字符转换为 UTF-8 字符串

/**
 * @brief 控件外边距或内边距（Margin / Padding）定义结构体。
 * Left: 左侧大小。
 * Top: 顶侧大小。
 * Right: 右侧大小。
 * Bottom: 底侧大小。
 */
struct Thickness {
    float left = 0.0f;   // 左边距像素宽
    float top = 0.0f;    // 顶边距像素宽
    float right = 0.0f;  // 右边距像素宽
    float bottom = 0.0f; // 底边距像素宽

    Thickness() = default;
    Thickness(float uniform) : left(uniform), top(uniform), right(uniform), bottom(uniform) {} // 构造四边等厚度的边距
    Thickness(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b) {} // 构造具体数值的四边边距

    bool operator==(const Thickness& o) const { // 厚度相等性判断
        return left == o.left && top == o.top && right == o.right && bottom == o.bottom;
    }
    bool operator!=(const Thickness& o) const { return !(*this == o); } // 厚度不等性判断

    static Thickness Parse(const std::string& str); // 从字符串解析 Thickness (例如 "10" 或 "10,20")
};

/**
 * @brief 表示 RGBA 颜色的结构体。
 * 支持与 Direct2D 中的 D2D1_COLOR_F 隐式转换。
 */
struct Color {
    float r = 0.0f; // 红色分量 (0.0f - 1.0f)
    float g = 0.0f; // 绿色分量 (0.0f - 1.0f)
    float b = 0.0f; // 蓝色分量 (0.0f - 1.0f)
    float a = 1.0f; // 透明度分量 (0.0f - 1.0f)

    Color() = default;
    Color(float r_, float g_, float b_, float a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {} // 构造带透明度的 RGBA 颜色
    Color(const D2D1_COLOR_F& c) : r(c.r), g(c.g), b(c.b), a(c.a) {} // 从 Direct2D 颜色对象初始化

    operator D2D1_COLOR_F() const { // 隐式转换为 D2D1_COLOR_F，方便与 D2D 交互
        D2D1_COLOR_F c;
        c.r = r;
        c.g = g;
        c.b = b;
        c.a = a;
        return c;
    }

    static Color RGBA(float r, float g, float b, float a = 1.0f) { // RGBA 颜色静态工厂
        return Color(r, g, b, a);
    }

    static Color FromRGBA(float r, float g, float b, float a = 1.0f) { // 从浮点 RGBA 组建颜色
        return Color(r, g, b, a);
    }

    static Color FromRGB(float r, float g, float b) { // 从浮点 RGB 组建不透明颜色
        return Color(r, g, b, 1.0f);
    }

    static Color Hex(const std::string& hexStr); // 从十六进制 Hex 字符串解析颜色
    static Color FromHex(const std::string& hexStr) { return Hex(hexStr); } // 从十六进制 Hex 字符串解析颜色别名

    // 预定义的常用行业标准颜色
    static const Color Transparent; // 全透明颜色 (0,0,0,0)
    static const Color Black;       // 纯黑色 (0,0,0,1)
    static const Color White;       // 纯白色 (1,1,1,1)
    static const Color Gray;        // 灰色 (0.5,0.5,0.5,1)
    static const Color LightGray;   // 浅灰色 (0.8,0.8,0.8,1)
    static const Color Red;         // 纯红色 (1,0,0,1)
    static const Color Green;       // 纯绿色 (0,1,0,1)
    static const Color Blue;        // 纯蓝色 (0,0,1,1)
    static const Color Yellow;      // 纯黄色 (1,1,0,1)
    static const Color Cyan;        // 青色 (0,1,1,1)
    static const Color Magenta;     // 洋红色 (1,0,1,1)
    static const Color Orange;      // 橙色 (1,0.6,0,1)
    static const Color Purple;      // 紫色 (0.5,0,0.5,1)
};

/**
 * @brief 二维尺寸结构体（宽度与高度）
 */
struct Size {
    float width = 0.0f;  // 逻辑宽度
    float height = 0.0f; // 逻辑高度

    Size() = default;
    Size(float w, float h) : width(w), height(h) {} // 构造尺寸
};

/**
 * @brief 二维点坐标结构体（X 与 Y 坐标）
 */
struct Point {
    float x = 0.0f; // 逻辑水平坐标值
    float y = 0.0f; // 逻辑垂直坐标值

    Point() = default;
    Point(float x_, float y_) : x(x_), y(y_) {} // 构造点坐标
};

/**
 * @brief 表示矩形区域的结构体（X, Y, Width, Height）
 */
struct Rect {
    float x = 0.0f;      // 逻辑左边沿横坐标值
    float y = 0.0f;      // 逻辑顶边沿纵坐标值
    float width = 0.0f;  // 逻辑宽度像素值
    float height = 0.0f; // 逻辑高度像素值

    Rect() = default;
    Rect(float x_, float y_, float w, float h) : x(x_), y(y_), width(w), height(h) {} // 构造矩形边界

    bool Contains(float px, float py) const { // 点碰撞测试判定
        return px >= x && px <= x + width && py >= y && py <= y + height;
    }

    bool IsEmpty() const { // 面积是否为空
        return width <= 0.0f || height <= 0.0f;
    }

    bool Intersects(const Rect& other) const { // 重叠交叉性判定
        if (IsEmpty() || other.IsEmpty()) return false;
        return x < other.x + other.width
            && x + width > other.x
            && y < other.y + other.height
            && y + height > other.y;
    }

    Rect Union(const Rect& other) const { // 两个矩形进行求并合并
        if (IsEmpty()) return other;
        if (other.IsEmpty()) return *this;

        float left = (x < other.x) ? x : other.x;
        float top = (y < other.y) ? y : other.y;
        float right = (x + width > other.x + other.width) ? (x + width) : (other.x + other.width);
        float bottom = (y + height > other.y + other.height) ? (y + height) : (other.y + other.height);
        return Rect(left, top, right - left, bottom - top);
    }

    Rect Inflate(float amount) const { // 四边向外垫宽以扩大矩形面积
        return Rect(x - amount, y - amount, width + amount * 2.0f, height + amount * 2.0f);
    }

    D2D1_RECT_F ToD2D() const { // 转换为 D2D 绘图 API 认知的结构体
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
    Visible,   // 可见：在屏幕上绘制显示，并且其 desiredSize 会占用布局排版空间
    Hidden,    // 隐藏：在屏幕上不可见，但其在布局树中依然保留其排版大小并参与测算
    Collapsed  // 折叠：彻底隐藏，不在屏幕上绘制，且排版系统会将其尺寸视为 0 不计入位置
};

/**
 * @brief 布局或滑块控件的朝向。
 * Horizontal: 水平。
 * Vertical: 垂直。
 */
enum class Orientation {
    Horizontal, // 水平方向
    Vertical    // 垂直方向
};

/**
 * @brief 对齐方向枚举。
 * Start: 起始端。
 * Center: 居中。
 * End: 终端。
 * Stretch: 拉伸。
 */
enum class Alignment {
    Start,   // 起始端对齐：左对齐或顶对齐
    Center,  // 居中对齐
    End,     // 终端对齐：右对齐或底对齐
    Stretch  // 强制拉伸：填满父容器所分配的全部空间
};

/**
 * @brief CUI 框架中的异构 Variant 数据类。
 * 用于属性系统中传递任意类型的通用属性值。
 */
class Value {
public:
    /**
     * @brief 数据存储的底层类型枚举。
     * None: 空。
     * Bool: 布尔。
     * Int: 整数。
     * Float: 浮点。
     * String: 字符串。
     * Color: 颜色。
     * Thickness: 边距。
     * Pointer: 弱引用原始指针。
     */
    enum class Type {
        None,      // 无类型（空值）
        Bool,      // 布尔型
        Int,       // 整型
        Float,     // 浮点型
        String,    // 字符串型
        Color,     // 颜色型
        Thickness, // 边距厚度型
        Pointer    // 指针弱引用型
    };

    Value() : m_type(Type::None) {} // 构造空值
    Value(bool v) : m_type(Type::Bool), m_val(v) {} // 从布尔型初始化
    Value(int v) : m_type(Type::Int), m_val(v) {} // 从整型初始化
    Value(float v) : m_type(Type::Float), m_val(v) {} // 从浮点型初始化
    Value(const std::string& v) : m_type(Type::String), m_val(v) {} // 从字符串强拷贝初始化
    Value(const char* v) : m_type(Type::String), m_val(std::string(v)) {} // 从 C 字符字面量初始化
    Value(D2D1_COLOR_F v) : m_type(Type::Color), m_val(v) {} // 从 D2D 颜色结构初始化
    Value(Thickness v) : m_type(Type::Thickness), m_val(v) {} // 从 Thickness 结构初始化
    Value(void* v) : m_type(Type::Pointer), m_val(v) {} // 从裸指针初始化

    Type GetType() const { return m_type; } // 获取当前 Variant 存储的数据类型
    bool IsEmpty() const { return m_type == Type::None; } // 判定当前是否是空值状态

    bool AsBool(bool def = false) const; // 转换为布尔值，失败返回默认值
    int AsInt(int def = 0) const; // 转换为整型数值，失败返回默认值
    float AsFloat(float def = 0.0f) const; // 转换为浮点值，失败返回默认值
    std::string AsString(const std::string& def = "") const; // 转换为字符串，失败返回默认值
    D2D1_COLOR_F AsColor(D2D1_COLOR_F def = D2D1::ColorF(D2D1::ColorF::White)) const; // 转换为 D2D 颜色结构，失败返回默认色
    Thickness AsThickness(Thickness def = {}) const; // 转换为 Thickness 结构，失败返回默认边距
    void* AsPointer(void* def = nullptr) const; // 转换为裸指针，失败返回默认指针

    static D2D1_COLOR_F ParseColor(const std::string& str); // 静态转换字符串得到 D2D 颜色结构
    static Value ParseAuto(const std::string& str); // 自动识别并推断字符串对应的数据类型转换为 Value

private:
    Type m_type = Type::None; // 存储的数据类型标志
    std::variant<std::monostate, bool, int, float, std::string, D2D1_COLOR_F, Thickness, void*> m_val; // Variant 数据存储联合体
};

} // namespace CUI
