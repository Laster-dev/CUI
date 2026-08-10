#include "InputHandler.h"
#include "CharsetMaps.h"
#include "UnicodeWidth.h"
#include <algorithm>

namespace CUI {
namespace Term {

namespace {
const char kBase64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string Base64Encode(const std::string& input) {
    std::string out;
    out.reserve(((input.size() + 2) / 3) * 4);
    size_t i = 0;
    while (i + 2 < input.size()) {
        const uint32_t v = (static_cast<uint8_t>(input[i]) << 16)
            | (static_cast<uint8_t>(input[i + 1]) << 8)
            | static_cast<uint8_t>(input[i + 2]);
        out.push_back(kBase64Chars[(v >> 18) & 0x3F]);
        out.push_back(kBase64Chars[(v >> 12) & 0x3F]);
        out.push_back(kBase64Chars[(v >> 6) & 0x3F]);
        out.push_back(kBase64Chars[v & 0x3F]);
        i += 3;
    }
    const size_t rest = input.size() - i;
    if (rest == 1) {
        const uint32_t v = static_cast<uint8_t>(input[i]) << 16;
        out.push_back(kBase64Chars[(v >> 18) & 0x3F]);
        out.push_back(kBase64Chars[(v >> 12) & 0x3F]);
        out.push_back('=');
        out.push_back('=');
    } else if (rest == 2) {
        const uint32_t v = (static_cast<uint8_t>(input[i]) << 16)
            | (static_cast<uint8_t>(input[i + 1]) << 8);
        out.push_back(kBase64Chars[(v >> 18) & 0x3F]);
        out.push_back(kBase64Chars[(v >> 12) & 0x3F]);
        out.push_back(kBase64Chars[(v >> 6) & 0x3F]);
        out.push_back('=');
    }
    return out;
}

bool Base64Decode(const std::string& input, std::string& out) {
    auto value = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };

    out.clear();
    uint32_t acc = 0;
    int bits = 0;
    for (char c : input) {
        if (c == '=' ) break;
        if (c == '\r' || c == '\n' || c == ' ' || c == '\t') continue;
        const int v = value(c);
        if (v < 0) {
            return false;
        }
        acc = (acc << 6) | static_cast<uint32_t>(v);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<char>((acc >> bits) & 0xFF));
        }
    }
    return true;
}

std::string Trim(const std::string& s) {
    size_t start = 0;
    size_t end = s.size();
    while (start < end && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) start++;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) end--;
    return s.substr(start, end - start);
}

bool TryParseInt(const std::string& s, int& value) {
    if (s.empty()) return false;
    int result = 0;
    for (char c : s) {
        if (c < '0' || c > '9') return false;
        result = result * 10 + (c - '0');
        if (result > 1000000) return false;
    }
    value = result;
    return true;
}
}

InputHandler::InputHandler(BufferSet& buffers,
                           VoidCallback requestRedraw,
                           StringCallback sendReply,
                           HyperlinkStore& links,
                           StringCallback setTitle,
                           StringCallback setClipboard,
                           ClipboardGetter getClipboard,
                           OscColorCallback onOscColor)
    : m_buffers(buffers)
    , m_requestRedraw(std::move(requestRedraw))
    , m_sendReply(std::move(sendReply))
    , m_links(links)
    , m_setTitle(std::move(setTitle))
    , m_setClipboard(std::move(setClipboard))
    , m_getClipboard(std::move(getClipboard))
    , m_onOscColor(std::move(onOscColor)) {
    ResetTabs(buffers.Active().Cols());
}

MouseReporter::Encoding InputHandler::MouseEncoding() const {
    if (m_mouseSgr || m_mousePixels) return MouseReporter::Encoding::Sgr;
    if (m_mouseUrxvt) return MouseReporter::Encoding::Urxvt;
    if (m_mouseUtf8) return MouseReporter::Encoding::Utf8;
    return MouseReporter::Encoding::X10;
}

void InputHandler::Attach(EscapeSequenceParser& parser) {
    parser.OnPrint = [this](int cp) { Print(cp); };
    parser.OnExecute = [this](uint8_t b) { Execute(b); };
    parser.OnCsi = [this](uint8_t final, const Params& p, int collect) { Csi(final, p, collect); };
    parser.OnEsc = [this](uint8_t final, int collect) { Esc(final, collect); };
    parser.OnOsc = [this](const std::string& id, const std::string& data) { Osc(id, data); };
    parser.OnDcs = [this](uint8_t final, const Params& p, int collect, const std::string& data) {
        Dcs(final, p, collect, data);
    };
}

