#pragma once
#include <memory>
#include <vector>
#include <cstdint>
#include "DrawOp.h"

namespace CUI {

class DisplayList : public std::enable_shared_from_this<DisplayList> {
public:
    DisplayList();
    ~DisplayList();

    // 指令添加
    void AddOp(std::unique_ptr<DrawOp> op);

    // 快捷常用绘图录制方法
    void ClearRect(const Rect& rect, Color color = Color::Transparent);
    void DrawRect(const Rect& rect, Color color, float strokeWidth = 1.0f);
    void FillRect(const Rect& rect, Color color);
    void DrawRoundedRect(const Rect& rect, float radius, Color color, float strokeWidth = 1.0f);
    void FillRoundedRect(const Rect& rect, float radius, Color color);
    void DrawLine(Point p1, Point p2, Color color, float strokeWidth = 1.0f);
    void DrawSmoothLine(Point p1, Point p2, Color color, float strokeWidth = 1.5f);
    void DrawSmoothArc(Point center, float radius, float startRad, float sweepRad, Color color, float strokeWidth = 1.5f);
    void DrawPolygon(std::vector<Point> points, Color color, float strokeWidth = 1.0f);
    void FillPolygon(std::vector<Point> points, Color color);
    void FillRoundedGeometry(std::vector<Point> points, float radius, Color color);
    void DrawChevron(const Rect& bounds, Color color, GraphicsContext::ChevronDirection dir, float strokeWidth = 1.6f);
    void DrawIcon(const std::string& icon, const Rect& dest, Color color, float opacity = 1.0f, const std::string& glyphFont = "Segoe UI Emoji", float glyphSize = 0.0f);
    void DrawSvg(const std::string& source, const Rect& dest, std::optional<Color> tint = std::nullopt, float opacity = 1.0f, std::optional<Color> foreground = std::nullopt);
    void DrawText(const std::string& text, const Rect& rect, Color color,
                  const std::string& fontName = "微软雅黑", float fontSize = 13.0f,
                  TextAlignment align = TextAlignment::Left,
                  TextVerticalAlignment vAlign = TextVerticalAlignment::Center,
                  FontWeight weight = FontWeight::Normal,
                  bool truncateWithEllipsis = false,
                  FontStyle style = FontStyle::Normal,
                  FontStretch stretch = FontStretch::Normal,
                  bool underline = false,
                  bool strikethrough = false);
    void DrawTextRotated(const std::string& text, Point center, float degrees, Color color,
                         const std::string& fontName = "Segoe UI", float fontSize = 11.0f,
                         FontWeight weight = FontWeight::Normal);

    // 裁剪层
    void PushClip(const Rect& rect);
    void PushRoundedClip(const Rect& rect, float radius);
    void PushEllipseClip(Point center, float radiusX, float radiusY);
    void PopClip();

    // 透明度与变换
    void PushOpacity(float opacity);
    void PopOpacity();
    void PushTransform(const Matrix3x2& transform);
    void PushTranslation(float dx, float dy);
    void PopTransform();

    // 动效专用
    void PushPopupReveal(const Rect& bounds, float progress, Point origin);
    void PopPopupReveal();

    // 自定义扩展与嵌套指令流
    void CustomDraw(std::function<void(GraphicsContext&)> callback, const Rect& bounds = Rect());
    void DrawSubDisplayList(std::shared_ptr<DisplayList> subList, Point offset = {0.0f, 0.0f}, float opacity = 1.0f);

    // 回放与执行
    void Playback(GraphicsContext& ctx) const;
    void PlaybackWithOffset(GraphicsContext& ctx, Point offset, float opacity = 1.0f) const;

    // 指令流批量优化与冗余消除
    void Optimize();

    // 统计度量
    size_t GetOpCount() const { return m_ops.size(); }
    bool IsEmpty() const { return m_ops.empty(); }
    Rect GetBounds() const { return m_bounds; }
    void Clear();

    size_t GetEstimatedMemoryBytes() const;
    uint32_t GetBatchReductionCount() const { return m_batchReductionCount; }

private:
    std::vector<std::unique_ptr<DrawOp>> m_ops;
    Rect m_bounds;
    uint32_t m_batchReductionCount = 0;
};

/**
 * @brief SubDisplayListOp 允许嵌套复用其他 DisplayList
 */
class SubDisplayListOp final : public DrawOp {
public:
    std::shared_ptr<DisplayList> subList;
    Point offset;
    float opacity = 1.0f;

    SubDisplayListOp(std::shared_ptr<DisplayList> list, Point off = {0.0f, 0.0f}, float op = 1.0f)
        : subList(std::move(list)), offset(off), opacity(op) {}

    DrawOpType GetType() const override { return DrawOpType::SubDisplayList; }
    void Execute(GraphicsContext& ctx) const override;
    Rect GetBounds() const override;
};

} // namespace CUI
