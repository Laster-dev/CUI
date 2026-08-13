#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "TeachingTip.h"
#include "../style/ThemeManager.h"
#include "../window/PopupPlacement.h"
#include <algorithm>
#include <cmath>

#ifndef VK_ESCAPE
#define VK_ESCAPE 0x1B
#endif

namespace CUI {

namespace {
constexpr float kPad = 14.0f;
constexpr float kGap = 8.0f;
constexpr float kClose = 22.0f;
constexpr float kActionH = 28.0f;
constexpr float kCaret = 7.0f;
constexpr float kRadius = 8.0f;

Size MeasureWrapped(const std::string& text, float fontSize, float maxWidth, DWRITE_FONT_WEIGHT weight) {
    if (text.empty()) {
        return Size(0.0f, 0.0f);
    }
    GraphicsContext::TextLayoutOptions options;
    options.maxWidth = (std::max)(32.0f, maxWidth);
    options.maxHeight = 480.0f;
    options.wrapping = DWRITE_WORD_WRAPPING_WRAP;
    ComPtr<IDWriteTextLayout> layout = GraphicsContext::CreateTextLayout(
        Utf8ToUtf16(text), "微软雅黑", fontSize, options, weight);
    if (!layout) {
        return Size(maxWidth, fontSize + 4.0f);
    }
    DWRITE_TEXT_METRICS metrics{};
    layout->GetMetrics(&metrics);
    return Size(
        std::ceil(metrics.widthIncludingTrailingWhitespace),
        std::ceil(metrics.height));
}

void DrawWrapped(
    GraphicsContext& ctx,
    const std::string& text,
    const Rect& rect,
    float fontSize,
    D2D1_COLOR_F color,
    DWRITE_FONT_WEIGHT weight)
{
    if (text.empty() || rect.IsEmpty()) {
        return;
    }
    GraphicsContext::TextLayoutOptions options;
    options.maxWidth = (std::max)(1.0f, rect.width);
    options.maxHeight = (std::max)(1.0f, rect.height);
    options.wrapping = DWRITE_WORD_WRAPPING_WRAP;
    ComPtr<IDWriteTextLayout> layout = GraphicsContext::CreateTextLayout(
        Utf8ToUtf16(text), "微软雅黑", fontSize, options, weight);
    if (layout) {
        ctx.DrawTextLayout(layout.Get(), rect, color);
    }
}
} // namespace

TeachingTip::TeachingTip() {
    SetVisibility(Visibility::Visible);
    SetClipToBounds(false);
    SetBackgroundToken(ThemeTokenId::CardBackground);
    SetBorderToken(ThemeTokenId::CardBorder);
    SetTitleColorToken(ThemeTokenId::TextPrimary);
    SetMessageColorToken(ThemeTokenId::TextSecondary);
    SetAccentColorToken(ThemeTokenId::AccentColor);
}

Value TeachingTip::GetProperty(PropertyId id) const {
    switch (id) {
    case PropertyId::Title: return Value(m_title);
    case PropertyId::Message: return Value(m_message);
    case PropertyId::ActionText: return Value(m_actionText);
    case PropertyId::IsClosable: return Value(m_closeVisible);
    case PropertyId::IsOpen: return Value(m_isOpen);
    default: return Control::GetProperty(id);
    }
}

bool TeachingTip::HasProperty(PropertyId id) const {
    switch (id) {
    case PropertyId::Title:
    case PropertyId::Message:
    case PropertyId::ActionText:
    case PropertyId::IsClosable:
    case PropertyId::IsOpen:
        return true;
    default:
        return Control::HasProperty(id);
    }
}

void TeachingTip::SetProperty(PropertyId id, const Value& val) {
    switch (id) {
    case PropertyId::Title: SetTitle(val.AsString()); return;
    case PropertyId::Message: SetMessage(val.AsString()); return;
    case PropertyId::ActionText: SetActionText(val.AsString()); return;
    case PropertyId::IsClosable: SetIsCloseVisible(val.AsBool()); return;
    case PropertyId::IsOpen:
        if (val.AsBool()) {
            if (m_anchor) {
                ShowAround(m_anchor);
            }
        } else {
            Close();
        }
        return;
    default:
        Control::SetProperty(id, val);
        return;
    }
}

TeachingTip::~TeachingTip() {
    if (PopupHost* host = PopupHost::Current()) {
        host->Close(this);
    }
}

void TeachingTip::SetTitle(const std::string& title) {
    if (m_title == title) {
        return;
    }
    m_title = title;
    if (m_isOpen) {
        Relayout();
        DirtyPopup();
    }
}

void TeachingTip::SetMessage(const std::string& message) {
    if (m_message == message) {
        return;
    }
    m_message = message;
    if (m_isOpen) {
        Relayout();
        DirtyPopup();
    }
}

void TeachingTip::SetActionText(const std::string& text) {
    if (m_actionText == text) {
        return;
    }
    m_actionText = text;
    if (m_isOpen) {
        Relayout();
        DirtyPopup();
    }
}

void TeachingTip::SetIsCloseVisible(bool visible) {
    if (m_closeVisible == visible) {
        return;
    }
    m_closeVisible = visible;
    if (m_isOpen) {
        Relayout();
        DirtyPopup();
    }
}

void TeachingTip::SetIsModal(bool modal) {
    if (m_isModal == modal) {
        return;
    }
    m_isModal = modal;
    if (m_isOpen) {
        DirtyPopup();
    }
}

void TeachingTip::SetPreferredPlacement(BubblePlacement placement) {
    if (m_preferredPlacement == placement) {
        return;
    }
    m_preferredPlacement = placement;
    if (m_isOpen) {
        Relayout();
        DirtyPopup();
    }
}

void TeachingTip::SetMaxWidth(float width) {
    width = (std::max)(160.0f, width);
    if (std::abs(m_maxWidth - width) < 0.01f) {
        return;
    }
    m_maxWidth = width;
    if (m_isOpen) {
        Relayout();
        DirtyPopup();
    }
}

Size TeachingTip::Measure(Size availableSize) {
    (void)availableSize;
    m_desiredSize = Size(0.0f, 0.0f);
    return m_desiredSize;
}

void TeachingTip::Arrange(Rect finalRect) {
    SetBounds(Rect(finalRect.x, finalRect.y, 0.0f, 0.0f));
}

void TeachingTip::Relayout() {
    const Rect viewport = GetPopupViewportOrDefault();
    const float innerMax = (std::max)(80.0f, m_maxWidth - kPad * 2.0f - (m_closeVisible ? (kClose + 6.0f) : 0.0f));
    const Size titleSize = MeasureWrapped(m_title, 14.0f, innerMax, DWRITE_FONT_WEIGHT_SEMI_BOLD);
    const Size bodySize = MeasureWrapped(m_message, 12.0f, (std::max)(80.0f, m_maxWidth - kPad * 2.0f), DWRITE_FONT_WEIGHT_NORMAL);

    Size actionSize(0.0f, 0.0f);
    if (!m_actionText.empty()) {
        Size label = MeasureWrapped(m_actionText, 12.0f, innerMax, DWRITE_FONT_WEIGHT_SEMI_BOLD);
        actionSize.width = (std::max)(64.0f, label.width + 20.0f);
        actionSize.height = kActionH;
    }

    float contentW = (std::max)(titleSize.width + (m_closeVisible ? (kClose + 6.0f) : 0.0f), bodySize.width);
    contentW = (std::max)(contentW, actionSize.width);
    contentW = std::clamp(contentW, 160.0f, (std::max)(160.0f, m_maxWidth - kPad * 2.0f));

    float contentH = 0.0f;
    if (titleSize.height > 0.0f) {
        contentH += (std::max)(titleSize.height, m_closeVisible ? kClose : 0.0f);
    } else if (m_closeVisible) {
        contentH += kClose;
    }
    if (bodySize.height > 0.0f) {
        if (contentH > 0.0f) {
            contentH += kGap;
        }
        contentH += bodySize.height;
    }
    if (actionSize.height > 0.0f) {
        contentH += 12.0f + actionSize.height;
    }

    const Size cardSize(contentW + kPad * 2.0f, contentH + kPad * 2.0f);
    const Rect anchor = m_anchor ? m_anchor->GetBounds() : Rect(viewport.x + viewport.width * 0.5f, viewport.y + viewport.height * 0.5f, 1.0f, 1.0f);
    m_layout = LayoutBubble(anchor, cardSize, viewport, m_preferredPlacement, 10.0f, kCaret, 8.0f);

    float y = m_layout.card.y + kPad;
    const float textLeft = m_layout.card.x + kPad;
    const float textWidth = m_layout.card.width - kPad * 2.0f;
    if (m_closeVisible) {
        m_closeRect = Rect(
            m_layout.card.x + m_layout.card.width - kPad - kClose,
            m_layout.card.y + kPad,
            kClose,
            kClose);
    } else {
        m_closeRect = Rect();
    }

    if (titleSize.height > 0.0f) {
        const float titleW = m_closeVisible
            ? (std::max)(0.0f, textWidth - kClose - 6.0f)
            : textWidth;
        m_titleRect = Rect(textLeft, y, titleW, titleSize.height);
        y += (std::max)(titleSize.height, m_closeVisible ? kClose : 0.0f);
    } else {
        m_titleRect = Rect();
        if (m_closeVisible) {
            y += kClose;
        }
    }

    if (bodySize.height > 0.0f) {
        if (!m_titleRect.IsEmpty() || m_closeVisible) {
            y += kGap;
        }
        m_bodyRect = Rect(textLeft, y, textWidth, bodySize.height);
        y += bodySize.height;
    } else {
        m_bodyRect = Rect();
    }

    if (actionSize.height > 0.0f) {
        y += 12.0f;
        m_actionRect = Rect(
            m_layout.card.x + m_layout.card.width - kPad - actionSize.width,
            y,
            actionSize.width,
            actionSize.height);
    } else {
        m_actionRect = Rect();
    }
}

void TeachingTip::ShowAround(UIElement* target) {
    if (!target) {
        return;
    }
    m_anchor = target;
    Relayout();
    m_isOpen = true;
    m_hotHover = Hotspot::None;
    m_actionPressed = false;
    m_popupAnim.SetTarget(1.0f);
    if (!UIElement::AreAnimationsEnabled()) {
        m_popupAnim.Reset(1.0f);
    }
    if (PopupHost* host = PopupHost::Current()) {
        host->Open(this);
    }
    RequestAnimationTicks();
    DirtyPopup();
}

void TeachingTip::Close() {
    if (!m_isOpen && m_popupAnim.Current() <= 0.001f) {
        return;
    }
    const bool wasOpen = m_isOpen;
    m_isOpen = false;
    m_hotHover = Hotspot::None;
    m_actionPressed = false;
    m_popupAnim.SetTarget(0.0f);
    if (!UIElement::AreAnimationsEnabled()) {
        m_popupAnim.Reset(0.0f);
    }
    if (PopupHost* host = PopupHost::Current()) {
        host->Close(this);
    }
    RequestAnimationTicks();
    DirtyPopup();
    if (wasOpen) {
        m_onClosed.Invoke();
    }
}

Rect TeachingTip::GetPopupBounds() const {
    return m_layout.total;
}

bool TeachingTip::HitDismissExempt(float x, float y) const {
    if (!m_isOpen && m_popupAnim.Current() <= 0.001f) {
        return false;
    }
    if (m_layout.total.Contains(x, y) || m_layout.card.Contains(x, y)) {
        return true;
    }
    if (m_anchor && m_anchor->GetBounds().Contains(x, y)) {
        return true;
    }
    return m_isModal;
}

UIElement* TeachingTip::HitTestPopup(float x, float y) {
    if (!m_isOpen || m_popupAnim.Current() < 0.4f) {
        return nullptr;
    }
    if (m_isModal) {
        return this;
    }
    if (m_layout.total.Contains(x, y) || m_layout.card.Contains(x, y)) {
        return this;
    }
    return nullptr;
}

void TeachingTip::OnLightDismiss() {
    Close();
}

void TeachingTip::CollectPopupDirty(Rect& dirtyRect, bool& hasDirty) const {
    if (m_isModal) {
        const Rect viewport = GetPopupViewportOrDefault();
        if (!viewport.IsEmpty()) {
            dirtyRect = hasDirty ? dirtyRect.Union(viewport) : viewport;
            hasDirty = true;
            return;
        }
    }
    Rect area = GetPopupBounds().Inflate(8.0f);
    if (area.IsEmpty()) {
        return;
    }
    dirtyRect = hasDirty ? dirtyRect.Union(area) : area;
    hasDirty = true;
}

void TeachingTip::DirtyPopup() {
    if (m_isModal) {
        MarkRenderRectDirty(GetPopupViewportOrDefault());
        return;
    }
    Rect area = GetPopupBounds().Inflate(8.0f);
    if (!area.IsEmpty()) {
        MarkRenderRectDirty(area);
    }
}

void TeachingTip::OnRenderOverlay(GraphicsContext& ctx) {
    if (PopupHost::Current() && m_isOpen) {
        return;
    }
    RenderPopup(ctx);
}

void TeachingTip::RenderPopup(GraphicsContext& ctx) {
    const float progress = UIElement::AreAnimationsEnabled()
        ? m_popupAnim.Current()
        : (m_isOpen ? 1.0f : 0.0f);
    if (progress <= 0.001f || m_layout.card.IsEmpty()) {
        return;
    }

    if (m_isModal) {
        const Rect viewport = GetPopupViewportOrDefault();
        D2D1_COLOR_F scrim = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.28f * progress);
        ctx.FillRect(viewport, scrim);
    }

