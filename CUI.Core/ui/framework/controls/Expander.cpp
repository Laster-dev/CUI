#include "Expander.h"
#include "ProgressBarDiag.h"
#include "ScrollViewer.h"
#include "../window/Window.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <windows.h>

namespace CUI {

namespace {
float Clamp01(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

D2D1_COLOR_F MixColor(D2D1_COLOR_F a, D2D1_COLOR_F b, float t) {
    t = Clamp01(t);
    return D2D1::ColorF(
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t
    );
}
}

Expander::Expander() {
    SetBackgroundToken(ThemeTokenId::CardBackground);
    SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
    SetPressedBackgroundToken(ThemeTokenId::PressedBackground);
    SetBorderToken(ThemeTokenId::CardBorder);
    SetColorToken(ThemeTokenId::TextPrimary);
    SetCornerRadius(kCornerRadius);
    SetBorderThickness(1.0f);
    SetPadding(Thickness(0.0f));
    SetFontFamily("Segoe UI");
    SetFontSize(14.0f);
    SetAlign(Alignment::Stretch);
    SetClipToBounds(false);
    m_expandAnim.Reset(0.0f);
}

Expander::Expander(const std::string& headerText) : Expander() {
    m_header = headerText;
}

std::vector<PropertyMeta> Expander::GetPropertyMetas() const {
    auto metas = Control::GetPropertyMetas();
    metas.push_back({ "header", "标题 (Header)", "基本信息", "string" });
    metas.push_back({ "subtitle", "副标题 (Subtitle)", "基本信息", "string" });
    metas.push_back({ "isExpanded", "展开 (IsExpanded)", "Expander", "bool" });
    metas.push_back({ "expandDirection", "展开方向 (ExpandDirection)", "Expander", "enum", { "Down", "Up" } });
    return metas;
}

HCURSOR Expander::GetCursor() const {
    return (IsEnabled() && m_headerHovered) ? LoadCursor(nullptr, IDC_HAND) : nullptr;
}

void Expander::SetHeader(const std::string& header) {
    if (m_header == header) {
        return;
    }
    m_header = header;
    InvalidateExpanderLayout();
}

void Expander::SetSubtitle(const std::string& subtitle) {
    if (m_subtitle == subtitle) {
        return;
    }
    m_subtitle = subtitle;
    InvalidateExpanderLayout();
}

void Expander::SetIsExpanded(bool expanded) {
    if (m_isExpanded == expanded) {
        ProgressBarDiag::Log("[EXP] SetIsExpanded skip this=%p header=%s expanded=%d",
            (void*)this, m_header.c_str(), expanded ? 1 : 0);
        return;
    }

    m_isExpanded = expanded;
    if (expanded) {
        m_onExpanding.Invoke(this);
    } else {
        m_onCollapsed.Invoke(this);
    }
    m_onExpandedChanged.Invoke(this, m_isExpanded);
    ProgressBarDiag::Log("[EXP] SetIsExpanded this=%p header=%s expanded=%d animEnabled=%d",
        (void*)this, m_header.c_str(), m_isExpanded ? 1 : 0, UIElement::AreAnimationsEnabled() ? 1 : 0);

    if (UIElement::AreAnimationsEnabled()) {
        // Animate from the current progress — do not snap to 1.0 on expand.
        m_expandAnim.SetTarget(expanded ? 1.0f : 0.0f);
        RequestAnimationTicks();
    } else {
        m_expandAnim.Reset(expanded ? 1.0f : 0.0f);
    }

    UpdateContentVisibility();
    InvalidateExpanderLayout();
}

void Expander::SetExpandDirection(ExpandDirection direction) {
    if (m_expandDirection == direction) {
        return;
    }
    m_expandDirection = direction;
    InvalidateExpanderLayout();
}

void Expander::SetContent(std::shared_ptr<UIElement> content) {
    if (m_content == content) {
        return;
    }

    if (m_content) {
        RemoveChild(m_content);
    }
    m_content = std::move(content);
    if (m_content) {
        AddChild(m_content);
    }

    UpdateContentVisibility();
    InvalidateExpanderLayout();
}

float Expander::MeasureHeaderHeight(float) const {
    return m_subtitle.empty() ? kHeaderMinHeight : 60.0f;
}

float Expander::MeasureBodyHeight(float width) {
    if (!m_content) {
        return 0.0f;
    }

    const float innerWidth = (std::max)(0.0f, width - kBodyPadding * 2.0f);
    const Visibility previousVisibility = m_content->GetVisibility();
    // Must not call SetVisibility: that InvalidateMeasure()s up to the window
    // root, so a collapsed Expander keeps the tree dirty forever (mouse-move
    // FlushLayout storm at display refresh).
    if (previousVisibility == Visibility::Collapsed) {
        m_content->SetVisibilityForMeasureProbe(Visibility::Visible);
    }

    const Size bodySize = m_content->Measure(Size(innerWidth, (std::numeric_limits<float>::max)()));

    if (previousVisibility == Visibility::Collapsed) {
        m_content->SetVisibilityForMeasureProbe(previousVisibility);
    }

    return (std::max)(0.0f, bodySize.height + kBodyPadding * 2.0f);
}

float Expander::GetHeaderTextRight() const {
    return GetChevronRect().x - 8.0f;
}

float Expander::GetExpandProgress() const {
    return Clamp01(m_expandAnim.Current());
}

float Expander::GetVisibleBodyHeight() const {
    return m_measuredBodyHeight * GetExpandProgress();
}

Rect Expander::GetHeaderRect() const {
    if (m_bounds.IsEmpty()) {
        return Rect();
    }

    if (m_expandDirection == ExpandDirection::Up) {
        return Rect(
            m_bounds.x,
            m_bounds.y + m_bounds.height - m_headerHeight,
            m_bounds.width,
            m_headerHeight);
    }

    return Rect(m_bounds.x, m_bounds.y, m_bounds.width, m_headerHeight);
}

Rect Expander::GetBodyRect() const {
    if (m_bounds.IsEmpty() || m_measuredBodyHeight <= 0.0f) {
        return Rect();
    }

    if (m_expandDirection == ExpandDirection::Up) {
        return Rect(m_bounds.x, m_bounds.y, m_bounds.width, m_measuredBodyHeight);
    }

    return Rect(
        m_bounds.x,
        m_bounds.y + m_headerHeight,
        m_bounds.width,
        m_measuredBodyHeight);
}

Rect Expander::GetBodyClipRect() const {
    if (m_bounds.IsEmpty()) {
        return Rect();
    }

    const float visibleHeight = GetVisibleBodyHeight();
    if (visibleHeight <= 0.01f) {
        return Rect();
    }

    if (m_expandDirection == ExpandDirection::Up) {
        return Rect(
            m_bounds.x,
            m_bounds.y + m_measuredBodyHeight - visibleHeight,
            m_bounds.width,
            visibleHeight);
    }

    return Rect(
        m_bounds.x,
        m_bounds.y + m_headerHeight,
        m_bounds.width,
        visibleHeight);
}

Rect Expander::GetChevronRect() const {
    const Rect header = GetHeaderRect();
    return Rect(
        header.x + header.width - kHeaderHorizontalPadding - kChevronHitSize,
        header.y + (header.height - kChevronHitSize) * 0.5f,
        kChevronHitSize,
        kChevronHitSize);
}

bool Expander::IsPointInHeader(Point pt) const {
    return GetHeaderRect().Contains(pt.x, pt.y);
}

void Expander::UpdateContentVisibility() {
    if (!m_content) {
        ProgressBarDiag::Log("[EXP] UpdateContentVisibility this=%p header=%s no-content",
            (void*)this, m_header.c_str());
        return;
    }

    const bool keepVisible = m_isExpanded
        || m_expandAnim.Target() > 0.01f
        || m_expandAnim.Current() > 0.01f
        || m_expandAnim.IsAnimating(0.01f);
    m_content->SetVisibility(keepVisible ? Visibility::Visible : Visibility::Collapsed);
    ProgressBarDiag::Log("[EXP] UpdateContentVisibility this=%p header=%s keepVisible=%d target=%.3f current=%.3f",
        (void*)this, m_header.c_str(), keepVisible ? 1 : 0, m_expandAnim.Target(), m_expandAnim.Current());
}

void Expander::InvalidateExpanderLayout() {
    m_measureDirty = true;
    m_arrangeDirty = true;
    if (UIElement* parent = GetParent()) {
        parent->InvalidateMeasure();
    }
    for (UIElement* walk = GetParent(); walk; walk = walk->GetParent()) {
        if (auto* scroll = dynamic_cast<ScrollViewer*>(walk)) {
            ProgressBarDiag::Log("[EXP] InvalidateContentLayout via ScrollViewer this=%p header=%s scroll=%p",
                (void*)this, m_header.c_str(), (void*)scroll);
            scroll->InvalidateContentLayout();
            break;
        }
    }
    MarkRenderContentDirty();
    if (Window* window = Window::Current()) {
        ProgressBarDiag::Log("[EXP] Force Window::Relayout this=%p header=%s window=%p",
            (void*)this, m_header.c_str(), (void*)window);
        window->Relayout();
    }
}

void Expander::InvalidateExpanderVisual() {
    MarkRenderRectDirty(m_bounds.Inflate(2.0f));
}

Size Expander::Measure(Size availableSize) {
    const Thickness margin = GetMargin();
    float availableWidth = (std::max)(0.0f, availableSize.width - margin.left - margin.right);

    if (GetWidth() >= 0.0f) {
        availableWidth = GetWidth();
    }

    m_headerHeight = MeasureHeaderHeight(availableWidth);
    m_measuredBodyHeight = MeasureBodyHeight(availableWidth);

    const float width = GetWidth() >= 0.0f ? GetWidth() : availableWidth;
    const float animatedBody = m_measuredBodyHeight * GetExpandProgress();
    const float height = GetHeight() >= 0.0f
        ? GetHeight()
        : (m_headerHeight + animatedBody);

    m_desiredSize = Size(
        width + margin.left + margin.right,
        height + margin.top + margin.bottom);
    m_lastMeasureAvailable = availableSize;
    m_measureDirty = false;
    ProgressBarDiag::Log("[EXP] Measure this=%p header=%s isExpanded=%d progress=%.3f availW=%.1f body=%.1f desired=%.1f x %.1f",
        (void*)this, m_header.c_str(), m_isExpanded ? 1 : 0, GetExpandProgress(), availableWidth, m_measuredBodyHeight,
        m_desiredSize.width, m_desiredSize.height);
    return m_desiredSize;
}

void Expander::Arrange(Rect finalRect) {
    SetBounds(finalRect);

    const float width = finalRect.width;
    m_headerHeight = MeasureHeaderHeight(width);

    if (!m_content) {
        m_arrangeDirty = false;
        return;
    }

    if (m_measuredBodyHeight <= 0.0f || (!m_isExpanded && m_expandAnim.Target() <= 0.01f && m_expandAnim.Current() <= 0.01f)) {
        m_content->Arrange(Rect(finalRect.x, finalRect.y, width, 0.0f));
        m_arrangeDirty = false;
        return;
    }

    const float bodyInnerHeight = (std::max)(0.0f, m_measuredBodyHeight - kBodyPadding * 2.0f);
    Rect bodyRect = GetBodyRect();
    Rect contentRect(
        bodyRect.x + kBodyPadding,
        bodyRect.y + kBodyPadding,
        (std::max)(0.0f, bodyRect.width - kBodyPadding * 2.0f),
        bodyInnerHeight);

    if (m_expandDirection == ExpandDirection::Up) {
        contentRect.y = bodyRect.y + bodyRect.height - kBodyPadding - bodyInnerHeight;
    }

    m_content->Arrange(contentRect);
    m_arrangeDirty = false;
}

UIElement* Expander::HitTest(float x, float y) {
    if (GetVisibility() != Visibility::Visible || !m_bounds.Contains(x, y)) {
        return nullptr;
    }

    if (m_content && m_content->GetVisibility() == Visibility::Visible) {
        const Rect clipRect = GetBodyClipRect();
        if (!clipRect.IsEmpty() && clipRect.Contains(x, y)) {
            if (UIElement* hit = m_content->HitTest(x, y)) {
                return hit;
            }
        }
    }

    return IsPointInHeader(Point(x, y)) ? this : this;
}

void Expander::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);
    const bool hovered = IsPointInHeader(pt);
    if (hovered != m_headerHovered) {
        m_headerHovered = hovered;
        InvalidateExpanderVisual();
    }
}

