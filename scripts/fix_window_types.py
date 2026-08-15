import os
import re

# 1. Window.h
path_win = r'E:\C++project\CUI\CUI.Core\ui\framework\window\Window.h'
with open(path_win, 'r', encoding='utf-8') as f:
    win = f.read()

win = win.replace('static LRESULT CALLBACK WindowProc(HWND hwnd,', 'static LRESULT CALLBACK WindowProc(::HWND hwnd,')
win = win.replace('HWND m_hwnd = nullptr;', '::HWND m_hwnd = nullptr;')
win = win.replace('BackdropType m_backdropType = BackdropType::None;', 'CUI::BackdropType m_backdropType = CUI::BackdropType::None;')
win = win.replace('ThemeMode m_themeMode = ThemeMode::Dark;', 'CUI::ThemeMode m_themeMode = CUI::ThemeMode::Dark;')
win = win.replace('Event<Window*, ThemeMode> m_onThemeChanged;', 'Event<Window*, CUI::ThemeMode> m_onThemeChanged;')
win = win.replace('Event<Window*, ThemeMode>& OnThemeChanged()', 'Event<Window*, CUI::ThemeMode>& OnThemeChanged()')
win = win.replace('void SetBackdropType(BackdropType type);', 'void SetBackdropType(CUI::BackdropType type);')
win = win.replace('BackdropType GetBackdropType() const', 'CUI::BackdropType GetBackdropType() const')
win = win.replace('void SetThemeMode(ThemeMode theme);', 'void SetThemeMode(CUI::ThemeMode theme);')
win = win.replace('ThemeMode GetThemeMode() const', 'CUI::ThemeMode GetThemeMode() const')
win = win.replace('void SetThemeModeWithRipple(ThemeMode theme, Point origin);', 'void SetThemeModeWithRipple(CUI::ThemeMode theme, Point origin);')

# Struct property implementations inside Window.h
win = re.sub(r'struct WindowThemeModeProperty \{[\s\S]*?\} HWND;', '''    struct WindowThemeModeProperty {
        Window* owner = nullptr;
        WindowThemeModeProperty() = default;
        explicit WindowThemeModeProperty(Window* o) : owner(o) {}
        WindowThemeModeProperty& operator=(CUI::ThemeMode m) { if (owner) owner->SetThemeMode(m); return *this; }
        operator CUI::ThemeMode() const { return owner ? owner->GetThemeMode() : CUI::ThemeMode::Dark; }
        CUI::ThemeMode Get() const { return owner ? owner->GetThemeMode() : CUI::ThemeMode::Dark; }
    } ThemeMode;

    struct WindowBackdropTypeProperty {
        Window* owner = nullptr;
        WindowBackdropTypeProperty() = default;
        explicit WindowBackdropTypeProperty(Window* o) : owner(o) {}
        WindowBackdropTypeProperty& operator=(CUI::BackdropType b) { if (owner) owner->SetBackdropType(b); return *this; }
        operator CUI::BackdropType() const { return owner ? owner->GetBackdropType() : CUI::BackdropType::None; }
        CUI::BackdropType Get() const { return owner ? owner->GetBackdropType() : CUI::BackdropType::None; }
    } BackdropType;

    struct WindowRenderStatsProperty {
        Window* owner = nullptr;
        WindowRenderStatsProperty() = default;
        explicit WindowRenderStatsProperty(Window* o) : owner(o) {}
        WindowRenderStatsProperty& operator=(bool v) { if (owner) owner->SetRenderStatsOverlayVisible(v); return *this; }
        operator bool() const { return owner ? owner->IsRenderStatsOverlayVisible() : false; }
        bool Get() const { return owner ? owner->IsRenderStatsOverlayVisible() : false; }
    } RenderStatsOverlayVisible;

    struct WindowRootElementProperty {
        Window* owner = nullptr;
        WindowRootElementProperty() = default;
        explicit WindowRootElementProperty(Window* o) : owner(o) {}
        WindowRootElementProperty& operator=(std::shared_ptr<UIElement> r) { if (owner) owner->SetRootElement(std::move(r)); return *this; }
        operator std::shared_ptr<UIElement>() const { return owner ? owner->GetRootElement() : nullptr; }
        std::shared_ptr<UIElement> Get() const { return owner ? owner->GetRootElement() : nullptr; }
        std::shared_ptr<UIElement> operator->() const { return owner ? owner->GetRootElement() : nullptr; }
    } RootElement;

    struct WindowHWNDProperty {
        Window* owner = nullptr;
        WindowHWNDProperty() = default;
        explicit WindowHWNDProperty(Window* o) : owner(o) {}
        operator ::HWND() const { return owner ? owner->GetHWND() : nullptr; }
        ::HWND Get() const { return owner ? owner->GetHWND() : nullptr; }
    } HWND;''', win)

with open(path_win, 'w', encoding='utf-8') as f:
    f.write(win)

# 2. Window.cpp
path_win_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\window\Window.cpp'
with open(path_win_cpp, 'r', encoding='utf-8') as f:
    wcpp = f.read()

wcpp = wcpp.replace('void Window::SetBackdropType(BackdropType type)', 'void Window::SetBackdropType(CUI::BackdropType type)')
wcpp = wcpp.replace('void Window::SetThemeMode(ThemeMode theme)', 'void Window::SetThemeMode(CUI::ThemeMode theme)')
wcpp = wcpp.replace('void Window::SetThemeModeWithRipple(ThemeMode theme, Point origin)', 'void Window::SetThemeModeWithRipple(CUI::ThemeMode theme, Point origin)')
wcpp = wcpp.replace('LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)',
                    'LRESULT CALLBACK Window::WindowProc(::HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)')
wcpp = wcpp.replace('HWND other = (HWND)wParam;', '::HWND other = (::HWND)wParam;')
wcpp = wcpp.replace('HWND other = (HWND)lParam;', '::HWND other = (::HWND)lParam;')
wcpp = wcpp.replace('HWND fore = ::GetForegroundWindow();', '::HWND fore = ::GetForegroundWindow();')
wcpp = wcpp.replace('(HWND)', '(::HWND)')

with open(path_win_cpp, 'w', encoding='utf-8') as f:
    f.write(wcpp)

print("Updated Window.h and Window.cpp types.")
