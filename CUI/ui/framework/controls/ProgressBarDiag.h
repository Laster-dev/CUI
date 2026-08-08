#pragma once
// Temporary diagnostics for ProgressBar indeterminate / compose path.
// Log: %TEMP%\CUI_ProgressBar.log  (also copied path via OutputDebugString)

#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include <mutex>
#include <string>

namespace CUI {
namespace ProgressBarDiag {

inline std::mutex& Mutex() {
    static std::mutex m;
    return m;
}

inline FILE*& File() {
    static FILE* f = nullptr;
    return f;
}

inline const char* LogPathA() {
    static char path[MAX_PATH] = {};
    if (path[0] == '\0') {
        char tempPath[MAX_PATH] = {};
        GetTempPathA(MAX_PATH, tempPath);
        snprintf(path, sizeof(path), "%sCUI_ProgressBar.log", tempPath);
    }
    return path;
}

inline void EnsureOpen() {
    if (File()) {
        return;
    }
    // Shareable append so Notepad/readers can tail the log while the app runs.
    File() = _fsopen(LogPathA(), "a", _SH_DENYNO);
    if (File()) {
        fprintf(File(), "\n==== CUI ProgressBar diag session start ====\n");
        fprintf(File(), "logPath=%s\n", LogPathA());
        fflush(File());
        OutputDebugStringA("[PB] log opened: ");
        OutputDebugStringA(LogPathA());
        OutputDebugStringA("\n");
    } else {
        OutputDebugStringA("[PB] FAILED to open log file\n");
        OutputDebugStringA(LogPathA());
        OutputDebugStringA("\n");
    }
}

inline void Log(const char* fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    // Timestamp
    SYSTEMTIME st{};
    GetLocalTime(&st);
    char line[1200];
    snprintf(
        line,
        sizeof(line),
        "%02u:%02u:%02u.%03u %s",
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
        buf);

    std::lock_guard<std::mutex> lock(Mutex());
    EnsureOpen();
    OutputDebugStringA(line);
    OutputDebugStringA("\n");
    if (File()) {
        fprintf(File(), "%s\n", line);
        fflush(File());
    }
}

inline unsigned& TickCount() {
    static unsigned n = 0;
    return n;
}

inline unsigned& ComposeOk() {
    static unsigned n = 0;
    return n;
}

inline unsigned& ComposeFail() {
    static unsigned n = 0;
    return n;
}

inline unsigned& CommitComposeOnly() {
    static unsigned n = 0;
    return n;
}

inline unsigned& CommitFullPaint() {
    static unsigned n = 0;
    return n;
}

inline unsigned& OnPaintCount() {
    static unsigned n = 0;
    return n;
}

inline unsigned& OnRenderIndeterminate() {
    static unsigned n = 0;
    return n;
}

inline bool ShouldLogDetail(unsigned n) {
    return n <= 120 || (n % 30) == 0;
}

} // namespace ProgressBarDiag
} // namespace CUI
