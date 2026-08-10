#include "MouseReporter.h"
#include <algorithm>

namespace CUI {
namespace Term {

bool MouseReporter::Encode(Encoding encoding, int button, int col, int row, bool press,
                           unsigned mods, std::string& out) {
    // 1-based cell coords
    col = std::clamp(col + 1, 1, 9999);
    row = std::clamp(row + 1, 1, 9999);

    int cb = button;
    if (mods & ModShift) cb |= 4;
    if (mods & ModAlt) cb |= 8;
    if (mods & ModControl) cb |= 16;

    switch (encoding) {
    case Encoding::Sgr:
        out = EncodeSgr(cb, col, row, press);
        return true;
    case Encoding::Urxvt:
        out = "\x1b[" + std::to_string(cb) + ";" + std::to_string(col) + ";" + std::to_string(row) + "M";
        return true;
    case Encoding::Utf8:
        return EncodeX10Like(cb, col, row, true, out);
    case Encoding::X10:
    default:
        return EncodeX10Like(cb, col, row, false, out);
    }
}

std::string MouseReporter::EncodeSgr(int cb, int col, int row, bool press) {
    // SGR: release uses lowercase 'm', press/motion uses 'M'
    const char final = press ? 'M' : 'm';
    return "\x1b[<" + std::to_string(cb) + ";" + std::to_string(col) + ";" + std::to_string(row) + final;
}

bool MouseReporter::EncodeX10Like(int cb, int col, int row, bool utf8, std::string& out) {
    // X10 limited to 223
    if (!utf8 && (col > 223 || row > 223)) {
        return false;
    }

    cb = std::clamp(cb + 32, 32, 255);
    col = std::clamp(col + 32, 32, utf8 ? 0x7FF : 255);
    row = std::clamp(row + 32, 32, utf8 ? 0x7FF : 255);

    if (!utf8) {
        out = "\x1b[M";
        out.push_back(static_cast<char>(static_cast<unsigned char>(cb)));
        out.push_back(static_cast<char>(static_cast<unsigned char>(col)));
        out.push_back(static_cast<char>(static_cast<unsigned char>(row)));
        return true;
    }

    out = "\x1b[M" + EncodeUtf8Char(cb) + EncodeUtf8Char(col) + EncodeUtf8Char(row);
    return true;
}

std::string MouseReporter::EncodeUtf8Char(int value) {
    std::string s;
    if (value < 0x80) {
        s.push_back(static_cast<char>(value));
        return s;
    }
    // 2-byte UTF-8
    s.push_back(static_cast<char>(static_cast<unsigned char>(0xC0 | (value >> 6))));
    s.push_back(static_cast<char>(static_cast<unsigned char>(0x80 | (value & 0x3F))));
    return s;
}

int MouseReporter::ButtonFromLogical(TermMouseButton button) {
    switch (button) {
    case TermMouseButton::Left:   return ButtonLeft;
    case TermMouseButton::Middle: return ButtonMiddle;
    case TermMouseButton::Right:  return ButtonRight;
    default:                      return ButtonLeft;
    }
}

} // namespace Term
} // namespace CUI
