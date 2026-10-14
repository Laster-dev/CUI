#pragma once

#include "../core/Value.h"
#include <cstdint>

namespace CUI {

/**
 * @brief 抽象光标样式类型，与平台原生光标解耦。
 */
enum class CursorType {
    Arrow = 0,    ///< 默认箭头光标 (IDC_ARROW)
    Hand,         ///< 手型/超链接光标 (IDC_HAND)
    IBeam,        ///< 文本编辑光标 (IDC_IBEAM)
    Wait,         ///< 等待/沙漏光标 (IDC_WAIT)
    Cross,        ///< 十字准星光标 (IDC_CROSS)
    SizeAll,      ///< 四向移动光标 (IDC_SIZEALL)
    SizeNS,       ///< 垂直双向调整大小 (IDC_SIZENS)
    SizeWE,       ///< 水平双向调整大小 (IDC_SIZEWE)
    SizeNESW,     ///< 东北-西南斜向调整大小 (IDC_SIZENESW)
    SizeNWSE,     ///< 西北-东南斜向调整大小 (IDC_SIZENWSE)
    NotAllowed,   ///< 禁止操作光标 (IDC_NO)
    None          ///< 隐藏光标
};

/**
 * @brief 通用键盘按键键码枚举。
 */
enum class Key {
    None = 0,
    Backspace,
    Tab,
    Clear,
    Enter,
    Shift,
    Control,
    Alt,
    Pause,
    CapsLock,
    Escape,
    Space,
    PageUp,
    PageDown,
    End,
    Home,
    Left,
    Up,
    Right,
    Down,
    Select,
    Print,
    Execute,
    PrintScreen,
    Insert,
    Delete,
    Help,
    D0, D1, D2, D3, D4, D5, D6, D7, D8, D9,
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    LeftWin,
    RightWin,
    Apps,
    Sleep,
    NumPad0, NumPad1, NumPad2, NumPad3, NumPad4,
    NumPad5, NumPad6, NumPad7, NumPad8, NumPad9,
    Multiply,
    Add,
    Separator,
    Subtract,
    Decimal,
    Divide,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24,
    NumLock,
    ScrollLock,
    OemSemicolon,
    OemPlus,
    OemComma,
    OemMinus,
    OemPeriod,
    OemQuestion,
    OemTilde,
    OemOpenBrackets,
    OemPipe,
    OemCloseBrackets,
    OemQuotes
};

/**
 * @brief 组合修饰键位掩码。
 */
enum class ModifierKeys : uint8_t {
    None    = 0,
    Alt     = 1 << 0,
    Control = 1 << 1,
    Shift   = 1 << 2,
    Windows = 1 << 3
};

inline ModifierKeys operator|(ModifierKeys a, ModifierKeys b) {
    return static_cast<ModifierKeys>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline ModifierKeys operator&(ModifierKeys a, ModifierKeys b) {
    return static_cast<ModifierKeys>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline ModifierKeys& operator|=(ModifierKeys& a, ModifierKeys b) {
    a = a | b;
    return a;
}

/**
 * @brief 键盘事件参数结构体。
 */
struct KeyEventArgs {
    Key key = Key::None;
    int virtualKey = 0;              ///< 平台原始虚拟键码（兼容旧代码）
    ModifierKeys modifiers = ModifierKeys::None;
    bool ctrl = false;
    bool alt = false;
    bool shift = false;
    bool repeat = false;
    bool handled = false;

    KeyEventArgs() = default;
    KeyEventArgs(Key k, ModifierKeys m = ModifierKeys::None, int vk = 0, bool isRepeat = false)
        : key(k), virtualKey(vk), modifiers(m), repeat(isRepeat) {
        ctrl = (static_cast<uint8_t>(m) & static_cast<uint8_t>(ModifierKeys::Control)) != 0;
        alt = (static_cast<uint8_t>(m) & static_cast<uint8_t>(ModifierKeys::Alt)) != 0;
        shift = (static_cast<uint8_t>(m) & static_cast<uint8_t>(ModifierKeys::Shift)) != 0;
    }
};

/**
 * @brief 鼠标按键枚举。
 */
enum class MouseButton {
    None = 0,
    Left,
    Right,
    Middle,
    XButton1,
    XButton2
};

/**
 * @brief 指针/鼠标事件参数结构体。
 */
struct PointerEventArgs {
    Point position{ 0.0f, 0.0f };
    MouseButton button = MouseButton::None;
    int clickCount = 1;
    float wheelDelta = 0.0f;
    ModifierKeys modifiers = ModifierKeys::None;
    bool ctrl = false;
    bool alt = false;
    bool shift = false;
    bool handled = false;

    PointerEventArgs() = default;
    PointerEventArgs(Point pos, MouseButton btn = MouseButton::None, int clicks = 1, ModifierKeys m = ModifierKeys::None)
        : position(pos), button(btn), clickCount(clicks), modifiers(m) {
        ctrl = (static_cast<uint8_t>(m) & static_cast<uint8_t>(ModifierKeys::Control)) != 0;
        alt = (static_cast<uint8_t>(m) & static_cast<uint8_t>(ModifierKeys::Alt)) != 0;
        shift = (static_cast<uint8_t>(m) & static_cast<uint8_t>(ModifierKeys::Shift)) != 0;
    }
};

// 平台映射转换接口
Key KeyFromVirtualKey(int vk);
int VirtualKeyFromKey(Key key);

} // namespace CUI
