#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "BreadcrumbBar.h"
#include "ContextMenu.h"
#include "../style/ThemeManager.h"
#include <algorithm>

namespace CUI {

namespace {
constexpr float kPadX = 4.0f;
constexpr float kGap = 2.0f;
constexpr const char* kSep = ">";
constexpr const char* kEllipsis = "...";
} // namespace

BreadcrumbBar::BreadcrumbBar() {
    m_pathNodes = { "Home", "Controls", "BreadcrumbBar" };
    SetBackgroundToken(ThemeTokenId::PaneBackground);
    SetBorderToken(ThemeTokenId::CardBorder);
    SetColorToken(ThemeTokenId::TextSecondary);
    SetActiveColorToken(ThemeTokenId::TextPrimary);
    SetBackground(ThemeManager::Instance().GetColor(ThemeTokenId::PaneBackground));
    SetBorderBrush(ThemeManager::Instance().GetColor(ThemeTokenId::CardBorder));
    SetBorderThickness(1.0f);
    SetColor(ThemeManager::Instance().GetColor(ThemeTokenId::TextSecondary));
    SetCornerRadius(0.0f);
    SetFontFamily("微软雅黑");
    SetFontSize(12.0f);
    SetWidth(-1.0f);
    SetHeight(34.0f);
}

Size BreadcrumbBar::Measure(Size availableSize) {
    float expW = GetWidth();
    float expH = GetHeight();
    if (expW < 0.0f) {
        expW = (availableSize.width > 0.0f && availableSize.width < 1.0e6f)
                   ? availableSize.width
                   : 320.0f;
    }
    if (expH < 0.0f) expH = 34.0f;
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

void BreadcrumbBar::SetPath(const std::vector<std::string>& pathNodes) {
    m_pathNodes = pathNodes;
    DismissOverflowMenu();
    m_slots.clear();
    MarkRenderContentDirty();
}

void BreadcrumbBar::PushNode(const std::string& node) {
    m_pathNodes.push_back(node);
    DismissOverflowMenu();
    m_slots.clear();
    MarkRenderContentDirty();
}

void BreadcrumbBar::PopNode() {
    if (m_pathNodes.empty()) {
        return;
    }
    m_pathNodes.pop_back();
    DismissOverflowMenu();
    m_slots.clear();
    MarkRenderContentDirty();
}

void BreadcrumbBar::DismissOverflowMenu() {
    if (m_overflowMenu && m_overflowMenu->IsOpen()) {
        m_overflowMenu->Hide();
    }
}

bool BreadcrumbBar::IsOverflowMenuOpen() const {
    return m_overflowMenu && m_overflowMenu->IsOpen();
}

Rect BreadcrumbBar::GetOverflowMenuClientBounds() const {
    if (!m_overflowMenu || !m_overflowMenu->IsOpen() || m_overflowMenu->IsExternallyHosted()) {
        return Rect();
    }
    return m_overflowMenu->GetTotalBounds();
}

float BreadcrumbBar::MeasureNodeWidth(GraphicsContext& ctx, size_t index) const {
    if (index >= m_pathNodes.size()) {
        return 0.0f;
    }
    const bool isLast = (index + 1 == m_pathNodes.size());
    const DWRITE_FONT_WEIGHT weight = isLast ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
    return ctx.MeasureText(m_pathNodes[index], GetFontFamily(), GetFontSize(), weight).width;
}

float BreadcrumbBar::MeasureSepWidth(GraphicsContext& ctx) const {
    return ctx.MeasureText(kSep, GetFontFamily(), GetFontSize(), DWRITE_FONT_WEIGHT_NORMAL).width;
}

float BreadcrumbBar::MeasureEllipsisWidth(GraphicsContext& ctx) const {
    return ctx.MeasureText(kEllipsis, GetFontFamily(), GetFontSize(), DWRITE_FONT_WEIGHT_NORMAL).width;
}

void BreadcrumbBar::RebuildVisualSlots(GraphicsContext& ctx) {
    m_slots.clear();
    if (m_pathNodes.empty() || m_bounds.width <= 0.0f) {
        return;
    }

    const size_t n = m_pathNodes.size();
    std::vector<float> nodeW(n);
    for (size_t i = 0; i < n; ++i) {
        nodeW[i] = MeasureNodeWidth(ctx, i);
    }
    const float sepW = MeasureSepWidth(ctx);
    const float ellipsisW = MeasureEllipsisWidth(ctx);
    const float available = (std::max)(0.0f, m_bounds.width - kPadX * 2.0f);
    const float join = kGap + sepW + kGap; // gap + separator + gap between crumbs

    auto widthOfRange = [&](size_t first, size_t lastIncl) -> float {
        if (n == 0 || first > lastIncl || lastIncl >= n) {
            return 0.0f;
        }
        float w = 0.0f;
        for (size_t i = first; i <= lastIncl; ++i) {
            if (i > first) {
                w += join;
            }
            w += nodeW[i];
        }
        return w;
    };

    // paintIndices: path index, or SIZE_MAX for the "..." overflow button.
    std::vector<size_t> paintIndices;
    std::vector<int> collapsed;

    if (widthOfRange(0, n - 1) <= available + 0.5f) {
        for (size_t i = 0; i < n; ++i) {
            paintIndices.push_back(i);
        }
    } else {
        // Explorer-style: keep trailing crumbs; collapse the front into "...".
        // Prefer "root > ... > trail" when the root still fits.
        size_t trailStart = n; // first visible trailing index
        bool keepRoot = false;

        const float rootPrefix = nodeW[0] + join + ellipsisW + join;
        if (n >= 3 && rootPrefix < available) {
            float budget = available - rootPrefix;
            trailStart = n;
            for (size_t i = n; i-- > 1;) {
                const float piece = nodeW[i] + ((trailStart < n) ? join : 0.0f);
                if (piece > budget + 0.5f) {
                    break;
                }
                budget -= piece;
                trailStart = i;
            }
            if (trailStart > 1 && trailStart < n) {
                keepRoot = true;
            }
        }

        if (!keepRoot) {
            // "... > trail" only (root also collapsed into the menu).
            float budget = available - ellipsisW;
            trailStart = n;
            for (size_t i = n; i-- > 0;) {
                const float piece = join + nodeW[i];
                if (piece > budget + 0.5f) {
                    break;
                }
                budget -= piece;
                trailStart = i;
            }
            if (trailStart == 0) {
                // Path still cannot fit with ellipsis — show last crumb only.
                paintIndices.push_back(n - 1);
            }
        }

        if (paintIndices.empty()) {
            if (trailStart >= n) {
                trailStart = n - 1; // always keep the leaf crumb visible
            }
            if (keepRoot) {
                paintIndices.push_back(0);
                for (size_t i = 1; i < trailStart; ++i) {
                    collapsed.push_back(static_cast<int>(i));
                }
            } else {
                for (size_t i = 0; i < trailStart; ++i) {
                    collapsed.push_back(static_cast<int>(i));
                }
            }
            if (!collapsed.empty()) {
                paintIndices.push_back(SIZE_MAX);
            }
            for (size_t i = trailStart; i < n; ++i) {
                paintIndices.push_back(i);
            }
            if (paintIndices.empty()) {
                paintIndices.push_back(n - 1);
                collapsed.clear();
            }
        }
    }

    float currX = m_bounds.x + kPadX;
    for (size_t s = 0; s < paintIndices.size(); ++s) {
        const size_t idx = paintIndices[s];
        if (idx == SIZE_MAX) {
            VisualSlot ell;
            ell.kind = SlotKind::Ellipsis;
            ell.collapsedIndices = collapsed;
            ell.bounds = Rect(currX, m_bounds.y, ellipsisW, m_bounds.height);
            m_slots.push_back(ell);
            currX += ellipsisW + kGap;
        } else {
            VisualSlot item;
            item.kind = SlotKind::Item;
            item.pathIndex = static_cast<int>(idx);
            item.bounds = Rect(currX, m_bounds.y, nodeW[idx], m_bounds.height);
            m_slots.push_back(item);
            currX += nodeW[idx] + kGap;
        }

        if (s + 1 < paintIndices.size()) {
            VisualSlot sep;
            sep.kind = SlotKind::Separator;
            sep.bounds = Rect(currX, m_bounds.y, sepW, m_bounds.height);
            m_slots.push_back(sep);
            currX += sepW + kGap;
        }
    }
}

void BreadcrumbBar::ShowOverflowMenu(const VisualSlot& ellipsisSlot) {
    if (ellipsisSlot.collapsedIndices.empty()) {
        return;
    }

    if (!m_overflowMenu) {
        m_overflowMenu = std::make_shared<ContextMenu>();
    }
    m_overflowMenu->ClearItems();

    for (int index : ellipsisSlot.collapsedIndices) {
        if (index < 0 || index >= static_cast<int>(m_pathNodes.size())) {
            continue;
        }
        const std::string label = m_pathNodes[static_cast<size_t>(index)];
        m_overflowMenu->AddItem(label, [this, index, label]() {
            m_onItemClickedEvent.Invoke(this, index, label);
        });
    }

    const float x = ellipsisSlot.bounds.x;
    const float y = ellipsisSlot.bounds.y + ellipsisSlot.bounds.height;
    m_overflowMenu->ShowAt(x, y);
}

void BreadcrumbBar::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);