void InputHandler::Reset() {
    SoftReset();
    m_applicationCursor = false;
    m_applicationKeypad = false;
    m_g0 = m_g1 = L'B';
    m_gl = 0;
    m_links.Clear();
    ResetTabs(Buf().Cols());
}

void InputHandler::SoftReset() {
    m_bracketedPaste = false;
    m_cursorVisible = true;
    m_cursorBlink = true;
    m_insertMode = false;
    m_focusReporting = false;
    m_mouseMode1000 = m_mouseMode1002 = m_mouseMode1003 = false;
    m_mouseUtf8 = m_mouseSgr = m_mouseUrxvt = m_mousePixels = false;
    m_syncOutput = false;
    m_cursorStyle = 0;
    Buf().OriginMode = false;
    Buf().Wraparound = true;
    Buf().InsertMode = false;
    Buf().SetScrollRegion(0, Buf().Rows() - 1);
    Buf().CurAttr = CellData::Empty();
    m_lastPrinted = -1;
    m_gl = 0;
}

void InputHandler::ClearMouseTracking() {
    m_mouseMode1000 = m_mouseMode1002 = m_mouseMode1003 = false;
    m_mouseUtf8 = m_mouseSgr = m_mouseUrxvt = m_mousePixels = false;
}

void InputHandler::Print(int codePoint) {
    const wchar_t charset = (m_gl == 0) ? m_g0 : m_g1;
    codePoint = CharsetMaps::Map(charset, codePoint);
    int width = UnicodeWidth::GetWidth(codePoint);
    if (width == 0 && codePoint != 0) {
        // combining: attach to previous cell visually as ignore for grid
        if (m_requestRedraw) m_requestRedraw();
        return;
    }

    if (width <= 0) width = 1;

    Buf().ActiveLinkId = m_links.ActiveId();
    Buf().PrintChar(codePoint, width);
    m_lastPrinted = codePoint;
    if (!m_syncOutput && m_requestRedraw) {
        m_requestRedraw();
    }
}

void InputHandler::Execute(uint8_t b) {
    switch (b) {
    case 0x07:
        break;
    case 0x08:
        if (Buf().CursorX > 0) {
            Buf().CursorX--;
        } else if (Buf().Wraparound && Buf().CursorY > 0) {
            Buf().CursorY--;
            Buf().CursorX = Buf().Cols() - 1;
        }
        break;
    case 0x09:
        AdvanceTab();
        break;
    case 0x0A:
    case 0x0B:
    case 0x0C:
        Buf().LineFeed();
        break;
    case 0x0D:
        Buf().CarriageReturn();
        break;
    case 0x0E:
        m_gl = 1; // SO
        break;
    case 0x0F:
        m_gl = 0; // SI
        break;
    default:
        break;
    }
    if (!m_syncOutput && m_requestRedraw) {
        m_requestRedraw();
    }
}

void InputHandler::Esc(uint8_t final, int collect) {
    const int first = collect & 0xFF;

    // Charset designation: ESC ( B, ESC ) 0, ESC ( 0, etc.
    if (final >= 0x30 && final <= 0x7E &&
        (first == '(' || first == ')' || first == '*' || first == '+')) {
        const wchar_t id = static_cast<wchar_t>(final);
        if (first == '(') {
            m_g0 = id;
        } else if (first == ')') {
            m_g1 = id;
        }
        if (m_requestRedraw) m_requestRedraw();
        return;
    }

    switch (final) {
    case '7': Buf().SaveCursor(); m_savedAttr = Buf().CurAttr; break;
    case '8': Buf().RestoreCursor(); Buf().CurAttr = m_savedAttr; break;
    case 'D': Buf().Index(); break;
    case 'E': Buf().CarriageReturn(); Buf().LineFeed(); break;
    case 'H': SetTabStop(Buf().CursorX); break;
    case 'M': Buf().ReverseIndex(); break;
    case 'c': m_buffers.Reset(); Reset(); break;
    case '=': m_applicationKeypad = true; break;
    case '>': m_applicationKeypad = false; break;
    case 'n': m_gl = 1; break; // LS2-ish simplified
    case 'o': m_gl = 0; break;
    default: break;
    }
    if (!m_syncOutput && m_requestRedraw) {
        m_requestRedraw();
    }
}

