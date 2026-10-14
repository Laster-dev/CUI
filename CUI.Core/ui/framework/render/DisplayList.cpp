#include "DisplayList.h"
#include <algorithm>
#include <cmath>

namespace CUI {

DisplayList::DisplayList() = default;
DisplayList::~DisplayList() = default;

void DisplayList::AddOp(std::unique_ptr<DrawOp> op) {
    if (!op) return;
    const Rect bounds = op->GetBounds();
    if (!bounds.IsEmpty()) {
        if (m_bounds.IsEmpty()) {
            m_bounds = bounds;
        } else {
            m_bounds = m_bounds.Union(bounds);
        }
    }
    m_ops.push_back(std::move(op));
}

void DisplayList::ClearRect(const Rect& rect, Color color) {
    AddOp(std::make_unique<ClearRectOp>(rect, color));
}

void DisplayList::DrawRect(const Rect& rect, Color color, float strokeWidth) {
    AddOp(std::make_unique<DrawRectOp>(rect, color, strokeWidth));
}

void DisplayList::FillRect(const Rect& rect, Color color) {
    AddOp(std::make_unique<FillRectOp>(rect, color));
}

void DisplayList::DrawRoundedRect(const Rect& rect, float radius, Color color, float strokeWidth) {
    AddOp(std::make_unique<DrawRoundedRectOp>(rect, radius, color, strokeWidth));
}

void DisplayList::FillRoundedRect(const Rect& rect, float radius, Color color) {
    AddOp(std::make_unique<FillRoundedRectOp>(rect, radius, color));
}

void DisplayList::DrawLine(Point p1, Point p2, Color color, float strokeWidth) {
    AddOp(std::make_unique<DrawLineOp>(p1, p2, color, strokeWidth));
}

void DisplayList::DrawSmoothLine(Point p1, Point p2, Color color, float strokeWidth) {
    AddOp(std::make_unique<DrawSmoothLineOp>(p1, p2, color, strokeWidth));
}

void DisplayList::DrawSmoothArc(Point center, float radius, float startRad, float sweepRad, Color color, float strokeWidth) {
    AddOp(std::make_unique<DrawSmoothArcOp>(center, radius, startRad, sweepRad, color, strokeWidth));
}

void DisplayList::DrawPolygon(std::vector<Point> points, Color color, float strokeWidth) {
    AddOp(std::make_unique<DrawPolygonOp>(std::move(points), color, strokeWidth));
}

void DisplayList::FillPolygon(std::vector<Point> points, Color color) {
    AddOp(std::make_unique<FillPolygonOp>(std::move(points), color));
}

void DisplayList::FillRoundedGeometry(std::vector<Point> points, float radius, Color color) {
    AddOp(std::make_unique<FillRoundedGeometryOp>(std::move(points), radius, color));
}

void DisplayList::DrawChevron(const Rect& bounds, Color color, GraphicsContext::ChevronDirection dir, float strokeWidth) {
    AddOp(std::make_unique<DrawChevronOp>(bounds, color, dir, strokeWidth));
}

void DisplayList::DrawIcon(const std::string& icon, const Rect& dest, Color color, float opacity, const std::string& glyphFont, float glyphSize) {
    AddOp(std::make_unique<DrawIconOp>(icon, dest, color, opacity, glyphFont, glyphSize));
}

void DisplayList::DrawSvg(const std::string& source, const Rect& dest, std::optional<Color> tint, float opacity, std::optional<Color> foreground) {
    AddOp(std::make_unique<DrawSvgOp>(source, dest, tint, opacity, foreground));
}

void DisplayList::DrawText(const std::string& text, const Rect& rect, Color color,
                           const std::string& fontName, float fontSize,
                           TextAlignment align, TextVerticalAlignment vAlign,
                           FontWeight weight, bool truncateWithEllipsis,
                           FontStyle style, FontStretch stretch,
                           bool underline, bool strikethrough) {
    AddOp(std::make_unique<DrawTextOp>(text, rect, color, fontName, fontSize, align, vAlign, weight, truncateWithEllipsis, style, stretch, underline, strikethrough));
}

void DisplayList::DrawTextRotated(const std::string& text, Point center, float degrees, Color color,
                                  const std::string& fontName, float fontSize, FontWeight weight) {
    AddOp(std::make_unique<DrawTextRotatedOp>(text, center, degrees, color, fontName, fontSize, weight));
}

void DisplayList::PushClip(const Rect& rect) {
    AddOp(std::make_unique<PushClipOp>(rect));
}

void DisplayList::PushRoundedClip(const Rect& rect, float radius) {
    AddOp(std::make_unique<PushRoundedClipOp>(rect, radius));
}

void DisplayList::PushEllipseClip(Point center, float radiusX, float radiusY) {
    AddOp(std::make_unique<PushEllipseClipOp>(center, radiusX, radiusY));
}

void DisplayList::PopClip() {
    AddOp(std::make_unique<PopClipOp>());
}

void DisplayList::PushOpacity(float opacity) {
    AddOp(std::make_unique<PushOpacityOp>(opacity));
}

void DisplayList::PopOpacity() {
    AddOp(std::make_unique<PopOpacityOp>());
}

void DisplayList::PushTransform(const Matrix3x2& transform) {
    AddOp(std::make_unique<PushTransformOp>(transform));
}

void DisplayList::PushTranslation(float dx, float dy) {
    AddOp(std::make_unique<PushTranslationOp>(dx, dy));
}

void DisplayList::PopTransform() {
    AddOp(std::make_unique<PopTransformOp>());
}

void DisplayList::PushPopupReveal(const Rect& bounds, float progress, Point origin) {
    AddOp(std::make_unique<PushPopupRevealOp>(bounds, progress, origin));
}

void DisplayList::PopPopupReveal() {
    AddOp(std::make_unique<PopPopupRevealOp>());
}

void DisplayList::CustomDraw(std::function<void(GraphicsContext&)> callback, const Rect& bounds) {
    AddOp(std::make_unique<CustomDrawOp>(std::move(callback), bounds));
}

void DisplayList::DrawSubDisplayList(std::shared_ptr<DisplayList> subList, Point offset, float opacity) {
    AddOp(std::make_unique<SubDisplayListOp>(std::move(subList), offset, opacity));
}

void DisplayList::Playback(GraphicsContext& ctx) const {
    for (const auto& op : m_ops) {
        if (op) {
            op->Execute(ctx);
        }
    }
}

void DisplayList::PlaybackWithOffset(GraphicsContext& ctx, Point offset, float opacity) const {
    const bool useOffset = (std::abs(offset.x) > 0.001f || std::abs(offset.y) > 0.001f);
    const bool useOpacity = (opacity < 0.999f);

    if (useOffset) {
        ctx.PushTranslation(offset.x, offset.y);
    }
    if (useOpacity) {
        ctx.PushOpacity(opacity);
    }

    Playback(ctx);

    if (useOpacity) {
        ctx.PopOpacity();
    }
    if (useOffset) {
        ctx.PopTransform();
    }
}

void DisplayList::Optimize() {
    if (m_ops.size() < 2) return;

    std::vector<std::unique_ptr<DrawOp>> optimized;
    optimized.reserve(m_ops.size());
    uint32_t saved = 0;

    for (size_t i = 0; i < m_ops.size(); ++i) {
        if (!m_ops[i]) continue;

        // 消除空裁剪对：PushClip 后立即跟随 PopClip
        if (m_ops[i]->GetType() == DrawOpType::PushClip && i + 1 < m_ops.size()) {
            if (m_ops[i + 1] && m_ops[i + 1]->GetType() == DrawOpType::PopClip) {
                ++saved;
                ++i; // 跳过此对
                continue;
            }
        }

        // 消除单位透明度对：PushOpacity(1.0f) 紧随 PopOpacity
        if (m_ops[i]->GetType() == DrawOpType::PushOpacity) {
            const auto* op = static_cast<const PushOpacityOp*>(m_ops[i].get());
            if (std::abs(op->opacity - 1.0f) < 0.001f && i + 1 < m_ops.size()) {
                if (m_ops[i + 1] && m_ops[i + 1]->GetType() == DrawOpType::PopOpacity) {
                    ++saved;
                    ++i;
                    continue;
                }
            }
        }

        // 合并连续的同色同半径填充矩形（若矩形相邻或完全重合）
        if (m_ops[i]->GetType() == DrawOpType::FillRect && i + 1 < m_ops.size()) {
            const auto* curr = static_cast<const FillRectOp*>(m_ops[i].get());
            if (m_ops[i + 1] && m_ops[i + 1]->GetType() == DrawOpType::FillRect) {
                const auto* next = static_cast<const FillRectOp*>(m_ops[i + 1].get());
                if (curr->color == next->color) {
                    // 若下一个矩形完全被当前包含
                    if (curr->rect.Contains(next->rect.x, next->rect.y) &&
                        curr->rect.Contains(next->rect.Right(), next->rect.Bottom())) {
                        ++saved;
                        ++i; // 跳过被包含的冗余绘制
                    }
                }
            }
        }

        optimized.push_back(std::move(m_ops[i]));
    }

    m_batchReductionCount += saved;
    m_ops = std::move(optimized);
}

void DisplayList::Clear() {
    m_ops.clear();
    m_bounds = Rect();
    m_batchReductionCount = 0;
}

size_t DisplayList::GetEstimatedMemoryBytes() const {
    size_t bytes = sizeof(DisplayList) + m_ops.capacity() * sizeof(std::unique_ptr<DrawOp>);
    for (const auto& op : m_ops) {
        if (!op) continue;
        bytes += sizeof(*op);
        if (op->GetType() == DrawOpType::DrawText) {
            const auto* textOp = static_cast<const DrawTextOp*>(op.get());
            bytes += textOp->text.capacity() + textOp->fontName.capacity();
        } else if (op->GetType() == DrawOpType::DrawSvg) {
            const auto* svgOp = static_cast<const DrawSvgOp*>(op.get());
            bytes += svgOp->source.capacity();
        } else if (op->GetType() == DrawOpType::DrawIcon) {
            const auto* iconOp = static_cast<const DrawIconOp*>(op.get());
            bytes += iconOp->icon.capacity() + iconOp->glyphFont.capacity();
        }
    }
    return bytes;
}

void SubDisplayListOp::Execute(GraphicsContext& ctx) const {
    if (subList) {
        subList->PlaybackWithOffset(ctx, offset, opacity);
    }
}

Rect SubDisplayListOp::GetBounds() const {
    if (subList) {
        Rect b = subList->GetBounds();
        if (!b.IsEmpty()) {
            return Rect(b.x + offset.x, b.y + offset.y, b.width, b.height);
        }
    }
    return Rect();
}

} // namespace CUI