    for (const VisualSlot& slot : m_slots) {
        if (!slot.bounds.Contains(pt.x, pt.y)) {
            continue;
        }
        if (slot.kind == SlotKind::Item && slot.pathIndex >= 0 &&
            slot.pathIndex < static_cast<int>(m_pathNodes.size())) {
            m_onItemClickedEvent.Invoke(
                this,
                slot.pathIndex,
                m_pathNodes[static_cast<size_t>(slot.pathIndex)]);
            return;
        }
        if (slot.kind == SlotKind::Ellipsis) {
            ShowOverflowMenu(slot);
            return;
        }
    }
}

void BreadcrumbBar::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    // Bottom hairline under the address/breadcrumb strip (standalone bar only).
    if (GetBorderThickness() > 0.0f) {
        D2D1_COLOR_F border = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);
        ctx.DrawLine(
            Point(m_bounds.x, m_bounds.y + m_bounds.height - 1.0f),
            Point(m_bounds.x + m_bounds.width, m_bounds.y + m_bounds.height - 1.0f),
            border,
            1.0f);
    }

    RebuildVisualSlots(ctx);

    const std::string& font = GetFontFamily();
    const float fontH = GetFontSize();
    D2D1_COLOR_F textColor = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextSecondary);
    D2D1_COLOR_F activeColor = ResolveThemeColor(GetActiveColorToken(), ThemeTokenId::TextPrimary);
    D2D1_COLOR_F sepColor = ThemeManager::Instance().GetTokens().textMuted;

    for (const VisualSlot& slot : m_slots) {
        if (slot.kind == SlotKind::Separator) {
            ctx.DrawText(
                kSep,
                slot.bounds,
                sepColor,
                font,
                fontH,
                DWRITE_TEXT_ALIGNMENT_LEADING,
                DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            continue;
        }
        if (slot.kind == SlotKind::Ellipsis) {
            ctx.DrawText(
                kEllipsis,
                slot.bounds,
                textColor,
                font,
                fontH,
                DWRITE_TEXT_ALIGNMENT_LEADING,
                DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            continue;
        }
        if (slot.pathIndex < 0 || slot.pathIndex >= static_cast<int>(m_pathNodes.size())) {
            continue;
        }
        const bool isLast = (static_cast<size_t>(slot.pathIndex) + 1 == m_pathNodes.size());
        ctx.DrawText(
            m_pathNodes[static_cast<size_t>(slot.pathIndex)],
            slot.bounds,
            isLast ? activeColor : textColor,
            font,
            fontH,
            DWRITE_TEXT_ALIGNMENT_LEADING,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
            isLast ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
            true);
    }
}

} // namespace CUI
