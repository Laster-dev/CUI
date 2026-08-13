#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "FilePicker.h"
#include "BreadcrumbBar.h"
#include "TreeView.h"
#include "../style/ThemeManager.h"
#include "../window/PopupPlacement.h"
#include <windows.h>

namespace CUI {

namespace {
constexpr float kBrowseW = 34.0f;
constexpr float kDefaultH = 32.0f;
} // namespace

FilePicker::FilePicker() {
    SetPlaceholder("未选择文件...");
    SetBackgroundToken(ThemeTokenId::InputBackground);
    SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
    SetPressedBackgroundToken(ThemeTokenId::PressedBackground);
    SetBorderToken(ThemeTokenId::InputBorder);
    SetFocusedBorderToken(ThemeTokenId::FocusedBorder);
    SetColorToken(ThemeTokenId::TextPrimary);
    SetBackground(ThemeManager::Instance().GetColor("inputBackground"));
    SetBorderBrush(ThemeManager::Instance().GetColor("inputBorder"));
    SetBorderThickness(1.0f);
    SetColor(ThemeManager::Instance().GetColor("textPrimary"));
    SetFontFamily("Segoe UI");
    SetFontSize(12.0f);
    SetPadding(Thickness(8.0f, 4.0f, 4.0f, 4.0f));
    SetCornerRadius(4.0f);
    SetWidth(320.0f);
    SetHeight(kDefaultH);
    SetFilter("所有文件", "*.*");

    m_breadcrumbHost.AttachTo(this);
    m_treeHost.AttachTo(this);

    m_breadcrumbHost.SetNavigateHandler([this](const std::string& path) {
        m_browser.SetCurrentPath(path);
        m_treeHost.NavigateTo(path, m_browser);
        SyncBrowserChrome();
        MarkPickerDirty();
        RequestAnimationTicks();
    });

    m_treeHost.SetPathChangedHandler([this](const std::string& path) {
        m_browser.SetCurrentPath(path);
        SyncBrowserChrome();
        MarkPickerDirty();
    });

    m_treeHost.SetConfirmHandler([this](const std::string& path) {
        SetPath(path);
        SetPopupOpen(false);
    });
}

void FilePicker::SyncBrowserChrome() {
    m_breadcrumbHost.Sync(m_browser.GetCurrentPath());
    if (m_isPopupOpen || PopupProgress() > 0.001f) {
        m_breadcrumbHost.Layout(m_browser, GetPopupBounds());
        m_treeHost.Layout(m_browser.ListRect(GetPopupBounds()));
    }
}

float FilePicker::PopupProgress() const {
    if (!UIElement::AreAnimationsEnabled()) {
        return m_isPopupOpen ? 1.0f : 0.0f;
    }
    return m_popupAnim.Current();
}

void FilePicker::SetPath(const std::string& path) {
    if (GetText() == path) {
        return;
    }
    SetText(path);
    NotifyFieldChanged(PropertyId::Text, Value(path));
    m_onPathChangedEvent.Invoke(this, path);
    MarkPickerDirty();
}

void FilePicker::SetFilter(const std::string& name, const std::string& spec) {
    m_filters.clear();
    m_filters.emplace_back(name, spec);
}

void FilePicker::AddFilter(const std::string& name, const std::string& spec) {
    m_filters.emplace_back(name, spec);
}

void FilePicker::ClearFilters() {
    m_filters.clear();
}

std::vector<PropertyMeta> FilePicker::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "text", "文件路径 (Path)", "文件选择", "string" });
    return metas;
}

Rect FilePicker::PathRect() const {
    const Thickness pad = GetPadding();
    const float border = GetBorderThickness();
    return Rect(
        m_bounds.x + border + pad.left,
        m_bounds.y + border,
        (std::max)(0.0f, m_bounds.width - border * 2.0f - kBrowseW - pad.left - 2.0f),
        (std::max)(0.0f, m_bounds.height - border * 2.0f));
}

