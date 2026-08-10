#pragma once
#include <string>

namespace CUI {
namespace Term {

// Modifier flags mirroring System.Windows.Input.ModifierKeys.
enum TermModifiers : unsigned {
    ModNone = 0,
    ModAlt = 1,
    ModControl = 2,
    ModShift = 4,
    ModWindows = 8
};

// Logical mouse buttons (host-agnostic replacement for System.Windows.Input.MouseButton).
enum class TermMouseButton {
    Left,
    Middle,
    Right,
    XButton1,
    XButton2
};

// Encodes mouse events into VT sequences (X10 / SGR 1006 / urxvt 1015).
class MouseReporter {
public:
    static const int ButtonLeft = 0;
    static const int ButtonMiddle = 1;
    static const int ButtonRight = 2;
    static const int ButtonRelease = 3;
    static const int ButtonWheelUp = 64;
    static const int ButtonWheelDown = 65;
    static const int ButtonWheelLeft = 66;
    static const int ButtonWheelRight = 67;
    static const int Motion = 32;

    enum class Encoding {
        X10,
        Utf8,   // 1005
        Sgr,    // 1006
        Urxvt   // 1015
    };

    // Returns false when the event cannot be encoded (X10 coordinate overflow).
    static bool Encode(Encoding encoding, int button, int col, int row, bool press,
                       unsigned mods, std::string& out);

    static int ButtonFromLogical(TermMouseButton button);

private:
    static std::string EncodeSgr(int cb, int col, int row, bool press);
    static bool EncodeX10Like(int cb, int col, int row, bool utf8, std::string& out);
    static std::string EncodeUtf8Char(int value);
};

} // namespace Term
} // namespace CUI