void InputHandler::Csi(uint8_t final, const Params& p, int collect) {
    const int low = collect & 0xFF;
    const char priv = (low == '?' || low == '>' || low == '!' || low == '=')
        ? static_cast<char>(low) : '\0';
    const int intermediate = (collect >> 8) & 0xFF;

    switch (final) {
    case 'A': CursorUp(p.GetNonZero(0)); break;
    case 'B': CursorDown(p.GetNonZero(0)); break;
    case 'C': CursorForward(p.GetNonZero(0)); break;
    case 'D': CursorBackward(p.GetNonZero(0)); break;
    case 'E': CursorDown(p.GetNonZero(0)); Buf().CursorX = 0; break;
    case 'F': CursorUp(p.GetNonZero(0)); Buf().CursorX = 0; break;
    case 'G':
    case '`': Buf().CursorX = std::clamp(p.GetNonZero(0) - 1, 0, Buf().Cols() - 1); break;
    case 'H':
    case 'f': Buf().SetCursor(p.GetNonZero(1) - 1, p.GetNonZero(0) - 1); break;
    case 'J': Buf().EraseInDisplay(p.Get(0)); break;
    case 'K': Buf().EraseInLine(p.Get(0)); break;
    case 'L': Buf().InsertLines(p.GetNonZero(0)); break;
    case 'M': Buf().DeleteLines(p.GetNonZero(0)); break;
    case 'P': Buf().DeleteChars(p.GetNonZero(0)); break;
    case 'S': Buf().ScrollUp(p.GetNonZero(0)); break;
    case 'T': Buf().ScrollDown(p.GetNonZero(0)); break;
    case 'X': Buf().EraseChars(p.GetNonZero(0)); break;
    case '@': Buf().InsertChars(p.GetNonZero(0)); break;
    case 'a': CursorForward(p.GetNonZero(0)); break;
    case 'b': RepeatLast(p.GetNonZero(0)); break;
    case 'c': DeviceAttributes(priv); break;
    case 'd': Buf().SetCursor(Buf().CursorX, p.GetNonZero(0) - 1); break;
    case 'e': CursorDown(p.GetNonZero(0)); break;
    case 'g': ClearTabs(p.Get(0)); break;
    case 'm':
        if (priv == '\0') {
            CharAttributes(p);
        }
        break;
    case 'n': DeviceStatusReport(p, priv); break;
    case 'p':
        if (intermediate == '!' || priv == '!') {
            SoftReset();
        }
        break;
    case 'q':
        if (intermediate == ' ' || low == ' ') { // DECSCUSR
            const int style = p.Get(0);
            m_cursorStyle = (style == 3 || style == 4) ? 1 : ((style == 5 || style == 6) ? 2 : 0);
            m_cursorBlink = (style == 0 || style == 1 || style == 3 || style == 5);
        }
        break;
    case 'r':
        if (priv == '\0') {
            Buf().SetScrollRegion(p.GetNonZero(0) - 1, p.Get(1, Buf().Rows()) - 1);
        }
        break;
    case 's': Buf().SaveCursor(); m_savedAttr = Buf().CurAttr; break;
    case 'u': Buf().RestoreCursor(); Buf().CurAttr = m_savedAttr; break;
    case 'h': SetMode(p, priv, true); break;
    case 'l': SetMode(p, priv, false); break;
    case 't': WindowOps(p); break;
    default: break;
    }

    if (!m_requestRedraw) {
        return;
    }
    if (!m_syncOutput) {
        m_requestRedraw();
    } else if (final == 'q' || final == 'h' || final == 'l') {
        m_requestRedraw();
    }
}

void InputHandler::CursorUp(int n) { Buf().SetCursor(Buf().CursorX, Buf().CursorY - n); }
void InputHandler::CursorDown(int n) { Buf().SetCursor(Buf().CursorX, Buf().CursorY + n); }
void InputHandler::CursorForward(int n) { Buf().CursorX = (std::min)(Buf().Cols() - 1, Buf().CursorX + n); }
void InputHandler::CursorBackward(int n) { Buf().CursorX = (std::max)(0, Buf().CursorX - n); }

void InputHandler::RepeatLast(int n) {
    if (m_lastPrinted < 0) {
        return;
    }
    for (int i = 0; i < n; ++i) {
        Buf().PrintChar(m_lastPrinted, UnicodeWidth::GetWidth(m_lastPrinted));
    }
}

