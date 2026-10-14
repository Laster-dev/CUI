#include "OverdrawVisualizer.h"

namespace CUI {

OverdrawVisualizer::OverdrawVisualizer() = default;

void OverdrawVisualizer::RecordDirtyRects(const std::vector<Rect>& rects) {
    const auto now = std::chrono::steady_clock::now();

    // 剔除超过 260ms 的过期脏区
    m_history.erase(
        std::remove_if(m_history.begin(), m_history.end(), [&](const TrackedOverdrawRect& item) {
            float sec = std::chrono::duration<float>(now - item.timestamp).count();
            return sec > 0.26f;
        }),
        m_history.end());

    // 压入本帧新脏区
    for (const Rect& r : rects) {
        if (r.IsEmpty()) continue;

        bool merged = false;
        for (auto& old : m_history) {
            if (old.bounds.Intersects(r)) {
                old.bounds = old.bounds.Union(r);
                old.timestamp = now;
                old.hitCount = (std::min)(old.hitCount + 1, 5);
                merged = true;
                break;
            }
        }
        if (!merged) {
            m_history.push_back({ r, now, 1 });
        }
    }
}

void OverdrawVisualizer::Clear() {
    m_history.clear();
}

void OverdrawVisualizer::Render(GraphicsContext& ctx, Size windowSize) {
    if (m_history.empty()) return;
    const auto now = std::chrono::steady_clock::now();

    for (const auto& item : m_history) {
        float elapsed = std::chrono::duration<float>(now - item.timestamp).count();
        if (elapsed > 0.26f) continue;

        // 随着时间衰减透明度
        float fade = (std::max)(0.0f, 1.0f - (elapsed / 0.26f));

        // 根据 hitCount 分级热力图色彩
        Color fillColor;
        Color strokeColor;
        switch (item.hitCount) {
        case 1:
            // 1x 重绘: 柔和草绿
            fillColor = Color(0.20f, 0.85f, 0.40f, 0.20f * fade);
            strokeColor = Color(0.20f, 0.85f, 0.40f, 0.70f * fade);
            break;
        case 2:
            // 2x 重绘: 天蓝/青色
            fillColor = Color(0.20f, 0.70f, 0.95f, 0.28f * fade);
            strokeColor = Color(0.20f, 0.70f, 0.95f, 0.80f * fade);
            break;
        case 3:
            // 3x 重绘: 暖黄/琥珀
            fillColor = Color(0.98f, 0.75f, 0.20f, 0.35f * fade);
            strokeColor = Color(0.98f, 0.75f, 0.20f, 0.85f * fade);
            break;
        default:
            // 4x+ 重绘: 警示亮红
            fillColor = Color(0.95f, 0.25f, 0.25f, 0.45f * fade);
            strokeColor = Color(0.95f, 0.25f, 0.25f, 0.95f * fade);
            break;
        }

        ctx.FillRect(item.bounds, fillColor);
        ctx.DrawRect(item.bounds, strokeColor, 1.0f);
    }

    // 绘制底部图例卡片 (Legend)
    constexpr float legW = 260.0f;
    constexpr float legH = 26.0f;
    const float legX = windowSize.width - legW - 14.0f;
    const float legY = windowSize.height - legH - 12.0f;

    const Rect legendRect(legX, legY, legW, legH);
    ctx.FillRoundedRect(legendRect, 5.0f, Color(0.10f, 0.12f, 0.16f, 0.90f));
    ctx.DrawRoundedRect(legendRect, 5.0f, Color(0.30f, 0.35f, 0.42f, 0.75f), 1.0f);

    ctx.DrawText(
        "重绘热力图:",
        Rect(legX + 8.0f, legY, 96.0f, legH),
        Color(0.85f, 0.88f, 0.92f, 0.90f),
        "Segoe UI",
        9.5f,
        TextAlignment::Left,
        TextVerticalAlignment::Center,
        FontWeight::SemiBold);

    // 1x 绿色方块
    ctx.FillRoundedRect(Rect(legX + 88.0f, legY + 8.0f, 10.0f, 10.0f), 2.0f, Color(0.20f, 0.85f, 0.40f, 0.9f));
    ctx.DrawText("1次", Rect(legX + 102.0f, legY, 24.0f, legH), Color(0.8f, 0.8f, 0.8f, 0.9f), "Segoe UI", 9.0f, TextAlignment::Left, TextVerticalAlignment::Center);

    // 2x 蓝色方块
    ctx.FillRoundedRect(Rect(legX + 128.0f, legY + 8.0f, 10.0f, 10.0f), 2.0f, Color(0.20f, 0.70f, 0.95f, 0.9f));
    ctx.DrawText("2次", Rect(legX + 142.0f, legY, 24.0f, legH), Color(0.8f, 0.8f, 0.8f, 0.9f), "Segoe UI", 9.0f, TextAlignment::Left, TextVerticalAlignment::Center);

    // 3x 黄色方块
    ctx.FillRoundedRect(Rect(legX + 168.0f, legY + 8.0f, 10.0f, 10.0f), 2.0f, Color(0.98f, 0.75f, 0.20f, 0.9f));
    ctx.DrawText("3次", Rect(legX + 182.0f, legY, 24.0f, legH), Color(0.8f, 0.8f, 0.8f, 0.9f), "Segoe UI", 9.0f, TextAlignment::Left, TextVerticalAlignment::Center);

    // 4x+ 红色方块
    ctx.FillRoundedRect(Rect(legX + 208.0f, legY + 8.0f, 10.0f, 10.0f), 2.0f, Color(0.95f, 0.25f, 0.25f, 0.9f));
    ctx.DrawText("4次+", Rect(legX + 222.0f, legY, 28.0f, legH), Color(0.8f, 0.8f, 0.8f, 0.9f), "Segoe UI", 9.0f, TextAlignment::Left, TextVerticalAlignment::Center);
}

} // namespace CUI
