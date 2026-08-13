#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "PagingControl.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cmath>
#include <windows.h>

namespace CUI {

namespace {
AnimationSpec PillSlideSpec() {
    AnimationSpec s;
    s.responseAt60Hz = 0.26f;
    s.epsilon = 0.02f;
    s.maxDurationSeconds = 0.22f;
    return s;
}

D2D1_COLOR_F WithAlpha(D2D1_COLOR_F c, float alpha) {
    c.a = alpha;
    return c;
}
} // namespace

PagingControl::PagingControl() {
    SetColorToken(ThemeTokenId::TextMuted);
    SetBackgroundToken(ThemeTokenId::Unset);
    SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
    SetPressedBackgroundToken(ThemeTokenId::PressedBackground);
    SetColor(ThemeManager::Instance().GetColor("textMuted"));
    SetFontFamily("Segoe UI");
    SetFontSize(12.0f);
    SetHeight(kHeight);
    SetWidth(-1.0f);
    RebuildPageList();
}

std::vector<PropertyMeta> PagingControl::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "currentPage", "当前页 (CurrentPage)", "分页配置", "number" });
    metas.push_back({ "totalPages", "总页数 (TotalPages)", "分页配置", "number" });
    return metas;
}

Value PagingControl::GetProperty(PropertyId id) const {
    switch (id) {
    case PropertyId::CurrentPage: return Value(m_currentPage);
    case PropertyId::TotalPages: return Value(m_totalPages);
    default: return UIElement::GetProperty(id);
    }
}

bool PagingControl::HasProperty(PropertyId id) const {
    return id == PropertyId::CurrentPage || id == PropertyId::TotalPages || UIElement::HasProperty(id);
}

void PagingControl::SetProperty(PropertyId id, const Value& val) {
    switch (id) {
    case PropertyId::CurrentPage: SetCurrentPage(val.AsInt()); return;
    case PropertyId::TotalPages: SetTotalPages(val.AsInt()); return;
    default: UIElement::SetProperty(id, val); return;
    }
}

void PagingControl::RebuildPageList() {
    m_pageItems.clear();
    const int total = (std::max)(1, m_totalPages);
    const int current = std::clamp(m_currentPage, 1, total);

    auto addPage = [&](int page) {
        PageItem item;
        item.kind = ItemKind::Page;
        item.page = page;
        m_pageItems.push_back(item);
    };
    auto addEllipsis = [&]() {
        PageItem item;
        item.kind = ItemKind::Ellipsis;
        item.page = 0;
        m_pageItems.push_back(item);
    };

    if (total <= 7) {
        for (int i = 1; i <= total; ++i) {
            addPage(i);
        }
        return;
    }

    addPage(1);
    if (current <= 4) {
        for (int i = 2; i <= 5; ++i) {
            addPage(i);
        }
        addEllipsis();
        addPage(total);
    } else if (current >= total - 3) {
        addEllipsis();
        for (int i = total - 4; i <= total; ++i) {
            addPage(i);
        }
    } else {
        addEllipsis();
        for (int i = current - 1; i <= current + 1; ++i) {
            addPage(i);
        }
        addEllipsis();
        addPage(total);
    }
}

float PagingControl::PageCellWidth(int page) const {
    GraphicsContext probe;
    const float textW = probe.MeasureText(std::to_string(page), GetFontFamily(), GetFontSize()).width;
    return (std::max)(kNavSize, textW + kPagePadH * 2.0f);
}

float PagingControl::MeasureContentWidth() const {
    float w = kNavSize + kGap;
    for (const PageItem& item : m_pageItems) {
        if (item.kind == ItemKind::Ellipsis) {
            w += 24.0f + kGap;
        } else {
            w += PageCellWidth(item.page) + kGap;
        }
    }
    w += kNavSize + kGap + 8.0f;

    GraphicsContext probe;
    const std::string info = "共 " + std::to_string((std::max)(1, m_totalPages)) + " 页";
    w += probe.MeasureText(info, GetFontFamily(), 11.0f).width + 4.0f;
    return w;
}

