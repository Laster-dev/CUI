#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "CUIWindow.h"
#include "../style/ThemeManager.h"

namespace CUI {

CUIWindow::CUIWindow(const std::string& title, int width, int height) {
    auto& theme = ThemeManager::Instance();
    m_rootContainer = std::make_shared<StackPanel>();
    m_rootContainer->SetProperty("orientation", Value("Vertical"));
    m_rootContainer->SetProperty("theme.backgroundToken", Value("windowBackground"));
    m_rootContainer->SetProperty("background", Value(theme.GetColor("windowBackground")));

    SetupTitleBar(title);

    m_contentContainer = std::make_shared<Panel>();
    m_contentContainer->SetProperty("flexGrow", Value(1.0f));
    m_contentContainer->SetProperty("align", Value("Stretch"));

    m_rootContainer->AddChild(m_titleBar);
    m_rootContainer->AddChild(m_contentContainer);

    m_window.Create(title, width, height);
    m_window.SetRootElement(m_rootContainer);
}

void CUIWindow::SetupTitleBar(const std::string& title) {
    auto& theme = ThemeManager::Instance();
    m_titleBar = std::make_shared<Panel>();
    m_titleBar->SetProperty("height", Value(32.0f));
    m_titleBar->SetProperty("theme.backgroundToken", Value("titleBarBackground"));
    m_titleBar->SetProperty("background", Value(theme.GetColor("titleBarBackground")));

    auto txtTitle = std::make_shared<TextBlock>(title);
    txtTitle->SetProperty("fontSize", Value(12.0f));
    txtTitle->SetProperty("theme.colorToken", Value("titleBarText"));
    txtTitle->SetProperty("color", Value(theme.GetColor("titleBarText")));
    txtTitle->SetProperty("margin", Value("12, 8, 0, 0"));
    m_titleBar->AddChild(txtTitle);
}

void CUIWindow::SetContent(std::shared_ptr<UIElement> rootContent) {
    if (m_contentContainer) {
        m_contentContainer->ClearChildren();
        if (rootContent) {
            m_contentContainer->AddChild(rootContent);
        }
    }
}

void CUIWindow::Show() {
    m_window.Show();
}

void CUIWindow::Run() {
    m_window.RunMessageLoop();
}

} // namespace CUI
