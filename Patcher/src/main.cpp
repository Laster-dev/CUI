#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "framework/window/Window.h"
#include "view/MainView.h"

int main() {
    CUI::Window window;
    auto mainView = std::make_unique<Patcher::View::MainView>(&window);
    auto root = mainView->Build();

    window.Fluent()
        .Title("PE Patch 工具")
        .Size(680, 580)
        .MinimumSize(560, 320)
        .Root(root)
        .Build()
        .Show()
        .Run();

    return 0;
}
