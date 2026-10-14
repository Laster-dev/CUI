#include "BoxModelOverlay.h"
#include "../window/Window.h"

#include <cstdio>
#include <sstream>

namespace CUI {

BoxModelMetrics BoxModelOverlay::ExtractMetrics(const UIElement* element) {
    BoxModelMetrics m;
    if (!element) return m;

    m.bounds = element->GetBounds();
    m.margin = element->GetMargin();
    m.padding = element->GetPadding();
    m.borderThickness = element->GetBorderThickness();
    m.typeName = ExtractElementTypeName(element);

    char addrBuf[32];
    std::snprintf(addrBuf, sizeof(addrBuf), "0x%p", static_cast<const void*>(element));
    m.elementAddress = addrBuf;

    m.visibility = element->GetVisibility();
    m.isEnabled = element->IsEnabled();
    m.isFocused = (Window::Current() && Window::Current()->GetFocusedElement() == element);
    m.childCount = element->GetChildren().size();

    // 1. 外边距包围盒
    m.marginRect = Rect(
        m.bounds.x - m.margin.left,
        m.bounds.y - m.margin.top,
        m.bounds.width + m.margin.left + m.margin.right,
        m.bounds.height + m.margin.top + m.margin.bottom);

    // 2. 边框盒 (即 bounds 自身)
    m.borderRect = m.bounds;

    // 3. 内边距盒 (bounds 内缩边框线)
    const float bt = (std::max)(0.0f, m.borderThickness);
    m.paddingRect = Rect(
        m.bounds.x + bt,
        m.bounds.y + bt,
        (std::max)(0.0f, m.bounds.width - bt * 2.0f),
        (std::max)(0.0f, m.bounds.height - bt * 2.0f));

    // 4. 内容盒 (padding 内部净有效区域)
    m.contentRect = Rect(
        m.paddingRect.x + m.padding.left,
        m.paddingRect.y + m.padding.top,
        (std::max)(0.0f, m.paddingRect.width - m.padding.left - m.padding.right),
        (std::max)(0.0f, m.paddingRect.height - m.padding.top - m.padding.bottom));

    return m;
}

void BoxModelOverlay::RenderOverlay(GraphicsContext& ctx, const UIElement* element, Size windowSize) {
    if (!element) return;
    const BoxModelMetrics m = ExtractMetrics(element);
    if (m.bounds.IsEmpty() && m.marginRect.IsEmpty()) return;

    // 1. 渲染外边距带 (Margin) - 橙色
    if (m.margin.left > 0.0f || m.margin.top > 0.0f || m.margin.right > 0.0f || m.margin.bottom > 0.0f) {
        // 上边距
        if (m.margin.top > 0.0f) {
            ctx.FillRect(Rect(m.marginRect.x, m.marginRect.y, m.marginRect.width, m.margin.top), DevToolsColors::Margin());
        }
        // 下边距
        if (m.margin.bottom > 0.0f) {
            ctx.FillRect(Rect(m.marginRect.x, m.bounds.y + m.bounds.height, m.marginRect.width, m.margin.bottom), DevToolsColors::Margin());
        }
        // 左边距
        if (m.margin.left > 0.0f) {
            ctx.FillRect(Rect(m.marginRect.x, m.bounds.y, m.margin.left, m.bounds.height), DevToolsColors::Margin());
        }
        // 右边距
        if (m.margin.right > 0.0f) {
            ctx.FillRect(Rect(m.bounds.x + m.bounds.width, m.bounds.y, m.margin.right, m.bounds.height), DevToolsColors::Margin());
        }
        // 外围描边
        ctx.DrawRect(m.marginRect, DevToolsColors::MarginStroke(), 1.0f);
    }

    // 2. 渲染边框带 (Border) - 鹅黄色
    if (m.borderThickness > 0.0f) {
        ctx.DrawRect(m.borderRect, DevToolsColors::BorderStroke(), (std::max)(1.0f, m.borderThickness));
    } else {
        ctx.DrawRect(m.borderRect, DevToolsColors::BorderStroke(), 1.0f);
    }

    // 3. 渲染内边距带 (Padding) - 草绿色
    if (m.padding.left > 0.0f || m.padding.top > 0.0f || m.padding.right > 0.0f || m.padding.bottom > 0.0f) {
        if (m.padding.top > 0.0f) {
            ctx.FillRect(Rect(m.paddingRect.x, m.paddingRect.y, m.paddingRect.width, m.padding.top), DevToolsColors::Padding());
        }
        if (m.padding.bottom > 0.0f) {
            ctx.FillRect(Rect(m.paddingRect.x, m.contentRect.y + m.contentRect.height, m.paddingRect.width, m.padding.bottom), DevToolsColors::Padding());
        }
        if (m.padding.left > 0.0f) {
            ctx.FillRect(Rect(m.paddingRect.x, m.contentRect.y, m.padding.left, m.contentRect.height), DevToolsColors::Padding());
        }
        if (m.padding.right > 0.0f) {
            ctx.FillRect(Rect(m.contentRect.x + m.contentRect.width, m.contentRect.y, m.padding.right, m.contentRect.height), DevToolsColors::Padding());
        }
        ctx.DrawRect(m.paddingRect, DevToolsColors::PaddingStroke(), 1.0f);
    }

    // 4. 渲染内容区 (Content) - 天蓝色
    if (!m.contentRect.IsEmpty()) {
        ctx.FillRect(m.contentRect, DevToolsColors::Content());
        ctx.DrawRect(m.contentRect, DevToolsColors::ContentStroke(), 1.0f);
    }

    // 5. 悬浮信息气泡 Badge
    char dimBuf[64];
    std::snprintf(dimBuf, sizeof(dimBuf), "%.1f × %.1f", m.bounds.width, m.bounds.height);
    std::string badgeTitle = m.typeName + "  " + dimBuf;

    const float badgeWidth = (std::max)(180.0f, static_cast<float>(badgeTitle.length() * 7.5f + 24.0f));
    constexpr float badgeHeight = 26.0f;

    float bx = m.bounds.x;
    float by = m.bounds.y - badgeHeight - 6.0f;
    if (by < 4.0f) {
        by = m.bounds.y + m.bounds.height + 6.0f;
    }
    if (bx + badgeWidth > windowSize.width - 8.0f) {
        bx = windowSize.width - badgeWidth - 8.0f;
    }
    if (bx < 8.0f) {
        bx = 8.0f;
    }

    const Rect badgeRect(bx, by, badgeWidth, badgeHeight);
    ctx.FillRoundedRect(badgeRect, 5.0f, DevToolsColors::BadgeBackground());
    ctx.DrawRoundedRect(badgeRect, 5.0f, DevToolsColors::BadgeBorder(), 1.0f);

    // 绘制小图标方块
    ctx.FillRoundedRect(Rect(bx + 6.0f, by + 6.0f, 14.0f, 14.0f), 3.0f, DevToolsColors::BadgeAccent());

    // 绘制标题
    ctx.DrawText(
        badgeTitle,
        Rect(bx + 26.0f, by, badgeWidth - 30.0f, badgeHeight),
        DevToolsColors::BadgeText(),
        "Segoe UI",
        12.0f,
        TextAlignment::Left,
        TextVerticalAlignment::Center,
        FontWeight::SemiBold);
}

void BoxModelOverlay::RenderDiagram(GraphicsContext& ctx, const BoxModelMetrics& metrics, Rect destRect) {
    if (destRect.width < 120.0f || destRect.height < 80.0f) return;

    // 1. 外部 margin 盒
    ctx.FillRoundedRect(destRect, 6.0f, DevToolsColors::Margin());
    ctx.DrawRoundedRect(destRect, 6.0f, DevToolsColors::MarginStroke(), 1.0f);

    // margin 标签与数值
    ctx.DrawText("外边距 margin", Rect(destRect.x + 6.0f, destRect.y + 2.0f, 90.0f, 16.0f),
                 Color(0.2f, 0.2f, 0.2f, 0.85f), "Segoe UI", 9.5f, TextAlignment::Left, TextVerticalAlignment::Center);

    char valBuf[32];
    // Top
    std::snprintf(valBuf, sizeof(valBuf), "%.0f", metrics.margin.top);
    ctx.DrawText(valBuf, Rect(destRect.x, destRect.y + 2.0f, destRect.width, 16.0f),
                 Color(0.15f, 0.15f, 0.15f, 0.95f), "Segoe UI", 10.0f, TextAlignment::Center, TextVerticalAlignment::Center);
    // Bottom
    std::snprintf(valBuf, sizeof(valBuf), "%.0f", metrics.margin.bottom);
    ctx.DrawText(valBuf, Rect(destRect.x, destRect.y + destRect.height - 18.0f, destRect.width, 16.0f),
                 Color(0.15f, 0.15f, 0.15f, 0.95f), "Segoe UI", 10.0f, TextAlignment::Center, TextVerticalAlignment::Center);
    // Left
    std::snprintf(valBuf, sizeof(valBuf), "%.0f", metrics.margin.left);
    ctx.DrawText(valBuf, Rect(destRect.x + 4.0f, destRect.y, 24.0f, destRect.height),
                 Color(0.15f, 0.15f, 0.15f, 0.95f), "Segoe UI", 10.0f, TextAlignment::Left, TextVerticalAlignment::Center);
    // Right
    std::snprintf(valBuf, sizeof(valBuf), "%.0f", metrics.margin.right);
    ctx.DrawText(valBuf, Rect(destRect.x + destRect.width - 28.0f, destRect.y, 24.0f, destRect.height),
                 Color(0.15f, 0.15f, 0.15f, 0.95f), "Segoe UI", 10.0f, TextAlignment::Right, TextVerticalAlignment::Center);

    // 2. border 盒
    const Rect borderRect = destRect.Inflate(-20.0f);
    if (borderRect.width <= 0.0f || borderRect.height <= 0.0f) return;
    ctx.FillRoundedRect(borderRect, 4.0f, DevToolsColors::Border());
    ctx.DrawRoundedRect(borderRect, 4.0f, DevToolsColors::BorderStroke(), 1.0f);

    ctx.DrawText("边框 border", Rect(borderRect.x + 6.0f, borderRect.y + 2.0f, 80.0f, 16.0f),
                 Color(0.2f, 0.2f, 0.2f, 0.85f), "Segoe UI", 9.5f, TextAlignment::Left, TextVerticalAlignment::Center);
    std::snprintf(valBuf, sizeof(valBuf), "%.0f", metrics.borderThickness);
    ctx.DrawText(valBuf, Rect(borderRect.x, borderRect.y + 2.0f, borderRect.width, 16.0f),
                 Color(0.15f, 0.15f, 0.15f, 0.95f), "Segoe UI", 10.0f, TextAlignment::Center, TextVerticalAlignment::Center);

    // 3. padding 盒
    const Rect paddingRect = borderRect.Inflate(-18.0f);
    if (paddingRect.width <= 0.0f || paddingRect.height <= 0.0f) return;
    ctx.FillRoundedRect(paddingRect, 4.0f, DevToolsColors::Padding());
    ctx.DrawRoundedRect(paddingRect, 4.0f, DevToolsColors::PaddingStroke(), 1.0f);

    ctx.DrawText("内边距 padding", Rect(paddingRect.x + 6.0f, paddingRect.y + 2.0f, 90.0f, 16.0f),
                 Color(0.2f, 0.2f, 0.2f, 0.85f), "Segoe UI", 9.5f, TextAlignment::Left, TextVerticalAlignment::Center);
    // Padding numbers
    std::snprintf(valBuf, sizeof(valBuf), "%.0f", metrics.padding.top);
    ctx.DrawText(valBuf, Rect(paddingRect.x, paddingRect.y + 2.0f, paddingRect.width, 16.0f),
                 Color(0.15f, 0.15f, 0.15f, 0.95f), "Segoe UI", 10.0f, TextAlignment::Center, TextVerticalAlignment::Center);
    std::snprintf(valBuf, sizeof(valBuf), "%.0f", metrics.padding.bottom);
    ctx.DrawText(valBuf, Rect(paddingRect.x, paddingRect.y + paddingRect.height - 18.0f, paddingRect.width, 16.0f),
                 Color(0.15f, 0.15f, 0.15f, 0.95f), "Segoe UI", 10.0f, TextAlignment::Center, TextVerticalAlignment::Center);
    std::snprintf(valBuf, sizeof(valBuf), "%.0f", metrics.padding.left);
    ctx.DrawText(valBuf, Rect(paddingRect.x + 4.0f, paddingRect.y, 24.0f, paddingRect.height),
                 Color(0.15f, 0.15f, 0.15f, 0.95f), "Segoe UI", 10.0f, TextAlignment::Left, TextVerticalAlignment::Center);
    std::snprintf(valBuf, sizeof(valBuf), "%.0f", metrics.padding.right);
    ctx.DrawText(valBuf, Rect(paddingRect.x + paddingRect.width - 28.0f, paddingRect.y, 24.0f, paddingRect.height),
                 Color(0.15f, 0.15f, 0.15f, 0.95f), "Segoe UI", 10.0f, TextAlignment::Right, TextVerticalAlignment::Center);

    // 4. content 盒
    const Rect contentRect = paddingRect.Inflate(-18.0f);
    if (contentRect.width <= 0.0f || contentRect.height <= 0.0f) return;
    ctx.FillRoundedRect(contentRect, 3.0f, DevToolsColors::Content());
    ctx.DrawRoundedRect(contentRect, 3.0f, DevToolsColors::ContentStroke(), 1.0f);

    std::snprintf(valBuf, sizeof(valBuf), "%.1f × %.1f", metrics.contentRect.width, metrics.contentRect.height);
    ctx.DrawText(valBuf, contentRect, Color(0.08f, 0.12f, 0.18f, 0.95f),
                 "Segoe UI", 11.0f, TextAlignment::Center, TextVerticalAlignment::Center, FontWeight::SemiBold);
}

} // namespace CUI