Rect FilePicker::BrowseRect() const {
    const float border = GetBorderThickness();
    return Rect(
        m_bounds.x + m_bounds.width - border - kBrowseW,
        m_bounds.y + border,
        kBrowseW,
        (std::max)(0.0f, m_bounds.height - border * 2.0f));
}

FilePicker::HitPart FilePicker::HitTestPart(Point pt) const {
    if (BrowseRect().Contains(pt.x, pt.y)) {
        return HitPart::Browse;
    }
    if (m_bounds.Contains(pt.x, pt.y)) {
        return HitPart::Path;
    }
    return HitPart::None;
}

HCURSOR FilePicker::GetCursor() const {
    if (!IsEnabled() || (m_hover == HitPart::None && !m_isPopupOpen)) {
        return nullptr;
    }
    return LoadCursor(nullptr, IDC_HAND);
}

Size FilePicker::Measure(Size availableSize) {
    (void)availableSize;
    float w = GetWidth() >= 0.0f ? GetWidth() : 320.0f;
    float h = GetHeight() >= 0.0f ? GetHeight() : kDefaultH;
    m_desiredSize = Size(w, h);
    m_measureDirty = false;
    return m_desiredSize;
}

void FilePicker::Arrange(Rect finalRect) {
    m_bounds = finalRect;
    m_arrangeDirty = false;
    if (PopupProgress() > 0.001f) {
        SyncBrowserChrome();
    }
}

void FilePicker::MarkPickerDirty() {
    MarkRenderRectDirty(m_bounds.Inflate(2.0f));
    if (PopupProgress() > 0.001f) {
        Rect dirty = GetPopupBounds().Inflate(4.0f);
        if (m_filterDropDownOpen) {
            dirty = dirty.Union(m_browser.FilterDropdownRect(GetPopupBounds()).Inflate(2.0f));
        }
        MarkRenderRectDirty(dirty);
    }
}

void FilePicker::SetFilterDropDownOpen(bool open) {
    if (m_filterDropDownOpen == open) {
        return;
    }
    m_filterDropDownOpen = open;
    if (!open) {
        m_hoverFilterItem = -1;
    }
    MarkPickerDirty();
}

void FilePicker::SetPopupOpen(bool open) {
    if (m_isPopupOpen == open) {
        return;
    }
    m_isPopupOpen = open;
    if (open) {
        m_browser.Configure(FileBrowserMode::OpenFile, GetText(), m_filters, 0);
        m_treeHost.Configure(m_browser);
        m_filterDropDownOpen = false;
        m_hoverFilterItem = -1;
        SyncBrowserChrome();
    } else {
        m_filterDropDownOpen = false;
        m_hoverFilterItem = -1;
        m_breadcrumbHost.DismissOverflowMenu();
    }
    if (PopupHost* host = PopupHost::Current()) {
        if (open) {
            host->Open(this);
        } else {
            host->Close(this);
        }
    }
    RequestAnimationTicks();
    MarkPickerDirty();
}

Rect FilePicker::GetPopupBounds() const {
    return PlacePopupNearAnchor(
        m_bounds,
        FileBrowserSession::kPopupW,
        FileBrowserSession::kPopupH,
        GetPopupViewportOrDefault(),
        4.0f);
}

bool FilePicker::HitDismissExempt(float x, float y) const {
    if (m_bounds.Contains(x, y)) {
        return true;
    }
    const Rect pop = GetPopupBounds();
    if (pop.Contains(x, y)) {
        return true;
    }
    if (m_filterDropDownOpen) {
        return m_browser.FilterDropdownRect(pop).Contains(x, y);
    }
    // In-window overflow menu is a nested popup; keep the picker open while using it.
    const Rect overflow = m_breadcrumbHost.GetOverflowMenuClientBounds();
    if (!overflow.IsEmpty() && overflow.Contains(x, y)) {
        return true;
    }
    return false;
}

