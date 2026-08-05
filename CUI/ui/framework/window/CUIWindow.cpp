#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "CUIWindow.h"
#include "../style/ThemeManager.h"

namespace CUI {

CUIWindow::CUIWindow(const std::string& title, int width, int height) {
    auto& theme = ThemeManager::Instance();
    m_rootContainer = std::make_shared<StackPanel>();
    m_rootContainer->SetOrientation(Orientation::Vertical);
    m_rootContainer->SetBackgroundToken(ThemeTokenId::WindowBackground);
    m_rootContainer->SetBackground(theme.GetColor("windowBackground"));

    SetupTitleBar(title);

    m_contentContainer = std::make_shared<Panel>();
    m_contentContainer->SetFlexGrow(1.0f);
    m_contentContainer->SetAlign(Alignment::Stretch);

    m_rootContainer->AddChild(m_titleBar);
    m_rootContainer->AddChild(m_contentContainer);

    m_window.Create(title, width, height);
    m_window.SetRootElement(m_rootContainer);
}

void CUIWindow::SetupTitleBar(const std::string& title) {
    auto& theme = ThemeManager::Instance();
    m_titleBar = std::make_shared<Panel>();
    m_titleBar->SetHeight(32.0f);
    m_titleBar->SetBackgroundToken(ThemeTokenId::TitleBarBackground);
    m_titleBar->SetBackground(theme.GetColor("titleBarBackground"));

    auto txtTitle = std::make_shared<TextBlock>(title);
    txtTitle->SetFontSize(12.0f);
    txtTitle->SetColorToken(ThemeTokenId::TitleBarText);
    txtTitle->SetColor(theme.GetColor("titleBarText"));
    txtTitle->SetMargin(Thickness(12, 8, 0, 0));
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
