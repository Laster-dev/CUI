#pragma once

#include "framework/window/Window.h"
#include "framework/controls/Image.h"
#include "framework/controls/Toast.h"
#include <memory>

struct ShowcaseContext {
    CUI::Window* windowRef = nullptr;
    std::shared_ptr<CUI::Image> streamImage;
    std::shared_ptr<CUI::Toast> toastTemplate;
};
