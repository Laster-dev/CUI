#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "CUIWindow.h"
#include "../style/ThemeManager.h"
#include "../core/CUIDsl.h"

namespace CUI {

CUIWindow::CUIWindow(const std::string& title, int width, int height) {
    auto& theme = ThemeManager::Instance();
    m_rootContainer = DSL::Fluent::StackPanel()
        .Orientation(Orientation::Vertical)
        .BackgroundToken(ThemeTokenId::WindowBackground)
        .Background(theme.GetColor("windowBackground"))
        .Build();

    SetupHeader(title);

    m_contentContainer = DSL::Fluent::Panel()
        .FlexGrow(1.0f)
        .Align(Alignment::Stretch)
        .Build();
    DSL::ElementBuilder<StackPanel>(m_rootContainer)
        .AddChild(m_headerBar)
        .AddChild(m_contentContainer);

    m_window.Fluent()
        .Title(title)
        .Size(width, height)
        .Root(m_rootContainer)
        .Build();
}

void CUIWindow::SetupHeader(const std::string& title) {
    auto& theme = ThemeManager::Instance();
    m_headerBar = DSL::Fluent::Panel()
        .Height(32.0f)
        .BackgroundToken(ThemeTokenId::PaneBackground)
        .Background(theme.GetColor("paneBackground"))
        .Build();

    auto txtTitle = DSL::Fluent::TextBlock(title)
        .FontSize(12.0f)
        .ForegroundToken(ThemeTokenId::TextPrimary)
        .Foreground(theme.GetColor("textPrimary"))
        .Margin(12.0f, 8.0f, 0.0f, 0.0f)
        .Build();
    DSL::ElementBuilder<Panel>(m_headerBar).AddChild(txtTitle);
}

CUIWindow& CUIWindow::Content(std::shared_ptr<UIElement> rootContent) {
    if (m_contentContainer) {
        DSL::ElementBuilder<Panel>(m_contentContainer).ClearChildren();
        if (rootContent) {
            DSL::ElementBuilder<Panel>(m_contentContainer).AddChild(rootContent);
        }
    }
    return *this;
}

CUIWindow& CUIWindow::Show() {
    m_window.Show();
    return *this;
}

CUIWindow& CUIWindow::Run() {
    m_window.RunMessageLoop();
    return *this;
}

} // namespace CUI



