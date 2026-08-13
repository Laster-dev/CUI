#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "StatusBar.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cmath>

namespace CUI {

namespace {
constexpr float kBarH = 24.0f;
constexpr float kPadX = 10.0f;
constexpr float kItemGap = 10.0f;
constexpr float kSepW = 1.0f;
constexpr float kProgressTrackH = 4.0f;
constexpr float kProgressMinW = 56.0f;
} // namespace

StatusBar::StatusBar() {
    SetBackgroundToken(ThemeTokenId::PaneBackground);
    SetBorderToken(ThemeTokenId::CardBorder);
    SetColorToken(ThemeTokenId::TextSecondary);
    SetBackground(ThemeManager::Instance().GetColor(ThemeTokenId::PaneBackground));
    SetBorderBrush(ThemeManager::Instance().GetColor(ThemeTokenId::CardBorder));
    SetBorderThickness(1.0f);
    SetColor(ThemeManager::Instance().GetColor(ThemeTokenId::TextSecondary));
    SetFontFamily("Segoe UI");
    SetFontSize(11.0f);
    SetHeight(kBarH);
    SetWidth(-1.0f);
    SetCornerRadius(0.0f);
}

Size StatusBar::Measure(Size availableSize) {
    float w = GetWidth();
    if (w < 0.0f) {
        w = (availableSize.width > 0.0f && availableSize.width < 1.0e6f)
                ? availableSize.width
                : 400.0f;
    }
    float h = GetHeight();
    if (h < 0.0f) {
        h = kBarH;
    }
    m_desiredSize = Size(w, h);
    return m_desiredSize;
}

StatusBarItem* StatusBar::FindItem(int id) {
    return MutableFind(id);
}

const StatusBarItem* StatusBar::FindItem(int id) const {
    for (const auto& item : m_items) {
        if (item.id == id) {
            return &item;
        }
    }
    return nullptr;
}

StatusBarItem* StatusBar::MutableFind(int id) {
    for (auto& item : m_items) {
        if (item.id == id) {
            return &item;
        }
    }
    return nullptr;
}

int StatusBar::AddTextItem(const std::string& text, StatusBarItemAlignment align, float fixedWidth) {
    StatusBarItem item;
    item.id = m_nextId++;
    item.kind = StatusBarItemKind::Text;
    item.align = align;
    item.text = text;
    item.fixedWidth = fixedWidth;
    m_items.push_back(item);
    MarkRenderContentDirty();
    InvalidateMeasure();
    return item.id;
}

int StatusBar::AddProgressItem(const std::string& text, StatusBarItemAlignment align, float fixedWidth) {
    StatusBarItem item;
    item.id = m_nextId++;
    item.kind = StatusBarItemKind::Progress;
    item.align = align;
    item.text = text;
    item.progress = 0.0f;
    item.fixedWidth = fixedWidth > 0.0f ? fixedWidth : 120.0f;
    m_items.push_back(item);
    MarkRenderContentDirty();
    InvalidateMeasure();
    return item.id;
}

int StatusBar::AddSeparator(StatusBarItemAlignment align) {
    StatusBarItem item;
    item.id = m_nextId++;
    item.kind = StatusBarItemKind::Separator;
    item.align = align;
    item.fixedWidth = kSepW;
    m_items.push_back(item);
    MarkRenderContentDirty();
    return item.id;
}

void StatusBar::SetItemText(int id, const std::string& text) {
    if (StatusBarItem* item = MutableFind(id)) {
        if (item->text == text) {
            return;
        }
        item->text = text;
        MarkRenderContentDirty();
    }
}

void StatusBar::SetItemIcon(int id, const std::string& icon) {
    if (StatusBarItem* item = MutableFind(id)) {
        if (item->icon == icon) {
            return;
        }
        item->icon = icon;
        MarkRenderContentDirty();
    }
}

void StatusBar::SetItemProgress(int id, float progress01) {
    if (StatusBarItem* item = MutableFind(id)) {
        const float clamped = std::clamp(progress01, 0.0f, 1.0f);
        if (item->kind == StatusBarItemKind::Progress && item->progress == clamped) {
            return;
        }
        item->kind = StatusBarItemKind::Progress;
        item->progress = clamped;
        MarkRenderContentDirty();
    }
}

void StatusBar::SetItemVisible(int id, bool visible) {
    if (StatusBarItem* item = MutableFind(id)) {
        if (item->visible == visible) {
            return;
        }
        item->visible = visible;
        MarkRenderContentDirty();
    }
}

void StatusBar::SetItemFixedWidth(int id, float width) {
    if (StatusBarItem* item = MutableFind(id)) {
        if (item->fixedWidth == width) {
            return;
        }
        item->fixedWidth = width;
        MarkRenderContentDirty();
    }
}

void StatusBar::ClearItems() {
    m_items.clear();
    MarkRenderContentDirty();
}

float StatusBar::MeasureItemWidth(GraphicsContext& ctx, const StatusBarItem& item) const {
    if (item.fixedWidth > 0.0f) {
        return item.fixedWidth;
    }
    if (item.kind == StatusBarItemKind::Separator) {
        return kSepW;
    }

    float w = 0.0f;
    if (!item.icon.empty()) {
        if (GraphicsContext::LooksLikeSvg(item.icon)) {
            w += GetFontSize() + 6.0f;
        } else {
            w += ctx.MeasureText(item.icon, GetFontFamily(), GetFontSize(), DWRITE_FONT_WEIGHT_NORMAL).width + 4.0f;
        }
    }
    if (!item.text.empty()) {
        w += ctx.MeasureText(item.text, GetFontFamily(), GetFontSize(), DWRITE_FONT_WEIGHT_NORMAL).width;
    }
    if (item.kind == StatusBarItemKind::Progress) {
        w += 8.0f + kProgressMinW;
    }
    return (std::max)(12.0f, w);
}

void StatusBar::LayoutItems(GraphicsContext& ctx, std::vector<LaidOutItem>& out) const {
    out.clear();
    if (m_bounds.width <= 1.0f) {
        return;
    }

    struct Measured {
        const StatusBarItem* item = nullptr;
        float width = 0.0f;
    };
    std::vector<Measured> left;
    std::vector<Measured> fill;
    std::vector<Measured> right;
    float leftW = 0.0f;
    float rightW = 0.0f;
    float fillWidths = 0.0f;

    auto push = [&](Measured m) {
        if (!m.item || !m.item->visible) {
            return;
        }
        switch (m.item->align) {
        case StatusBarItemAlignment::Left:
            if (!left.empty()) leftW += kItemGap;
            leftW += m.width;
            left.push_back(m);
            break;
        case StatusBarItemAlignment::Right:
            if (!right.empty()) rightW += kItemGap;
            rightW += m.width;
            right.push_back(m);
            break;
        case StatusBarItemAlignment::Fill:
            fillWidths += m.width;
            fill.push_back(m);
            break;
        }
    };

    for (const auto& item : m_items) {
        Measured m;
        m.item = &item;
        m.width = MeasureItemWidth(ctx, item);
        push(m);
    }

    const float inner = (std::max)(0.0f, m_bounds.width - kPadX * 2.0f);
    (void)inner;
    float x = m_bounds.x + kPadX;
    const float y = m_bounds.y;
    const float h = m_bounds.height;

    for (size_t i = 0; i < left.size(); ++i) {
        if (i > 0) x += kItemGap;
        out.push_back({ left[i].item->id, Rect(x, y, left[i].width, h) });
        x += left[i].width;
    }

    const float rightStart = m_bounds.x + m_bounds.width - kPadX - rightW;
    float fillAvail = rightStart - x;
    if (!fill.empty()) {
        fillAvail -= kItemGap;
    }
    fillAvail = (std::max)(0.0f, fillAvail);

    if (!fill.empty()) {
        x += kItemGap;
        const float totalGaps = kItemGap * static_cast<float>((std::max)(size_t{ 0 }, fill.size() - 1));
        float remain = (std::max)(0.0f, fillAvail - totalGaps);
        for (size_t i = 0; i < fill.size(); ++i) {
            if (i > 0) x += kItemGap;
            float share = fill[i].width;
            if (fillWidths > 0.001f) {
                share = remain * (fill[i].width / fillWidths);
            } else if (!fill.empty()) {
                share = remain / static_cast<float>(fill.size());
            }
            share = (std::max)(fill[i].width, share);
            const float maxShare = (std::max)(0.0f, rightStart - kItemGap - x);
            share = (std::min)(share, maxShare);
            out.push_back({ fill[i].item->id, Rect(x, y, share, h) });
            x += share;
        }
    }

    float rx = rightStart;
    for (size_t i = 0; i < right.size(); ++i) {
        if (i > 0) rx += kItemGap;
        // If left+fill ate space, still pack right from the right edge.
        out.push_back({ right[i].item->id, Rect(rx, y, right[i].width, h) });
        rx += right[i].width;
    }

    // If overflow: prefer keeping right items fully inside by shifting leftward as a group.
    if (!right.empty() && !out.empty()) {
        float maxRight = m_bounds.x + m_bounds.width - kPadX;
        float groupRight = 0.0f;
        for (const auto& lo : out) {
            const StatusBarItem* it = FindItem(lo.id);
            if (it && it->align == StatusBarItemAlignment::Right) {
                groupRight = (std::max)(groupRight, lo.bounds.x + lo.bounds.width);
            }
        }
        if (groupRight > maxRight + 0.5f) {
            const float shift = groupRight - maxRight;
            for (auto& lo : out) {
                const StatusBarItem* it = FindItem(lo.id);
                if (it && it->align == StatusBarItemAlignment::Right) {
                    lo.bounds.x -= shift;
                }
            }
        }
    }
}

void StatusBar::DrawItem(GraphicsContext& ctx, const StatusBarItem& item, const Rect& bounds) const {
    if (bounds.width <= 0.5f || bounds.height <= 0.5f) {
        return;
    }
    const auto& tokens = ThemeManager::Instance().GetTokens();
    D2D1_COLOR_F textColor = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextSecondary);

