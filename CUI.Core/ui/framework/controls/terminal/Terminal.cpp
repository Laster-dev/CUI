#include "Terminal.h"
#include "UnicodeWidth.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>
#include <cwctype>
#include <vector>

namespace CUI {
namespace Term {

namespace {
std::wstring ToLowerW(const std::wstring& s) {
    std::wstring out(s);
    for (auto& c : out) {
        c = static_cast<wchar_t>(std::towlower(static_cast<wint_t>(c)));
    }
    return out;
}

// Mirrors string.IndexOf(value, startIndex, StringComparison.OrdinalIgnoreCase).
int IndexOfIgnoreCase(const std::wstring& haystack, const std::wstring& needle, int startIndex) {
    if (needle.empty()) {
        return -1;
    }
    if (startIndex < 0) startIndex = 0;
    if (startIndex > static_cast<int>(haystack.size())) {
        return -1;
    }
    const std::wstring h = ToLowerW(haystack);
    const std::wstring n = ToLowerW(needle);
    const size_t found = h.find(n, static_cast<size_t>(startIndex));
    return found == std::wstring::npos ? -1 : static_cast<int>(found);
}

// Mirrors string.LastIndexOf(value, startIndex, StringComparison.OrdinalIgnoreCase):
// searches backwards, requiring the match to start at or before startIndex.
int LastIndexOfIgnoreCase(const std::wstring& haystack, const std::wstring& needle, int startIndex) {
    if (needle.empty()) {
        return -1;
    }
    if (haystack.empty()) {
        return -1;
    }
    if (startIndex < 0) {
        return -1;
    }
    if (startIndex > static_cast<int>(haystack.size()) - 1) {
        startIndex = static_cast<int>(haystack.size()) - 1;
    }
    const std::wstring h = ToLowerW(haystack);
    const std::wstring n = ToLowerW(needle);
    const size_t found = h.rfind(n, static_cast<size_t>(startIndex));
    return found == std::wstring::npos ? -1 : static_cast<int>(found);
}

std::string ToUpperHex2(uint8_t v) {
    static const char* digits = "0123456789ABCDEF";
    std::string s;
    s.push_back(digits[(v >> 4) & 0xF]);
    s.push_back(digits[v & 0xF]);
    return s;
}

std::string TrimAscii(const std::string& s) {
    size_t start = 0;
    size_t end = s.size();
    while (start < end && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) start++;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) end--;
    return s.substr(start, end - start);
}

bool ParseHexByte(const std::string& s, uint8_t& out) {
    if (s.empty()) {
        return false;
    }
    const std::string token = s.size() >= 2 ? s.substr(0, 2) : s;
    int value = 0;
    for (char c : token) {
        int digit;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
        else return false;
        value = value * 16 + digit;
    }
    out = static_cast<uint8_t>(value & 0xFF);
    return true;
}

bool StartsWithIgnoreCase(const std::string& s, const char* prefix) {
    const size_t len = strlen(prefix);
    if (s.size() < len) {
        return false;
    }
    for (size_t i = 0; i < len; ++i) {
        const char a = static_cast<char>(tolower(static_cast<unsigned char>(s[i])));
        const char b = static_cast<char>(tolower(static_cast<unsigned char>(prefix[i])));
        if (a != b) {
            return false;
        }
    }
    return true;
}
}

Terminal::Terminal(const TerminalOptions& options)
    : m_options(options)
    , m_buffers((std::max)(1, options.Cols), (std::max)(1, options.Rows), (std::max)(0, options.Scrollback)) {
    m_options.Cols = (std::max)(1, m_options.Cols);
    m_options.Rows = (std::max)(1, m_options.Rows);

    m_input = std::make_unique<InputHandler>(
        m_buffers,
        [this]() { RequestRedraw(); },
        [this](const std::string& data) { SendData(data); },
        m_links,
        [this](const std::string& title) {
            m_title = title;
            if (TitleChanged) TitleChanged(title);
        },
        [this](const std::string& text) {
            if (ClipboardSetRequested) ClipboardSetRequested(text);
        },
        [this](std::string& text) -> bool {
            return ClipboardGetRequested ? ClipboardGetRequested(text) : false;
        },
        [this](int which, const std::string& data) { HandleOscColor(which, data); });

    m_input->Attach(m_parser);
}

Terminal::~Terminal() {
    if (m_disposed) {
        return;
    }
    m_disposed = true;
    Detach();
}

void Terminal::Attach(ITerminalBackend* backend) {
    Detach();
    if (!backend) {
        return;
    }
    m_backend = backend;
    m_acceptingOutput.store(true);
    backend->SetOutputCallback([this](const char* data, size_t length) {
        OnBackendOutput(data, length);
    });
    backend->Start(Cols(), Rows());
}

void Terminal::Detach() {
    if (m_backend == nullptr) {
        return;
    }
    m_acceptingOutput.store(false);
    m_backend->SetOutputCallback(nullptr);
    m_backend = nullptr;

    // Unblock any reader thread waiting on back-pressure and drop queued bytes.
    {
        std::lock_guard<std::mutex> lock(m_sync);
        m_pendingOutput.clear();
        m_pendingBytes = 0;
    }
    m_backPressure.notify_all();
}

void Terminal::Write(const char* data, size_t length) {
    if (!data || length == 0) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_sync);
    m_parser.Parse(reinterpret_cast<const uint8_t*>(data), length);
}

