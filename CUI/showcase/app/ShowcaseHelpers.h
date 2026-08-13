#pragma once

#include "ShowcaseContext.h"
#include "framework/core/CUIDsl.h"

std::shared_ptr<CUI::UIElement> CreateShowcaseText(
    const std::string& content,
    float size,
    const std::string& color,
    bool bold = false,
    const std::string& fontFamily = "");

std::shared_ptr<CUI::UIElement> CreateShowcaseHeader(
    const std::string& title,
    const std::string& subtitle);

std::shared_ptr<CUI::UIElement> CreateDemoSurface(
    std::initializer_list<std::shared_ptr<CUI::UIElement>> children,
    float gap = 16.0f);

std::shared_ptr<CUI::UIElement> CreatePage(
    const std::string& title,
    const std::string& subtitle,
    const std::shared_ptr<CUI::UIElement>& demo);