void InputHandler::DeviceAttributes(char priv) {
    if (!m_sendReply) return;
    if (priv == '>') {
        m_sendReply("\x1b[>0;276;0c");
    } else if (priv == '\0') {
        m_sendReply("\x1b[?1;2c");
    }
}

void InputHandler::DeviceStatusReport(const Params& p, char priv) {
    if (!m_sendReply) return;
    const int code = p.Get(0);
    if (priv == '?') {
        if (code == 6) {
            m_sendReply("\x1b[?" + std::to_string(Buf().CursorY + 1) + ";" +
                        std::to_string(Buf().CursorX + 1) + ";1R");
        }
        return;
    }
    switch (code) {
    case 5:
        m_sendReply("\x1b[0n");
        break;
    case 6:
        m_sendReply("\x1b[" + std::to_string(Buf().CursorY + 1) + ";" +
                    std::to_string(Buf().CursorX + 1) + "R");
        break;
    default:
        break;
    }
}

void InputHandler::WindowOps(const Params& p) {
    if (!m_sendReply) return;
    switch (p.Get(0)) {
    case 11: // report window position
        m_sendReply("\x1b[3;0;0t");
        break;
    case 13: // report text area position
        m_sendReply("\x1b[3;0;0t");
        break;
    case 14:
        m_sendReply("\x1b[4;" + std::to_string(Buf().Rows() * 16) + ";" +
                    std::to_string(Buf().Cols() * 8) + "t");
        break;
    case 15: // screen size pixels
        m_sendReply("\x1b[5;" + std::to_string(Buf().Rows() * 16) + ";" +
                    std::to_string(Buf().Cols() * 8) + "t");
        break;
    case 16:
        m_sendReply("\x1b[6;16;8t");
        break;
    case 18:
        m_sendReply("\x1b[8;" + std::to_string(Buf().Rows()) + ";" +
                    std::to_string(Buf().Cols()) + "t");
        break;
    case 19: // screen size chars
        m_sendReply("\x1b[9;" + std::to_string(Buf().Rows()) + ";" +
                    std::to_string(Buf().Cols()) + "t");
        break;
    case 20: // icon label
        m_sendReply("\x1b]LCUIT\x1b\\");
        break;
    case 21:
        m_sendReply("\x1b]lCUITerminal\x1b\\");
        break;
    case 22:
    case 23:
        break;
    default:
        break;
    }
}

void InputHandler::SetMode(const Params& p, char priv, bool set) {
    const int count = (std::max)(1, p.Length());
    for (int i = 0; i < count; ++i) {
        const int mode = p.Length() == 0 ? 0 : p.Get(i);
        if (priv == '?') {
            switch (mode) {
            case 1:
                m_applicationCursor = set;
                break;
            case 6:
                Buf().OriginMode = set;
                Buf().SetCursor(0, set ? Buf().ScrollTop : 0);
                break;
            case 7:
                Buf().Wraparound = set;
                break;
            case 12:
                m_cursorBlink = set;
                break;
            case 25:
                m_cursorVisible = set;
                break;
            case 1000:
                // Master mouse-tracking switch: off disables 1000/1002/1003.
                // Apps often enable 1002/1003 then only send ?1000l on exit;
                // leaving 1002/1003 set kept reporting motion into the shell.
                if (set) {
                    m_mouseMode1000 = true;
                    m_mouseMode1002 = false;
                    m_mouseMode1003 = false;
                } else {
                    ClearMouseTracking();
                }
                break;
            case 1002:
                if (set) {
                    m_mouseMode1000 = true;
                    m_mouseMode1002 = true;
                    m_mouseMode1003 = false;
                } else {
                    m_mouseMode1002 = false;
                    m_mouseMode1003 = false;
                }
                break;
            case 1003:
                if (set) {
                    m_mouseMode1000 = true;
                    m_mouseMode1002 = true;
                    m_mouseMode1003 = true;
                } else {
                    m_mouseMode1003 = false;
                }
                break;
            case 1004:
                m_focusReporting = set;
                break;
            case 1005:
                m_mouseUtf8 = set;
                if (set) { m_mouseSgr = false; m_mouseUrxvt = false; }
                break;
            case 1006:
                m_mouseSgr = set;
                if (set) { m_mouseUtf8 = false; m_mouseUrxvt = false; }
                break;
            case 1015:
                m_mouseUrxvt = set;
                if (set) { m_mouseUtf8 = false; m_mouseSgr = false; }
                break;
            case 1016:
                m_mousePixels = set;
                if (set) { m_mouseSgr = true; m_mouseUtf8 = false; m_mouseUrxvt = false; }
                else { m_mousePixels = false; }
                break;
            case 1049:
            case 1047:
            case 47:
                if (set) {
                    m_buffers.Normal().SaveCursor();
                    m_buffers.ActivateAlt(true);
                } else {
                    m_buffers.ActivateNormal();
                    m_buffers.Normal().RestoreCursor();
                    // TUI crash / incomplete teardown: don't leave mouse floods in the shell.
                    ClearMouseTracking();
                }
                break;
            case 1048:
                if (set) Buf().SaveCursor(); else Buf().RestoreCursor();
                break;
            case 1007: // alternate scroll
                break;
            case 1034: // meta sends escape
                break;
            case 2004:
                m_bracketedPaste = set;
                break;
            case 2026:
                m_syncOutput = set;
                if (!set && m_requestRedraw) m_requestRedraw();
                break;
            default:
                break;
            }
        } else {
            switch (mode) {
            case 4:
                m_insertMode = set;
                Buf().InsertMode = set;
                break;
            default:
                break;
            }
        }
    }
}

