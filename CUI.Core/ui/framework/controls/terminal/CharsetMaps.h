#pragma once

namespace CUI {
namespace Term {

// DEC Special Graphics / VT line-drawing map (G0 '0').
class CharsetMaps {
public:
    static int Map(wchar_t charsetId, int codePoint);
};

} // namespace Term
} // namespace CUI
