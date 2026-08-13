#include "Window.h"
#include "Dpi.h"
#include "IWindowChrome.h"
#include "MenuPopupWindow.h"
#include "../style/ThemeManager.h"
#include "../parser/StyleManager.h"
#include "../controls/TextBox.h"
#include "../controls/ContextMenu.h"
#include "../controls/MenuBar.h"
#include "../controls/DatePicker.h"
#include "../controls/TimePicker.h"
#include "../controls/ColorPicker.h"
#include "../controls/ComboBox.h"
#include "../controls/Flyout.h"
#include "../controls/ProgressBarDiag.h"
#include "../controls/docking/DockFloatWindow.h"
#include "../animation/FrameScheduler.h"
#include "../input/RoutedEvent.h"
#include <windowsx.h>
#include <dwmapi.h>
#include <imm.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <functional>
#include <sstream>
#include <vector>
#include <new>

#include <shellapi.h>
#include <ole2.h>

#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

namespace CUI {

Window* Window::s_current = nullptr;

Window* Window::Current() {
    return s_current;
}

namespace {
constexpr UINT WM_CUI_RASTER_COMPLETE = WM_APP + 45;
constexpr UINT WM_CUI_CLOSE_POPUPS = WM_APP + 46;

class WindowOleDropTarget final : public ::IDropTarget {
public:
    explicit WindowOleDropTarget(Window* window) : m_window(window) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) {
            return E_POINTER;
        }
        if (riid == IID_IUnknown || riid == IID_IDropTarget) {
            *ppv = static_cast<::IDropTarget*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return static_cast<ULONG>(InterlockedIncrement(&m_ref));
    }

    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG n = static_cast<ULONG>(InterlockedDecrement(&m_ref));
        if (n == 0) {
            delete this;
        }
        return n;
    }

    HRESULT STDMETHODCALLTYPE DragEnter(::IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override {
        (void)grfKeyState;
        if (!pdwEffect) {
            return E_POINTER;
        }
        *pdwEffect = DROPEFFECT_NONE;
        if (!m_window || !pDataObj) {
            return E_INVALIDARG;
        }
        DataPackage pkg = ToPackage(pDataObj);
        if (!pkg.HasFiles() && !pkg.HasText()) {
            return S_OK;
        }
        m_window->GetDragDrop().BeginExternal(std::move(pkg));
        const Point logical = ToLogical(pt);
        m_window->GetDragDrop().Update(logical, m_window->GetRootElement().get());
        *pdwEffect = ToOleEffect(m_window->GetDragDrop().GetCurrentEffect());
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override {
        (void)grfKeyState;
        if (!pdwEffect) {
            return E_POINTER;
        }
        *pdwEffect = DROPEFFECT_NONE;
        if (!m_window || !m_window->GetDragDrop().IsDragging()) {
            return S_OK;
        }
        const Point logical = ToLogical(pt);
        m_window->GetDragDrop().Update(logical, m_window->GetRootElement().get());
        *pdwEffect = ToOleEffect(m_window->GetDragDrop().GetCurrentEffect());
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE DragLeave() override {
        if (m_window) {
            m_window->GetDragDrop().EndExternal();
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Drop(::IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override {
        (void)grfKeyState;
        if (!pdwEffect) {
            return E_POINTER;
        }
        *pdwEffect = DROPEFFECT_NONE;
        if (!m_window) {
            return E_INVALIDARG;
        }
        const Point logical = ToLogical(pt);
        if (pDataObj) {
            DataPackage pkg = ToPackage(pDataObj);
            if (pkg.HasFiles() || pkg.HasText()) {
                m_window->GetDragDrop().BeginExternal(std::move(pkg));
            }
        }
        m_window->GetDragDrop().CompleteDrop(logical, m_window->GetRootElement().get());
        *pdwEffect = DROPEFFECT_COPY;
        return S_OK;
    }

private:
    static DWORD ToOleEffect(DragDropEffects effect) {
        if (effect == DragDropEffects::Copy) {
            return DROPEFFECT_COPY;
        }
        if (effect == DragDropEffects::Move) {
            return DROPEFFECT_MOVE;
        }
        if (effect == DragDropEffects::Link) {
            return DROPEFFECT_LINK;
        }
        return DROPEFFECT_NONE;
    }

    Point ToLogical(POINTL pt) const {
        POINT client{ pt.x, pt.y };
        ScreenToClient(m_window->GetHWND(), &client);
        return m_window->ClientPointToLogical(client.x, client.y);
    }

    static DataPackage ToPackage(::IDataObject* data) {
        DataPackage pkg;
        if (!data) {
            return pkg;
        }

        FORMATETC dropFmt{ CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        STGMEDIUM stg{};
        if (SUCCEEDED(data->GetData(&dropFmt, &stg))) {
            if (stg.hGlobal) {
                auto hdrop = static_cast<HDROP>(GlobalLock(stg.hGlobal));
                if (hdrop) {
                    const UINT count = DragQueryFileW(hdrop, 0xFFFFFFFF, nullptr, 0);
                    std::vector<std::string> files;
                    files.reserve(count);
                    for (UINT i = 0; i < count; ++i) {
                        const UINT len = DragQueryFileW(hdrop, i, nullptr, 0);
                        std::wstring path(len, L'\0');
                        if (len > 0) {
                            DragQueryFileW(hdrop, i, path.data(), len + 1);
                            files.push_back(Utf16ToUtf8(path));
                        }
                    }
                    pkg.SetFiles(std::move(files));
                    GlobalUnlock(stg.hGlobal);
                }
            }
            ReleaseStgMedium(&stg);
        }

        FORMATETC textFmt{ CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        if (SUCCEEDED(data->GetData(&textFmt, &stg))) {
            if (stg.hGlobal) {
                if (const auto* text = static_cast<const wchar_t*>(GlobalLock(stg.hGlobal))) {
                    pkg.SetText(Utf16ToUtf8(text));
                    GlobalUnlock(stg.hGlobal);
                }
            }
            ReleaseStgMedium(&stg);
        }
        return pkg;
    }

    Window* m_window = nullptr;
    volatile LONG m_ref = 1;
};

float GetWindowRefreshRateHz(HWND hwnd) {
    // EnumDisplaySettings is relatively expensive — cache per monitor briefly.
    struct Cache {
        HMONITOR monitor = nullptr;
        float hz = 60.0f;
        std::chrono::steady_clock::time_point at{};
    };
    static Cache cache;

    if (!hwnd) {
        return 60.0f;
    }

    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    const auto now = std::chrono::steady_clock::now();
    if (cache.monitor == monitor
        && std::chrono::duration<float>(now - cache.at).count() < 2.0f
        && cache.hz > 1.0f) {
        return cache.hz;
    }

    MONITORINFOEX monitorInfo = {};
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (!GetMonitorInfo(monitor, &monitorInfo)) {
        return 60.0f;
    }

    DEVMODE devMode = {};
    devMode.dmSize = sizeof(devMode);
    if (!EnumDisplaySettings(monitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &devMode)) {
        return 60.0f;
    }

    const DWORD hz = devMode.dmDisplayFrequency;
    cache.monitor = monitor;
    cache.at = now;
    cache.hz = (hz == 0 || hz == 1) ? 60.0f : static_cast<float>(hz);
    return cache.hz;
}

bool CoversRect(const Rect& outer, const Rect& inner, float epsilon = 1.0f) {
    if (inner.IsEmpty()) {
        return true;
    }
    if (outer.IsEmpty()) {
        return false;
    }

    return outer.x <= inner.x + epsilon
        && outer.y <= inner.y + epsilon
        && outer.x + outer.width >= inner.x + inner.width - epsilon
        && outer.y + outer.height >= inner.y + inner.height - epsilon;
}

// True only if some individual dirty rect covers the viewport — NOT the AABB of
// disjoint rects (pane ripple + content dirty), which would false-trigger full repaints.
bool AnyDirtyRectCovers(const DirtyRegion& region, const Rect& viewport, float epsilon = 1.0f) {
    for (const auto& rect : region.GetRects()) {
        if (CoversRect(rect, viewport, epsilon)) {
            return true;
        }
    }
    return false;
}

bool IsOverlayScrimAnimating(UIElement* element) {
    if (!element) {
        return false;
    }
    // Only freeze the scene during ContentDialog open/close animation.
    // Freezing for the whole modal lifetime left under-scrim button ripples stuck
    // (ticks advanced, but dirty rects were discarded without updating the cache).
    if (element->HasSelfAnimation() && element->IsModalOverlayOpen()) {
        return true;
    }
    for (const auto& child : element->GetChildren()) {
        if (IsOverlayScrimAnimating(child.get())) {
            return true;
        }
    }
    return false;
}

Rect GetClientBounds(HWND hwnd) {
    RECT rc = {};
    if (hwnd) {
        GetClientRect(hwnd, &rc);
    }
    return Rect(
        static_cast<float>(rc.left),
        static_cast<float>(rc.top),
        static_cast<float>(rc.right - rc.left),
        static_cast<float>(rc.bottom - rc.top)
    );
}

Rect PhysicalRectToLogical(const Rect& rect, float dpiScale) {
    const float scale = (dpiScale > 0.001f) ? dpiScale : 1.0f;
    return Rect(
        rect.x / scale,
        rect.y / scale,
        rect.width / scale,
        rect.height / scale
    );
}

Rect GetLogicalClientBounds(HWND hwnd, float dpiScale) {
    return PhysicalRectToLogical(GetClientBounds(hwnd), dpiScale);
}

IWindowChrome* FindWindowChrome(UIElement* root) {
    if (!root) {
        return nullptr;
    }
    for (const auto& child : root->GetChildren()) {
        if (auto* chrome = dynamic_cast<IWindowChrome*>(child.get())) {
            return chrome;
        }
    }
    return nullptr;
}

Rect GetChromeBounds(UIElement* root) {
    if (IWindowChrome* chrome = FindWindowChrome(root)) {
        if (const UIElement* chromeElement = chrome->GetChromeElement()) {
            return chromeElement->GetBounds();
        }
    }
    return Rect();
}

// Paint always resolves colors through ResolveThemeColor(Get*Token()), so this
// walk only needs to seed sensible default tokens where a control hasn't set
// its own — no legacy ColorF/string mirroring required.
void ApplyThemeToTree(UIElement* element, bool systemBackdrop) {
    if (!element) {
        return;
    }

    const std::string className = element->GetClassName();
    const ThemeTokens& tokens = ThemeManager::Instance().GetTokens();
    if (className == "MenuBar") {
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackgroundToken(ThemeTokenId::PaneBackground);
            element->SetBackground(ThemeManager::Instance().GetColor("paneBackground"));
        }
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColor(tokens.textPrimary);
        }
    } else if (className == "Button") {
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColor(tokens.accentForeground);
        }
        // Ghost icon buttons keep Unset tokens + an explicit transparent fill.
        // Do not stamp accent onto those — that made copy/clear look like solid squares.
        if (element->GetBackgroundToken() == ThemeTokenId::Unset && !element->HasBackgroundColor()) {
            element->SetBackground(tokens.accentColor);
        }
        if (element->GetHoverBackgroundToken() == ThemeTokenId::Unset && !element->HasHoverBackgroundColor()) {
            element->SetHoverBackground(tokens.accentColor);
        }
        if (element->GetPressedBackgroundToken() == ThemeTokenId::Unset && !element->HasPressedBackgroundColor()) {
            element->SetPressedBackground(tokens.accentColor);
        }
        if (element->GetBorderToken() == ThemeTokenId::Unset && !element->HasBorderBrushColor()) {
            element->SetBorderBrush(tokens.accentColor);
        }
    } else if (className == "PropertyGrid") {
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackgroundToken(ThemeTokenId::PaneBackground);
        }
        if (element->GetBorderToken() == ThemeTokenId::Unset) {
            element->SetBorderToken(ThemeTokenId::CardBorder);
        }
    } else if (className == "ContextMenu") {
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackground(tokens.cardBackground);
        }
        if (element->GetBorderToken() == ThemeTokenId::Unset) {
            element->SetBorderBrush(tokens.cardBorder);
        }
    } else if (className == "MenuItem") {
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColor(tokens.textSecondary);
        }
    } else if (className == "TextBox" || className == "FilePicker" || className == "FolderPicker") {
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColor(tokens.textPrimary);
        }
    } else if (className == "ComboBox") {
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColorToken(ThemeTokenId::TextPrimary);
        }
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackgroundToken(ThemeTokenId::InputBackground);
        }
        if (element->GetHoverBackgroundToken() == ThemeTokenId::Unset) {
            element->SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
        }
        if (element->GetBorderToken() == ThemeTokenId::Unset) {
            element->SetBorderToken(ThemeTokenId::InputBorder);
        }
        if (element->GetFocusedBorderToken() == ThemeTokenId::Unset) {
            element->SetFocusedBorderToken(ThemeTokenId::FocusedBorder);
        }
        if (element->GetDropdownBackgroundToken() == ThemeTokenId::Unset) {
            element->SetDropdownBackgroundToken(ThemeTokenId::CardBackground);
        }
        if (element->GetSelectedItemBackgroundToken() == ThemeTokenId::Unset) {
            element->SetSelectedItemBackgroundToken(ThemeTokenId::SelectedBackground);
        }
    } else if (className == "CheckBox") {
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColor(tokens.textPrimary);
        }
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackground(tokens.inputBackground);
        }
    } else if (className == "RadioButton") {
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColor(tokens.textPrimary);
        }
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackground(tokens.inputBackground);
        }
    } else if (className == "ToggleSwitch") {
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColor(tokens.textPrimary);
        }
        if (element->GetBorderToken() == ThemeTokenId::Unset) {
            element->SetBorderBrush(tokens.cardBorder);
        }
        if (element->GetHoverBackgroundToken() == ThemeTokenId::Unset) {
            element->SetHoverBackground(tokens.hoverBackground);
        }
        if (element->GetPressedBackgroundToken() == ThemeTokenId::Unset) {
            element->SetPressedBackground(tokens.pressedBackground);
        }
    } else if (className == "Slider") {
        if (element->GetTrackColorToken() == ThemeTokenId::Unset) {
            element->SetTrackColorToken(ThemeTokenId::CardBorder);
        }
        if (element->GetActiveTrackColorToken() == ThemeTokenId::Unset) {
            element->SetActiveTrackColorToken(ThemeTokenId::AccentColor);
        }
        if (element->GetThumbColorToken() == ThemeTokenId::Unset) {
            element->SetThumbColorToken(ThemeTokenId::AccentColor);
        }
    } else if (className == "Expander" || className == "CollapsePanel") {
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackgroundToken(ThemeTokenId::PaneBackground);
        }
        if (element->GetBorderToken() == ThemeTokenId::Unset) {
            element->SetBorderToken(ThemeTokenId::CardBorder);
        }
    } else if (className == "ListBox") {
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackgroundToken(ThemeTokenId::CardBackground);
        }
        if (element->GetBorderToken() == ThemeTokenId::Unset) {
            element->SetBorderToken(ThemeTokenId::CardBorder);
        }
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColorToken(ThemeTokenId::TextPrimary);
        }
        if (element->GetSelectedBackgroundToken() == ThemeTokenId::Unset) {
            element->SetSelectedBackgroundToken(ThemeTokenId::SelectedBackground);
        }
        if (element->GetHoverBackgroundToken() == ThemeTokenId::Unset) {
            element->SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
        }
    } else if (className == "ListView") {
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackgroundToken(ThemeTokenId::CardBackground);
        }
        if (element->GetHeaderBackgroundToken() == ThemeTokenId::Unset) {
            element->SetHeaderBackgroundToken(ThemeTokenId::PaneBackground);
        }
        if (element->GetBorderToken() == ThemeTokenId::Unset) {
            element->SetBorderToken(ThemeTokenId::CardBorder);
        }
        if (element->GetGridLineBrushToken() == ThemeTokenId::Unset) {
            element->SetGridLineBrushToken(ThemeTokenId::InputBorder);
        }
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColorToken(ThemeTokenId::TextPrimary);
        }
        if (element->GetSelectedBackgroundToken() == ThemeTokenId::Unset) {
            element->SetSelectedBackgroundToken(ThemeTokenId::SelectedBackground);
        }
        if (element->GetHoverBackgroundToken() == ThemeTokenId::Unset) {
            element->SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
        }
    } else if (className == "TabView") {
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackground(tokens.windowBackground);
        }
    } else if (className == "TreeView") {
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackgroundToken(ThemeTokenId::CardBackground);
        }
        if (element->GetBorderToken() == ThemeTokenId::Unset) {
            element->SetBorderToken(ThemeTokenId::CardBorder);
        }
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColorToken(ThemeTokenId::TextPrimary);
        }
        if (element->GetSelectedBackgroundToken() == ThemeTokenId::Unset) {
            element->SetSelectedBackgroundToken(ThemeTokenId::SelectedBackground);
        }
        if (element->GetHoverBackgroundToken() == ThemeTokenId::Unset) {
            element->SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
        }
    } else if (className == "BreadcrumbBar") {
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackground(tokens.cardBackground);
        }
        if (element->GetBorderToken() == ThemeTokenId::Unset) {
            element->SetBorderBrush(tokens.cardBorder);
        }
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColor(tokens.textSecondary);
        }
    } else if (className == "NavigationView") {
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackground(tokens.windowBackground);
        }
    } else if (className == "NavigationViewItem" || className == "NavigationViewItemHeader") {
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColorToken(ThemeTokenId::TextPrimary);
        }
    } else if (className == "TextBlock" || className == "HyperlinkButton") {
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColor(className == "HyperlinkButton" ? tokens.accentColor : tokens.textSecondary);
        }
    }

    for (const auto& child : element->GetChildren()) {
        ApplyThemeToTree(child.get(), systemBackdrop);
    }
}

