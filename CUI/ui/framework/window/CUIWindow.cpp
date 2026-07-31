#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "CUIWindow.h"

namespace CUI {

CUIWindow::CUIWindow(const std::string& title, int width, int height) {
    m_rootContainer = std::make_shared<StackPanel>();
    m_rootContainer->SetProperty("orientation", Value("Vertical"));
    m_rootContainer->SetProperty("background", Value(D2D1::ColorF(0x1E / 255.0f, 0x1E / 255.0f, 0x1E / 255.0f)));

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
    m_titleBar = std::make_shared<Panel>();
    m_titleBar->SetProperty("height", Value(32.0f));
    m_titleBar->SetProperty("background", Value(D2D1::ColorF(0x18 / 255.0f, 0x18 / 255.0f, 0x18 / 255.0f))); // VS Code TitleBar Dark #181818

    // App Title Text
    auto txtTitle = std::make_shared<TextBlock>(title);
    txtTitle->SetProperty("fontSize", Value(12.0f));
    txtTitle->SetProperty("color", Value(D2D1::ColorF(0xCC / 255.0f, 0xCC / 255.0f, 0xCC / 255.0f)));
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
