#include "CharsetMaps.h"

namespace CUI {
namespace Term {

namespace {
// ESC ( 0 - indices 0x60..0x7E mapped relative to '`'
const wchar_t kDecSpecial[] = {
    0x25C6, // ` diamond
    0x2592, // a checkerboard
    0x2409, // b HT
    0x240C, // c FF
    0x240D, // d CR
    0x240A, // e LF
    0x00B0, // f degree
    0x00B1, // g +/-
    0x2424, // h NL
    0x240B, // i VT
    0x2518, // j lower-right
    0x2510, // k upper-right
    0x250C, // l upper-left
    0x2514, // m lower-left
    0x253C, // n crossing
    0x23BA, // o scan 1
    0x23BB, // p scan 3
    0x2500, // q horizontal
    0x23BC, // r scan 7
    0x23BD, // s scan 9
    0x251C, // t left tee
    0x2524, // u right tee
    0x2534, // v bottom tee
    0x252C, // w top tee
    0x2502, // x vertical
    0x2264, // y <=
    0x2265, // z >=
    0x03C0, // { pi
    0x2260, // | !=
    0x00A3, // } pound
    0x00B7, // ~ bullet
};
}

int CharsetMaps::Map(wchar_t charsetId, int codePoint) {
    if (charsetId == L'0' && codePoint >= 0x60 && codePoint <= 0x7E) {
        return static_cast<int>(kDecSpecial[codePoint - 0x60]);
    }
    if (charsetId == L'A' && codePoint == '#') {
        return 0x00A3; // pound sign
    }
    return codePoint;
}

} // namespace Term
} // namespace CUI
