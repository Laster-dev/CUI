#include "PerformanceOverlay.h"

#include <cstdio>

namespace CUI {

PerformanceOverlay::PerformanceOverlay() {
    m_stats.frameTimeHistory.reserve(kMaxHistory);
    // 预填默认 16.6ms 历史基准
    for (size_t i = 0; i < kMaxHistory; ++i) {
        m_stats.frameTimeHistory.push_back(16.6f);
    }
}

void PerformanceOverlay::RecordFrame(float frameTimeMs, float layoutMs, float renderMs,
                                     uint32_t drawOps, uint32_t batchReductions, uint32_t layerCacheHits) {
    m_stats.frameTimeMs = frameTimeMs;
    m_stats.layoutDurationMs = layoutMs;
    m_stats.renderDurationMs = renderMs;
    m_stats.totalDrawOps = drawOps;
    m_stats.batchReductions = batchReductions;
    m_stats.layerCacheHits = layerCacheHits;

    // 压入折线历史环形队列
    if (m_stats.frameTimeHistory.size() >= kMaxHistory) {
        m_stats.frameTimeHistory.erase(m_stats.frameTimeHistory.begin());
    }
    m_stats.frameTimeHistory.push_back(frameTimeMs);

    // 平滑 FPS 采样 (每 500ms 统计一次均值)
    using clock = std::chrono::steady_clock;
    const auto now = clock::now();
    if (m_fpsSampleStart.time_since_epoch().count() == 0) {
        m_fpsSampleStart = now;
    }
    m_fpsFrameCount++;

    const float elapsedSec = std::chrono::duration<float>(now - m_fpsSampleStart).count();
    if (elapsedSec >= 0.5f) {
        m_stats.fps = static_cast<float>(m_fpsFrameCount) / elapsedSec;
        m_fpsFrameCount = 0;
        m_fpsSampleStart = now;
    }
}

void PerformanceOverlay::Render(GraphicsContext& ctx, Size windowSize, Point anchorPos) {
    constexpr float cardWidth = 240.0f;
    constexpr float cardHeight = 126.0f;

    float cx = anchorPos.x;
    float cy = anchorPos.y;
    if (cx < 0.0f || cy < 0.0f) {
        // 右上角避让
        cx = windowSize.width - cardWidth - 14.0f;
        cy = 44.0f; // 避开顶层标题栏
    }

    const Rect cardRect(cx, cy, cardWidth, cardHeight);

    // 1. 卡片底盘 (半透明高斯质感深色面板 + 圆角 + 微描边)
    ctx.FillRoundedRect(cardRect, 8.0f, Color(0.10f, 0.12f, 0.16f, 0.92f));
    ctx.DrawRoundedRect(cardRect, 8.0f, Color(0.28f, 0.33f, 0.40f, 0.75f), 1.0f);

    // 2. 状态小圆点
    Color statusDotColor = Color(0.22f, 0.82f, 0.45f, 1.0f);
    if (m_stats.fps < 30.0f) {
        statusDotColor = Color(0.95f, 0.32f, 0.32f, 1.0f);
    } else if (m_stats.fps < 55.0f) {
        statusDotColor = Color(0.96f, 0.75f, 0.22f, 1.0f);
    }
    ctx.FillRoundedRect(Rect(cx + 12.0f, cy + 12.0f, 8.0f, 8.0f), 4.0f, statusDotColor);

    // 3. 头部标题
    ctx.DrawText(
        "实时性能仪表盘",
        Rect(cx + 26.0f, cy + 9.0f, 140.0f, 16.0f),
        Color(0.70f, 0.75f, 0.82f, 1.0f),
        "Segoe UI",
        10.0f,
        TextAlignment::Left,
        TextVerticalAlignment::Center,
        FontWeight::SemiBold);

    // 4. 大字号 FPS
    char fpsBuf[32];
    std::snprintf(fpsBuf, sizeof(fpsBuf), "%.1f FPS", m_stats.fps > 0.0f ? m_stats.fps : 60.0f);
    ctx.DrawText(
        fpsBuf,
        Rect(cx + cardWidth - 90.0f, cy + 7.0f, 78.0f, 20.0f),
        statusDotColor,
        "Segoe UI",
        13.0f,
        TextAlignment::Right,
        TextVerticalAlignment::Center,
        FontWeight::Bold);

    // 5. 耗时分解文本
    char timeBuf[128];
    std::snprintf(timeBuf, sizeof(timeBuf), "总耗时: %.1fms | 布局: %.1fms | 绘制: %.1fms",
                  m_stats.frameTimeMs, m_stats.layoutDurationMs, m_stats.renderDurationMs);
    ctx.DrawText(
        timeBuf,
        Rect(cx + 12.0f, cy + 28.0f, cardWidth - 24.0f, 15.0f),
        Color(0.85f, 0.88f, 0.92f, 0.90f),
        "Segoe UI",
        9.5f,
        TextAlignment::Left,
        TextVerticalAlignment::Center);

    // 6. Sparkline 走势图画布
    const float graphX = cx + 12.0f;
    const float graphY = cy + 48.0f;
    const float graphW = cardWidth - 24.0f;
    const float graphH = 42.0f;

    // 走势图背景
    ctx.FillRoundedRect(Rect(graphX, graphY, graphW, graphH), 4.0f, Color(0.06f, 0.07f, 0.09f, 0.80f));
    ctx.DrawRoundedRect(Rect(graphX, graphY, graphW, graphH), 4.0f, Color(0.20f, 0.24f, 0.30f, 0.60f), 1.0f);

    // 16.6ms 目标基准线 (60Hz)
    const float targetY = graphY + graphH - (16.6f / 33.3f) * graphH;
    ctx.DrawLine(Point(graphX, targetY), Point(graphX + graphW, targetY), Color(0.40f, 0.45f, 0.52f, 0.50f), 1.0f);

    // 绘制折线
    if (m_stats.frameTimeHistory.size() >= 2) {
        const size_t n = m_stats.frameTimeHistory.size();
        const float step = graphW / static_cast<float>(n - 1);

        for (size_t i = 0; i < n - 1; ++i) {
            const float t1 = m_stats.frameTimeHistory[i];
            const float t2 = m_stats.frameTimeHistory[i + 1];

            const float y1 = graphY + graphH - std::clamp(t1 / 33.3f, 0.0f, 1.0f) * (graphH - 4.0f) - 2.0f;
            const float y2 = graphY + graphH - std::clamp(t2 / 33.3f, 0.0f, 1.0f) * (graphH - 4.0f) - 2.0f;

            Color segColor = Color(0.25f, 0.85f, 0.50f, 0.90f);
            if (t2 > 33.3f) {
                segColor = Color(0.95f, 0.30f, 0.30f, 0.90f);
            } else if (t2 > 17.0f) {
                segColor = Color(0.96f, 0.75f, 0.22f, 0.90f);
            }

            ctx.DrawLine(Point(graphX + i * step, y1), Point(graphX + (i + 1) * step, y2), segColor, 1.5f);
        }
    }

    // 7. 底部流水线度量数据
    char pipeBuf[128];
    std::snprintf(pipeBuf, sizeof(pipeBuf), "指令数: %u (已合批: %u) | 缓存命中: %u",
                  m_stats.totalDrawOps, m_stats.batchReductions, m_stats.layerCacheHits);
    ctx.DrawText(
        pipeBuf,
        Rect(cx + 12.0f, cy + cardHeight - 26.0f, cardWidth - 24.0f, 20.0f),
        Color(0.65f, 0.70f, 0.78f, 0.85f),
        "Segoe UI",
        9.5f,
        TextAlignment::Left,
        TextVerticalAlignment::Center);
}

} // namespace CUI