void Expander::OnMouseLeave() {
    Control::OnMouseLeave();
    if (m_headerHovered || m_headerPressed) {
        m_headerHovered = false;
        m_headerPressed = false;
        InvalidateExpanderVisual();
    }
}

void Expander::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);
    if (!IsEnabled() || !IsPointInHeader(pt)) {
        ProgressBarDiag::Log("[EXP] OnMouseDown ignore this=%p header=%s enabled=%d inHeader=%d pt=(%.1f,%.1f)",
            (void*)this, m_header.c_str(), IsEnabled() ? 1 : 0, IsPointInHeader(pt) ? 1 : 0, pt.x, pt.y);
        return;
    }
    m_headerPressed = true;
    ProgressBarDiag::Log("[EXP] OnMouseDown this=%p header=%s pt=(%.1f,%.1f)",
        (void*)this, m_header.c_str(), pt.x, pt.y);
    InvalidateExpanderVisual();
}

void Expander::OnMouseUp(Point pt) {
    Control::OnMouseUp(pt);
    const bool shouldToggle = IsEnabled() && m_headerPressed && IsPointInHeader(pt);
    ProgressBarDiag::Log("[EXP] OnMouseUp this=%p header=%s shouldToggle=%d pressed=%d inHeader=%d pt=(%.1f,%.1f)",
        (void*)this, m_header.c_str(), shouldToggle ? 1 : 0, m_headerPressed ? 1 : 0,
        IsPointInHeader(pt) ? 1 : 0, pt.x, pt.y);
    m_headerPressed = false;
    if (shouldToggle) {
        SetIsExpanded(!m_isExpanded);
    } else {
        InvalidateExpanderVisual();
    }
}

