#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "FilePicker.h"
#include "FileDialogHelper.h"
#include "../style/ThemeManager.h"
#include "../window/Window.h"
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
    if (!IsEnabled() || m_hover == HitPart::None) {
        return nullptr;
    }
    return LoadCursor(nullptr, m_hover == HitPart::Path ? IDC_IBEAM : IDC_HAND);
}

Size FilePicker::Measure(Size availableSize) {
    (void)availableSize;
    float w = GetWidth() >= 0.0f ? GetWidth() : 320.0f;
    float h = GetHeight() >= 0.0f ? GetHeight() : kDefaultH;
    m_desiredSize = Size(w, h);
    m_measureDirty = false;
    return m_desiredSize;
}

void FilePicker::MarkPickerDirty() {
    MarkRenderRectDirty(m_bounds.Inflate(2.0f));
}

void FilePicker::OpenDialog() {
    if (!IsEnabled()) {
        return;
    }
    HWND owner = Window::Current() ? Window::Current()->GetHWND() : nullptr;

    std::vector<FileDialogFilter> filters;
    filters.reserve(m_filters.size());
    for (const auto& filter : m_filters) {
        filters.push_back({ Utf8ToUtf16(filter.first), Utf8ToUtf16(filter.second) });
    }

    std::string selected;
    if (!ShowOpenFileDialog(owner, selected, Utf8ToUtf16(m_dialogTitle), filters)) {
        return;
    }
    SetPath(selected);
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
    const bool browseHot = (m_hover == HitPart::Browse || m_pressed == HitPart::Browse);
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
    ctx.DrawChevron(glyph, tokens.textSecondary, GraphicsContext::ChevronDirection::Down, 1.5f);

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

void FilePicker::OnMouseDown(Point pt) {
    if (!IsEnabled()) {
        return;
    }
    m_pressed = HitTestPart(pt);
    MarkPickerDirty();
}

void FilePicker::OnMouseUp(Point pt) {
    const HitPart pressed = m_pressed;
    m_pressed = HitPart::None;
    if (pressed != HitPart::None && HitTestPart(pt) == pressed) {
        OpenDialog();
    }
    MarkPickerDirty();
}

void FilePicker::OnMouseMove(Point pt) {
    if (!IsEnabled()) {
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
    MarkPickerDirty();
}

void FilePicker::OnKeyDown(int vkCode) {
    if (!IsEnabled()) {
        return;
    }
    if (vkCode == VK_RETURN || vkCode == VK_SPACE) {
        OpenDialog();
        return;
    }
    Control::OnKeyDown(vkCode);
}

} // namespace CUI