void PagingControl::UpdateLayout() {
    RebuildPageList();

    const float originX = m_bounds.x;
    const float originY = m_bounds.y + (m_bounds.height - kNavSize) * 0.5f;
    float x = originX;

    m_prevRect = Rect(x, originY, kNavSize, kNavSize);
    x += kNavSize + kGap;

    for (PageItem& item : m_pageItems) {
        const float cellW = (item.kind == ItemKind::Ellipsis) ? 24.0f : PageCellWidth(item.page);
        item.bounds = Rect(x, originY, cellW, kNavSize);
        x += cellW + kGap;
    }

    m_nextRect = Rect(x, originY, kNavSize, kNavSize);
    x += kNavSize + kGap + 8.0f;

    GraphicsContext probe;
    const std::string info = "共 " + std::to_string((std::max)(1, m_totalPages)) + " 页";
    const float infoW = probe.MeasureText(info, GetFontFamily(), 11.0f).width + 4.0f;
    m_infoRect = Rect(x, m_bounds.y, infoW, m_bounds.height);
    m_contentWidth = (x - originX) + infoW;
}

Rect PagingControl::SelectionPillTarget() const {
    for (const PageItem& item : m_pageItems) {
        if (item.kind == ItemKind::Page && item.page == m_currentPage) {
            return Rect(item.bounds.x + 1.0f, item.bounds.y + 1.0f,
                item.bounds.width - 2.0f, item.bounds.height - 2.0f);
        }
    }
    return Rect();
}

void PagingControl::SyncSelectionPillTarget(bool immediate) {
    const Rect target = SelectionPillTarget();
    if (target.IsEmpty()) {
        return;
    }
    if (immediate || !UIElement::AreAnimationsEnabled()) {
        m_pillX.Reset(target.x);
        m_pillW.Reset(target.width);
        return;
    }
    m_pillX.SetTarget(target.x);
    m_pillW.SetTarget(target.width);
    RequestAnimationTicks();
}

bool PagingControl::IsNavEnabled(bool prev) const {
    if (!IsEnabled()) {
        return false;
    }
    return prev ? (m_currentPage > 1) : (m_currentPage < m_totalPages);
}

PagingControl::HitTarget PagingControl::HitTestTarget(Point pt) const {
    if (m_prevRect.Contains(pt.x, pt.y)) {
        return HitTarget::Prev;
    }
    if (m_nextRect.Contains(pt.x, pt.y)) {
        return HitTarget::Next;
    }
    for (size_t i = 0; i < m_pageItems.size(); ++i) {
        if (m_pageItems[i].kind == ItemKind::Page && m_pageItems[i].bounds.Contains(pt.x, pt.y)) {
            return static_cast<HitTarget>(static_cast<int>(i));
        }
    }
    return HitTarget::None;
}

HCURSOR PagingControl::GetCursor() const {
    if (!IsEnabled() || m_hover == HitTarget::None) {
        return nullptr;
    }
    if (m_hover == HitTarget::Prev && !IsNavEnabled(true)) {
        return nullptr;
    }
    if (m_hover == HitTarget::Next && !IsNavEnabled(false)) {
        return nullptr;
    }
    return LoadCursor(nullptr, IDC_HAND);
}

void PagingControl::ActivateTarget(HitTarget target) {
    if (target == HitTarget::Prev) {
        if (IsNavEnabled(true)) {
            SetCurrentPage(m_currentPage - 1);
        }
        return;
    }
    if (target == HitTarget::Next) {
        if (IsNavEnabled(false)) {
            SetCurrentPage(m_currentPage + 1);
        }
        return;
    }
    const int index = static_cast<int>(target);
    if (index >= 0 && index < static_cast<int>(m_pageItems.size())) {
        const PageItem& item = m_pageItems[static_cast<size_t>(index)];
        if (item.kind == ItemKind::Page) {
            SetCurrentPage(item.page);
        }
    }
}

void PagingControl::MarkPagingDirty() {
    MarkRenderRectDirty(m_bounds.Inflate(2.0f));
}

Size PagingControl::Measure(Size availableSize) {
    (void)availableSize;
    RebuildPageList();
    float w = MeasureContentWidth();
    float h = kHeight;
    if (GetWidth() >= 0.0f) {
        w = GetWidth();
    }
    if (GetHeight() >= 0.0f) {
        h = GetHeight();
    }
    m_desiredSize = Size(w, h);
    m_lastMeasureAvailable = availableSize;
    m_measureDirty = false;
    return m_desiredSize;
}

