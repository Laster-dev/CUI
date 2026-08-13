#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "chrome/GalleryShell.h"

#include "framework/window/Window.h"

#include <windows.h>
#include <objbase.h>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        SetProcessDPIAware();
    }

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        return -1;
    }

    int exitCode = -1;
    {
        CUI::Window window;
        if (window.Create("CUI Gallery", 1280, 800, false)) {
            window.SetRootElement(Gallery::BuildGalleryRoot());
            window.Show();
            window.RunMessageLoop();
            exitCode = 0;
        }
    }

    CoUninitialize();
    return exitCode;
}