void InputHandler::CharAttributes(const Params& p) {
    if (p.Length() == 0) {
        Buf().CurAttr = CellData::Empty();
        return;
    }
    CellData attr = Buf().CurAttr;
    for (int i = 0; i < p.Length(); ++i) {
        const int psn = p.Get(i);
        if (psn >= 30 && psn <= 37) {
            attr.Fg = psn - 30;
            continue;
        }
        if (psn >= 40 && psn <= 47) {
            attr.Bg = psn - 40;
            continue;
        }
        if (psn >= 90 && psn <= 97) {
            attr.Fg = psn - 90 + 8;
            continue;
        }
        if (psn >= 100 && psn <= 107) {
            attr.Bg = psn - 100 + 8;
            continue;
        }
        switch (psn) {
        case 0: attr = CellData::Empty(); break;
        case 1: attr.Attrs |= CellData::AttrBold; break;
        case 2: attr.Attrs |= CellData::AttrDim; break;
        case 3: attr.Attrs |= CellData::AttrItalic; break;
        case 4: attr.Attrs |= CellData::AttrUnderline; break;
        case 5:
        case 6: attr.Attrs |= CellData::AttrBlink; break;
        case 7: attr.Attrs |= CellData::AttrInverse; break;
        case 8: attr.Attrs |= CellData::AttrInvisible; break;
        case 9: attr.Attrs |= CellData::AttrStrikethrough; break;
        case 22: attr.Attrs &= ~(CellData::AttrBold | CellData::AttrDim); break;
        case 23: attr.Attrs &= ~CellData::AttrItalic; break;
        case 24: attr.Attrs &= ~CellData::AttrUnderline; break;
        case 25: attr.Attrs &= ~CellData::AttrBlink; break;
        case 27: attr.Attrs &= ~CellData::AttrInverse; break;
        case 28: attr.Attrs &= ~CellData::AttrInvisible; break;
        case 29: attr.Attrs &= ~CellData::AttrStrikethrough; break;
        case 38: i = ParseExtendedColor(p, i, attr, true); break;
        case 39: attr.Fg = CellData::DefaultColor; break;
        case 48: i = ParseExtendedColor(p, i, attr, false); break;
        case 49: attr.Bg = CellData::DefaultColor; break;
        default: break;
        }
    }
    if (attr.GetCodePoint() == 0) {
        attr.SetCodePoint(' ');
        attr.SetWidth(1);
    }
    attr.LinkId = m_links.ActiveId();
    Buf().CurAttr = attr;
}

int InputHandler::ParseExtendedColor(const Params& p, int i, CellData& attr, bool foreground) {
    if (i + 1 >= p.Length()) {
        return i;
    }
    const int mode = p.Get(i + 1);
    if (mode == 5 && i + 2 < p.Length()) {
        if (foreground) attr.Fg = p.Get(i + 2); else attr.Bg = p.Get(i + 2);
        return i + 2;
    }
    if (mode == 2 && i + 4 < p.Length()) {
        const int r = p.Get(i + 2) & 0xFF;
        const int g = p.Get(i + 3) & 0xFF;
        const int b = p.Get(i + 4) & 0xFF;
        const int rgb = CellData::ColorModeRgb | (r << 16) | (g << 8) | b;
        if (foreground) attr.Fg = rgb; else attr.Bg = rgb;
        return i + 4;
    }
    return i + 1;
}

