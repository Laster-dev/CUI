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
    return "";
}

inline void EnsureOpen() {
    // Disabled: do not write to %TEMP%\CUI_ProgressBar.log
}

inline void Log(const char* /*fmt*/, ...) {
    // Disabled: do not write to %TEMP%\CUI_ProgressBar.log
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

inline bool ShouldLogDetail(unsigned /*n*/) {
    return false;
}

} // namespace ProgressBarDiag
} // namespace CUI
