#pragma once
// Diagnostics for ScrollViewer / PropertyGrid scroll path.
// Log: %TEMP%\CUI_ScrollViewer.log

#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include <mutex>

namespace CUI {
namespace ScrollDiag {

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
        snprintf(path, sizeof(path), "%sCUI_ScrollViewer.log", tempPath);
    }
    return path;
}

inline void EnsureOpen() {
    if (File()) {
        return;
    }
    // Allow other processes to read the log while we append (diagnostics).
    File() = _fsopen(LogPathA(), "a", _SH_DENYNO);
    if (File()) {
        fprintf(File(), "\n==== CUI ScrollViewer diag session start ====\n");
        fprintf(File(), "logPath=%s\n", LogPathA());
        fflush(File());
    }
}

inline void Log(const char* fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

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

inline unsigned& RenderCount() {
    static unsigned n = 0;
    return n;
}

inline bool ShouldLogDetail(unsigned n) {
    return n <= 40u || (n % 15u) == 0u;
}

} // namespace ScrollDiag
} // namespace CUI