void InputHandler::Osc(const std::string& ident, const std::string& data) {
    if (ident == "0" || ident == "2") {
        if (m_setTitle) m_setTitle(data);
        return;
    }
    if (ident == "8") {
        ParseHyperlink(data);
        return;
    }
    if (ident == "52") {
        ParseClipboard(data);
        return;
    }
    if (ident == "10" || ident == "11" || ident == "12") {
        int which = 0;
        if (m_onOscColor && TryParseInt(ident, which)) {
            m_onOscColor(which, data);
        }
        return;
    }
    if (ident == "104" || ident == "110" || ident == "111" || ident == "112") {
        if (m_onOscColor) {
            int which = 104;
            if (ident == "104" || TryParseInt(ident, which)) {
                if (ident == "104") which = 104;
                m_onOscColor(which, "reset");
            }
        }
        return;
    }
    if (ident == "4") {
        // palette set/query - pass through
        if (m_onOscColor) m_onOscColor(4, data);
        return;
    }
}

void InputHandler::ParseHyperlink(const std::string& data) {
    // OSC 8 ; params ; uri
    const size_t semi = data.find(';');
    const std::string uri = (semi == std::string::npos) ? data : data.substr(semi + 1);
    if (uri.empty()) {
        m_links.End();
    } else {
        m_links.Begin(uri);
    }

    Buf().ActiveLinkId = m_links.ActiveId();
}

void InputHandler::ParseClipboard(const std::string& data) {
    // Pc;Pd - Pc is c/p/s, Pd is base64 or ?
    const size_t semi = data.find(';');
    if (semi == std::string::npos) {
        return;
    }
    const std::string pd = data.substr(semi + 1);
    if (pd == "?") {
        std::string clip;
        if (m_getClipboard) {
            m_getClipboard(clip);
        }
        if (m_sendReply) {
            m_sendReply("\x1b]52;c;" + Base64Encode(clip) + "\x07");
        }
        return;
    }
    std::string text;
    if (Base64Decode(Trim(pd), text) && m_setClipboard) {
        m_setClipboard(text);
    }
}

void InputHandler::Dcs(uint8_t final, const Params& p, int collect, const std::string& data) {
    (void)p;
    if (!m_sendReply) return;
    // DECRQSS: DCS $ q Pt ST
    const int intermediate = collect & 0xFF;
    if (final == 'q' && intermediate == '$') {
        if (data == "r") { // DECSTBM
            m_sendReply("\x1bP1$r" + std::to_string(Buf().ScrollTop + 1) + ";" +
                        std::to_string(Buf().ScrollBottom + 1) + "r\x1b\\");
        } else if (data == "m") { // SGR
            m_sendReply("\x1bP1$r0m\x1b\\");
        } else if (data == " q") { // DECSCUSR
            m_sendReply("\x1bP1$r" + std::to_string(m_cursorStyle * 2 + (m_cursorBlink ? 1 : 2)) +
                        " q\x1b\\");
        } else {
            m_sendReply("\x1bP0$r\x1b\\");
        }
    }
}

void InputHandler::ResetTabs(int cols) {
    m_tabs.assign(static_cast<size_t>((std::max)(1, cols)), false);
    for (size_t i = 8; i < m_tabs.size(); i += 8) {
        m_tabs[i] = true;
    }
}

void InputHandler::SetTabStop(int col) {
    if (col >= 0 && col < static_cast<int>(m_tabs.size())) {
        m_tabs[static_cast<size_t>(col)] = true;
    }
}

void InputHandler::ClearTabs(int mode) {
    if (mode == 0 && Buf().CursorX < static_cast<int>(m_tabs.size())) {
        if (Buf().CursorX >= 0) {
            m_tabs[static_cast<size_t>(Buf().CursorX)] = false;
        }
    } else if (mode == 3) {
        std::fill(m_tabs.begin(), m_tabs.end(), false);
    }
}

void InputHandler::AdvanceTab() {
    int x = Buf().CursorX + 1;
    while (x < Buf().Cols()) {
        if (x < static_cast<int>(m_tabs.size()) && m_tabs[static_cast<size_t>(x)]) {
            Buf().CursorX = x;
            return;
        }
        x++;
    }
    Buf().CursorX = Buf().Cols() - 1;
}

} // namespace Term
} // namespace CUI