void ForceThemeRefresh(UIElement* element, const std::string& refreshStamp, bool systemBackdrop) {
    if (!element) {
        return;
    }
    (void)refreshStamp;
    (void)systemBackdrop;

    element->OnThemeChanged();

    if (auto* menu = dynamic_cast<ContextMenu*>(element)) {
        if (auto subMenu = menu->GetActiveSubMenu()) {
            ApplyThemeToTree(subMenu.get(), systemBackdrop);
            ForceThemeRefresh(subMenu.get(), refreshStamp, systemBackdrop);
        }
    }

    for (const auto& child : element->GetChildren()) {
        ForceThemeRefresh(child.get(), refreshStamp, systemBackdrop);
    }
}
}

Window::Window() {
    s_current = this;
    m_sceneLayer.SetCacheable(true);
}

std::shared_ptr<UIElement> Window::CaptureElementRef(UIElement* element) {
    if (!element) return nullptr;
    try {
        return std::dynamic_pointer_cast<UIElement>(element->shared_from_this());
    } catch (const std::bad_weak_ptr&) {
        return nullptr;
    }
}

std::shared_ptr<UIElement> Window::LockElement(const std::weak_ptr<UIElement>& element) const {
    return element.lock();
}

bool Window::NeedsContinuousMouseRedraw(UIElement* element) {
    (void)element;
    // TitleBar/MenuBar already mark local dirty when hover index changes.
    // Returning true here forced a full title-bar Present on every WM_MOUSEMOVE pixel.
    return false;
}

void Window::SetHoveredElement(UIElement* element) {
    m_hoveredElement = CaptureElementRef(element);
}

void Window::SetPressedElement(UIElement* element) {
    m_pressedElement = CaptureElementRef(element);
}

void Window::SetFocusedElement(UIElement* element) {
    m_focusedElement = CaptureElementRef(element);
}