    if (item.kind == StatusBarItemKind::Separator) {
        const float midX = bounds.x + bounds.width * 0.5f;
        ctx.DrawLine(
            Point(midX, bounds.y + 5.0f),
            Point(midX, bounds.y + bounds.height - 5.0f),
            tokens.cardBorder,
            1.0f);
        return;
    }

    float x = bounds.x;
    if (!item.icon.empty()) {
        const float iconSize = GraphicsContext::LooksLikeSvg(item.icon)
            ? GetFontSize() + 2.0f
            : ctx.MeasureText(item.icon, GetFontFamily(), GetFontSize(), DWRITE_FONT_WEIGHT_NORMAL).width;
        const Rect iconRect(x, bounds.y + (bounds.height - iconSize) * 0.5f, iconSize, iconSize);
        ctx.DrawIcon(item.icon, iconRect, textColor, 1.0f, GetFontFamily(), GetFontSize());
        x += iconSize + 4.0f;
    }

    if (item.kind == StatusBarItemKind::Progress) {
        const float trackW = (std::max)(kProgressMinW, bounds.x + bounds.width - x - 4.0f);
        float labelW = 0.0f;
        if (!item.text.empty()) {
            labelW = ctx.MeasureText(item.text, GetFontFamily(), GetFontSize(), DWRITE_FONT_WEIGHT_NORMAL).width + 6.0f;
        }
        const float barW = (std::max)(24.0f, trackW - labelW);
        if (!item.text.empty()) {
            ctx.DrawText(
                item.text,
                Rect(x, bounds.y, labelW, bounds.height),
                textColor,
                GetFontFamily(),
                GetFontSize(),
                DWRITE_TEXT_ALIGNMENT_LEADING,
                DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
                DWRITE_FONT_WEIGHT_NORMAL,
                true);
            x += labelW;
        }
        const float trackY = bounds.y + (bounds.height - kProgressTrackH) * 0.5f;
        Rect track(x, trackY, barW, kProgressTrackH);
        D2D1_COLOR_F trackBg = tokens.cardBorder;
        trackBg.a *= 0.55f;
        ctx.FillRoundedRect(track, 2.0f, trackBg);
        if (item.progress >= 0.0f) {
            const float fillW = barW * std::clamp(item.progress, 0.0f, 1.0f);
            if (fillW > 0.5f) {
                ctx.FillRoundedRect(Rect(track.x, track.y, fillW, track.height), 2.0f, tokens.accentColor);
            }
        }
        return;
    }

