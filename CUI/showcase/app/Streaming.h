#pragma once

#include "framework/window/Window.h"
#include "framework/controls/Image.h"
#include <memory>

std::shared_ptr<CUI::Image> CreateStreamImage();
void StartStreamingThread(CUI::Window* window, const std::shared_ptr<CUI::Image>& image);
void StopStreamingThread();
