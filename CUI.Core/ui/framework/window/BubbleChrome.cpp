#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "BubbleChrome.h"
#include <algorithm>
#include <cmath>

namespace CUI {

namespace {
float ClampVal(float v, float lo, float hi) {
    if (hi < lo) {
        return lo;
    }
    return std::clamp(v, lo, hi);
}

void PlaceCaret(BubbleLayout& out, const Rect& anchor, float caret, float radius) {
    const float ax = anchor.x + anchor.width * 0.5f;
    const float ay = anchor.y + anchor.height * 0.5f;
    const float inset = radius + caret + 2.0f;
    const Rect& card = out.card;

    switch (out.placement) {
    case BubblePlacement::Top: {
        const float x = ClampVal(ax, card.x + inset, card.x + card.width - inset);
        const float y = card.y + card.height;
        out.caret[0] = Point(x, y + caret);
        out.caret[1] = Point(x - caret, y - 1.0f);
        out.caret[2] = Point(x + caret, y - 1.0f);
        break;
    }
    case BubblePlacement::Left: {
        const float x = card.x + card.width;
        const float y = ClampVal(ay, card.y + inset, card.y + card.height - inset);
        out.caret[0] = Point(x + caret, y);
        out.caret[1] = Point(x - 1.0f, y - caret);
        out.caret[2] = Point(x - 1.0f, y + caret);
        break;
    }
    case BubblePlacement::Right: {
        const float x = card.x;
        const float y = ClampVal(ay, card.y + inset, card.y + card.height - inset);
        out.caret[0] = Point(x - caret, y);
        out.caret[1] = Point(x + 1.0f, y - caret);
        out.caret[2] = Point(x + 1.0f, y + caret);
        break;
    }
    case BubblePlacement::Bottom:
    default: {
        const float x = ClampVal(ax, card.x + inset, card.x + card.width - inset);
        const float y = card.y;
        out.caret[0] = Point(x, y - caret);
        out.caret[1] = Point(x - caret, y + 1.0f);
        out.caret[2] = Point(x + caret, y + 1.0f);
        break;
    }
    }

    const float minX = (std::min)({ out.caret[0].x, out.caret[1].x, out.caret[2].x });
    const float minY = (std::min)({ out.caret[0].y, out.caret[1].y, out.caret[2].y });
    const float maxX = (std::max)({ out.caret[0].x, out.caret[1].x, out.caret[2].x });
    const float maxY = (std::max)({ out.caret[0].y, out.caret[1].y, out.caret[2].y });
    out.total = card.Union(Rect(minX, minY, maxX - minX, maxY - minY));
}

float PlaceSide(
    BubbleLayout& out,
    BubblePlacement side,
    const Rect& anchor,
    Size cardSize,
    const Rect& viewport,
    float gap,
    float caret,
    float margin)
{
    const float left = viewport.x + margin;
    const float top = viewport.y + margin;
    const float right = viewport.x + viewport.width - margin;
    const float bottom = viewport.y + viewport.height - margin;
    const float maxW = (std::max)(16.0f, right - left);
    const float maxH = (std::max)(16.0f, bottom - top);
    cardSize.width = std::clamp(cardSize.width, 16.0f, maxW);
    cardSize.height = std::clamp(cardSize.height, 16.0f, maxH);

    float x = 0.0f;
    float y = 0.0f;
    switch (side) {
    case BubblePlacement::Top:
        x = anchor.x + (anchor.width - cardSize.width) * 0.5f;
        y = anchor.y - gap - caret - cardSize.height;
        break;
    case BubblePlacement::Left:
        x = anchor.x - gap - caret - cardSize.width;
        y = anchor.y + (anchor.height - cardSize.height) * 0.5f;
        break;
    case BubblePlacement::Right:
        x = anchor.x + anchor.width + gap + caret;
        y = anchor.y + (anchor.height - cardSize.height) * 0.5f;
        break;
    case BubblePlacement::Bottom:
    default:
        x = anchor.x + (anchor.width - cardSize.width) * 0.5f;
        y = anchor.y + anchor.height + gap + caret;
        break;
    }

    const float maxX = (std::max)(left, right - cardSize.width);
    const float maxY = (std::max)(top, bottom - cardSize.height);
    const float clampedX = ClampVal(x, left, maxX);
    const float clampedY = ClampVal(y, top, maxY);

    out.placement = side;
    out.card = Rect(clampedX, clampedY, cardSize.width, cardSize.height);
    PlaceCaret(out, anchor, caret, 6.0f);
    return std::abs(clampedX - x) + std::abs(clampedY - y);
}
} // namespace

BubbleLayout LayoutBubble(
    const Rect& anchor,
    Size cardSize,
    const Rect& viewport,
    BubblePlacement preferred,
    float gap,
    float caret,
    float margin)
{
    const BubblePlacement first = (preferred == BubblePlacement::Auto)
        ? BubblePlacement::Bottom
        : preferred;
    const BubblePlacement order[] = {
        first,
        BubblePlacement::Bottom,
        BubblePlacement::Top,
        BubblePlacement::Right,
        BubblePlacement::Left
    };

    BubbleLayout best{};
    float bestOverflow = 1.0e9f;
    for (BubblePlacement side : order) {
        BubbleLayout trial{};
        const float overflow = PlaceSide(trial, side, anchor, cardSize, viewport, gap, caret, margin);
        if (overflow < bestOverflow) {
            bestOverflow = overflow;
            best = trial;
        }
        if (overflow <= 0.75f) {
            break;
        }
        if (preferred != BubblePlacement::Auto && side == preferred) {
            // Honor an explicit side unless another side fits with zero overflow.
            continue;
        }
    }
    return best;
}

void PaintBubble(
    GraphicsContext& ctx,
    const BubbleLayout& layout,
    D2D1_COLOR_F background,
    D2D1_COLOR_F border,
    float radius)
{
    if (layout.card.IsEmpty()) {
        return;
    }
    ctx.FillRoundedRect(layout.card, radius, background);
    ctx.FillPolygon(layout.caret, 3, background);
    ctx.DrawRoundedRect(layout.card, radius, border, 1.0f);
    ctx.DrawPolygon(layout.caret, 3, border, 1.0f);
}

} // namespace CUI
