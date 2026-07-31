#pragma once
#include "Window.h"
#include "../controls/Panel.h"
#include "../controls/TextBlock.h"
#include "../controls/Button.h"
#include <string>
#include <memory>

namespace CUI {

class CUIWindow {
public:
    CUIWindow(const std::string& title = "CUI Modern Window", int width = 1200, int height = 750);
    ~CUIWindow() = default;

    void SetContent(std::shared_ptr<UIElement> rootContent);
    void Show();
    void Run();

    Window& GetNativeWindow() { return m_window; }
    HWND GetHWND() const { return m_window.GetHWND(); }

private:
    void SetupTitleBar(const std::string& title);

    Window m_window;
    std::shared_ptr<StackPanel> m_rootContainer;
    std::shared_ptr<Panel> m_titleBar;
    std::shared_ptr<Panel> m_contentContainer;
};

} // namespace CUI