void Window::RequestFullRepaint() {
    if (!m_hwnd) {
        return;
    }

    m_sceneLayer.Invalidate(RenderLayer::ContentDirty | RenderLayer::StructureDirty | RenderLayer::SizeDirty);
    m_pendingDirtyRegion.Clear();
    if (m_rootElement) {
        m_pendingDirtyRegion.AddRect(m_rootElement->GetBounds());
    }
    RedrawWindow(m_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void Window::InvalidateLogicalRect(const Rect& rect) {
    if (rect.IsEmpty() || !m_hwnd) {
        return;
    }
    m_pendingDirtyRegion.AddRect(rect);
    InvalidatePendingRenderRegions(false);
}

void Window::InvalidateDragFeedback() {
    if (!m_hwnd) {
        return;
    }
    Rect all(0.0f, 0.0f, m_logicalClientSize.width, m_logicalClientSize.height);
    if (all.IsEmpty() && m_rootElement) {
        all = m_rootElement->GetBounds();
    }
    if (!all.IsEmpty()) {
        m_pendingDirtyRegion.AddRect(all);
    }
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void Window::RegisterShellDropTarget() {
    if (!m_hwnd || m_oleDropRegistered) {
        return;
    }
    const HRESULT ole = OleInitialize(nullptr);
    if (ole == S_OK) {
        m_needOleUninit = true;
    }
    auto* target = new (std::nothrow) WindowOleDropTarget(this);
    if (!target) {
        DragAcceptFiles(m_hwnd, TRUE);
        return;
    }
    const HRESULT hr = RegisterDragDrop(m_hwnd, target);
    if (SUCCEEDED(hr)) {
        m_oleDropRegistered = true;
        m_oleDropTarget = target;
        return;
    }
    target->Release();
    DragAcceptFiles(m_hwnd, TRUE);
}

void Window::RevokeShellDropTarget() {
    if (m_oleDropRegistered && m_hwnd) {
        RevokeDragDrop(m_hwnd);
        m_oleDropRegistered = false;
    }
    if (m_oleDropTarget) {
        static_cast<::IDropTarget*>(m_oleDropTarget)->Release();
        m_oleDropTarget = nullptr;
    }
}

void Window::InvalidatePendingRenderRegions(bool fallbackToFullWindow) {
    if (!m_hwnd) {
        return;
    }

    if (m_rootElement) {
        m_rootElement->CollectRenderDirtyRegion(m_pendingDirtyRegion, true);
    }

    if (m_pendingDirtyRegion.IsEmpty()) {
        if (fallbackToFullWindow) {
            RequestFullRepaint();
        }
        return;
    }

    // Many small/overlapping rects → one bounds invalidate. Do NOT RequestFullRepaint
    // (that StructureDirties the scene and locks 整帧 for the frame).
    if (m_pendingDirtyRegion.GetRectCount() > 8) {
        m_pendingDirtyRegion.CollapseToBounds();
    }

    // Dirty regions are DIP; InvalidateRect expects client pixels.
    const float scale = (m_dpiScale > 0.001f) ? m_dpiScale : 1.0f;
    for (const auto& rect : m_pendingDirtyRegion.GetRects()) {
        if (rect.IsEmpty()) {
            continue;
        }

        RECT rc = {
            static_cast<LONG>(std::floor(rect.x * scale)),
            static_cast<LONG>(std::floor(rect.y * scale)),
            static_cast<LONG>(std::ceil((rect.x + rect.width) * scale)),
            static_cast<LONG>(std::ceil((rect.y + rect.height) * scale))
        };
        InvalidateRect(m_hwnd, &rc, FALSE);
    }
}

bool Window::HasPendingNativePaint() const {
    if (!m_hwnd) {
        return false;
    }

    return GetUpdateRect(m_hwnd, nullptr, FALSE) != FALSE;
}

void Window::InvalidateAnimatedRegions(bool animationStillActive) {
    if (!m_hwnd || !m_rootElement) {
        return;
    }

    // Fresh dirty from current animators only (compose-only ProgressBar contributes nothing).
    Rect freshDirty;
    bool hasFresh = false;
    m_animationManager.CollectAnimatingBounds(freshDirty, hasFresh);
    m_popupHost.CollectDirty(freshDirty, hasFresh);
    m_dragDrop.CollectDirty(freshDirty, hasFresh);

    // Stale footprint from a previous frame must be erased once — but must NOT be
    // re-stored as lastAnimDirty while animationStillActive (that locked ProgressBar
    // into FULL_PAINT forever: lastAnimDirty=1 every CommitFrame).
    if (m_hasLastAnimationDirtyRect) {
        if (hasFresh && freshDirty.Intersects(m_lastAnimationDirtyRect.Inflate(8.0f))) {
            freshDirty = freshDirty.Union(m_lastAnimationDirtyRect);
            hasFresh = true;
        } else {
            m_pendingDirtyRegion.AddRect(m_lastAnimationDirtyRect.Inflate(2.0f));
        }
        m_lastAnimationDirtyRect = Rect();
        m_hasLastAnimationDirtyRect = false;
    }

    if (hasFresh) {
        const Rect expandedDirtyRect = freshDirty.Inflate(2.0f);
        m_pendingDirtyRegion.AddRect(expandedDirtyRect);
        InvalidatePendingRenderRegions(false);

        if (animationStillActive) {
            m_lastAnimationDirtyRect = expandedDirtyRect;
            m_hasLastAnimationDirtyRect = true;
        }
    } else if (!m_pendingDirtyRegion.IsEmpty()) {
        // Stale footprint was queued above — flush it.
        InvalidatePendingRenderRegions(false);
    }

    if (IsOverlayScrimAnimating(m_rootElement.get())) {
        InvalidateRect(m_hwnd, nullptr, FALSE);
    } else if (m_themeRippleActive) {
        // One InvalidateRect per frame tick (from the scheduler), never from
        // OnPaint — otherwise the WM_PAINT pump re-enters until the wave ends.
        m_frameScheduler.ScheduleFrame();
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void Window::CommitFrame(bool animationStillActive) {
    // ProgressBar indeterminate uses scene dirty + Present1 (DComp Commit-only
    // overlays are not visible on CreateSwapChainForComposition without a real
    // Present, and DXGI_PRESENT_DO_NOT_SEQUENCE stalled the flip queue to ~1 FPS).
    const bool composeFlushed = m_animationManager.FlushComposePresent(m_gfxContext);
    const bool sceneContrib = m_animationManager.HasSceneContributingAnimators();
    const bool hasComposeOnly = m_animationManager.HasComposeOnlyAnimating();
    const bool pendingEmpty = m_pendingDirtyRegion.IsEmpty();
    const bool scrim = IsOverlayScrimAnimating(m_rootElement.get());

    const bool onlyComposePresent =
        composeFlushed
        && !sceneContrib
        && pendingEmpty
        && !scrim;

    if (onlyComposePresent) {
        m_lastAnimationDirtyRect = Rect();
        m_hasLastAnimationDirtyRect = false;
        m_gfxContext.CommitComposition();
        const unsigned n = ++ProgressBarDiag::CommitComposeOnly();
        if (ProgressBarDiag::ShouldLogDetail(n)) {
            ProgressBarDiag::Log(
                "[PB] CommitFrame composeFlushed=1 sceneContrib=0 hasComposeOnly=%d "
                "pendingEmpty=1 scrim=0 => COMPOSE_ONLY(Commit) #%u",
                hasComposeOnly ? 1 : 0,
                n);
        }
        // Theme ripple still needs a WM_PAINT compose pass (old+ellipse clip).
        if (m_themeRippleActive && m_hwnd) {
            m_frameScheduler.ScheduleFrame();
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
        return;
    }

    if (!m_layerRasterizer.KickAsync()) {
        m_layerRasterizer.FlushSync(m_gfxContext);
    }

    // ProgressBar indeterminate is scene-strip (IsComposeOnlyAnimation=false). Log
    // that path too — otherwise the diag file dies after Window::Show.
    if (hasComposeOnly || composeFlushed || ProgressBarDiag::TickCount() > 0) {
        const unsigned n = ++ProgressBarDiag::CommitFullPaint();
        if (ProgressBarDiag::ShouldLogDetail(n)) {
            ProgressBarDiag::Log(
                "[PB] CommitFrame composeFlushed=%d sceneContrib=%d hasComposeOnly=%d "
                "lastAnimDirty=%d pendingEmpty=%d scrim=%d pbTicks=%u => FULL_PAINT_PATH #%u",
                composeFlushed ? 1 : 0,
                sceneContrib ? 1 : 0,
                hasComposeOnly ? 1 : 0,
                m_hasLastAnimationDirtyRect ? 1 : 0,
                pendingEmpty ? 1 : 0,
                scrim ? 1 : 0,
                ProgressBarDiag::TickCount(),
                n);
        }
    }

    InvalidateAnimatedRegions(animationStillActive);
    InvalidatePendingRenderRegions(false);
}

void Window::FlushLayoutIfNeeded() {
    if (!m_rootElement || !m_hwnd) {
        return;
    }
    if (!m_rootElement->IsMeasureDirty() && !m_rootElement->IsArrangeDirty()) {
        return;
    }
    ProgressBarDiag::Log(
        "[EXP-WIN] FlushLayoutIfNeeded rootMeasure=%d rootArrange=%d",
        m_rootElement->IsMeasureDirty() ? 1 : 0,
        m_rootElement->IsArrangeDirty() ? 1 : 0);
    RECT rc{};
    GetClientRect(m_hwnd, &rc);
    const float padLeft = 0.0f;
    const float padTop = 0.0f;
    const float layoutW = m_logicalClientSize.width > 0.0f
        ? m_logicalClientSize.width
        : static_cast<float>(rc.right - rc.left) / (m_dpiScale > 0.0f ? m_dpiScale : 1.0f);
    const float layoutH = m_logicalClientSize.height > 0.0f
        ? m_logicalClientSize.height
        : static_cast<float>(rc.bottom - rc.top) / (m_dpiScale > 0.0f ? m_dpiScale : 1.0f);
    m_rootElement->FlushLayout(Size(layoutW, layoutH), Rect(padLeft, padTop, layoutW, layoutH));
}

void Window::DispatchRoutedPointer(RoutedEventType type, Point pt, UIElement* target) {
    if (!target) {
        return;
    }
    std::vector<UIElement*> path;
    for (UIElement* walk = target; walk; walk = walk->GetParent()) {
        path.push_back(walk);
    }

    RoutedEventArgs args;
    args.type = type;
    args.position = pt;
    args.originalSource = target;

    args.phase = RoutedEventPhase::Tunnel;
    for (auto it = path.rbegin(); it != path.rend() && !args.handled; ++it) {
        if (*it != target) {
            (*it)->OnRoutedEvent(args);
        }
    }

    if (!args.handled) {
        args.phase = RoutedEventPhase::Target;
        target->OnRoutedEvent(args);
    }

    args.phase = RoutedEventPhase::Bubble;
    for (UIElement* walk : path) {
        if (args.handled) {
            break;
        }
        if (walk != target) {
            walk->OnRoutedEvent(args);
        }
    }
}

bool Window::TryMoveFocus(bool forward) {
    if (!m_rootElement) {
        return false;
    }
    std::vector<UIElement*> focusable;
    std::function<void(UIElement*)> walk = [&](UIElement* el) {
        if (!el || el->GetVisibility() != Visibility::Visible || !el->IsEnabled()) {
            return;
        }
        // TextBox and similar accept focus via classic focus path.
        if (el->AcceptsTabFocus()) {
            focusable.push_back(el);
        }
        for (auto& child : el->GetChildren()) {
            walk(child.get());
        }
    };
    walk(m_rootElement.get());
    if (focusable.empty()) {
        return false;
    }

    int index = -1;
    if (auto focused = LockElement(m_focusedElement)) {
        for (int i = 0; i < static_cast<int>(focusable.size()); ++i) {
            if (focusable[i] == focused.get()) {
                index = i;
                break;
            }
        }
    }
    int next = forward
        ? (index + 1) % static_cast<int>(focusable.size())
        : (index <= 0 ? static_cast<int>(focusable.size()) - 1 : index - 1);
    SetFocusedElement(focusable[next]);
    return true;
}

Window::~Window() {
    StopMiddleClickAutoscroll();
    if (s_current == this) {
        s_current = nullptr;
    }
    if (FrameScheduler::Current() == &m_frameScheduler) {
        FrameScheduler::SetCurrent(nullptr);
    }
    if (AnimationManager::Current() == &m_animationManager) {
        AnimationManager::SetCurrent(nullptr);
    }
    if (PopupHost::Current() == &m_popupHost) {
        PopupHost::SetCurrent(nullptr);
    }
    if (DragDropService::Current() == &m_dragDrop) {
        m_dragDrop.Cancel();
        DragDropService::SetCurrent(nullptr);
    }
    m_popupHost.CloseAll();
    RevokeShellDropTarget();
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
    }
    if (m_needOleUninit) {
        OleUninitialize();
        m_needOleUninit = false;
    }
}

bool Window::Create(const std::string& title, int width, int height, bool transparentMode) {
    EnsureProcessDpiAwareness();
    m_transparentMode = transparentMode;
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    // BLACK_BRUSH zeros alpha on erase for any remaining GDI path.
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszClassName = L"CUI_WindowClass";

    RegisterClassEx(&wc);

    std::wstring wTitle(title.begin(), title.end());

    // DirectComposition host requires WS_EX_NOREDIRECTIONBITMAP so GDI's opaque
    // redirection surface does not cover premul swap-chain alpha.
    DWORD dwStyle = WS_OVERLAPPEDWINDOW;
#ifndef WS_EX_NOREDIRECTIONBITMAP
#define WS_EX_NOREDIRECTIONBITMAP 0x00200000L
#endif
    DWORD dwExStyle = WS_EX_NOREDIRECTIONBITMAP;
    if (m_transparentMode) {
        dwExStyle |= (WS_EX_LAYERED | WS_EX_APPWINDOW);
    }

    // Create() width/height are DIPs (design size). Win32 CreateWindowEx expects
    // physical outer size under Per-Monitor V2.
    UINT dpi = GetDpiForSystem();
    if (dpi == 0) {
        dpi = 96;
    }
    const int physWidth = MulDiv((std::max)(1, width), static_cast<int>(dpi), 96);
    const int physHeight = MulDiv((std::max)(1, height), static_cast<int>(dpi), 96);

    m_hwnd = CreateWindowEx(
        dwExStyle,
        L"CUI_WindowClass",
        wTitle.c_str(),
        dwStyle,
        CW_USEDEFAULT, CW_USEDEFAULT,
        physWidth, physHeight,
        nullptr, nullptr,
        hInstance,
        this
    );

    if (!m_hwnd) return false;

    m_dpiScale = GetDpiScaleForWindow(m_hwnd);

    m_themeMode = ThemeManager::Instance().GetThemeMode();
    UpdateDwmChrome();
    MaterialHost::Apply(m_hwnd, BackdropType::None, m_themeMode);

    if (m_transparentMode) {
        SetLayeredWindowAttributes(m_hwnd, 0, 255, LWA_ALPHA);
    }

    m_gfxContext.SetRequirePerPixelAlpha(m_transparentMode);
    if (!m_gfxContext.Initialize(m_hwnd)) {
        return false;
    }

    m_layerRasterizer.BindDevice(m_gfxContext.GetD2DDevice());
    m_layerRasterizer.SetCompletionCallback([hwnd = m_hwnd]() {
        if (hwnd) {
            PostMessage(hwnd, WM_CUI_RASTER_COMPLETE, 0, 0);
        }
    });
    // Worker ready; jobs still FlushSync unless callers enable async + KickAsync.
    m_layerRasterizer.SetAsyncEnabled(true);

    // Graphics may add WS_EX_NOREDIRECTIONBITMAP for the composition fallback —
    // re-apply DWM alpha/backdrop so the final present path is wired correctly.
    MaterialHost::Apply(m_hwnd, BackdropType::None, m_themeMode);
    UpdateDwmChrome();

    PopupHost::SetCurrent(&m_popupHost);
    m_popupHost.SetOwnerHwnd(m_hwnd);
    AnimationManager::SetCurrent(&m_animationManager);
    FrameScheduler::SetCurrent(&m_frameScheduler);
    DragDropService::SetCurrent(&m_dragDrop);
    RegisterShellDropTarget();
    return true;
}

void Window::SetBackdropType(BackdropType type) {
    m_backdropType = BackdropType::None;
    if (m_hwnd) {
        m_gfxContext.SetRequirePerPixelAlpha(m_transparentMode);
        m_layerRasterizer.BindDevice(m_gfxContext.GetD2DDevice());
        MaterialHost::Apply(m_hwnd, BackdropType::None, m_themeMode);
        UpdateDwmChrome();
        m_gfxContext.GetResources().ReleaseDeviceResources();
        m_sceneLayer.ResetCache();
        ApplyVisualState();
        RequestFullRepaint();
    } else {
        ThemeManager::Instance().SetBackdropType(BackdropType::None);
    }
}

void Window::SetThemeMode(ThemeMode theme) {
    Point origin(m_logicalClientSize.width * 0.5f, m_logicalClientSize.height * 0.5f);
    if (m_hwnd) {
        POINT pt{};
        if (::GetCursorPos(&pt) && ::ScreenToClient(m_hwnd, &pt)) {
            origin = ClientPointToLogical(pt.x, pt.y);
        }
    }
    SetThemeModeWithRipple(theme, origin);
}

void Window::SetThemeModeWithRipple(ThemeMode theme, Point originPoint) {
    // Idle no-op only when already on this theme and no wave is playing.
    // Mid-wave clicks must always start a brand-new ripple.
    if (m_themeMode == theme && !m_themeRippleActive) {
        return;
    }

    const Size sceneSize = m_logicalClientSize;
    const Rect viewportBounds(0.0f, 0.0f, sceneSize.width, sceneSize.height);
    if (!m_hwnd || sceneSize.width < 0.5f || sceneSize.height < 0.5f) {
        m_themeMode = theme;
        m_themeRippleActive = false;
        ThemeManager::Instance().SetThemeMode(theme);
        StyleManager::Instance().ReloadFromTheme();
        return;
    }

    // If a wave is already playing, keep the current live destination scene as the
    // next "from" snapshot so the new click starts a fresh wave immediately.
    D2D1_COLOR_F oldClearBg = ThemeManager::Instance().GetColor(ThemeTokenId::WindowBackground);
    if (!m_sceneLayer.IsValid() || !m_sceneLayer.GetCacheBitmap()) {
        if (m_gfxContext.PushLayerTarget(m_sceneLayer, sceneSize, viewportBounds, oldClearBg, true)) {
            m_gfxContext.SetPaintBounds(viewportBounds);
            if (m_rootElement) {
                m_rootElement->Render(m_gfxContext);
            }
            m_gfxContext.PopLayerTarget(m_sceneLayer);
            m_sceneLayer.Validate();
        }
    }
    if (m_sceneLayer.GetCacheBitmap()) {
        if (m_gfxContext.PushLayerTarget(m_themeOldSceneLayer, sceneSize, viewportBounds, oldClearBg, true)) {
            m_gfxContext.DrawLayer(m_sceneLayer, viewportBounds);
            m_gfxContext.PopLayerTarget(m_themeOldSceneLayer);
            m_themeOldSceneLayer.Validate();
        }
    }

    // Apply theme tokens + refresh control caches before capturing the destination scene.
    m_themeMode = theme;
    ThemeManager::Instance().SetThemeMode(theme);
    StyleManager::Instance().ReloadFromTheme();
    MaterialHost::Apply(m_hwnd, BackdropType::None, theme);
    m_gfxContext.GetResources().ClearBrushCaches();
    if (m_rootElement) {
        ApplyThemeToTree(m_rootElement.get(), false);
        static unsigned long long s_rippleThemeNonce = 0;
        ForceThemeRefresh(m_rootElement.get(), std::to_string(++s_rippleThemeNonce), false);
    }
    m_onThemeChanged.Invoke(this, theme);

    // Pre-render destination theme into the live scene layer (one heavy frame only).
    D2D1_COLOR_F newClearBg = ThemeManager::Instance().GetColor(ThemeTokenId::WindowBackground);
    if (m_gfxContext.PushLayerTarget(m_sceneLayer, sceneSize, viewportBounds, newClearBg, true)) {
        m_gfxContext.SetPaintBounds(viewportBounds);
        if (m_rootElement) {
            m_rootElement->Render(m_gfxContext);
        }
        m_gfxContext.PopLayerTarget(m_sceneLayer);
        m_sceneLayer.Validate();
    }

    // Drop dirties created by ForceThemeRefresh — destination is already fully painted.
    if (m_rootElement) {
        DirtyRegion sink;
        m_rootElement->CollectRenderDirtyRegion(sink, true);
    }
    m_pendingDirtyRegion.Clear();

    // Start or restart the wave from the click.
    m_themeRippleActive = true;
    m_themeRippleOrigin = originPoint;
    m_themeRippleProgress = 0.0f;
    m_themeRippleStartTime = std::chrono::steady_clock::now();

    // Present-only: do NOT RequestFullRepaint() — that StructureDirties the scene and
    // forces a full tree rebuild on every ripple frame (blocking input).
    m_frameScheduler.ScheduleFrame();
    if (m_hwnd) {
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void Window::SetTransparentMode(bool enabled) {
    m_transparentMode = enabled;
    if (m_hwnd) {
        m_gfxContext.SetRequirePerPixelAlpha(m_transparentMode);
        m_layerRasterizer.BindDevice(m_gfxContext.GetD2DDevice());
        RequestFullRepaint();
    }
}

void Window::Show() {
    ProgressBarDiag::Log(
        "[PB] Window::Show hwnd=%p usesCompSC=%d — ProgressBar tick/OnRender lines appear "
        "only after opening nav: 值/进度 > ProgressBar",
        (void*)m_hwnd,
        m_gfxContext.UsesCompositionSwapChain() ? 1 : 0);
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_SHOW);
        // The very first layout often happens before the shown window settles on
        // its final client metrics/DPI-backed swap-chain size. If we paint using
        // that pre-show geometry, text and 1px edges can land on fractional
        // device pixels and look soft until the first WM_SIZE arrives.
        // Force one post-show relayout so startup uses the same path as a manual
        // resize, which keeps the initial frame crisp.
        Relayout();
        RequestFullRepaint();
        UpdateWindow(m_hwnd);
    }
}

void Window::SetLowPerformanceMode(bool enabled) {
    if (m_lowPerformanceMode == enabled) {
        return;
    }

    m_lowPerformanceMode = enabled;
    UIElement::SetAnimationsEnabled(!enabled);

    if (m_rootElement) {
        UIElement::SetAnimationDeltaSeconds(1.0f / 60.0f);
        m_rootElement->OnAnimationTick();
        m_rootElement->SyncRenderState();
    }

    m_lastAnimationDirtyRect = Rect();
    m_hasLastAnimationDirtyRect = false;
    RequestFullRepaint();
}

void Window::RunMessageLoop() {
    MSG msg = {};
    using clock = std::chrono::steady_clock;
    bool animationActive = false;

    for (;;) {
        // Theme ripple must never starve input. Drain mouse/keyboard before any
        // paint/frame work so a second theme click can restart the wave immediately.
        if (m_themeRippleActive) {
            MSG inputMsg = {};
            for (;;) {
                const BOOL gotMouse = PeekMessage(&inputMsg, m_hwnd, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE);
                const BOOL gotKey = gotMouse ? FALSE
                    : PeekMessage(&inputMsg, m_hwnd, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE);
                if (!gotMouse && !gotKey) {
                    break;
                }
                if (inputMsg.message == WM_QUIT) {
                    return;
                }
                TranslateMessage(&inputMsg);
                DispatchMessage(&inputMsg);
            }
        }

        bool hadMessage = false;
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            hadMessage = true;
            if (msg.message == WM_QUIT) {
                return;
            }
            if (msg.message == WM_MOUSEMOVE) {
                MSG newest = msg;
                while (PeekMessage(&newest, m_hwnd, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE)) {
                    msg = newest;
                }
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (m_flushInputDirty) {
            m_flushInputDirty = false;
            InvalidatePendingRenderRegions(false);
        }

        const bool wasAnimationActive = animationActive;
        const auto now = clock::now();
        const float refreshHz = GetWindowRefreshRateHz(m_hwnd);
        const double targetFps = m_lowPerformanceMode
            ? 8.0
            : static_cast<double>(std::clamp(refreshHz, 30.0f, 240.0f));
        const auto targetFrame = std::chrono::duration_cast<clock::duration>(
            std::chrono::duration<double>(1.0 / targetFps));
        m_frameScheduler.SetMinFrameInterval(targetFrame);
        m_animationManager.SetTargetFrameSeconds(static_cast<float>(1.0 / targetFps));

        m_animationManager.DispatchDueWakes(now);
        const bool hasPendingLayout =
            m_rootElement
            && (m_rootElement->IsMeasureDirty() || m_rootElement->IsArrangeDirty());
        if (m_animationManager.HasAnimating()
            || m_themeRippleActive
            || m_animationManager.ConsumeFrameRequest()
            || hasPendingLayout
            || !m_pendingDirtyRegion.IsEmpty()
            || HasPendingNativePaint()) {
            m_frameScheduler.ScheduleFrame();
        }
        if (auto focused = LockElement(m_focusedElement)) {
            if (focused->NeedsAutoScrollTick()) {
                m_frameScheduler.ScheduleFrame();
            }
        }
        if (auto middle = LockElement(m_middleScrollElement)) {
            if (middle->NeedsAutoScrollTick()) {
                m_frameScheduler.ScheduleFrame();
            }
        }
        if (animationActive) {
            m_frameScheduler.ScheduleFrame();
        }

        const bool frameDue = m_frameScheduler.ConsumeDue(now);
        bool animating = animationActive || m_themeRippleActive;
        bool didFrame = false;
        if (frameDue && m_rootElement) {
            FlushLayoutIfNeeded();

            m_animationManager.BeginFrame(now, animationActive);
            UIElement::SetAnimationDeltaSeconds(m_animationManager.GetDeltaSeconds());
            animating = m_animationManager.Tick();
            if (m_popupHost.TickAnimations()) {
                animating = true;
            }
            DockFloatWindow::PresentAll();
            if (auto focused = LockElement(m_focusedElement)) {
                if (focused->NeedsAutoScrollTick()) {
                    focused->OnAutoScrollTick();
                    animating = true;
                }
            }
            if (auto middle = LockElement(m_middleScrollElement)) {
                const auto focused = LockElement(m_focusedElement);
                if (middle.get() != focused.get() && middle->NeedsAutoScrollTick()) {
                    middle->OnAutoScrollTick();
                    animating = true;
                }
            }
            didFrame = true;
        }
        animationActive = animating;

        if (didFrame) {
            CommitFrame(animating || wasAnimationActive);

            // Sync paint is safe again: OnPaint no longer InvalidateRect during
            // ripple, so UpdateWindow cannot re-enter until the wave ends.
            if (HasPendingNativePaint()) {
                UpdateWindow(m_hwnd);
            }
            MSG paintMsg = {};
            while (PeekMessage(&paintMsg, m_hwnd, WM_PAINT, WM_PAINT, PM_REMOVE)) {
                TranslateMessage(&paintMsg);
                DispatchMessage(&paintMsg);
            }
            if (animating || m_themeRippleActive) {
                m_frameScheduler.ScheduleFrame();
            }
        }

        if (!hadMessage) {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE)) {
                continue;
            }
            if (HasPendingNativePaint()) {
                continue;
            }
            const bool wantContinuous = animationActive || m_animationManager.HasAnimating() || m_themeRippleActive;
            if (wantContinuous) {
                // After a frame, do not sleep — DXGI Present(1) already paced us.
                // Sleeping targetFrame here stacked on Present and halved FPS.
                const DWORD waitMs = didFrame ? 0
                    : static_cast<DWORD>((std::max)(1, static_cast<int>(
                        std::chrono::duration_cast<std::chrono::milliseconds>(targetFrame).count())));
                MsgWaitForMultipleObjectsEx(0, nullptr, waitMs, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            } else if (m_frameScheduler.HasPending() || m_animationManager.HasPendingWake()) {
                const int schedMs = m_frameScheduler.GetMsUntilDeadline(clock::now());
                const int wakeMs = m_animationManager.GetMsUntilNextWake(clock::now());
                int waitMs = -1;
                if (schedMs >= 0) {
                    waitMs = schedMs;
                }
                if (wakeMs >= 0) {
                    waitMs = (waitMs < 0) ? wakeMs : (std::min)(waitMs, wakeMs);
                }
                if (waitMs < 0) {
                    waitMs = 0;
                }
                MsgWaitForMultipleObjectsEx(
                    0, nullptr, static_cast<DWORD>(waitMs), QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            } else {
                WaitMessage();
            }
        }
    }
}

void Window::SetRootElement(std::shared_ptr<UIElement> root) {
    m_rootElement = root;
    m_animationManager.SetLiveRoot(m_rootElement.get());
    if (m_rootElement) {
        ApplyVisualState();
        m_rootElement->SyncRenderState();
    }
    Relayout();
}

void Window::ApplyVisualState() {
    const bool systemBackdrop = false;
    ThemeManager::Instance().SetBackdropType(BackdropType::None);
    if (m_rootElement) {
        ApplyThemeToTree(m_rootElement.get(), systemBackdrop);
        static unsigned long long s_themeRefreshNonce = 0;
        const std::string refreshStamp = std::to_string(++s_themeRefreshNonce);
        ForceThemeRefresh(m_rootElement.get(), refreshStamp, systemBackdrop);
    }
    if (m_activeContextMenu) {
        ApplyThemeToTree(m_activeContextMenu.get(), systemBackdrop);
        static unsigned long long s_themeMenuRefreshNonce = 0;
        const std::string refreshStamp = std::to_string(++s_themeMenuRefreshNonce);
        ForceThemeRefresh(m_activeContextMenu.get(), refreshStamp, systemBackdrop);
    }
    if (m_hwnd) {
        UpdateDwmChrome();
        m_sceneLayer.Invalidate(RenderLayer::ContentDirty | RenderLayer::StructureDirty | RenderLayer::SizeDirty);
        RedrawWindow(m_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
    }
}

void Window::Relayout() {
    if (!m_hwnd) return;
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    OnResize(
        static_cast<UINT>(std::max<LONG>(0, rc.right - rc.left)),
        static_cast<UINT>(std::max<LONG>(0, rc.bottom - rc.top))
    );
}

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    Window* pThis = nullptr;
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<Window*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hwnd = hwnd;
    } else {
        pThis = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT Window::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // Win11 Snap Layout + custom title bar notes:
    // 1. Do not call DwmDefWindowProc for every non-client message up front.
    //    If DWM/DefWindowProc gets WM_NCCALCSIZE, WM_NCACTIVATE, or WM_NCPAINT
    //    before our custom handling, Windows can restore and paint the native
    //    caption/title bar over the CUI-drawn one.
    // 2. Keep WM_NCCALCSIZE returning 0 so the client area owns the whole window.
    //    That is what makes the CUI TitleBar the visible title bar.
    // 3. For Snap Layout hover, WM_NCHITTEST must return HTMAXBUTTON on the
    //    custom maximize button rectangle. That rectangle must be tested before
    //    the top resize border, otherwise a windowed app returns HTTOP/HTTOPRIGHT
    //    and Windows never sees a maximize button under the cursor.
    // 4. DwmDefWindowProc can still participate narrowly in non-client mouse
    //    movement, but it must not own the whole non-client message pipeline.
    switch (uMsg) {
    case WM_NCCALCSIZE:
        if (wParam == TRUE) {
            auto* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
            if (IsZoomed(m_hwnd)) {
                // Use the full maximized window rect as the client area so DWM does
                // not leave a 1px non-client strip (white border) around rcWork.
                params->rgrc[0] = params->rgrc[1];
            }
        }
        // Custom chrome must consume both NCCALCSIZE forms. During the hidden
        // CreateWindow path Windows can ask with wParam == FALSE; if that falls
        // through to DefWindowProc, the first client rect is the classic
        // OVERLAPPEDWINDOW area (for example 1280x780 becomes 1264x741). The
        // swap chain is then created too small, leaving an unpainted strip on
        // the right/bottom until the first manual resize.
        return 0;

    case WM_ACTIVATEAPP:
        if (wParam == FALSE) {
            // Defer so a menu click's mouse-up can still run ExecuteCommand.
            PostMessageW(m_hwnd, WM_CUI_CLOSE_POPUPS, 0, 0);
        }
        break;
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) {
            HWND other = reinterpret_cast<HWND>(lParam);
            // Clicking a WS_EX_NOACTIVATE menu popup must not dismiss mid-click.
            if (MenuPopupWindow::IsMenuPopupHwnd(other)) {
                break;
            }
            PostMessageW(m_hwnd, WM_CUI_CLOSE_POPUPS, 0, 0);
        }
        break;
    case WM_CUI_CLOSE_POPUPS: {
        HWND fore = GetForegroundWindow();
        if (MenuPopupWindow::IsMenuPopupHwnd(fore)) {
            break;
        }
        m_popupHost.CloseAll();
        break;
    }
    case WM_NCACTIVATE:
        return TRUE;

    case WM_NCPAINT:
        return 0;

    case WM_SYSCOMMAND:
        // Handle window system commands with native animations
        if ((wParam & 0xFFF0) == SC_MINIMIZE) {
            ShowWindow(m_hwnd, SW_MINIMIZE);
            return 0;
        } else if ((wParam & 0xFFF0) == SC_RESTORE) {
            ShowWindow(m_hwnd, SW_RESTORE);
            return 0;
        } else if ((wParam & 0xFFF0) == SC_MAXIMIZE) {
            ShowWindow(m_hwnd, SW_MAXIMIZE);
            return 0;
        }
        break;

    case WM_NCHITTEST: {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        POINT clientPt = pt;
        ScreenToClient(m_hwnd, &clientPt);
        Point logicalPt = ClientPointToLogical(clientPt.x, clientPt.y);
        float fx = logicalPt.x;
        float fy = logicalPt.y;

        RECT rc;
        GetClientRect(m_hwnd, &rc);
        const float scale = (m_dpiScale > 0.001f) ? m_dpiScale : 1.0f;

        const Rect chromeBounds = GetChromeBounds(m_rootElement.get());
        if (!chromeBounds.IsEmpty() && chromeBounds.Contains(fx, fy)) {
            if (IWindowChrome* chrome = FindWindowChrome(m_rootElement.get())) {
                if (chrome->IsInteractiveHit(fx, fy)) {
                    return HTCLIENT;
                }
                const LRESULT chromeNcHit = chrome->HitTestNonClient(fx, fy);
                if (chromeNcHit != HTNOWHERE) {
                    return chromeNcHit;
                }
            }
        }

        // 2. Resizing border handles (8-direction border resize) - only when NOT maximized
        if (!IsZoomed(m_hwnd)) {
            const int borderThickness = (std::max)(1, static_cast<int>(std::lround(8.0f * scale)));
            bool left = (clientPt.x < borderThickness);
            bool right = (clientPt.x >= rc.right - borderThickness);
            bool top = (clientPt.y < borderThickness);
            bool bottom = (clientPt.y >= rc.bottom - borderThickness);

            if (top && left) return HTTOPLEFT;
            if (top && right) return HTTOPRIGHT;
            if (bottom && left) return HTBOTTOMLEFT;
            if (bottom && right) return HTBOTTOMRIGHT;
            if (left) return HTLEFT;
            if (right) return HTRIGHT;
            if (top) return HTTOP;
            if (bottom) return HTBOTTOM;
        }

        if (!chromeBounds.IsEmpty() && chromeBounds.Contains(fx, fy) && m_rootElement) {
            UIElement* hit = m_rootElement->HitTest(fx, fy);
            if (IWindowChrome* chrome = FindWindowChrome(m_rootElement.get())) {
                if (chrome->IsCaptionDragHit(fx, fy, hit)) {
                    return HTCAPTION;
                }
                if (chrome->IsInteractiveHit(fx, fy)) {
                    return HTCLIENT;
                }
            }
        }

        return HTCLIENT;
    }

    case WM_NCLBUTTONDOWN: {
        LRESULT res = DefWindowProc(m_hwnd, uMsg, wParam, lParam);
        if (wParam == HTCLOSE) {
            PostMessage(m_hwnd, WM_CLOSE, 0, 0);
            return 0;
        } else if (wParam == HTMAXBUTTON) {
            if (IsZoomed(m_hwnd)) {
                SendMessage(m_hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
            } else {
                SendMessage(m_hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
            }
            return 0;
        } else if (wParam == HTMINBUTTON) {
            SendMessage(m_hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
            return 0;
        }
        return res;
    }

    case WM_PAINT:
        OnPaint();
        return 0;

    case WM_CUI_RASTER_COMPLETE:
        if (FrameScheduler* sched = FrameScheduler::Current()) {
            sched->ScheduleFrame();
        }
        InvalidatePendingRenderRegions(false);
        return 0;

    case WM_SIZE:
        UpdateDwmChrome();
        // Do not trust WM_SIZE's lParam in the custom-chrome startup path. A
        // queued/synchronous size message can still carry the old native
        // OVERLAPPEDWINDOW client size (for example height 741) even after
        // WM_NCCALCSIZE has made GetClientRect report the full 780px surface.
        // Always relayout from GetClientRect so stale WM_SIZE data cannot shrink
        // the root tree and leave an unpainted strip at the bottom.
        Relayout();
        RequestFullRepaint();
        return 0;

    case WM_SETTINGCHANGE:
    case WM_THEMECHANGED:
        if (ThemeManager::Instance().CheckAndUpdateSystemTheme()) {
            SetThemeMode(ThemeManager::Instance().GetThemeMode());
        }
        break;

    case WM_DPICHANGED:
        {
            // When Windows changes the DPI for this HWND, it provides the
            // recommended outer window bounds. Apply them immediately and then
            // relayout/recreate the render target at the new DPI so text stays
            // crisp instead of being temporarily bitmap-scaled.
            const RECT* suggestedRect = reinterpret_cast<const RECT*>(lParam);
            if (suggestedRect) {
                SetWindowPos(
                    m_hwnd,
                    nullptr,
                    suggestedRect->left,
                    suggestedRect->top,
                    suggestedRect->right - suggestedRect->left,
                    suggestedRect->bottom - suggestedRect->top,
                    SWP_NOZORDER | SWP_NOACTIVATE
                );
            }
            UpdateDwmChrome();
            Relayout();
            RequestFullRepaint();
        }
        return 0;

    case WM_MOUSEMOVE:
        {
            Rect oldMenuBounds;
            if (m_activeContextMenu && m_activeContextMenu->IsOpen()) {
                oldMenuBounds = m_activeContextMenu->GetTotalBounds();
            }

            bool moved = OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

            Rect newMenuBounds;
            if (m_activeContextMenu && m_activeContextMenu->IsOpen()) {
                newMenuBounds = m_activeContextMenu->GetTotalBounds();
            }

            bool menuBoundsChanged = (oldMenuBounds.x != newMenuBounds.x || oldMenuBounds.y != newMenuBounds.y ||
                                      oldMenuBounds.width != newMenuBounds.width || oldMenuBounds.height != newMenuBounds.height);

            if (moved || menuBoundsChanged) {
                if (!oldMenuBounds.IsEmpty()) {
                    m_pendingDirtyRegion.AddRect(oldMenuBounds.Inflate(4.0f));
                }
                if (!newMenuBounds.IsEmpty()) {
                    m_pendingDirtyRegion.AddRect(newMenuBounds.Inflate(4.0f));
                }
                // Defer flush to the message loop so a burst of hover changes coalesces
                // into one Collect+Invalidate instead of one Present setup per TextBox.
                m_flushInputDirty = true;
            }
            // No dirty → no Present (cursor-only moves stay free).
        }
        return 0;

    case WM_NCMOUSEMOVE:
        {
            // Caption buttons are HTMIN/MAX/CLOSE — updates arrive here, not WM_MOUSEMOVE.
            POINT screenPt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            POINT clientPt = screenPt;
            ScreenToClient(m_hwnd, &clientPt);
            const Point logicalPt = ClientPointToLogical(clientPt.x, clientPt.y);
            if (IWindowChrome* chrome = FindWindowChrome(m_rootElement.get())) {
                chrome->NotifyNonClientMouseMove(logicalPt.x, logicalPt.y);
                bool dirty = chrome->ConsumeChromeDirty();
                // Chrome that paints caption hover from the cursor (or forgets to
                // dirty in NotifyNonClient) still needs a paint while over HTMIN/MAX/CLOSE.
                if (!dirty
                    && chrome->HitTestNonClient(logicalPt.x, logicalPt.y) != HTNOWHERE) {
                    if (UIElement* el = chrome->GetChromeElement()) {
                        const Rect bounds = el->GetBounds();
                        if (!bounds.IsEmpty()) {
                            el->MarkRenderRectDirty(bounds.Inflate(2.0f));
                            dirty = true;
                        }
                    }
                }
                if (dirty) {
                    m_flushInputDirty = true;
                }
            }
            // Need WM_NCMOUSELEAVE when the cursor leaves the non-client chrome.
            TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE | TME_NONCLIENT, m_hwnd, 0 };
            TrackMouseEvent(&tme);

            // Narrow DWM handoff: Snap Layouts on maximize, without full native chrome paint.
            LRESULT dwmResult = 0;
            if (DwmDefWindowProc(m_hwnd, uMsg, wParam, lParam, &dwmResult)) {
                return dwmResult;
            }
        }
        return DefWindowProc(m_hwnd, uMsg, wParam, lParam);

    case WM_NCMOUSELEAVE:
        if (IWindowChrome* chrome = FindWindowChrome(m_rootElement.get())) {
            chrome->NotifyNonClientMouseLeave();
            if (chrome->ConsumeChromeDirty()) {
                m_flushInputDirty = true;
            }
        }
        return DefWindowProc(m_hwnd, uMsg, wParam, lParam);

    case WM_LBUTTONDOWN:
        StopMiddleClickAutoscroll();
        if (OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) {
            InvalidatePendingRenderRegions(false);
        }
        return 0;

    case WM_LBUTTONDBLCLK:
        StopMiddleClickAutoscroll();
        OnLButtonDblClick(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        InvalidatePendingRenderRegions(true);
        return 0;

    case WM_LBUTTONUP:
        if (OnLButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) {
            InvalidatePendingRenderRegions(true);
        }
        return 0;

    case WM_RBUTTONDOWN:
        StopMiddleClickAutoscroll();
        OnRButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        InvalidatePendingRenderRegions(true);
        return 0;

    case WM_RBUTTONUP:
        OnRButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        InvalidatePendingRenderRegions(true);
        return 0;

    case WM_MBUTTONDOWN:
        OnMButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        InvalidatePendingRenderRegions(false);
        return 0;

    case WM_MBUTTONUP:
        OnMButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        InvalidatePendingRenderRegions(false);
        return 0;

    case WM_CAPTURECHANGED:
        if (IsMiddleClickAutoscrollActive() && reinterpret_cast<HWND>(lParam) != m_hwnd) {
            StopMiddleClickAutoscroll();
        }
        return 0;

    case WM_MOUSEWHEEL: {
        StopMiddleClickAutoscroll();
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(m_hwnd, &pt);
        Point logicalPt = ClientPointToLogical(pt.x, pt.y);
        float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;

        // Same hit order as mouse move/click: popup → overlay → tree.
        UIElement* target = m_popupHost.HitTest(logicalPt.x, logicalPt.y);
        if (!target && m_rootElement) {
            target = m_rootElement->HitTestOverlay(logicalPt.x, logicalPt.y);
            if (!target) {
                target = m_rootElement->HitTest(logicalPt.x, logicalPt.y);
            }
        }
        if (target) {
            target->OnMouseWheel(delta);
            InvalidatePendingRenderRegions(false);
        }
        return 0;
    }

    case WM_SYSKEYDOWN:
        if (wParam == VK_DOWN || wParam == VK_UP || wParam == VK_F4) {
            if (auto focused = LockElement(m_focusedElement)) {
                if (focused->IsEnabled()) {
                    focused->OnKeyDown(static_cast<int>(wParam));
                    InvalidatePendingRenderRegions(true);
                    return 0;
                }
            }
        }
        break;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE && m_dragDrop.IsDragging()) {
            m_dragDrop.Cancel();
            if (auto pressed = LockElement(m_pressedElement)) {
                pressed->OnMouseUp(Point());
            }
            m_pressedElement.reset();
            if (GetCapture() == m_hwnd && !LockElement(m_rpressedElement) && !LockElement(m_middleScrollElement)) {
                ReleaseCapture();
            }
            InvalidatePendingRenderRegions(false);
            return 0;
        }
        if (wParam == VK_ESCAPE && IsMiddleClickAutoscrollActive()) {
            StopMiddleClickAutoscroll();
            InvalidatePendingRenderRegions(false);
            return 0;
        }
        if (wParam == VK_F8) {
            m_showRenderStatsOverlay = !m_showRenderStatsOverlay;
            RequestFullRepaint();
            return 0;
        }
        if (wParam == VK_TAB) {
            const bool forward = (GetKeyState(VK_SHIFT) & 0x8000) == 0;
            if (TryMoveFocus(forward)) {
                InvalidatePendingRenderRegions(false);
                return 0;
            }
        }
        if (auto focused = LockElement(m_focusedElement)) {
            if (focused->IsEnabled()) {
                RoutedEventArgs args;
                args.type = RoutedEventType::KeyDown;
                args.phase = RoutedEventPhase::Target;
                args.keyCode = static_cast<int>(wParam);
                args.originalSource = focused.get();
                focused->OnRoutedEvent(args);
                if (!args.handled) {
                    // Bubble key to ancestors for global shortcuts.
                    args.phase = RoutedEventPhase::Bubble;
                    for (UIElement* walk = focused->GetParent(); walk && !args.handled; walk = walk->GetParent()) {
                        walk->OnRoutedEvent(args);
                    }
                }
                InvalidatePendingRenderRegions(true);
            }
        }
        if (wParam == VK_ESCAPE && m_popupHost.HasOpenPopups()) {
            m_popupHost.DismissTop();
            InvalidatePendingRenderRegions(true);
        }
        return 0;

    case WM_CHAR:
        if (auto focused = LockElement(m_focusedElement)) {
            if (focused->IsEnabled()) {
                focused->OnCharInput(static_cast<wchar_t>(wParam));
                InvalidatePendingRenderRegions(true);
            }
        }
        return 0;

    case WM_IME_COMPOSITION:
        if (auto focused = LockElement(m_focusedElement)) {
            if (auto tb = dynamic_cast<TextBox*>(focused.get())) {
                HIMC hIMC = ImmGetContext(m_hwnd);
                if (hIMC) {
                    if (lParam & GCS_COMPSTR) {
                        LONG bytes = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, nullptr, 0);
                        if (bytes > 0) {
                            std::wstring compStr(bytes / sizeof(wchar_t), 0);
                            ImmGetCompositionStringW(hIMC, GCS_COMPSTR, &compStr[0], bytes);
                            tb->SetCompositionString(compStr);
                        } else {
                            tb->SetCompositionString(L"");
                        }
                    }
                    if (lParam & GCS_RESULTSTR) {
                        LONG bytes = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, nullptr, 0);
                        if (bytes > 0) {
                            std::wstring resultStr(bytes / sizeof(wchar_t), 0);
                            ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, &resultStr[0], bytes);
                            tb->CommitImeResult(resultStr);
                        } else {
                            tb->SetCompositionString(L"");
                        }
                    }
                    ImmReleaseContext(m_hwnd, hIMC);
                    InvalidatePendingRenderRegions(true);
                }
            }
        }
        break;

    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT) {
            if (auto middle = LockElement(m_middleScrollElement)) {
                if (middle->IsMiddleScrollActive()) {
                    if (HCURSOR hCur = middle->GetCursor()) {
                        SetCursor(hCur);
                        return TRUE;
                    }
                }
            }
            auto hovered = LockElement(m_hoveredElement);
            HCURSOR hCur = hovered ? hovered->GetCursor() : nullptr;
            if (hCur) {
                SetCursor(hCur);
                return TRUE;
            }
        }
        break;

    case WM_MOUSELEAVE:
        // Leaving HTCLIENT only. Min/max/close are HTMIN/MAX/CLOSE, so moving onto
        // them fires WM_MOUSELEAVE after (or before) WM_NCMOUSEMOVE. Calling
        // chrome OnMouseLeave here would wipe the caption hover that NC just set.
        {
            bool overCaptionButtons = false;
            if (IWindowChrome* chrome = FindWindowChrome(m_rootElement.get())) {
                float lx = 0.0f;
                float ly = 0.0f;
                if (TryGetCursorClientLogical(m_hwnd, lx, ly)
                    && chrome->HitTestNonClient(lx, ly) != HTNOWHERE) {
                    overCaptionButtons = true;
                }
            }

            if (auto hovered = LockElement(m_hoveredElement)) {
                if (overCaptionButtons && dynamic_cast<IWindowChrome*>(hovered.get())) {
                    m_hoveredElement.reset();
                    m_trackingMouse = false;
                    return 0;
                }
                hovered->OnMouseLeave();
                m_hoveredElement.reset();
                if (m_rootElement) {
                    DirtyRegion probe;
                    m_rootElement->CollectRenderDirtyRegion(probe, false);
                    if (!probe.IsEmpty()) {
                        InvalidatePendingRenderRegions(false);
                    }
                }
            }
            m_trackingMouse = false;
        }
        return 0;

    case WM_DROPFILES:
        {
            const HDROP hdrop = reinterpret_cast<HDROP>(wParam);
            POINT clientPt{};
            if (!DragQueryPoint(hdrop, &clientPt)) {
                POINT screen{};
                GetCursorPos(&screen);
                ScreenToClient(m_hwnd, &screen);
                clientPt = screen;
            }
            const Point logical = ClientPointToLogical(clientPt.x, clientPt.y);
            const UINT count = DragQueryFileW(hdrop, 0xFFFFFFFF, nullptr, 0);
            std::vector<std::string> files;
            files.reserve(count);
            for (UINT i = 0; i < count; ++i) {
                const UINT len = DragQueryFileW(hdrop, i, nullptr, 0);
                std::wstring path(len, L'\0');
                if (len > 0) {
                    DragQueryFileW(hdrop, i, path.data(), len + 1);
                    files.push_back(Utf16ToUtf8(path));
                }
            }
            DragFinish(hdrop);
            DataPackage pkg;
            pkg.SetFiles(std::move(files));
            m_dragDrop.DeliverExternal(logical, std::move(pkg), m_rootElement.get());
            InvalidatePendingRenderRegions(true);
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

void Window::OnPaint() {
    PAINTSTRUCT ps;
    BeginPaint(m_hwnd, &ps);

    if (m_animationManager.HasComposeOnlyAnimating()) {
        const unsigned n = ++ProgressBarDiag::OnPaintCount();
        if (ProgressBarDiag::ShouldLogDetail(n)) {
            ProgressBarDiag::Log(
                "[PB] OnPaint#%u rcPaint=(%d,%d)-(%d,%d) pendingRects=%zu fullRepaintProbe pendingEmpty=%d",
                n,
                (int)ps.rcPaint.left, (int)ps.rcPaint.top,
                (int)ps.rcPaint.right, (int)ps.rcPaint.bottom,
                m_pendingDirtyRegion.GetRectCount(),
                m_pendingDirtyRegion.IsEmpty() ? 1 : 0);
        }
    }

    const float dpiScale = (m_dpiScale > 0.001f) ? m_dpiScale : 1.0f;
    Rect paintBounds = PhysicalRectToLogical(
        Rect(
            static_cast<float>(ps.rcPaint.left),
            static_cast<float>(ps.rcPaint.top),
            static_cast<float>(ps.rcPaint.right - ps.rcPaint.left),
            static_cast<float>(ps.rcPaint.bottom - ps.rcPaint.top)
        ),
        dpiScale
    );
    m_gfxContext.SetPaintBounds(paintBounds);
    DirtyRegion frameDirtyRegion = m_pendingDirtyRegion;
    if (frameDirtyRegion.IsEmpty()) {
        frameDirtyRegion.AddRect(paintBounds);
    }

    // Layout, hit-testing, and D2D drawing all use DIP coordinates.
    Rect viewportBounds(0.0f, 0.0f, m_logicalClientSize.width, m_logicalClientSize.height);
    if (viewportBounds.IsEmpty()) {
        viewportBounds = GetLogicalClientBounds(m_hwnd, dpiScale);
    }
    Rect dirtyBounds = frameDirtyRegion.GetBounds();
    const Size sceneSize(viewportBounds.width, viewportBounds.height);
    const bool sceneSizeChanged =
        std::abs(m_sceneLayer.GetCacheSurfaceSize().width - sceneSize.width) > 0.5f
        || std::abs(m_sceneLayer.GetCacheSurfaceSize().height - sceneSize.height) > 0.5f;
    const bool coversViewport = frameDirtyRegion.GetRectCount() == 0
        || viewportBounds.IsEmpty()
        || CoversRect(paintBounds, viewportBounds)
        || AnyDirtyRectCovers(frameDirtyRegion, viewportBounds)
        || !m_sceneLayer.IsValid()
        || sceneSizeChanged;
    // Gap-aware patch: pad dirty rects so StackPanel/Column gaps & category margins
    // are cleared+repainted together (avoids black bars). Clear transparent; clip==clear.
    constexpr float kDirtyGapPad = 16.0f;
    // Temporary hard fallback: rebuild the scene cache for the frame instead of
    // patching sub-rects into the previous bitmap. With full-frame Present now
    // forced, this cleanly isolates whether the recurring light/dark seams are
    // produced by the scene-patch path itself.
    const bool canRestoreScene = false;
    const bool fullRepaint = !canRestoreScene;

    m_compositionContext.BeginFrame(viewportBounds, frameDirtyRegion, fullRepaint);
    m_gfxContext.SetCompositionContext(&m_compositionContext);

    m_gfxContext.BeginDraw();

    const bool systemBackdrop = false;
    const bool usePerPixelAlpha =
        systemBackdrop
        && m_gfxContext.SupportsPerPixelAlpha();
    const D2D1_COLOR_F sceneClearColor = (usePerPixelAlpha || (m_transparentMode && !IsZoomed(m_hwnd)))
        ? D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f)
        : ThemeManager::Instance().GetColor(ThemeTokenId::WindowBackground);

    auto renderScene = [&]() {
        if (m_rootElement) {
            m_rootElement->Render(m_gfxContext);
        }
    };

    auto renderOverlaysOnly = [&]() {
        // Overlay is a separate pass above the scene cache. Do not inherit the
        // scene patch dirty strip here: overlay sub-controls (dialog buttons,
        // popup content, etc.) may sit outside that strip and would be wrongly
        // culled even though the overlay root itself is visible.
        const Rect savedPaintBounds = m_gfxContext.GetPaintBounds();
        m_gfxContext.SetPaintBounds(viewportBounds);
        if (m_rootElement) {
            m_rootElement->RenderOverlay(m_gfxContext);
        }
        m_popupHost.Render(m_gfxContext);
        m_dragDrop.RenderOverlay(m_gfxContext);
        m_gfxContext.SetPaintBounds(savedPaintBounds);
    };

    // Do not freeze the scene during ContentDialog scrim animations. Reusing the
    // old scene cache here makes under-scrim interactions look stalled for a few
    // frames: animations continue ticking, but the user sees the old cached scene
    // until the dialog open/close pass finishes.

    // ProgressBar / hover: keep gap pad small for thin controls so we don't
    // re-record half the page. 4 dips is enough for the 3px bar + AA.
    constexpr float kAnimDirtyPad = 4.0f;
    const float patchPad = (frameDirtyRegion.GetRectCount() == 1
        && dirtyBounds.height <= 24.0f
        && dirtyBounds.width <= viewportBounds.width * 0.75f)
        ? kAnimDirtyPad
        : kDirtyGapPad;

    bool scenePatched = false;
    Rect unionPatch;
    // Theme ripple: keep the destination scene alive for control animations.
    // Freeze-only compose swallowed dirties and locked every other animator for
    // the whole wave. Patch dirty rects into m_sceneLayer; skip work only when
    // nothing interactive/animating actually changed.
    const bool rippleLayersReady =
        m_themeRippleActive
        && m_sceneLayer.IsValid()
        && m_sceneLayer.GetCacheBitmap() != nullptr
        && m_themeOldSceneLayer.IsValid()
        && m_themeOldSceneLayer.GetCacheBitmap() != nullptr;

    if (rippleLayersReady) {
        DirtyRegion ripplePatchRegion = m_pendingDirtyRegion;
        if (m_rootElement) {
            m_rootElement->CollectRenderDirtyRegion(ripplePatchRegion, false);
        }

        bool hasPatch = false;
        for (const Rect& rect : ripplePatchRegion.GetRects()) {
            if (rect.IsEmpty()) {
                continue;
            }
            const Rect patch = GraphicsContext::SnapExpandRect(rect, dpiScale, patchPad);
            unionPatch = hasPatch ? unionPatch.Union(patch) : patch;
            hasPatch = true;
        }

        // Full-window pending is usually the ripple InvalidateRect itself — that
        // must not force a whole-tree scene rebuild every frame.
        if (hasPatch && CoversRect(unionPatch, viewportBounds) && m_pendingDirtyRegion.IsEmpty()) {
            hasPatch = false;
        }

        if (hasPatch
            && m_gfxContext.PushLayerTarget(
                m_sceneLayer,
                sceneSize,
                unionPatch,
                sceneClearColor,
                false)) {
            for (const Rect& rect : ripplePatchRegion.GetRects()) {
                if (!rect.IsEmpty()) {
                    m_gfxContext.ClearRect(
                        GraphicsContext::SnapExpandRect(rect, dpiScale, patchPad),
                        sceneClearColor);
                }
            }
            m_gfxContext.SetPaintBounds(unionPatch);
            m_gfxContext.PushClip(unionPatch);
            renderScene();
            m_gfxContext.PopClip();
            m_gfxContext.PopLayerTarget(m_sceneLayer);
            m_sceneLayer.Validate();
            scenePatched = true;
        } else {
            unionPatch = viewportBounds;
            scenePatched = true;
        }

        if (m_rootElement) {
            DirtyRegion sink;
            m_rootElement->CollectRenderDirtyRegion(sink, true);
        }
        m_pendingDirtyRegion.Clear();
    } else {
        if (canRestoreScene) {
            bool hasPatch = false;
            const auto& dirtyRects = frameDirtyRegion.GetRects();
            for (const Rect& rect : dirtyRects) {
                if (rect.IsEmpty()) {
                    continue;
                }
                const Rect patch = GraphicsContext::SnapExpandRect(rect, dpiScale, patchPad);
                unionPatch = hasPatch ? unionPatch.Union(patch) : patch;
                hasPatch = true;
            }
            if (hasPatch
                && m_gfxContext.PushLayerTarget(
                    m_sceneLayer,
                    sceneSize,
                    unionPatch,
                    sceneClearColor,
                    false)) {
                for (const Rect& rect : dirtyRects) {
                    if (!rect.IsEmpty()) {
                        m_gfxContext.ClearRect(
                            GraphicsContext::SnapExpandRect(rect, dpiScale, patchPad),
                            sceneClearColor);
                    }
                }
                m_gfxContext.SetPaintBounds(unionPatch);
                m_gfxContext.PushClip(unionPatch);
                renderScene();
                m_gfxContext.PopClip();
                m_gfxContext.PopLayerTarget(m_sceneLayer);
                m_sceneLayer.Validate();
                scenePatched = true;
            }
        }
        if (!scenePatched
            && m_gfxContext.PushLayerTarget(
                m_sceneLayer,
                sceneSize,
                viewportBounds,
                sceneClearColor,
                true)) {
            m_gfxContext.SetPaintBounds(viewportBounds);
            renderScene();
            m_gfxContext.PopLayerTarget(m_sceneLayer);
            m_sceneLayer.Validate();
            unionPatch = viewportBounds;
        }
    }

    // Dirty-rect Present on flip-model swapchains is not reliable enough for this
    // renderer. After switching opaque windows back to a HWND swapchain it caused
    // untouched regions to disappear until another hover/click repainted them.
    // Keep the scene patch optimization, but always publish the final composed
    // frame with a normal Present.
    const bool partialPresent = false;

    if (partialPresent) {
        m_gfxContext.PushClip(unionPatch);
        m_gfxContext.DrawLayer(m_sceneLayer, viewportBounds);
        renderOverlaysOnly();
        m_gfxContext.PopClip();

        const RECT dirtyPx{
            static_cast<LONG>(std::floor(unionPatch.x * dpiScale)),
            static_cast<LONG>(std::floor(unionPatch.y * dpiScale)),
            static_cast<LONG>(std::ceil((unionPatch.x + unionPatch.width) * dpiScale)),
            static_cast<LONG>(std::ceil((unionPatch.y + unionPatch.height) * dpiScale))
        };
        m_gfxContext.EndDraw(&dirtyPx);
    } else {
        if (systemBackdrop || (m_transparentMode && !IsZoomed(m_hwnd))) {
            m_gfxContext.GetD2DContext()->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
        } else {
            m_gfxContext.GetD2DContext()->Clear(sceneClearColor);
        }

        if (m_themeRippleActive && m_themeOldSceneLayer.GetCacheBitmap()) {
            auto now = std::chrono::steady_clock::now();
            float elapsedSec = std::chrono::duration<float>(now - m_themeRippleStartTime).count();
            constexpr float kThemeRippleDuration = 0.38f; // 380ms smooth visual radial wave
            m_themeRippleProgress = std::clamp(elapsedSec / kThemeRippleDuration, 0.0f, 1.0f);

            // Cubic Ease-Out
            float t = m_themeRippleProgress;
            float easeProgress = 1.0f - std::pow(1.0f - t, 3.0f);

            // Compute maximum radius to cover entire window from click origin
            const float dx1 = m_themeRippleOrigin.x;
            const float dx2 = std::abs(viewportBounds.width - m_themeRippleOrigin.x);
            const float dy1 = m_themeRippleOrigin.y;
            const float dy2 = std::abs(viewportBounds.height - m_themeRippleOrigin.y);
            const float maxRadius = std::hypot((std::max)(dx1, dx2), (std::max)(dy1, dy2));
            const float currentRadius = maxRadius * easeProgress;

            // 1. Draw Old Theme Snapshot as base
            m_gfxContext.DrawLayer(m_themeOldSceneLayer, viewportBounds);

            if (m_themeRippleProgress < 1.0f) {
                // 2. Draw New Theme Scene clipped by an expanding radial circle from click position
                m_gfxContext.PushEllipseClip(m_themeRippleOrigin, currentRadius, currentRadius);
                m_gfxContext.DrawLayer(m_sceneLayer, viewportBounds);
                m_gfxContext.PopClip();

                // Pace the next frame via the scheduler ONLY.
                // Do NOT InvalidateRect here: that re-queues WM_PAINT from inside
                // the paint handler and the message loop would keep dispatching
                // paints until progress==1, never seeing WM_LBUTTONDOWN.
                m_frameScheduler.ScheduleFrame();
                m_animationManager.RequestWake(m_rootElement.get(), now + std::chrono::milliseconds(16));
            } else {
                // Transition finished
                m_gfxContext.DrawLayer(m_sceneLayer, viewportBounds);
                m_themeRippleActive = false;
                m_themeOldSceneLayer.ResetCache();
            }
        } else if (m_themeRippleActive) {
            // Old snapshot lost — finish immediately so we never stick with
            // m_themeRippleActive=true and a silent no-op paint path.
            m_gfxContext.DrawLayer(m_sceneLayer, viewportBounds);
            m_themeRippleActive = false;
            m_themeOldSceneLayer.ResetCache();
        } else {
            m_gfxContext.DrawLayer(m_sceneLayer, viewportBounds);
        }

        renderOverlaysOnly();
        SampleDisplayFps();
        DrawRenderStatsOverlay();
        m_gfxContext.EndDraw();
    }

    m_gfxContext.SetCompositionContext(nullptr);
    m_compositionContext.EndFrame();
    m_pendingDirtyRegion.Clear();

    EndPaint(m_hwnd, &ps);
}

void Window::SampleDisplayFps() {
    using clock = std::chrono::steady_clock;
    if (m_overlayFpsSampleStart.time_since_epoch().count() == 0) {
        m_overlayFpsSampleStart = clock::now();
    }
    ++m_overlayFrameCounter;
}

float Window::GetDisplayFps() const {
    using clock = std::chrono::steady_clock;
    const auto now = clock::now();
    if (m_overlayFpsSampleStart.time_since_epoch().count() == 0) {
        return 0.0f;
    }
    const float elapsed = std::chrono::duration<float>(now - m_overlayFpsSampleStart).count();
    // Flush on the same cadence as the status bar (~0.5s) so idle mouse-move
    // no longer shows the last Present storm's Hz.
    if (elapsed >= 0.5f) {
        m_overlayFps = static_cast<float>(m_overlayFrameCounter) / elapsed;
        m_overlayFrameCounter = 0;
        m_overlayFpsSampleStart = now;
    }
    return m_overlayFps;
}

void Window::DrawRenderStatsOverlay() {
    if (!m_showRenderStatsOverlay) {
        return;
    }

    const auto& stats = m_compositionContext.GetStats();
    std::ostringstream ss;
    ss
        << "帧模式: " << (stats.fullRepaint ? "整帧" : "局部")
        << "  脏区: " << stats.dirtyRectCount
        << "  光栅: " << stats.rasterizedNodeCount
        << "  布局: " << stats.layoutPassCount
        << "  命中: " << stats.layerCacheHitCount
        << "  未命中: " << stats.layerCacheMissCount
        << "  重录: " << stats.layerCacheRerenderCount
        << "  复用: " << stats.layerCacheReuseCount
        << "  显示帧率: " << static_cast<int>(std::round(GetDisplayFps()));

    const std::string text = ss.str();
    Rect panel(12.0f, (std::max)(12.0f, m_rootElement ? (m_rootElement->GetBounds().height - 40.0f) : 12.0f), 820.0f, 28.0f);
    m_gfxContext.FillRoundedRect(panel, 6.0f, D2D1::ColorF(0.05f, 0.05f, 0.05f, 0.82f));
    m_gfxContext.DrawRoundedRect(panel, 6.0f, D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.10f), 1.0f);
    m_gfxContext.DrawText(
        text,
        Rect(panel.x + 10.0f, panel.y, panel.width - 20.0f, panel.height),
        D2D1::ColorF(0.92f, 0.92f, 0.92f, 1.0f),
        "Consolas",
        12.0f,
        DWRITE_TEXT_ALIGNMENT_LEADING,
        DWRITE_PARAGRAPH_ALIGNMENT_CENTER
    );
}

void Window::UpdateDwmChrome() {
    if (!m_hwnd) return;

    const bool maximized = IsZoomed(m_hwnd) != FALSE;
    const MARGINS margins = maximized ? MARGINS{ 0, 0, 0, 0 } : MARGINS{ 1, 1, 1, 1 };
    DwmExtendFrameIntoClientArea(m_hwnd, &margins);

    // 1. Force Native Windows 11 DWM Rounded Corners (DWMWA_WINDOW_CORNER_PREFERENCE = 33)
    DWM_WINDOW_CORNER_PREFERENCE preference = maximized ? DWMWCP_DONOTROUND : DWMWCP_ROUND;
    DwmSetWindowAttribute(m_hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));

    // Match the app background on Win11's 1px window border.
    const D2D1_COLOR_F border = ThemeManager::Instance().GetTokens().windowBackground;
    const COLORREF borderColor = RGB(
        static_cast<int>(border.r * 255.0f),
        static_cast<int>(border.g * 255.0f),
        static_cast<int>(border.b * 255.0f)
    );
    DwmSetWindowAttribute(m_hwnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
}

void Window::OnResize(UINT width, UINT height) {
    m_dpiScale = GetDpiScaleForWindow(m_hwnd);
    m_logicalClientSize = Size(
        PhysicalToLogical(static_cast<float>(width), m_hwnd),
        PhysicalToLogical(static_cast<float>(height), m_hwnd)
    );
    m_gfxContext.Resize(width, height);
    m_sceneLayer.Invalidate(RenderLayer::ContentDirty | RenderLayer::StructureDirty | RenderLayer::SizeDirty);
    if (m_rootElement) {
        bool isMaximized = IsZoomed(m_hwnd);
        const float resizeBorder = 8.0f / m_dpiScale;

        float padLeft = isMaximized ? resizeBorder : 0.0f;
        float padTop = isMaximized ? resizeBorder : 0.0f;
        float padRight = isMaximized ? resizeBorder : 0.0f;
        // Only inset when maximized (avoid screen-edge underlap). A permanent
        // bottom pad left an empty strip under the status bar in normal windows.
        float padBottom = isMaximized ? resizeBorder : 0.0f;

        const float layoutW = (std::max)(0.0f, PhysicalToLogical(static_cast<float>(width), m_hwnd) - padLeft - padRight);
        const float layoutH = (std::max)(0.0f, PhysicalToLogical(static_cast<float>(height), m_hwnd) - padTop - padBottom);
        Size avail(layoutW, layoutH);
        m_rootElement->Measure(avail);
        m_rootElement->Arrange(Rect(padLeft, padTop, layoutW, layoutH));
        m_popupHost.SetViewport(Rect(padLeft, padTop, layoutW, layoutH));
    }
}

Point Window::ClientPointToLogical(int x, int y) const {
    float lx = 0.0f;
    float ly = 0.0f;
    ClientPhysicalToLogical(m_hwnd, x, y, lx, ly);
    return Point(lx, ly);
}

UIElement* Window::HitTestChrome(float x, float y) const {
    if (!m_rootElement) {
        return nullptr;
    }
    const Rect chromeBounds = GetChromeBounds(m_rootElement.get());
    if (chromeBounds.IsEmpty() || !chromeBounds.Contains(x, y)) {
        return nullptr;
    }
    if (IWindowChrome* chrome = FindWindowChrome(m_rootElement.get())) {
        UIElement* chromeElement = chrome->GetChromeElement();
        if (chromeElement) {
            if (UIElement* hit = chromeElement->HitTest(x, y)) {
                return hit;
            }
            if (chromeElement->GetBounds().Contains(x, y)) {
                return chromeElement;
            }
        }
    }
    return nullptr;
}

bool Window::OnMouseMove(int x, int y) {
    if (!m_trackingMouse) {
        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, m_hwnd, 0 };
        TrackMouseEvent(&tme);
        m_trackingMouse = true;
    }

    const uint64_t dirtyAtStart = UIElement::GetRenderDirtySerial();

    Point logicalPt = ClientPointToLogical(x, y);
    float fx = logicalPt.x;
    float fy = logicalPt.y;

    if (auto middle = LockElement(m_middleScrollElement)) {
        if (middle->IsMiddleScrollActive()) {
            middle->OnMouseMove(Point(fx, fy));
            return UIElement::GetRenderDirtySerial() != dirtyAtStart;
        }
        m_middleScrollElement.reset();
    }

    if (m_dragDrop.IsDragging()) {
        m_dragDrop.Update(Point(fx, fy), m_rootElement.get());
        return true;
    }

    if (auto pressed = LockElement(m_pressedElement)) {
        pressed->OnMouseMove(Point(fx, fy));
        if (m_dragDrop.IsDragging()) {
            m_dragDrop.Update(Point(fx, fy), m_rootElement.get());
            return true;
        }
        if (NeedsContinuousMouseRedraw(pressed.get())) {
            return true;
        }
        // Thumb-drag / splitter etc. mark local dirty — flush those.
        // Idle press+move (e.g. holding on a rippling nav item) must NOT invalidate.
        return UIElement::GetRenderDirtySerial() != dirtyAtStart;
    }

    if (auto rpressed = LockElement(m_rpressedElement)) {
        rpressed->OnMouseMove(Point(fx, fy));
        return UIElement::GetRenderDirtySerial() != dirtyAtStart;
    }

    // Same hit order as LButtonDown (no dismiss): popup → chrome → overlay → tree.
    // ContextMenu hover goes through PopupHost only — not a second HitTestOverlay path.
    UIElement* newHover = m_popupHost.HitTest(fx, fy);
    if (!newHover) {
        newHover = HitTestChrome(fx, fy);
    }
    if (!newHover && m_rootElement) {
        newHover = m_rootElement->HitTestOverlay(fx, fy);
    }

    auto hovered = LockElement(m_hoveredElement);
    if (!newHover && hovered && hovered->GetBounds().Contains(fx, fy)) {
        // Still inside the current hover — only search that subtree instead of
        // walking PropertyGrid / NavigationView on every pixel.
        newHover = hovered->HitTest(fx, fy);
        if (!newHover) {
            newHover = hovered.get();
        }
    }
    if (!newHover && m_rootElement) {
        newHover = m_rootElement->HitTest(fx, fy);
    }

    if (newHover != hovered.get()) {
        if (hovered) {
            hovered->OnMouseLeave();
        }
        SetHoveredElement(newHover);
        hovered = LockElement(m_hoveredElement);
        if (hovered) {
            hovered->OnMouseEnter();
        }
        UIElement* curr = hovered.get();
        while (curr) {
            auto menu = curr->GetContextMenu();
            if (menu && menu->IsOpen()) {
                m_activeContextMenu = menu;
                break;
            }
            curr = curr->GetParent();
        }
    }

    if (hovered) {
        hovered->OnMouseMove(Point(fx, fy));
        if (auto* chrome = dynamic_cast<IWindowChrome*>(hovered.get())) {
            (void)chrome->ConsumeChromeDirty();
        }
    }

    // Cursor-only / no-chrome hover must NOT Present. Only flush when someone
    // actually MarkRender*Dirty during enter/leave/move.
    return UIElement::GetRenderDirtySerial() != dirtyAtStart;
}

bool Window::OnLButtonDown(int x, int y) {
    Point logicalPt = ClientPointToLogical(x, y);
    float fx = logicalPt.x;
    float fy = logicalPt.y;

    const bool hadContextMenu = m_activeContextMenu && m_activeContextMenu->IsOpen();
    Rect oldMenuBounds = hadContextMenu ? m_activeContextMenu->GetTotalBounds() : Rect();

    auto noteClosedContextMenu = [&]() {
        if (m_activeContextMenu && !m_activeContextMenu->IsOpen()) {
            m_activeContextMenu = nullptr;
            if (!oldMenuBounds.IsEmpty()) {
                m_pendingDirtyRegion.AddRect(oldMenuBounds.Inflate(4.0f));
            }
        }
    };

    auto syncActiveContextMenuFrom = [&](UIElement* from) {
        UIElement* curr = from;
        while (curr) {
            auto menu = curr->GetContextMenu();
            if (menu && menu->IsOpen()) {
                m_activeContextMenu = menu;
                break;
            }
            curr = curr->GetParent();
        }
    };

    auto pressElement = [&](UIElement* el) {
        SetPressedElement(el);
        SetCapture(m_hwnd);
        el->OnMouseDown(Point(fx, fy));
        noteClosedContextMenu();
        syncActiveContextMenuFrom(el);
    };

    auto applyFocus = [&](UIElement* target) -> bool {
        auto focused = LockElement(m_focusedElement);
        if (target && !target->IsEnabled()) {
            if (focused) {
                focused->OnBlur();
                SetFocusedElement(nullptr);
            }
            return false;
        }
        if (focused && focused.get() != target) {
            focused->OnBlur();
        }
        SetFocusedElement(target);
        focused = LockElement(m_focusedElement);
        if (focused) {
            focused->OnFocus();
        }
        return target != nullptr;
    };

    // 1. PopupHost (ContextMenu / Flyout / pickers) — exclusive; do not also tree-hit.
    if (UIElement* popupHit = m_popupHost.HitTest(fx, fy)) {
        pressElement(popupHit);
        return true;
    }

    // 2. Light-dismiss any open popup when the click is outside.
    if (m_popupHost.DismissIfOutside(fx, fy)) {
        if (hadContextMenu && (!m_activeContextMenu || !m_activeContextMenu->IsOpen())) {
            m_activeContextMenu = nullptr;
            if (!oldMenuBounds.IsEmpty()) {
                m_pendingDirtyRegion.AddRect(oldMenuBounds.Inflate(4.0f));
            }
            // Clear MenuBar active highlight state when context menu is dismissed
            ClearMenuBarInteractionState();
            if (auto hovered = LockElement(m_hoveredElement)) {
                hovered->OnMouseLeave();
            }
            m_hoveredElement.reset();
        }
    }

    // 3. Chrome / TitleBar band (toggles handled inside TitleBar::OnMouseDown).
    if (UIElement* chromeHit = HitTestChrome(fx, fy)) {
        if (applyFocus(chromeHit)) {
            pressElement(chromeHit);
        }
        return true;
    }

    // 4. Non-popup overlays still on the visual tree.
    if (m_rootElement) {
        if (UIElement* overlayHit = m_rootElement->HitTestOverlay(fx, fy)) {
            pressElement(overlayHit);
            return true;
        }
    }

    // 5. Document tree.
    UIElement* target = m_rootElement ? m_rootElement->HitTest(fx, fy) : nullptr;
    if (applyFocus(target) && target) {
        pressElement(target);
    }
    return target != nullptr || m_activeContextMenu != nullptr;
}

void Window::OnLButtonDblClick(int x, int y) {
    Point logicalPt = ClientPointToLogical(x, y);
    float fx = logicalPt.x;
    float fy = logicalPt.y;

    UIElement* target = m_popupHost.HitTest(fx, fy);
    if (!target) {
        target = HitTestChrome(fx, fy);
    }
    if (!target && m_rootElement) {
        target = m_rootElement->HitTestOverlay(fx, fy);
    }
    if (!target && m_rootElement) {
        target = m_rootElement->HitTest(fx, fy);
    }
    if (target && target->IsEnabled()) {
        target->OnMouseDblClick(Point(fx, fy));
    }
}

bool Window::OnLButtonUp(int x, int y) {
    Point logicalPt = ClientPointToLogical(x, y);
    float fx = logicalPt.x;
    float fy = logicalPt.y;
    bool dirty = false;

    if (m_dragDrop.IsDragging()) {
        m_dragDrop.CompleteDrop(Point(fx, fy), m_rootElement.get());
        dirty = true;
    }

    if (auto pressed = LockElement(m_pressedElement)) {
        pressed->OnMouseUp(Point(fx, fy));
        dirty = true;
        // MenuBar / TitleBar may open a dropdown on mouse-up; track it for light-dismiss.
        UIElement* curr = pressed.get();
        while (curr) {
            auto menu = curr->GetContextMenu();
            if (menu && menu->IsOpen()) {
                m_activeContextMenu = menu;
                break;
            }
            curr = curr->GetParent();
        }
    }
    m_pressedElement.reset();
    if (GetCapture() == m_hwnd && !LockElement(m_rpressedElement) && !LockElement(m_middleScrollElement)) {
        ReleaseCapture();
    }
    return dirty;
}

void Window::ClearMenuBarInteractionState() {
    if (IWindowChrome* chrome = FindWindowChrome(m_rootElement.get())) {
        chrome->ResetMenuInteraction();
    }
    if (!m_rootElement) return;
    std::function<void(UIElement*)> clearMenuBar = [&](UIElement* elem) {
        if (!elem) return;
        if (auto* mb = dynamic_cast<MenuBar*>(elem)) {
            mb->ResetInteractionState();
        }
        for (auto& child : elem->GetChildren()) {
            clearMenuBar(child.get());
        }
    };
    clearMenuBar(m_rootElement.get());
}

void Window::OnRButtonDown(int x, int y) {
    Point logicalPt = ClientPointToLogical(x, y);
    float fx = logicalPt.x;
    float fy = logicalPt.y;

    if (m_activeContextMenu && m_activeContextMenu->IsOpen()) {
        m_activeContextMenu->Hide();
        m_activeContextMenu = nullptr;
        ClearMenuBarInteractionState();
    }

    m_pendingContextMenuTarget = nullptr;
    m_pendingContextMenu.reset();
    m_rpressedElement.reset();

    UIElement* target = m_rootElement ? m_rootElement->HitTest(fx, fy) : nullptr;
    if (!target) return;

    auto focused = LockElement(m_focusedElement);
    if (focused && focused.get() != target) {
        focused->OnBlur();
    }
    SetFocusedElement(target);
    if (auto curFocused = LockElement(m_focusedElement)) {
        curFocused->OnFocus();
    }

    target->OnMouseRightClick(Point(fx, fy));
    m_rpressedElement = CaptureElementRef(target);
    SetCapture(m_hwnd);

    m_pendingContextMenuTarget = target;
    m_pendingContextMenuPt = Point(fx, fy);
    UIElement* curr = target;
    while (curr) {
        if (auto menu = curr->GetContextMenu()) {
            m_pendingContextMenu = menu;
            break;
        }
        curr = curr->GetParent();
    }
}

void Window::OnRButtonUp(int x, int y) {
    Point logicalPt = ClientPointToLogical(x, y);
    float fx = logicalPt.x;
    float fy = logicalPt.y;

    UIElement* target = m_pendingContextMenuTarget;
    m_pendingContextMenuTarget = nullptr;
    auto pendingMenu = m_pendingContextMenu;
    m_pendingContextMenu.reset();
    m_rpressedElement.reset();
    if (GetCapture() == m_hwnd && !LockElement(m_pressedElement) && !LockElement(m_middleScrollElement)) {
        ReleaseCapture();
    }

    if (!target) return;

    const Point releasePt(fx, fy);
    if (target->OnContextMenuRelease(releasePt)) {
        UIElement* curr = target;
        while (curr) {
            auto menu = curr->GetContextMenu();
            if (menu && menu->IsOpen()) {
                m_activeContextMenu = menu;
                break;
            }
            curr = curr->GetParent();
        }
        return;
    }

    if (pendingMenu) {
        m_activeContextMenu = pendingMenu;
        m_activeContextMenu->ShowAt(releasePt.x, releasePt.y);
    }
}

bool Window::IsMiddleClickAutoscrollActive() const {
    if (auto middle = LockElement(m_middleScrollElement)) {
        return middle->IsMiddleScrollActive();
    }
    return false;
}

void Window::StopMiddleClickAutoscroll() {
    if (auto middle = LockElement(m_middleScrollElement)) {
        if (middle->IsMiddleScrollActive()) {
            middle->OnMiddleButtonUp(Point());
        }
    }
    m_middleScrollElement.reset();
    if (GetCapture() == m_hwnd && !LockElement(m_pressedElement) && !LockElement(m_rpressedElement)) {
        ReleaseCapture();
    }
}

void Window::OnMButtonDown(int x, int y) {
    Point logicalPt = ClientPointToLogical(x, y);
    float fx = logicalPt.x;
    float fy = logicalPt.y;

    // Browser toggle-off: another middle click while autoscrolling exits the mode.
    if (IsMiddleClickAutoscrollActive()) {
        StopMiddleClickAutoscroll();
        return;
    }

    UIElement* target = m_popupHost.HitTest(fx, fy);
    if (!target && m_rootElement) {
        target = m_rootElement->HitTestOverlay(fx, fy);
        if (!target) {
            target = m_rootElement->HitTest(fx, fy);
        }
    }
    if (!target) return;

    // Walk ancestors so a nested control inside ScrollViewer/ListView can start.
    for (UIElement* curr = target; curr; curr = curr->GetParent()) {
        if (curr->OnMiddleButtonDown(Point(fx, fy))) {
            m_middleScrollElement = CaptureElementRef(curr);
            SetFocusedElement(curr);
            SetCapture(m_hwnd);
            return;
        }
    }
}

void Window::OnMButtonUp(int x, int y) {
    (void)x;
    (void)y;
    // Browser toggle mode: releasing middle button keeps autoscroll active until
    // left/right click, Escape, wheel, or another middle click.
    if (IsMiddleClickAutoscrollActive()) {
        return;
    }
    if (GetCapture() == m_hwnd && !LockElement(m_pressedElement) && !LockElement(m_rpressedElement)) {
        ReleaseCapture();
    }
}

} // namespace CUI
