#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ComboBox.h"
#include "../style/ThemeManager.h"
#include "../window/PopupPlacement.h"
#include <windows.h>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace CUI {

namespace {
Rect ComboBoxMenuRect(const Rect& bounds, float itemHeight, size_t itemCount) {
    float menuH = itemHeight * static_cast<float>((std::max)(itemCount, size_t{ 1 }));
    const Rect viewport = GetPopupViewportOrDefault();
    return PlacePopupNearAnchor(bounds, bounds.width, menuH, viewport, 2.0f);
}
} // namespace

std::vector<PropertyMeta> ComboBox::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "fontFamily", "字体名称 (FontFamily)", "字体文本", "enum", { "微软雅黑", "Segoe UI", "Consolas", "Times New Roman" } });
    metas.push_back({ "fontSize", "字体大小 (FontSize)", "字体文本", "number" });
    metas.push_back({ "itemHeight", "下拉项高度 (ItemHeight)", "下拉控制", "number" });
    return metas;
}

ComboBox::ComboBox() {
    SetPlaceholder("Select option...");
    SetBackgroundToken(ThemeTokenId::InputBackground);
    SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
    SetBorderToken(ThemeTokenId::InputBorder);
    SetFocusedBorderToken(ThemeTokenId::FocusedBorder);
    SetColorToken(ThemeTokenId::TextPrimary);
    SetDropdownBackgroundToken(ThemeTokenId::CardBackground);
    SetSelectedItemBackgroundToken(ThemeTokenId::SelectedBackground);
    SetBackground(ThemeManager::Instance().GetColor("inputBackground"));
    SetHoverBackground(ThemeManager::Instance().GetColor("hoverBackground"));
    SetBorderBrush(ThemeManager::Instance().GetColor("inputBorder"));
    SetBorderThickness(1.0f);
    SetKeyboardNavigationMode(KeyboardNavigationMode::Contained);
    SetColor(ThemeManager::Instance().GetColor("textPrimary"));
    SetFontSize(13.0f);
    SetFontFamily("微软雅黑");
    SetPadding(Thickness(10, 6, 10, 6));
    SetCornerRadius(3.0f);
    SetWidth(200.0f);
    SetHeight(32.0f);
}

void ComboBox::SetProperty(PropertyId id, const Value& val) {
    if (id == PropertyId::Items) { SetItems(val.AsString("")); return; }
    Control::SetProperty(id, val);
}

void ComboBox::SetItems(const std::string& itemsCsv) {
    ClearItems();
    std::stringstream ss(itemsCsv);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (!item.empty()) AddItem(item);
    }
}

void ComboBox::AddItem(const std::string& item) {
    m_items.push_back(item);
    if (m_selectedIndex == -1) {
        SetSelectedIndex(0);
    }
}

void ComboBox::ClearItems() {
    m_items.clear();
    m_selectedIndex = -1;
}

void ComboBox::SetSelectedIndex(int index) {
    if (index >= 0 && index < static_cast<int>(m_items.size())) {
        if (m_selectedIndex != index) {
            m_selectedIndex = index;
            m_onSelectionChangedEvent.Invoke(this, m_selectedIndex, m_items[m_selectedIndex]);
        }
    }
}

std::string ComboBox::GetSelectedItem() const {
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_items.size())) {
        return m_items[m_selectedIndex];
    }
    return "";
}