bool Terminal::HasPendingOutput() const {
    std::lock_guard<std::mutex> lock(m_sync);
    return !m_pendingOutput.empty();
}

bool Terminal::FlushPendingOutput() {
    // Drain under the lock, parse outside it. Holding m_sync across Parse()
    // stalls the PTY reader for the entire VT workload and looks like a freeze
    // under `yes` / Linux bash floods.
    std::vector<std::string> batch;
    size_t taken = 0;
    {
        std::unique_lock<std::mutex> lock(m_sync);
        while (!m_pendingOutput.empty() && taken < MaxFlushBytes) {
            batch.push_back(std::move(m_pendingOutput.front()));
            m_pendingOutput.pop_front();
            const size_t n = batch.back().size();
            m_pendingBytes = n >= m_pendingBytes ? 0 : m_pendingBytes - n;
            taken += n;
        }
    }
    if (!batch.empty()) {
        m_backPressure.notify_all();
    }

    bool parsed = false;
    for (const std::string& chunk : batch) {
        m_parser.Parse(reinterpret_cast<const uint8_t*>(chunk.data()), chunk.size());
        parsed = true;
    }

    m_flushScheduled.store(0);

    bool more;
    {
        std::lock_guard<std::mutex> lock(m_sync);
        more = !m_pendingOutput.empty();
    }
    if (more) {
        ScheduleOutputFlush();
    }

    if (parsed) {
        RequestRedraw();
    }
    return more;
}

void Terminal::SendData(const std::string& data) {
    if (data.empty()) {
        return;
    }
    if (m_backend) {
        m_backend->Write(data.data(), data.size());
    }
}

void Terminal::SendBinary(const char* data, size_t length) {
    if (!data || length == 0) {
        return;
    }
    if (m_backend) {
        m_backend->Write(data, length);
    }
}

void Terminal::NotifyFocus(bool focused) {
    if (m_focused == focused) {
        return;
    }
    m_focused = focused;
    if (m_input->FocusReporting()) {
        SendData(focused ? "\x1b[I" : "\x1b[O");
    }
    RequestRedraw();
}

bool Terminal::TryReportMouse(int button, int col, int row, bool press, unsigned mods,
                              double cellW, double cellH) {
    if (!m_input->MouseTracking()) {
        return false;
    }
    if (m_input->MousePixels()) {
        col = static_cast<int>(col * cellW);
        row = static_cast<int>(row * cellH);
    }
    std::string seq;
    if (!MouseReporter::Encode(m_input->MouseEncoding(), button, col, row, press, mods, seq)) {
        return false;
    }
    SendData(seq);
    return true;
}

void Terminal::Resize(int cols, int rows) {
    cols = (std::max)(1, cols);
    rows = (std::max)(1, rows);
    if (cols == m_options.Cols && rows == m_options.Rows) {
        return;
    }
    m_options.Cols = cols;
    m_options.Rows = rows;
    m_buffers.Resize(cols, rows);
    m_input->OnColsChanged(cols);
    if (m_backend) {
        m_backend->Resize(cols, rows);
    }
    if (ResizeCompleted) ResizeCompleted();
    RequestRedraw();
}

