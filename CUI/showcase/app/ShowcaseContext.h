#pragma once

#include "framework/core/Widget.h"

#include "framework/window/Window.h"
#include "framework/controls/Image.h"
#include "framework/controls/Toast.h"
#include <memory>

struct ShowcaseContext {
    CUI::Window* windowRef = nullptr;
    CUI::Widgets::Ref<::CUI::Image> streamImage;
    CUI::Widgets::Ref<::CUI::Toast> toastTemplate;
};