UIElement* FilePicker::OnHitTestOverlay(float x, float y) {
    const float progress = PopupProgress();
    if (progress <= 0.5f) {
        return nullptr;
    }
    const Rect pop = GetPopupBounds();
    const float currentH = (m_isPopupOpen && progress >= 0.98f) ? pop.height : (pop.height * progress);
    const Rect clip(pop.x, pop.y, pop.width, currentH);
    if (m_filterDropDownOpen && m_browser.FilterDropdownRect(pop).Contains(x, y)) {
        return this;
    }
    if (!clip.Contains(x, y)) {
        return nullptr;
    }
    if (UIElement* hit = m_breadcrumbHost.HitTest(x, y)) {
        return hit;
    }
    if (UIElement* hit = m_treeHost.HitTest(x, y)) {
        return hit;
    }
    return this;
}

void FilePicker::UpdateHover(Point pt) {
    if (!m_isPopupOpen) {
        return;
    }
    const Rect pop = GetPopupBounds();
    const bool up = m_browser.UpButtonRect(pop).Contains(pt.x, pt.y);
    const bool filter = m_browser.FilterButtonRect(pop).Contains(pt.x, pt.y);
    const bool cancel = m_browser.CancelButtonRect(pop).Contains(pt.x, pt.y);
    const bool confirm = m_browser.ConfirmButtonRect(pop).Contains(pt.x, pt.y);
    const int filterItem = m_filterDropDownOpen ? m_browser.HitTestFilterItem(pop, pt) : -1;
    if (up != m_hoverUp || filter != m_hoverFilter || cancel != m_hoverCancel
        || confirm != m_hoverConfirm || filterItem != m_hoverFilterItem) {
        m_hoverUp = up;
        m_hoverFilter = filter;
        m_hoverCancel = cancel;
        m_hoverConfirm = confirm;
        m_hoverFilterItem = filterItem;
        MarkPickerDirty();
    }
}

bool FilePicker::HandleBrowserClick(Point pt) {
    const Rect pop = GetPopupBounds();

    if (m_filterDropDownOpen) {
        const int filterItem = m_browser.HitTestFilterItem(pop, pt);
        if (filterItem >= 0) {
            m_browser.SetFilterIndex(filterItem);
            m_treeHost.ApplyFilter(m_browser);
            SetFilterDropDownOpen(false);
            SyncBrowserChrome();
            MarkPickerDirty();
            RequestAnimationTicks();
            return true;
        }
        if (m_browser.FilterButtonRect(pop).Contains(pt.x, pt.y)) {
            SetFilterDropDownOpen(false);
            return true;
        }
        if (m_browser.FilterDropdownRect(pop).Contains(pt.x, pt.y)) {
            return true;
        }
        SetFilterDropDownOpen(false);
    }

    if (m_browser.FilterButtonRect(pop).Contains(pt.x, pt.y)) {
        if (m_browser.GetFilters().size() > 1) {
            SetFilterDropDownOpen(true);
        }
        return true;
    }

    if (m_browser.UpButtonRect(pop).Contains(pt.x, pt.y)) {
        m_treeHost.GoUp(m_browser);
        SyncBrowserChrome();
        MarkPickerDirty();
        RequestAnimationTicks();
        return true;
    }
    if (m_browser.CancelButtonRect(pop).Contains(pt.x, pt.y)) {
        SetPopupOpen(false);
        return true;
    }
    if (m_browser.ConfirmButtonRect(pop).Contains(pt.x, pt.y)) {
        std::string path;
        if (m_treeHost.TryConfirm(m_browser, path)) {
            SetPath(path);
            SetPopupOpen(false);
        }
        return true;
    }
    return false;
}