Size ComboBox::Measure(Size availableSize) {
    (void)availableSize;
    const float expW = (GetWidth() >= 0.0f) ? GetWidth() : 200.0f;
    const float expH = (GetHeight() >= 0.0f) ? GetHeight() : 32.0f;
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

void ComboBox::OnRender(GraphicsContext& ctx) {
    float radius = GetCornerRadius();
    D2D1_COLOR_F bg = GetAnimatedBackground(ThemeManager::Instance().GetFlatColor(ThemeTokenId::InputBackground));
    if (m_isDropDownOpen) {
        bg = BlendColor(bg, ResolveThemeColor(GetHoverBackgroundToken(), ThemeTokenId::HoverBackground), 0.8f);
    }

    if (radius > 0.0f) {
        ctx.FillRoundedRect(m_bounds, radius, bg);
    } else {
        ctx.FillRect(m_bounds, bg);
    }

    D2D1_COLOR_F borderBrush = (m_isFocused || m_isDropDownOpen)
        ? ResolveThemeColor(GetFocusedBorderToken(), ThemeTokenId::FocusedBorder)
        : ResolveThemeColor(GetBorderToken(), ThemeTokenId::InputBorder);

    float borderThickness = GetBorderThickness();
    if (borderThickness > 0.0f) {
        if (radius > 0.0f) {
            ctx.DrawRoundedRect(m_bounds, radius, borderBrush, (m_isFocused || m_isDropDownOpen) ? 1.5f : borderThickness);
        } else {
            ctx.DrawRect(m_bounds, borderBrush, (m_isFocused || m_isDropDownOpen) ? 1.5f : borderThickness);
        }
    }

    Thickness padding = GetPadding();

    // Render current text
    std::string displayText = GetSelectedItem();
    if (displayText.empty()) {
        displayText = GetPlaceholder();
        if (displayText.empty()) displayText = "Select option...";
    }

    D2D1_COLOR_F textColor = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextPrimary);
    const std::string& font = GetFontFamily();
    float fontSize = GetFontSize();

    constexpr float kChevron = 12.0f;
    constexpr float kChevronPad = 8.0f;
    Rect textRect(
        m_bounds.x + padding.left,
        m_bounds.y + padding.top,
        (std::max)(0.0f, m_bounds.width - padding.left - padding.right - kChevron - kChevronPad),
        m_bounds.height - padding.top - padding.bottom);
    ctx.DrawText(displayText, textRect, textColor, font, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    const bool open = m_isDropDownOpen || m_arrowAnim.Current() > 0.5f;
    const Rect arrowRect(
        m_bounds.x + m_bounds.width - padding.right - kChevron,
        m_bounds.y + (m_bounds.height - kChevron) * 0.5f,
        kChevron,
        kChevron);
    ctx.DrawChevron(
        arrowRect,
        ThemeManager::Instance().GetColor(ThemeTokenId::TextSecondary),
        open ? GraphicsContext::ChevronDirection::Up : GraphicsContext::ChevronDirection::Down);
}

Rect ComboBox::GetPopupBounds() const {
    float itemHeight = GetItemHeight();
    if (itemHeight < 0.0f) itemHeight = 28.0f;
    return ComboBoxMenuRect(m_bounds, itemHeight, m_items.size());
}

bool ComboBox::HitDismissExempt(float x, float y) const {
    if (m_bounds.Contains(x, y)) return true;
    return GetPopupBounds().Contains(x, y);
}

void ComboBox::RenderPopup(GraphicsContext& ctx) {
    float progress = UIElement::AreAnimationsEnabled() ? m_popupAnim.Current() : (m_isDropDownOpen ? 1.0f : 0.0f);
    if (progress <= 0.001f || m_items.empty()) return;

    float itemHeight = GetItemHeight();
    if (itemHeight < 0.0f) itemHeight = 28.0f;
    Rect menuRect = ComboBoxMenuRect(m_bounds, itemHeight, m_items.size());
    const float menuH = menuRect.height;
    ctx.PushPopupReveal(menuRect, progress, Point(menuRect.x + menuRect.width * 0.5f, menuRect.y));

    const std::string& font = GetFontFamily();
    float fontSize = GetFontSize();

    D2D1_COLOR_F dropBg = ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBackground);
    D2D1_COLOR_F selBg = ThemeManager::Instance().GetFlatColor(ThemeTokenId::SelectedBackground);
    float radius = GetCornerRadius();
    if (radius < 0.0f) radius = 4.0f;

    ctx.FillRoundedRect(menuRect, radius, dropBg);
    ctx.DrawRoundedRect(menuRect, radius, ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBorder), 1.0f);

    for (size_t i = 0; i < m_items.size(); ++i) {
        Rect itemRect(menuRect.x + 2, menuRect.y + i * itemHeight + 2 - m_scrollOffset, menuRect.width - 4, itemHeight - 4);
        bool isSelected = (static_cast<int>(i) == m_selectedIndex);

        if (isSelected) {
            ctx.FillRoundedRect(itemRect, 2.0f, selBg);
        }

        D2D1_COLOR_F itemColor = ThemeManager::Instance().GetFlatColor(ThemeTokenId::TextPrimary);
        ctx.DrawText(m_items[i], Rect(itemRect.x + 8, itemRect.y, itemRect.width - 16, itemRect.height), itemColor, font, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    // Scrollbar
    {
        const float contentH = itemHeight * static_cast<float>(m_items.size());
        const float maxScroll = (std::max)(0.0f, contentH - menuH);
        if (maxScroll > 0.001f && menuH > 0.001f && m_scrollbarAutoHide.IsDrawn()) {
            constexpr float kScrollW = 8.0f;
            const float trackX = menuRect.x + menuRect.width - kScrollW;
            const Rect trackRect(trackX, menuRect.y, kScrollW, menuH);
            const float vis = m_scrollbarAutoHide.Opacity();

            D2D1_COLOR_F trackColor = ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBorder);
            trackColor.a = 0.35f * vis;
            ctx.DrawRoundedRect(trackRect, 4.0f, trackColor, 1.0f);

            const float thumbH = (std::max)(16.0f, (menuH * menuH) / contentH);
            const float travel = (std::max)(0.0f, menuH - thumbH);
            const float thumbY = trackRect.y + (m_scrollOffset / maxScroll) * travel;
            Rect thumbRect(trackX + 1.5f, thumbY, kScrollW - 3.0f, thumbH);

            D2D1_COLOR_F thumbColor = ThemeManager::Instance().GetFlatColor(ThemeTokenId::AccentColor);
            thumbColor.a = 0.45f * vis;
            ctx.FillRoundedRect(thumbRect, 4.0f, thumbColor);
        }
    }

    ctx.PopPopupReveal();
}