void Terminal::Reset() {
    m_parser.Reset();
    m_input->Reset();
    m_buffers.Reset();
    m_selection.Clear();
    m_links.Clear();
    RequestRedraw();
}

void Terminal::Clear() {
    m_buffers.Active().ClearViewport();
    RequestRedraw();
}

void Terminal::ScrollLines(int delta) {
    TerminalBuffer& buf = m_buffers.Active();
    const int max = (std::max)(0, buf.BaseY());
    buf.YDisp = std::clamp(buf.YDisp + delta, 0, max);
    if (ScrollChanged) ScrollChanged();
    RequestRedraw();
}

void Terminal::ScrollToBottom() {
    m_buffers.Active().YDisp = 0;
    if (ScrollChanged) ScrollChanged();
    RequestRedraw();
}

void Terminal::SetScrollDisp(int yDisp) {
    const int max = (std::max)(0, m_buffers.Active().BaseY());
    m_buffers.Active().YDisp = std::clamp(yDisp, 0, max);
    if (ScrollChanged) ScrollChanged();
    RequestRedraw();
}

std::wstring Terminal::GetSelectionTextW() {
    SelectionModel& m = m_selection.Model();
    if (!m.HasSelection) {
        return std::wstring();
    }
    m.Normalize();
    return m_buffers.Active().GetSelectedText(m.StartCol, m.StartRow, m.EndCol, m.EndRow);
}

std::string Terminal::GetSelectionText() {
    return Utf8FromUtf16(GetSelectionTextW());
}

int Terminal::CountMatches(const std::wstring& query) {
    if (query.empty()) {
        return 0;
    }
    int n = 0;
    TerminalBuffer& buf = m_buffers.Active();
    for (int y = 0; y < buf.Length(); ++y) {
        const std::wstring text = buf.GetLine(y).GetTrimmedText();
        int start = 0;
        for (;;) {
            const int idx = IndexOfIgnoreCase(text, query, start);
            if (idx < 0) {
                break;
            }
            n++;
            start = idx + (std::max)(1, static_cast<int>(query.size()));
        }
    }
    return n;
}

bool Terminal::Find(const std::wstring& query, int& absRow, int& col, bool forward) {
    if (query.empty()) {
        return false;
    }
    TerminalBuffer& buf = m_buffers.Active();
    if (forward) {
        for (int y = (std::max)(0, absRow); y < buf.Length(); ++y) {
            const std::wstring text = buf.GetLine(y).GetTrimmedText();
            const int start = (y == absRow) ? (std::max)(0, col) : 0;
            const int idx = IndexOfIgnoreCase(text, query, start);
            if (idx >= 0) {
                ApplyFindHit(y, idx, static_cast<int>(query.size()));
                absRow = y;
                col = idx;
                return true;
            }
        }
    } else {
        for (int y = (std::min)(absRow, buf.Length() - 1); y >= 0; --y) {
            const std::wstring text = buf.GetLine(y).GetTrimmedText();
            const int end = (y == absRow)
                ? (std::min)(static_cast<int>(text.size()), (std::max)(0, col))
                : static_cast<int>(text.size());
            const int idx = LastIndexOfIgnoreCase(text, query, (std::max)(0, end - 1));
            if (idx >= 0 && (y != absRow || idx < col)) {
                ApplyFindHit(y, idx, static_cast<int>(query.size()));
                absRow = y;
                col = idx;
                return true;
            }
        }
    }
    return false;
}

void Terminal::ApplyFindHit(int y, int idx, int len) {
    TerminalBuffer& buf = m_buffers.Active();
    SelectionModel& model = m_selection.Model();
    model.HasSelection = true;
    model.StartRow = model.EndRow = y;
    model.StartCol = idx;
    model.EndCol = idx + (std::max)(1, len) - 1;
    const int top = buf.BaseY() - buf.YDisp;
    if (y < top || y >= top + Rows()) {
        SetScrollDisp((std::max)(0, buf.BaseY() - y));
    }
    RequestRedraw();
}

