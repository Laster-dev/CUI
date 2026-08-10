#include "EverythingApp.h"
#include <windows.h>
#include <shellscalingapi.h>
#include <objbase.h>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        SetProcessDPIAware();
    }

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    int exitCode = 0;
    try {
        EverythingNEO::EverythingApp app;
        exitCode = app.Run();
    } catch (const std::exception& ex) {
        MessageBoxA(nullptr, ex.what(), "EverythingNEO Error", MB_ICONERROR | MB_OK);
        exitCode = 1;
    }

    if (SUCCEEDED(hr)) {
        CoUninitialize();
    }
    return exitCode;
}