void ComboBox::OnRenderOverlay(GraphicsContext& ctx) {
    // When open, PopupHost owns paint; keep tree path for close-animation frames.
    if (PopupHost::Current() && m_isDropDownOpen) return;
    RenderPopup(ctx);
}

UIElement* ComboBox::HitTestOverlay(float x, float y) {
    float progress = UIElement::AreAnimationsEnabled() ? m_popupAnim.Current() : (m_isDropDownOpen ? 1.0f : 0.0f);
    if (progress <= 0.2f || m_items.empty()) return nullptr;

    float itemHeight = GetItemHeight();
    if (itemHeight < 0.0f) itemHeight = 28.0f;
    Rect menuRect = ComboBoxMenuRect(m_bounds, itemHeight, m_items.size());
    if (menuRect.Contains(x, y)) {
        return this;
    }
    return nullptr;
}

void ComboBox::OnMouseWheel(float delta) {
    if (!m_isDropDownOpen || m_items.empty()) return;

    float progress = UIElement::AreAnimationsEnabled() ? m_popupAnim.Current() : 1.0f;
    if (progress <= 0.5f) return;

    float itemHeight = GetItemHeight();
    if (itemHeight < 0.0f) itemHeight = 28.0f;

    Rect menuRect = ComboBoxMenuRect(m_bounds, itemHeight, m_items.size());
    const float contentH = itemHeight * static_cast<float>(m_items.size());
    const float visibleH = menuRect.height;
    const float maxScroll = (std::max)(0.0f, contentH - visibleH);
    if (maxScroll <= 0.001f) return;

    // delta > 0 => wheel up => scroll towards smaller indices.
    const float step = itemHeight; // about one row
    m_scrollOffset = std::clamp(m_scrollOffset - delta * step, 0.0f, maxScroll);
    m_scrollbarAutoHide.NotifyActivity(this);
    RequestAnimationTicks();
    MarkRenderContentDirty();
}

bool ComboBox::OnAnimationTick() {
    float dt = UIElement::GetAnimationDeltaSeconds();
    const AnimationSpec spec = PopupReveal::kSpec;

    m_popupAnim.SetTarget(m_isDropDownOpen ? 1.0f : 0.0f);
    bool animating = m_popupAnim.Tick(dt, spec);

    m_arrowAnim.SetTarget(m_isDropDownOpen ? 1.0f : 0.0f);
    if (m_arrowAnim.Tick(dt, spec)) animating = true;

    if (m_scrollbarAutoHide.Tick(dt)) {
        animating = true;
    }

    if (animating) {
        MarkRenderRectDirty(m_bounds.Inflate(4.0f));
        if (m_isDropDownOpen || m_popupAnim.Current() > 0.001f) {
            MarkRenderRectDirty(GetPopupBounds().Inflate(6.0f));
        }
        RequestAnimationTicks();
    }
    return animating;
}

bool ComboBox::HasSelfAnimation() const {
    return std::abs(m_popupAnim.Target() - m_popupAnim.Current()) > 0.001f ||
           std::abs(m_arrowAnim.Target() - m_arrowAnim.Current()) > 0.001f ||
           m_scrollbarAutoHide.NeedsTicks();
}

void ComboBox::CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const {
    if (HasSelfAnimation() && !m_bounds.IsEmpty()) {
        float itemHeight = GetItemHeight();
        if (itemHeight < 0.0f) itemHeight = 28.0f;
        float menuH = itemHeight * static_cast<float>((std::max)(m_items.size(), size_t{ 1 }));
        Rect menuRect(m_bounds.x, m_bounds.y + m_bounds.height + 2.0f, m_bounds.width, menuH);
        Rect area = m_bounds.Union(menuRect).Inflate(4.0f);
        dirtyRect = hasDirty ? dirtyRect.Union(area) : area;
        hasDirty = true;
    }
}

