#pragma once
#include "../core/Value.h"
#include "../render/GraphicsContext.h"

namespace CUI {

enum class BubblePlacement : uint8_t {
    Auto = 0,
    Top,
    Bottom,
    Left,
    Right
};

struct BubbleLayout {
    Rect card;
    Rect total;
    Point caret[3]{};
    BubblePlacement placement = BubblePlacement::Bottom;
};

BubbleLayout LayoutBubble(
    const Rect& anchor,
    Size cardSize,
    const Rect& viewport,
    BubblePlacement preferred = BubblePlacement::Auto,
    float gap = 8.0f,
    float caret = 7.0f,
    float margin = 6.0f);

void PaintBubble(
    GraphicsContext& ctx,
    const BubbleLayout& layout,
    D2D1_COLOR_F background,
    D2D1_COLOR_F border,
    float radius = 6.0f);

} // namespace CUI