    ctx.PushOpacity(progress);
    D2D1_COLOR_F bg = ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBackground);
    D2D1_COLOR_F border = ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBorder);
    PaintBubble(ctx, m_layout, bg, border, kRadius);

    D2D1_COLOR_F titleColor = ResolveThemeColor(GetTitleColorToken(), ThemeTokenId::TextPrimary);
    D2D1_COLOR_F bodyColor = ResolveThemeColor(GetMessageColorToken(), ThemeTokenId::TextSecondary);
    DrawWrapped(ctx, m_title, m_titleRect, 14.0f, titleColor, DWRITE_FONT_WEIGHT_SEMI_BOLD);
    DrawWrapped(ctx, m_message, m_bodyRect, 12.0f, bodyColor, DWRITE_FONT_WEIGHT_NORMAL);

    if (m_closeVisible && !m_closeRect.IsEmpty()) {
        const bool hot = m_hotHover == Hotspot::Close;
        if (hot) {
            D2D1_COLOR_F hover = ThemeManager::Instance().GetFlatColor(ThemeTokenId::HoverBackground);
            ctx.FillRoundedRect(m_closeRect, 4.0f, hover);
        }
        D2D1_COLOR_F xColor = bodyColor;
        const float cx = m_closeRect.x + m_closeRect.width * 0.5f;
        const float cy = m_closeRect.y + m_closeRect.height * 0.5f;
        const float arm = 5.0f;
        ctx.DrawSmoothLine(Point(cx - arm, cy - arm), Point(cx + arm, cy + arm), xColor, 1.4f);
        ctx.DrawSmoothLine(Point(cx + arm, cy - arm), Point(cx - arm, cy + arm), xColor, 1.4f);
    }

    if (!m_actionRect.IsEmpty()) {
        D2D1_COLOR_F accent = ResolveThemeColor(GetAccentColorToken(), ThemeTokenId::AccentColor);
        if (m_actionPressed) {
            accent.r *= 0.86f;
            accent.g *= 0.86f;
            accent.b *= 0.86f;
        } else if (m_hotHover == Hotspot::Action) {
            accent.r = std::clamp(accent.r * 1.08f, 0.0f, 1.0f);
            accent.g = std::clamp(accent.g * 1.08f, 0.0f, 1.0f);
            accent.b = std::clamp(accent.b * 1.08f, 0.0f, 1.0f);
        }
        ctx.FillRoundedRect(m_actionRect, 4.0f, accent);
        D2D1_COLOR_F onAccent = ThemeManager::Instance().GetFlatColor(ThemeTokenId::AccentForeground);
        ctx.DrawText(
            m_actionText,
            m_actionRect,
            onAccent,
            "微软雅黑",
            12.0f,
            DWRITE_TEXT_ALIGNMENT_CENTER,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
    ctx.PopOpacity();
}

TeachingTip::Hotspot TeachingTip::HitHotspot(float x, float y) const {
    if (m_closeVisible && m_closeRect.Contains(x, y)) {
        return Hotspot::Close;
    }
    if (!m_actionRect.IsEmpty() && m_actionRect.Contains(x, y)) {
        return Hotspot::Action;
    }
    if (m_layout.card.Contains(x, y) || m_layout.total.Contains(x, y)) {
        return Hotspot::Card;
    }
    return Hotspot::None;
}

bool TeachingTip::SetHotHover(Hotspot hotspot) {
    if (m_hotHover == hotspot) {
        return false;
    }
    m_hotHover = hotspot;
    DirtyPopup();
    return true;
}

void TeachingTip::OnMouseMove(Point pt) {
    if (!m_isOpen) {
        return;
    }
    SetHotHover(HitHotspot(pt.x, pt.y));
}

void TeachingTip::OnMouseDown(Point pt) {
    if (!m_isOpen) {
        return;
    }
    const Hotspot hit = HitHotspot(pt.x, pt.y);
    SetHotHover(hit);
    if (hit == Hotspot::Close) {
        Close();
        return;
    }
    if (hit == Hotspot::Action) {
        m_actionPressed = true;
        DirtyPopup();
        return;
    }
    if (hit == Hotspot::None && m_isModal) {
        Close();
    }
}

void TeachingTip::OnMouseUp(Point pt) {
    if (!m_isOpen) {
        return;
    }
    if (m_actionPressed) {
        m_actionPressed = false;
        DirtyPopup();
        if (HitHotspot(pt.x, pt.y) == Hotspot::Action) {
            m_onAction.Invoke();
            Close();
        }
    }
}

bool TeachingTip::OnKeyDown(int vkCode) {
    if (!m_isOpen) {
        return false;
    }
    if (vkCode == VK_ESCAPE) {
        Close();
        return true;
    }
    if (vkCode == VK_RETURN && !m_actionText.empty()) {
        m_onAction.Invoke();
        Close();
        return true;
    }
    return false;
}

bool TeachingTip::OnAnimationTick() {
    const float dt = UIElement::GetAnimationDeltaSeconds();
    m_popupAnim.SetTarget(m_isOpen ? 1.0f : 0.0f);
    const bool animating = m_popupAnim.Tick(dt, AnimationSpec{ 0.22f, 0.01f });
    if (animating) {
        DirtyPopup();
        RequestAnimationTicks();
    }
    return animating;
}

bool TeachingTip::HasSelfAnimation() const {
    return std::abs(m_popupAnim.Target() - m_popupAnim.Current()) > 0.001f;
}

bool TeachingTip::IsModalOverlayOpen() const {
    return m_isModal && (m_isOpen || HasSelfAnimation());
}

void TeachingTip::CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const {
    if (m_isModal && (m_isOpen || m_popupAnim.Current() > 0.001f)) {
        const Rect viewport = GetPopupViewportOrDefault();
        if (!viewport.IsEmpty()) {
            dirtyRect = hasDirty ? dirtyRect.Union(viewport) : viewport;
            hasDirty = true;
            return;
        }
    }
    if (HasSelfAnimation() || m_isOpen || m_popupAnim.Current() > 0.001f) {
        Rect area = GetPopupBounds().Inflate(8.0f);
        if (!area.IsEmpty()) {
            dirtyRect = hasDirty ? dirtyRect.Union(area) : area;
            hasDirty = true;
        }
    }
}

void TeachingTip::CollectAnimationBounds(Rect& dirtyRect, bool& hasDirty) const {
    CollectSelfAnimationBounds(dirtyRect, hasDirty);
}

void TeachingTip::OnNavigatedFrom() {
    Close();
    UIElement::OnNavigatedFrom();
}

} // namespace CUI