void Expander::OnKeyDown(int vkCode) {
    Control::OnKeyDown(vkCode);
    if (!IsEnabled()) {
        return;
    }

    switch (vkCode) {
    case VK_SPACE:
    case VK_RETURN:
        SetIsExpanded(!m_isExpanded);
        break;
    case VK_LEFT:
    case VK_UP:
        if (m_isExpanded) {
            SetIsExpanded(false);
        }
        break;
    case VK_RIGHT:
    case VK_DOWN:
        if (!m_isExpanded) {
            SetIsExpanded(true);
        }
        break;
    }
}

bool Expander::OnAnimationTick() {
    const bool baseAnimating = Control::OnAnimationTick();
    const float before = m_expandAnim.Current();

    if (!UIElement::AreAnimationsEnabled()) {
        m_expandAnim.Reset(m_isExpanded ? 1.0f : 0.0f);
        UpdateContentVisibility();
        return baseAnimating;
    }

    // Keep target in sync with expanded flag (in case of interrupted toggles).
    m_expandAnim.SetTarget(m_isExpanded ? 1.0f : 0.0f);
    const bool stillAnimating = m_expandAnim.Tick(
        UIElement::GetAnimationDeltaSeconds(),
        AnimationSpec{ 0.22f, 0.01f, 0.28f });
    const bool changed = std::abs(m_expandAnim.Current() - before) > 0.0005f;

    if (changed) {
        UpdateContentVisibility();
        // Relayout so parent height follows the expand progress (clip alone is not enough).
        InvalidateExpanderLayout();
    }

    if (stillAnimating) {
        RequestAnimationTicks();
    } else {
        UpdateContentVisibility();
    }

    return baseAnimating || stillAnimating || changed;
}

