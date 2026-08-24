#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "App.h"
#include <windows.h>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    auto app = std::make_shared<AutoGuardApp::AutoGuardApp>();
    return app->Run();
}


