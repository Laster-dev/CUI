#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "CUIWindow.h"
#include "../style/ThemeManager.h"
#include "../core/CUIDsl.h"

namespace CUI {

CUIWindow::CUIWindow(const std::string& title, int width, int height) {
    auto& theme = ThemeManager::Instance();
    m_rootContainer = Widgets::StackPanel()
        .Orientation(Orientation::Vertical)
        .BackgroundToken(ThemeTokenId::WindowBackground)
        .Background(theme.GetColor("windowBackground"))
        .Shared();

    SetupHeader(title);

    m_contentContainer = Widgets::Panel()
        .FlexGrow(1.0f)
        .Align(Alignment::Stretch)
        .Shared();
    m_rootContainer->AddChild(m_headerBar);
    m_rootContainer->AddChild(m_contentContainer);

    m_window.Fluent()
        .Title(title)
        .Size(width, height)
        .Root(m_rootContainer)
        .Build();
}

void CUIWindow::SetupHeader(const std::string& title) {
    auto& theme = ThemeManager::Instance();
    m_headerBar = Widgets::Panel()
        .Height(32.0f)
        .BackgroundToken(ThemeTokenId::PaneBackground)
        .Background(theme.GetColor("paneBackground"))
        .Shared();

    auto txtTitle = Widgets::TextBlock(title)
        .FontSize(12.0f)
        .ForegroundToken(ThemeTokenId::TextPrimary)
        .Foreground(theme.GetColor("textPrimary"))
        .Margin(12.0f, 8.0f, 0.0f, 0.0f)
        .Shared();
    m_headerBar->AddChild(txtTitle);
}

CUIWindow& CUIWindow::Content(std::shared_ptr<UIElement> rootContent) {
    if (m_contentContainer) {
        m_contentContainer->ClearChildren();
        if (rootContent) {
            m_contentContainer->AddChild(rootContent);
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