void Terminal::HandleOscColor(int which, const std::string& data) {
    TerminalTheme& theme = m_options.Theme;
    if (!data.empty() && data[0] == '?') {
        // Report current color as rgb:RRRR/GGGG/BBBB
        TermColor c;
        switch (which) {
        case 10: c = theme.Foreground; break;
        case 11: c = theme.Background; break;
        case 12: c = theme.Cursor; break;
        default: c = theme.Foreground; break;
        }
        SendData("\x1b]" + std::to_string(which) + ";rgb:" +
                 ToUpperHex2(c.r) + ToUpperHex2(c.r) + "/" +
                 ToUpperHex2(c.g) + ToUpperHex2(c.g) + "/" +
                 ToUpperHex2(c.b) + ToUpperHex2(c.b) + "\x07");
        return;
    }

    if (data == "reset" || which == 104 || which == 110 || which == 111 || which == 112) {
        if (which == 104 || data == "reset") {
            const bool light = theme.Background.r > 128;
            m_options.Theme = light ? TerminalTheme::Light() : TerminalTheme::Dark();
        }
        // soft reset individual channels
        if (which == 110) m_options.Theme.Foreground = TerminalTheme::Dark().Foreground;
        if (which == 111) m_options.Theme.Background = TerminalTheme::Dark().Background;
        if (which == 112) m_options.Theme.Cursor = TerminalTheme::Dark().Cursor;
        if (ThemeChanged) ThemeChanged();
        RequestRedraw();
        return;
    }

    TermColor color;
    if (TryParseOscColor(data, color)) {
        switch (which) {
        case 10: theme.Foreground = color; break;
        case 11: theme.Background = color; break;
        case 12: theme.Cursor = color; break;
        case 4:
            // format index;color - optional palette poke
            break;
        default: break;
        }
        if (ThemeChanged) ThemeChanged();
        RequestRedraw();
    }
}

bool Terminal::TryParseOscColor(const std::string& raw, TermColor& color) {
    const std::string data = TrimAscii(raw);
    // rgb:RRRR/GGGG/BBBB or #RRGGBB
    if (!data.empty() && data[0] == '#' && data.size() >= 7) {
        uint8_t r, g, b;
        if (ParseHexByte(data.substr(1, 2), r) &&
            ParseHexByte(data.substr(3, 2), g) &&
            ParseHexByte(data.substr(5, 2), b)) {
            color = TermColor::FromRgb(r, g, b);
            return true;
        }
        return false;
    }

    if (StartsWithIgnoreCase(data, "rgb:")) {
        const std::string body = data.substr(4);
        std::string parts[3];
        int partIndex = 0;
        for (char c : body) {
            if (c == '/') {
                partIndex++;
                if (partIndex >= 3) {
                    break;
                }
                continue;
            }
            parts[partIndex].push_back(c);
        }
        if (partIndex >= 2) {
            uint8_t r, g, b;
            if (ParseHexByte(parts[0], r) && ParseHexByte(parts[1], g) && ParseHexByte(parts[2], b)) {
                color = TermColor::FromRgb(r, g, b);
                return true;
            }
        }
    }
    return false;
}

std::string Terminal::GetLinkAt(int absRow, int col) {
    TerminalBuffer& buf = m_buffers.Active();
    if (absRow < 0 || absRow >= buf.Length()) {
        return std::string();
    }
    BufferLine& line = buf.GetLine(absRow);
    if (col < 0 || col >= line.Length()) {
        return std::string();
    }
    return m_links.GetUrl(line[col].LinkId);
}

void Terminal::OnBackendOutput(const char* data, size_t length) {
    if (!data || length == 0) {
        return;
    }

    // Queue only - never parse on the reader thread (a `yes` flood would race the
    // UI thread and stall the window).
    {
        std::unique_lock<std::mutex> lock(m_sync);
        // Back-pressure: block the PTY reader when the UI is behind (no data loss).
        while (m_pendingBytes > MaxFlushBytes * 4 && m_acceptingOutput.load()) {
            m_backPressure.wait_for(lock, std::chrono::milliseconds(32));
        }
        m_pendingOutput.emplace_back(data, length);
        m_pendingBytes += length;
    }

    ScheduleOutputFlush();
}

void Terminal::ScheduleOutputFlush() {
    int expected = 0;
    if (!m_flushScheduled.compare_exchange_strong(expected, 1)) {
        return;
    }
    if (OutputFlushRequested) {
        OutputFlushRequested();
    }
}

void Terminal::RequestRedraw() {
    m_redrawVersion++;
    if (RedrawRequested) {
        RedrawRequested();
    }
}

} // namespace Term
} // namespace CUI