void ComboBox::CollectAnimationBounds(Rect& dirtyRect, bool& hasDirty) const {
    CollectSelfAnimationBounds(dirtyRect, hasDirty);
    for (const auto& child : GetChildren()) {
        if (child) {
            child->CollectAnimationBounds(dirtyRect, hasDirty);
        }
    }
}

UIElement* ComboBox::HitTest(float x, float y) {
    if (GetVisibility() != Visibility::Visible) return nullptr;

    if (m_bounds.Contains(x, y)) {
        return this;
    }

    return nullptr;
}

bool ComboBox::OnKeyDown(int vkCode) {
    if (!IsEnabled()) {
        return false;
    }
    const bool altDown = (GetKeyState(VK_MENU) & 0x8000) != 0;
    if (vkCode == VK_ESCAPE) {
        if (m_isDropDownOpen) {
            SetDropDownOpen(false);
            return true;
        }
        return false;
    }
    if (vkCode == VK_F4 || (vkCode == VK_DOWN && altDown) || (vkCode == VK_DOWN && !m_isDropDownOpen)
        || ((vkCode == VK_SPACE || vkCode == VK_RETURN) && !m_isDropDownOpen)) {
        SetDropDownOpen(true);
        return true;
    }
    if (m_isDropDownOpen && !m_items.empty()) {
        int idx = m_selectedIndex;
        if (idx < 0) {
            idx = 0;
        }
        if (vkCode == VK_DOWN) {
            idx = (std::min)(static_cast<int>(m_items.size()) - 1, idx + 1);
            SetSelectedIndex(idx);
            return true;
        }
        if (vkCode == VK_UP) {
            idx = (std::max)(0, idx - 1);
            SetSelectedIndex(idx);
            return true;
        }
        if (vkCode == VK_HOME) {
            SetSelectedIndex(0);
            return true;
        }
        if (vkCode == VK_END) {
            SetSelectedIndex(static_cast<int>(m_items.size()) - 1);
            return true;
        }
        if (vkCode == VK_RETURN || vkCode == VK_SPACE) {
            SetDropDownOpen(false);
            return true;
        }
    }
    return Control::OnKeyDown(vkCode);
}

void ComboBox::OnMouseDown(Point pt) {
    if (!IsEnabled()) {
        return;
    }
    Control::OnMouseDown(pt);

    if (m_isDropDownOpen && !m_items.empty()) {
        float itemHeight = GetItemHeight();
        if (itemHeight < 0.0f) itemHeight = 28.0f;
        Rect menuRect = ComboBoxMenuRect(m_bounds, itemHeight, m_items.size());

        float progress = UIElement::AreAnimationsEnabled() ? m_popupAnim.Current() : 1.0f;
        const float menuH = menuRect.height;

        if (progress > 0.2f && menuRect.Contains(pt.x, pt.y)) {
            // Scrollbar click: jump scroll position without closing the dropdown.
            constexpr float kScrollW = 8.0f;
            const Rect scrollRect(menuRect.x + menuRect.width - kScrollW, menuRect.y, kScrollW, menuH);
            if (scrollRect.Contains(pt.x, pt.y)) {
                const float contentH = itemHeight * static_cast<float>(m_items.size());
                const float maxScroll = (std::max)(0.0f, contentH - menuH);
                if (maxScroll > 0.001f && menuH > 0.001f) {
                    const float ratio = (std::clamp)((pt.y - menuRect.y) / menuH, 0.0f, 1.0f);
                    m_scrollOffset = ratio * maxScroll;
                    MarkRenderContentDirty();
                }
                return;
            }

            // itemRect top = menuRect.y + 2 - m_scrollOffset
            const float localY = (pt.y - (menuRect.y + 2.0f) + m_scrollOffset);
            int clickedIdx = static_cast<int>(localY / itemHeight);
            if (clickedIdx >= 0 && clickedIdx < static_cast<int>(m_items.size())) {
                SetSelectedIndex(clickedIdx);
            }
        }
        SetDropDownOpen(false);
    } else {
        SetDropDownOpen(true);
    }
}

void ComboBox::OnBlur() {
    Control::OnBlur();
    SetDropDownOpen(false);
}

void ComboBox::SetDropDownOpen(bool open) {
    if (m_isDropDownOpen == open) return;
    m_isDropDownOpen = open;
    if (open) {
        m_scrollOffset = 0.0f;
    }
    if (PopupHost* host = PopupHost::Current()) {
        if (open) {
            host->Open(this);
        } else {
            host->Close(this);
        }
    }
    RequestAnimationTicks();
    MarkRenderContentDirty();
}

} // namespace CUI
