#pragma once
#include "BufferSet.h"
#include "EscapeSequenceParser.h"
#include "HyperlinkStore.h"
#include "ITerminalBackend.h"
#include "InputHandler.h"
#include "MouseReporter.h"
#include "SelectionService.h"
#include "TerminalOptions.h"
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace CUI {
namespace Term {

// Headless VT terminal: buffer, parser, selection, and a pluggable ITerminalBackend.
// For CUI embedding prefer CUI::TerminalControl.
class Terminal {
public:
    static const size_t MaxFlushBytes = 64 * 1024;

    explicit Terminal(const TerminalOptions& options = TerminalOptions());
    ~Terminal();

    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;

    TerminalOptions& Options() { return m_options; }
    const TerminalOptions& Options() const { return m_options; }

    BufferSet& Buffers() { return m_buffers; }
    const BufferSet& Buffers() const { return m_buffers; }

    SelectionService& Selection() { return m_selection; }
    HyperlinkStore& Hyperlinks() { return m_links; }
    InputHandler& Input() { return *m_input; }
    const InputHandler& Input() const { return *m_input; }

    int Cols() const { return m_options.Cols; }
    int Rows() const { return m_options.Rows; }
    unsigned RedrawVersion() const { return m_redrawVersion; }
    const std::string& Title() const { return m_title; }
    bool IsFocused() const { return m_focused; }

    // True when a DECSET mouse tracking mode (1000/1002/1003) is active.
    bool MouseTracking() const { return m_input->MouseTracking(); }

    // Host hooks (all invoked on the UI thread except OutputFlushRequested).
    std::function<void()> RedrawRequested;
    std::function<void()> OutputFlushRequested; // any thread
    std::function<void()> ResizeCompleted;
    std::function<void(const std::string&)> TitleChanged;
    std::function<void()> ScrollChanged;
    std::function<void()> ThemeChanged;
    std::function<void(const std::string&)> ClipboardSetRequested;
    std::function<bool(std::string&)> ClipboardGetRequested;

    void Attach(ITerminalBackend* backend);
    void Detach();
    ITerminalBackend* Backend() const { return m_backend; }

    void Write(const char* data, size_t length);
    void Write(const std::string& data) { Write(data.data(), data.size()); }
    void Writeln(const std::string& data) { Write(data + "\r\n"); }

    // Drain coalesced backend output. Must run on the UI thread.
    // Returns true if more data remains (caller should schedule another flush).
    bool FlushPendingOutput();
    bool HasPendingOutput() const;

    void SendData(const std::string& data);
    void SendBinary(const char* data, size_t length);

    void NotifyFocus(bool focused);

    bool TryReportMouse(int button, int col, int row, bool press, unsigned mods,
                        double cellW = 8.0, double cellH = 16.0);

    void Resize(int cols, int rows);
    void Reset();
    void Clear();

    void ScrollLines(int delta);
    void ScrollToBottom();
    void SetScrollDisp(int yDisp);

    std::string GetSelectionText();

    bool FindNext(const std::wstring& query, int& absRow, int& col) {
        return Find(query, absRow, col, true);
    }
    bool FindPrev(const std::wstring& query, int& absRow, int& col) {
        return Find(query, absRow, col, false);
    }
    int CountMatches(const std::wstring& query);

    // Returns an empty string when the cell carries no OSC 8 link.
    std::string GetLinkAt(int absRow, int col);

private:
    bool Find(const std::wstring& query, int& absRow, int& col, bool forward);
    void ApplyFindHit(int y, int idx, int len);
    void HandleOscColor(int which, const std::string& data);
    static bool TryParseOscColor(const std::string& data, TermColor& color);
    void OnBackendOutput(const char* data, size_t length);
    void ScheduleOutputFlush();
    void RequestRedraw();

    TerminalOptions m_options;
    BufferSet m_buffers;
    SelectionService m_selection;
    HyperlinkStore m_links;
    EscapeSequenceParser m_parser;
    std::unique_ptr<InputHandler> m_input;

    mutable std::mutex m_sync;
    std::condition_variable m_backPressure;
    std::deque<std::string> m_pendingOutput;
    size_t m_pendingBytes = 0;
    std::atomic<int> m_flushScheduled{ 0 };
    // Cleared on detach so a blocked reader thread stops waiting for the UI.
    std::atomic<bool> m_acceptingOutput{ true };

    ITerminalBackend* m_backend = nullptr;
    bool m_disposed = false;
    unsigned m_redrawVersion = 0;
    bool m_focused = false;
    std::string m_title = "CUITerminal";
};

} // namespace Term
} // namespace CUI