bool Expander::HasSelfAnimation() const {
    return Control::HasSelfAnimation() || m_expandAnim.IsAnimating(0.01f);
}

void Expander::OnThemeChanged() {
    Control::OnThemeChanged();
    InvalidateExpanderVisual();
}

void Expander::OnRender(GraphicsContext& ctx) {
    const Rect headerRect = GetHeaderRect();
    const Rect bodyRect = GetBodyRect();

    D2D1_COLOR_F cardBg = ResolveThemeColor(GetBackgroundToken(), ThemeTokenId::CardBackground);
    D2D1_COLOR_F hoverBg = ResolveThemeColor(GetHoverBackgroundToken(), ThemeTokenId::HoverBackground);
    D2D1_COLOR_F pressedBg = ResolveThemeColor(GetPressedBackgroundToken(), ThemeTokenId::PressedBackground);
    D2D1_COLOR_F border = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);
    D2D1_COLOR_F textPrimary = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextPrimary);
    D2D1_COLOR_F textSecondary = ThemeManager::Instance().GetColor(ThemeTokenId::TextSecondary);

    const float radius = GetCornerRadius() > 0.0f ? GetCornerRadius() : kCornerRadius;
    const float borderThickness = GetBorderThickness() > 0.0f ? GetBorderThickness() : 1.0f;

    D2D1_COLOR_F fill = cardBg;
    if (m_headerPressed) {
        fill = pressedBg;
    } else if (m_headerHovered) {
        fill = MixColor(cardBg, hoverBg, 0.75f);
    }

    ctx.FillRoundedRect(m_bounds, radius, cardBg);
    if (m_headerHovered || m_headerPressed) {
        ctx.FillRoundedRect(headerRect, radius, fill);
        if (m_expandDirection == ExpandDirection::Down && GetExpandProgress() > 0.01f) {
            ctx.FillRect(Rect(headerRect.x, headerRect.y + headerRect.height - radius, headerRect.width, radius), fill);
        } else if (m_expandDirection == ExpandDirection::Up && GetExpandProgress() > 0.01f) {
            ctx.FillRect(Rect(headerRect.x, headerRect.y, headerRect.width, radius), fill);
        }
    }

    ctx.DrawRoundedRect(m_bounds, radius, border, borderThickness);

    const float textLeft = headerRect.x + kHeaderHorizontalPadding;
    const float textRight = GetHeaderTextRight();
    const float textWidth = (std::max)(0.0f, textRight - textLeft);

    if (m_subtitle.empty()) {
        Rect titleRect(textLeft, headerRect.y, textWidth, headerRect.height);
        ctx.DrawText(
            m_header,
            titleRect,
            textPrimary,
            GetFontFamily(),
            GetFontSize(),
            DWRITE_TEXT_ALIGNMENT_LEADING,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
            DWRITE_FONT_WEIGHT_SEMI_BOLD);
    } else {
        Rect titleRect(textLeft, headerRect.y + 10.0f, textWidth, 18.0f);
        Rect subtitleRect(textLeft, headerRect.y + 30.0f, textWidth, 16.0f);
        ctx.DrawText(
            m_header,
            titleRect,
            textPrimary,
            GetFontFamily(),
            GetFontSize(),
            DWRITE_TEXT_ALIGNMENT_LEADING,
            DWRITE_PARAGRAPH_ALIGNMENT_NEAR,
            DWRITE_FONT_WEIGHT_SEMI_BOLD);
        ctx.DrawText(
            m_subtitle,
            subtitleRect,
            textSecondary,
            GetFontFamily(),
            12.0f,
            DWRITE_TEXT_ALIGNMENT_LEADING,
            DWRITE_PARAGRAPH_ALIGNMENT_NEAR,
            DWRITE_FONT_WEIGHT_NORMAL);
    }

    const Rect chevronHit = GetChevronRect();
    const Rect chevronGlyph(
        chevronHit.x + (chevronHit.width - kChevronGlyphSize) * 0.5f,
        chevronHit.y + (chevronHit.height - kChevronGlyphSize) * 0.5f,
        kChevronGlyphSize,
        kChevronGlyphSize);
    ctx.DrawChevron(
        chevronGlyph,
        textSecondary,
        GetExpandProgress() >= 0.5f ? GraphicsContext::ChevronDirection::Up : GraphicsContext::ChevronDirection::Down,
        1.8f);

    if (GetExpandProgress() > 0.01f && bodyRect.height > 0.0f) {
        D2D1_COLOR_F divider = border;
        divider.a *= Clamp01(GetExpandProgress());
        if (m_expandDirection == ExpandDirection::Down) {
            const float y = headerRect.y + headerRect.height;
            ctx.DrawLine(Point(m_bounds.x + 1.0f, y), Point(m_bounds.x + m_bounds.width - 1.0f, y), divider, 1.0f);
        } else {
            const float y = headerRect.y;
            ctx.DrawLine(Point(m_bounds.x + 1.0f, y), Point(m_bounds.x + m_bounds.width - 1.0f, y), divider, 1.0f);
        }
    }
}

void Expander::Render(GraphicsContext& ctx) {
    if (GetVisibility() != Visibility::Visible || GetOpacity() <= 0.001f) {
        return;
    }

    const float radius = GetCornerRadius() > 0.0f ? GetCornerRadius() : kCornerRadius;
    if (radius > 0.01f) {
        ctx.PushRoundedClip(m_bounds, radius);
    } else {
        ctx.PushClip(m_bounds);
    }

    OnRender(ctx);

    if (m_content && m_content->GetVisibility() == Visibility::Visible) {
        const Rect clipRect = GetBodyClipRect();
        if (!clipRect.IsEmpty()) {
            ctx.PushClip(clipRect);
            m_content->Render(ctx);
            ctx.PopClip();
        }
    }

    ctx.PopClip();
}

} // namespace CUI