    const float tw = (std::max)(0.0f, bounds.x + bounds.width - x);
    if (!item.text.empty() && tw > 1.0f) {
        ctx.DrawText(
            item.text,
            Rect(x, bounds.y, tw, bounds.height),
            textColor,
            GetFontFamily(),
            GetFontSize(),
            DWRITE_TEXT_ALIGNMENT_LEADING,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
            DWRITE_FONT_WEIGHT_NORMAL,
            true);
    }
}

void StatusBar::OnRender(GraphicsContext& ctx) {
    if (m_bounds.IsEmpty()) {
        return;
    }

    const auto& tokens = ThemeManager::Instance().GetTokens();
    D2D1_COLOR_F bg = ResolveThemeColor(GetBackgroundToken(), ThemeTokenId::PaneBackground);
    ctx.FillRect(m_bounds, bg);

    // Top hairline separator.
    if (GetBorderThickness() > 0.0f) {
        D2D1_COLOR_F border = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);
        ctx.DrawLine(
            Point(m_bounds.x, m_bounds.y + 0.5f),
            Point(m_bounds.x + m_bounds.width, m_bounds.y + 0.5f),
            border,
            1.0f);
    }

    std::vector<LaidOutItem> laid;
    LayoutItems(ctx, laid);
    for (const auto& lo : laid) {
        const StatusBarItem* item = FindItem(lo.id);
        if (!item || !item->visible) {
            continue;
        }
        DrawItem(ctx, *item, lo.bounds);
    }
}

} // namespace CUI