void FilePicker::OnRender(GraphicsContext& ctx) {
    const float radius = GetCornerRadius();
    const auto& tokens = ThemeManager::Instance().GetTokens();
    D2D1_COLOR_F bg = ResolveThemeColor(GetBackgroundToken(), ThemeTokenId::InputBackground);
    D2D1_COLOR_F border = m_isFocused
        ? ResolveThemeColor(GetFocusedBorderToken(), ThemeTokenId::FocusedBorder)
        : ResolveThemeColor(GetBorderToken(), ThemeTokenId::InputBorder);
    const float borderW = m_isFocused ? 1.5f : GetBorderThickness();

    ctx.FillRoundedRect(m_bounds, radius, bg);
    if (borderW > 0.0f) {
        ctx.DrawRoundedRect(m_bounds, radius, border, borderW);
    }

    const Rect browse = BrowseRect();
    const bool browseHot = (m_hover == HitPart::Browse || m_pressed == HitPart::Browse || m_isPopupOpen);
    D2D1_COLOR_F browseFill = browseHot ? tokens.hoverBackground : tokens.cardBackground;
    if (m_pressed == HitPart::Browse) {
        browseFill = tokens.pressedBackground;
    }
    ctx.FillRect(Rect(browse.x + 1.0f, browse.y + 1.0f, browse.width - 1.0f, browse.height - 2.0f), browseFill);
    ctx.DrawLine(
        Point(browse.x, browse.y + 4.0f),
        Point(browse.x, browse.y + browse.height - 4.0f),
        tokens.cardBorder,
        1.0f);

    Rect glyph(
        browse.x + (browse.width - 14.0f) * 0.5f,
        browse.y + (browse.height - 14.0f) * 0.5f,
        14.0f,
        14.0f);
    ctx.DrawChevron(
        glyph,
        tokens.textSecondary,
        m_isPopupOpen ? GraphicsContext::ChevronDirection::Up : GraphicsContext::ChevronDirection::Down,
        1.5f);

    const std::string& path = GetText();
    const std::string& display = path.empty() ? GetPlaceholder() : path;
    D2D1_COLOR_F textColor = path.empty()
        ? tokens.textMuted
        : ResolveThemeColor(GetColorToken(), ThemeTokenId::TextPrimary);
    ctx.DrawText(
        display,
        PathRect(),
        textColor,
        GetFontFamily(),
        GetFontSize(),
        DWRITE_TEXT_ALIGNMENT_LEADING,
        DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
        DWRITE_FONT_WEIGHT_NORMAL,
        true);
}

void FilePicker::RenderPopup(GraphicsContext& ctx) {
    const float progress = PopupProgress();
    if (progress <= 0.001f) {
        return;
    }

    const Rect pop = GetPopupBounds();
    m_browser.RenderChrome(
        ctx,
        pop,
        progress,
        m_hoverUp,
        m_hoverFilter,
        m_hoverCancel,
        m_hoverConfirm,
        m_filterDropDownOpen);

    // Clip body content to animated popup height.
    const float currentH = (progress >= 0.98f) ? pop.height : (pop.height * progress);
    ctx.PushClip(Rect(pop.x, pop.y, pop.width, currentH));
    m_breadcrumbHost.Layout(m_browser, pop);
    m_breadcrumbHost.Render(ctx);
    m_treeHost.Layout(m_browser.ListRect(pop));
    m_treeHost.Render(ctx);
    ctx.PopClip();

    // Filter menu above tree/breadcrumb so it is not covered.
    if (m_filterDropDownOpen) {
        m_browser.RenderFilterDropdown(ctx, pop, progress, m_hoverFilterItem);
    }
}

void FilePicker::CollectPopupOwnedElements(std::vector<UIElement*>& out) const {
    if (TreeView* tree = m_treeHost.GetTree()) {
        out.push_back(tree);
    }
    if (BreadcrumbBar* bar = m_breadcrumbHost.GetBar()) {
        out.push_back(bar);
    }
}

void FilePicker::OnRenderOverlay(GraphicsContext& ctx) {
    if (PopupProgress() <= 0.001f) {
        return;
    }
    if (PopupHost::Current() && m_isPopupOpen) {
        return;
    }
    RenderPopup(ctx);
}

void FilePicker::OnMouseDown(Point pt) {
    if (!IsEnabled()) {
        return;
    }

    if (m_isPopupOpen) {
        const Rect pop = GetPopupBounds();
        const float progress = PopupProgress();
        const float currentH = (progress >= 0.98f) ? pop.height : (pop.height * progress);
        if (Rect(pop.x, pop.y, pop.width, currentH).Contains(pt.x, pt.y)) {
            HandleBrowserClick(pt);
            return;
        }
        SetPopupOpen(false);
        return;
    }

    m_pressed = HitTestPart(pt);
    MarkPickerDirty();
}

