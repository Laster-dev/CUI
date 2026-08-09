#include <shellscalingapi.h>
#include <windows.h>
#include <objbase.h>
#include "App.h"

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        SetProcessDPIAware();
    }

    const HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        return -1;
    }

    int exitCode = 0;
    {
        RegeditPlus::RegeditPlusApp app;
        exitCode = app.Run();
    }

    CoUninitialize();
    return exitCode;
}
