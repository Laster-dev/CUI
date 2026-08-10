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

    SetupHeader(title);

    m_contentContainer = std::make_shared<Panel>();
    m_contentContainer->SetFlexGrow(1.0f);
    m_contentContainer->SetAlign(Alignment::Stretch);

    m_rootContainer->AddChild(m_headerBar);
    m_rootContainer->AddChild(m_contentContainer);

    m_window.Create(title, width, height);
    m_window.SetRootElement(m_rootContainer);
}

void CUIWindow::SetupHeader(const std::string& title) {
    auto& theme = ThemeManager::Instance();
    m_headerBar = std::make_shared<Panel>();
    m_headerBar->SetHeight(32.0f);
    m_headerBar->SetBackgroundToken(ThemeTokenId::PaneBackground);
    m_headerBar->SetBackground(theme.GetColor("paneBackground"));

    auto txtTitle = std::make_shared<TextBlock>(title);
    txtTitle->SetFontSize(12.0f);
    txtTitle->SetColorToken(ThemeTokenId::TextPrimary);
    txtTitle->SetColor(theme.GetColor("textPrimary"));
    txtTitle->SetMargin(Thickness(12, 8, 0, 0));
    m_headerBar->AddChild(txtTitle);
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