void FilePicker::OnMouseUp(Point pt) {
    const HitPart pressed = m_pressed;
    m_pressed = HitPart::None;
    if (!m_isPopupOpen && pressed != HitPart::None && HitTestPart(pt) == pressed) {
        SetPopupOpen(true);
    }
    MarkPickerDirty();
}

void FilePicker::OnMouseMove(Point pt) {
    if (!IsEnabled()) {
        return;
    }
    if (m_isPopupOpen) {
        UpdateHover(pt);
        return;
    }
    const HitPart next = HitTestPart(pt);
    if (next != m_hover) {
        m_hover = next;
        MarkPickerDirty();
    }
}

void FilePicker::OnMouseLeave() {
    m_hover = HitPart::None;
    m_pressed = HitPart::None;
    m_hoverUp = false;
    m_hoverFilter = false;
    m_hoverCancel = false;
    m_hoverConfirm = false;
    m_hoverFilterItem = -1;
    MarkPickerDirty();
}

bool FilePicker::OnKeyDown(int vkCode) {
    if (!IsEnabled()) {
        return false;
    }
    if (m_isPopupOpen) {
        if (vkCode == VK_ESCAPE) {
            if (m_filterDropDownOpen) {
                SetFilterDropDownOpen(false);
            } else {
                SetPopupOpen(false);
            }
            return true;
        }
        if (m_filterDropDownOpen && (vkCode == VK_UP || vkCode == VK_DOWN)) {
            const int count = static_cast<int>(m_browser.GetFilters().size());
            if (count <= 0) {
                return true;
            }
            int next = m_hoverFilterItem;
            if (next < 0) {
                next = m_browser.GetFilterIndex();
            }
            if (vkCode == VK_UP) {
                next = (next <= 0) ? 0 : (next - 1);
            } else {
                next = (std::min)(count - 1, next + 1);
            }
            m_hoverFilterItem = next;
            MarkPickerDirty();
            return true;
        }
        if (m_filterDropDownOpen && vkCode == VK_RETURN) {
            if (m_hoverFilterItem >= 0) {
                m_browser.SetFilterIndex(m_hoverFilterItem);
                m_treeHost.ApplyFilter(m_browser);
            }
            SetFilterDropDownOpen(false);
            SyncBrowserChrome();
            return true;
        }
        if (vkCode == VK_RETURN) {
            std::string path;
            if (m_treeHost.TryConfirm(m_browser, path)) {
                SetPath(path);
                SetPopupOpen(false);
            }
            return true;
        }
        if (vkCode == VK_BACK) {
            m_treeHost.GoUp(m_browser);
            SyncBrowserChrome();
            MarkPickerDirty();
            RequestAnimationTicks();
            return true;
        }
        return true;
    }
    if (vkCode == VK_RETURN || vkCode == VK_SPACE) {
        SetPopupOpen(true);
        return true;
    }
    return Control::OnKeyDown(vkCode);
}

bool FilePicker::OnAnimationTick() {
    const float dt = UIElement::GetAnimationDeltaSeconds();
    m_popupAnim.SetTarget(m_isPopupOpen ? 1.0f : 0.0f);
    bool animating = m_popupAnim.Tick(dt, AnimationSpec{ 0.55f, 0.01f });
    // Popup open/close only. TreeView/Breadcrumb register themselves through
    // AnimationHost(this) — do not manually Tick them here.
    if (animating || m_isPopupOpen || PopupProgress() > 0.001f) {
        MarkPickerDirty();
        RequestAnimationTicks();
    }
    return animating
        || std::abs(m_popupAnim.Target() - m_popupAnim.Current()) > 0.001f;
}

bool FilePicker::HasSelfAnimation() const {
    return std::abs(m_popupAnim.Target() - m_popupAnim.Current()) > 0.001f;
}

} // namespace CUI
