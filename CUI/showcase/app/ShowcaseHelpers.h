#pragma once

#include "ShowcaseContext.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/PropertyGrid.h"

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

std::shared_ptr<CUI::UIElement> CreateRightPanel(
    std::initializer_list<std::shared_ptr<CUI::UIElement>> children);

std::shared_ptr<CUI::UIElement> CreateRightScrollPanel(
    std::initializer_list<std::shared_ptr<CUI::UIElement>> children);

std::shared_ptr<CUI::UIElement> CreatePropertyGrid(
    const ShowcaseContext& ctx,
    const std::shared_ptr<CUI::UIElement>& target);

std::shared_ptr<CUI::UIElement> CreateCodeExampleCollapse(
    const std::shared_ptr<CUI::UIElement>& target);

// WinUI Gallery-like page: demo on top, code inside CollapsePanel, PropertyGrid on the right.
std::shared_ptr<CUI::UIElement> CreatePage(
    const std::string& title,
    const std::string& subtitle,
    const std::shared_ptr<CUI::UIElement>& demo,
    const std::shared_ptr<CUI::UIElement>& side,
    const std::shared_ptr<CUI::UIElement>& sampleTarget = nullptr);