void PagingControl::Arrange(Rect finalRect) {
    m_bounds = finalRect;
    UpdateLayout();
    SyncSelectionPillTarget(true);
    m_arrangeDirty = false;
}

void PagingControl::OnRender(GraphicsContext& ctx) {
    if (m_bounds.IsEmpty()) {
        return;
    }
    UpdateLayout();

    const auto& tokens = ThemeManager::Instance().GetTokens();
    const D2D1_COLOR_F hoverBg = ResolveThemeColor(GetHoverBackgroundToken(), ThemeTokenId::HoverBackground);
    const D2D1_COLOR_F pressedBg = ResolveThemeColor(GetPressedBackgroundToken(), ThemeTokenId::PressedBackground);
    const D2D1_COLOR_F accent = tokens.accentColor;
    const D2D1_COLOR_F accentFg = tokens.accentForeground;
    const D2D1_COLOR_F textSecondary = tokens.textSecondary;
    const D2D1_COLOR_F textMuted = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextMuted);
    const D2D1_COLOR_F border = tokens.cardBorder;

    const float pillX = UIElement::AreAnimationsEnabled() ? m_pillX.Current() : SelectionPillTarget().x;
    const float pillW = UIElement::AreAnimationsEnabled() ? m_pillW.Current() : SelectionPillTarget().width;
    if (pillW > 1.0f) {
        const float pillY = m_prevRect.y + 1.0f;
        const float pillH = m_prevRect.height - 2.0f;
        ctx.FillRoundedRect(Rect(pillX, pillY, pillW, pillH), kCorner - 1.0f, accent);
    }

    auto drawNav = [&](const Rect& rc, bool prev, bool enabled) {
        const HitTarget part = prev ? HitTarget::Prev : HitTarget::Next;
        const bool hot = (m_hover == part);
        const bool pressed = (m_pressed == part);
        D2D1_COLOR_F fill = tokens.inputBackground;
        if (!enabled) {
            fill = WithAlpha(tokens.inputBackground, 0.55f);
        } else if (pressed) {
            fill = pressedBg;
        } else if (hot) {
            fill = hoverBg;
        }
        ctx.FillRoundedRect(rc, kCorner, fill);
        ctx.DrawRoundedRect(rc, kCorner, WithAlpha(border, enabled ? 1.0f : 0.45f), 1.0f);

        const float glyph = 10.0f;
        Rect glyphRect(
            rc.x + (rc.width - glyph) * 0.5f,
            rc.y + (rc.height - glyph) * 0.5f,
            glyph,
            glyph);
        ctx.DrawChevron(
            glyphRect,
            WithAlpha(textSecondary, enabled ? 1.0f : 0.35f),
            prev ? GraphicsContext::ChevronDirection::Left : GraphicsContext::ChevronDirection::Right,
            1.6f);
    };

    drawNav(m_prevRect, true, IsNavEnabled(true));
    drawNav(m_nextRect, false, IsNavEnabled(false));

    for (size_t i = 0; i < m_pageItems.size(); ++i) {
        const PageItem& item = m_pageItems[i];
        const HitTarget part = static_cast<HitTarget>(static_cast<int>(i));
        const bool hot = (m_hover == part);
        const bool pressed = (m_pressed == part);
        const bool active = (item.kind == ItemKind::Page && item.page == m_currentPage);

        if (item.kind == ItemKind::Ellipsis) {
            ctx.DrawText(
                "...",
                item.bounds,
                textMuted,
                GetFontFamily(),
                12.0f,
                DWRITE_TEXT_ALIGNMENT_CENTER,
                DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            continue;
        }

        if (!active && (hot || pressed)) {
            D2D1_COLOR_F fill = pressed ? pressedBg : hoverBg;
            ctx.FillRoundedRect(
                Rect(item.bounds.x + 1.0f, item.bounds.y + 1.0f, item.bounds.width - 2.0f, item.bounds.height - 2.0f),
                kCorner - 1.0f,
                fill);
        }

        ctx.DrawText(
            std::to_string(item.page),
            item.bounds,
            active ? accentFg : textSecondary,
            GetFontFamily(),
            GetFontSize(),
            DWRITE_TEXT_ALIGNMENT_CENTER,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
            active ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL);
    }

    const std::string info = "共 " + std::to_string((std::max)(1, m_totalPages)) + " 页";
    ctx.DrawText(
        info,
        m_infoRect,
        textMuted,
        GetFontFamily(),
        11.0f,
        DWRITE_TEXT_ALIGNMENT_LEADING,
        DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

void PagingControl::OnMouseDown(Point pt) {
    if (!IsEnabled()) {
        return;
    }
    m_pressed = HitTestTarget(pt);
    MarkPagingDirty();
}

void PagingControl::OnMouseUp(Point pt) {
    const HitTarget pressed = m_pressed;
    m_pressed = HitTarget::None;
    if (pressed != HitTarget::None && HitTestTarget(pt) == pressed) {
        ActivateTarget(pressed);
    }
    MarkPagingDirty();
}

void PagingControl::OnMouseMove(Point pt) {
    if (!IsEnabled()) {
        return;
    }
    const HitTarget next = HitTestTarget(pt);
    if (next != m_hover) {
        m_hover = next;
        MarkPagingDirty();
    }
}

void PagingControl::OnMouseLeave() {
    m_hover = HitTarget::None;
    m_pressed = HitTarget::None;
    MarkPagingDirty();
}

void PagingControl::OnMouseWheel(float delta) {
    if (!IsEnabled()) {
        return;
    }
    if (delta > 0.0f) {
        if (IsNavEnabled(true)) {
            SetCurrentPage(m_currentPage - 1);
        }
    } else if (delta < 0.0f) {
        if (IsNavEnabled(false)) {
            SetCurrentPage(m_currentPage + 1);
        }
    }
}

bool PagingControl::OnKeyDown(int vkCode) {
    if (!IsEnabled()) {
        return false;
    }
    switch (vkCode) {
    case VK_LEFT:
    case VK_PRIOR:
        if (IsNavEnabled(true)) {
            SetCurrentPage(m_currentPage - 1);
        }
        return true;
    case VK_RIGHT:
    case VK_NEXT:
        if (IsNavEnabled(false)) {
            SetCurrentPage(m_currentPage + 1);
        }
        return true;
    case VK_HOME:
        SetCurrentPage(1);
        return true;
    case VK_END:
        SetCurrentPage(m_totalPages);
        return true;
    default:
        return Control::OnKeyDown(vkCode);
    }
}

bool PagingControl::OnAnimationTick() {
    const float prevX = m_pillX.Current();
    const float prevW = m_pillW.Current();
    const bool pillAnim =
        m_pillX.Tick(UIElement::GetAnimationDeltaSeconds(), PillSlideSpec())
        | m_pillW.Tick(UIElement::GetAnimationDeltaSeconds(), PillSlideSpec());
    if (pillAnim || std::abs(m_pillX.Current() - prevX) > 0.01f || std::abs(m_pillW.Current() - prevW) > 0.01f) {
        MarkPagingDirty();
    }
    if (pillAnim) {
        RequestAnimationTicks();
    }
    return pillAnim;
}

bool PagingControl::HasSelfAnimation() const {
    return m_pillX.IsAnimating(0.02f) || m_pillW.IsAnimating(0.02f);
}

void PagingControl::SetCurrentPage(int page) {
    const int total = (std::max)(1, m_totalPages);
    page = std::clamp(page, 1, total);
    if (m_currentPage == page) {
        return;
    }
    m_currentPage = page;
    NotifyFieldChanged(PropertyId::CurrentPage, Value(page));
    RebuildPageList();
    UpdateLayout();
    SyncSelectionPillTarget(false);
    m_onPageChangedEvent.Invoke(this, page);
    MarkPagingDirty();
}

void PagingControl::SetTotalPages(int total) {
    const int validTotal = (std::max)(1, total);
    if (m_totalPages == validTotal) {
        return;
    }
    m_totalPages = validTotal;
    NotifyFieldChanged(PropertyId::TotalPages, Value(validTotal));
    if (m_currentPage > validTotal) {
        SetCurrentPage(validTotal);
    } else {
        RebuildPageList();
        UpdateLayout();
        SyncSelectionPillTarget(false);
        InvalidateMeasure();
        MarkPagingDirty();
    }
}

} // namespace CUI
