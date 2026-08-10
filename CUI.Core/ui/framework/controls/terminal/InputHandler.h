#pragma once
#include "BufferSet.h"
#include "EscapeSequenceParser.h"
#include "HyperlinkStore.h"
#include "MouseReporter.h"
#include <functional>
#include <string>
#include <vector>

namespace CUI {
namespace Term {

class InputHandler {
public:
    using VoidCallback = std::function<void()>;
    using StringCallback = std::function<void(const std::string&)>;
    using ClipboardGetter = std::function<bool(std::string&)>;
    using OscColorCallback = std::function<void(int, const std::string&)>;

    InputHandler(BufferSet& buffers,
                 VoidCallback requestRedraw,
                 StringCallback sendReply,
                 HyperlinkStore& links,
                 StringCallback setTitle = nullptr,
                 StringCallback setClipboard = nullptr,
                 ClipboardGetter getClipboard = nullptr,
                 OscColorCallback onOscColor = nullptr);

    bool ApplicationCursorKeys() const { return m_applicationCursor; }
    bool ApplicationKeypad() const { return m_applicationKeypad; }
    bool BracketedPasteMode() const { return m_bracketedPaste; }
    bool CursorVisible() const { return m_cursorVisible; }
    bool CursorBlink() const { return m_cursorBlink; }
    int CursorStyle() const { return m_cursorStyle; }
    bool FocusReporting() const { return m_focusReporting; }
    bool MouseTracking() const { return m_mouseMode1000 || m_mouseMode1002 || m_mouseMode1003; }
    bool MouseAnyEvent() const { return m_mouseMode1003; }
    bool MouseButtonEvent() const { return m_mouseMode1002 || m_mouseMode1003; }
    bool MousePixels() const { return m_mousePixels; }
    bool SyncOutput() const { return m_syncOutput; }

    MouseReporter::Encoding MouseEncoding() const;

    void Attach(EscapeSequenceParser& parser);

    void Reset();
    void SoftReset();

    void OnColsChanged(int cols) { ResetTabs(cols); }

private:
    TerminalBuffer& Buf() { return m_buffers.Active(); }

    void Print(int codePoint);
    void Execute(uint8_t b);
    void Esc(uint8_t final, int collect);
    void Csi(uint8_t final, const Params& p, int collect);
    void Osc(const std::string& ident, const std::string& data);
    void Dcs(uint8_t final, const Params& p, int collect, const std::string& data);

    void ClearMouseTracking();

    void CursorUp(int n);
    void CursorDown(int n);
    void CursorForward(int n);
    void CursorBackward(int n);
    void RepeatLast(int n);

    void DeviceAttributes(char priv);
    void DeviceStatusReport(const Params& p, char priv);
    void WindowOps(const Params& p);
    void SetMode(const Params& p, char priv, bool set);
    void CharAttributes(const Params& p);
    static int ParseExtendedColor(const Params& p, int i, CellData& attr, bool foreground);

    void ParseHyperlink(const std::string& data);
    void ParseClipboard(const std::string& data);

    void ResetTabs(int cols);
    void SetTabStop(int col);
    void ClearTabs(int mode);
    void AdvanceTab();

    BufferSet& m_buffers;
    VoidCallback m_requestRedraw;
    StringCallback m_sendReply;
    HyperlinkStore& m_links;
    StringCallback m_setTitle;
    StringCallback m_setClipboard;
    ClipboardGetter m_getClipboard;
    OscColorCallback m_onOscColor;

    bool m_applicationCursor = false;
    bool m_applicationKeypad = false;
    bool m_bracketedPaste = false;
    bool m_cursorVisible = true;
    bool m_cursorBlink = true;
    bool m_insertMode = false;
    bool m_focusReporting = false;
    bool m_mouseMode1000 = false;
    bool m_mouseMode1002 = false;
    bool m_mouseMode1003 = false;
    bool m_mouseUtf8 = false;
    bool m_mouseSgr = false;
    bool m_mouseUrxvt = false;
    bool m_mousePixels = false; // 1016
    bool m_syncOutput = false;
    int m_cursorStyle = 0; // 0 block, 1 underline, 2 bar
    std::vector<bool> m_tabs;
    int m_lastPrinted = -1;
    CellData m_savedAttr = CellData::Empty();

    // Charsets
    wchar_t m_g0 = L'B';
    wchar_t m_g1 = L'B';
    int m_gl = 0; // 0 = G0, 1 = G1
};

} // namespace Term
} // namespace CUI
