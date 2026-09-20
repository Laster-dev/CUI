#pragma once

#include "framework/window/Window.h"
#include "framework/controls/Image.h"
#include "framework/core/Widget.h"
#include <memory>

std::shared_ptr<CUI::Image> CreateStreamImage();
void StartStreamingThread(CUI::Window* window, const CUI::Widgets::Ref<CUI::Image>& image);
void StopStreamingThread();
