#pragma once
#include "../controls/UIElement.h"
#include "../render/GraphicsContext.h"
#include <windows.h>
#include <memory>
#include <string>

namespace CUI {

class Window {
public:
    Window();
    virtual ~Window();

    bool Create(const std::string& title, int width = 1280, int height = 800);
    void Show();
    void RunMessageLoop();

    void SetRootElement(std::shared_ptr<UIElement> root);
    std::shared_ptr<UIElement> GetRootElement() const { return m_rootElement; }

    HWND GetHWND() const { return m_hwnd; }
    GraphicsContext& GetGraphicsContext() { return m_gfxContext; }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void OnPaint();
    void OnResize(UINT width, UINT height);
    void OnMouseMove(int x, int y);
    void OnLButtonDown(int x, int y);
    void OnLButtonDblClick(int x, int y);
    void OnLButtonUp(int x, int y);
    void OnRButtonDown(int x, int y);

    HWND m_hwnd = nullptr;
    GraphicsContext m_gfxContext;
    std::shared_ptr<UIElement> m_rootElement;

    UIElement* m_hoveredElement = nullptr;
    UIElement* m_pressedElement = nullptr;
    UIElement* m_focusedElement = nullptr;
    std::shared_ptr<ContextMenu> m_activeContextMenu = nullptr;
    bool m_trackingMouse = false;
};

} // namespace CUI
