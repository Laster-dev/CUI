#pragma once
#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <functional>
#include "../core/Value.h"
#include "../text/TextTypes.h"
#include "GraphicsContext.h"

namespace CUI {

class DisplayList;

enum class DrawOpType : uint16_t {
    ClearRect,
    DrawRect,
    FillRect,
    DrawRoundedRect,
    FillRoundedRect,
    DrawLine,
    DrawSmoothLine,
    DrawSmoothArc,
    DrawPolygon,
    FillPolygon,
    FillRoundedGeometry,
    DrawChevron,
    DrawIcon,
    DrawSvg,
    DrawText,
    DrawTextRotated,
    PushClip,
    PushRoundedClip,
    PushEllipseClip,
    PopClip,
    PushOpacity,
    PopOpacity,
    PushTransform,
    PushTranslation,
    PopTransform,
    PushPopupReveal,
    PopPopupReveal,
    Custom,
    SubDisplayList
};

/**
 * @brief 绘图指令虚基类 (DrawOp)
 * 代表 DisplayList 中的单个不可变原子渲染操作。
 */
class DrawOp {
public:
    virtual ~DrawOp() = default;
    virtual DrawOpType GetType() const = 0;
    virtual void Execute(GraphicsContext& ctx) const = 0;
    virtual Rect GetBounds() const { return Rect(); }
    virtual bool CanBatchWith(const DrawOp& /*next*/) const { return false; }
};

class ClearRectOp final : public DrawOp {
public:
    Rect rect;
    Color color = Color::Transparent;

    ClearRectOp(Rect r, Color c) : rect(r), color(c) {}
    DrawOpType GetType() const override { return DrawOpType::ClearRect; }
    void Execute(GraphicsContext& ctx) const override { ctx.ClearRect(rect, color); }
    Rect GetBounds() const override { return rect; }
};

class DrawRectOp final : public DrawOp {
public:
    Rect rect;
    Color color;
    float strokeWidth = 1.0f;

    DrawRectOp(Rect r, Color c, float s = 1.0f) : rect(r), color(c), strokeWidth(s) {}
    DrawOpType GetType() const override { return DrawOpType::DrawRect; }
    void Execute(GraphicsContext& ctx) const override { ctx.DrawRect(rect, color, strokeWidth); }
    Rect GetBounds() const override { return rect.Inflate(strokeWidth * 0.5f); }
};

class FillRectOp final : public DrawOp {
public:
    Rect rect;
    Color color;

    FillRectOp(Rect r, Color c) : rect(r), color(c) {}
    DrawOpType GetType() const override { return DrawOpType::FillRect; }
    void Execute(GraphicsContext& ctx) const override { ctx.FillRect(rect, color); }
    Rect GetBounds() const override { return rect; }
    bool CanBatchWith(const DrawOp& next) const override {
        if (next.GetType() == DrawOpType::FillRect) {
            const auto& o = static_cast<const FillRectOp&>(next);
            return color == o.color;
        }
        return false;
    }
};

class DrawRoundedRectOp final : public DrawOp {
public:
    Rect rect;
    float radius = 0.0f;
    Color color;
    float strokeWidth = 1.0f;

    DrawRoundedRectOp(Rect r, float rad, Color c, float s = 1.0f)
        : rect(r), radius(rad), color(c), strokeWidth(s) {}
    DrawOpType GetType() const override { return DrawOpType::DrawRoundedRect; }
    void Execute(GraphicsContext& ctx) const override { ctx.DrawRoundedRect(rect, radius, color, strokeWidth); }
    Rect GetBounds() const override { return rect.Inflate(strokeWidth * 0.5f); }
};

class FillRoundedRectOp final : public DrawOp {
public:
    Rect rect;
    float radius = 0.0f;
    Color color;

    FillRoundedRectOp(Rect r, float rad, Color c)
        : rect(r), radius(rad), color(c) {}
    DrawOpType GetType() const override { return DrawOpType::FillRoundedRect; }
    void Execute(GraphicsContext& ctx) const override { ctx.FillRoundedRect(rect, radius, color); }
    Rect GetBounds() const override { return rect; }
    bool CanBatchWith(const DrawOp& next) const override {
        if (next.GetType() == DrawOpType::FillRoundedRect) {
            const auto& o = static_cast<const FillRoundedRectOp&>(next);
            return color == o.color && std::abs(radius - o.radius) < 0.01f;
        }
        return false;
    }
};

class DrawLineOp final : public DrawOp {
public:
    Point p1, p2;
    Color color;
    float strokeWidth = 1.0f;

    DrawLineOp(Point a, Point b, Color c, float s = 1.0f) : p1(a), p2(b), color(c), strokeWidth(s) {}
    DrawOpType GetType() const override { return DrawOpType::DrawLine; }
    void Execute(GraphicsContext& ctx) const override { ctx.DrawLine(p1, p2, color, strokeWidth); }
    Rect GetBounds() const override {
        float minX = (std::min)(p1.x, p2.x);
        float minY = (std::min)(p1.y, p2.y);
        float maxX = (std::max)(p1.x, p2.x);
        float maxY = (std::max)(p1.y, p2.y);
        return Rect(minX, minY, maxX - minX, maxY - minY).Inflate(strokeWidth * 0.5f);
    }
};

class DrawSmoothLineOp final : public DrawOp {
public:
    Point p1, p2;
    Color color;
    float strokeWidth = 1.5f;

    DrawSmoothLineOp(Point a, Point b, Color c, float s = 1.5f) : p1(a), p2(b), color(c), strokeWidth(s) {}
    DrawOpType GetType() const override { return DrawOpType::DrawSmoothLine; }
    void Execute(GraphicsContext& ctx) const override { ctx.DrawSmoothLine(p1, p2, color, strokeWidth); }
    Rect GetBounds() const override {
        float minX = (std::min)(p1.x, p2.x);
        float minY = (std::min)(p1.y, p2.y);
        float maxX = (std::max)(p1.x, p2.x);
        float maxY = (std::max)(p1.y, p2.y);
        return Rect(minX, minY, maxX - minX, maxY - minY).Inflate(strokeWidth * 0.5f);
    }
};

class DrawSmoothArcOp final : public DrawOp {
public:
    Point center;
    float radius = 0.0f;
    float startRad = 0.0f;
    float sweepRad = 0.0f;
    Color color;
    float strokeWidth = 1.5f;

    DrawSmoothArcOp(Point c, float r, float sRad, float swRad, Color col, float s = 1.5f)
        : center(c), radius(r), startRad(sRad), sweepRad(swRad), color(col), strokeWidth(s) {}
    DrawOpType GetType() const override { return DrawOpType::DrawSmoothArc; }
    void Execute(GraphicsContext& ctx) const override { ctx.DrawSmoothArc(center, radius, startRad, sweepRad, color, strokeWidth); }
    Rect GetBounds() const override {
        return Rect(center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f).Inflate(strokeWidth * 0.5f);
    }
};

class DrawPolygonOp final : public DrawOp {
public:
    std::vector<Point> points;
    Color color;
    float strokeWidth = 1.0f;

    DrawPolygonOp(std::vector<Point> pts, Color c, float s = 1.0f) : points(std::move(pts)), color(c), strokeWidth(s) {}
    DrawOpType GetType() const override { return DrawOpType::DrawPolygon; }
    void Execute(GraphicsContext& ctx) const override {
        if (!points.empty()) ctx.DrawPolygon(points.data(), static_cast<int>(points.size()), color, strokeWidth);
    }
};

class FillPolygonOp final : public DrawOp {
public:
    std::vector<Point> points;
    Color color;

    FillPolygonOp(std::vector<Point> pts, Color c) : points(std::move(pts)), color(c) {}
    DrawOpType GetType() const override { return DrawOpType::FillPolygon; }
    void Execute(GraphicsContext& ctx) const override {
        if (!points.empty()) ctx.FillPolygon(points.data(), static_cast<int>(points.size()), color);
    }
};

class FillRoundedGeometryOp final : public DrawOp {
public:
    std::vector<Point> points;
    float radius = 0.0f;
    Color color;

    FillRoundedGeometryOp(std::vector<Point> pts, float rad, Color c)
        : points(std::move(pts)), radius(rad), color(c) {}
    DrawOpType GetType() const override { return DrawOpType::FillRoundedGeometry; }
    void Execute(GraphicsContext& ctx) const override { ctx.FillRoundedGeometry(points, radius, color); }
};

class DrawChevronOp final : public DrawOp {
public:
    Rect bounds;
    Color color;
    GraphicsContext::ChevronDirection direction;
    float strokeWidth = 1.6f;

    DrawChevronOp(Rect b, Color c, GraphicsContext::ChevronDirection dir, float s = 1.6f)
        : bounds(b), color(c), direction(dir), strokeWidth(s) {}
    DrawOpType GetType() const override { return DrawOpType::DrawChevron; }
    void Execute(GraphicsContext& ctx) const override { ctx.DrawChevron(bounds, color, direction, strokeWidth); }
    Rect GetBounds() const override { return bounds; }
};

class DrawIconOp final : public DrawOp {
public:
    std::string icon;
    Rect dest;
    Color color;
    float opacity = 1.0f;
    std::string glyphFont = "Segoe UI Emoji";
    float glyphSize = 0.0f;

    DrawIconOp(std::string ic, Rect d, Color c, float op = 1.0f, std::string font = "Segoe UI Emoji", float sz = 0.0f)
        : icon(std::move(ic)), dest(d), color(c), opacity(op), glyphFont(std::move(font)), glyphSize(sz) {}
    DrawOpType GetType() const override { return DrawOpType::DrawIcon; }
    void Execute(GraphicsContext& ctx) const override { ctx.DrawIcon(icon, dest, color, opacity, glyphFont, glyphSize); }
    Rect GetBounds() const override { return dest; }
};

class DrawSvgOp final : public DrawOp {
public:
    std::string source;
    Rect dest;
    std::optional<Color> tint;
    float opacity = 1.0f;
    std::optional<Color> foreground;

    DrawSvgOp(std::string src, Rect d, std::optional<Color> t = std::nullopt, float op = 1.0f, std::optional<Color> fg = std::nullopt)
        : source(std::move(src)), dest(d), tint(t), opacity(op), foreground(fg) {}
    DrawOpType GetType() const override { return DrawOpType::DrawSvg; }
    void Execute(GraphicsContext& ctx) const override {
        const Color* pTint = tint.has_value() ? &tint.value() : nullptr;
        const Color* pFg = foreground.has_value() ? &foreground.value() : nullptr;
        ctx.DrawSvg(source, dest, pTint, opacity, pFg);
    }
    Rect GetBounds() const override { return dest; }
};

class DrawTextOp final : public DrawOp {
public:
    std::string text;
    Rect rect;
    Color color;
    std::string fontName = "微软雅黑";
    float fontSize = 13.0f;
    TextAlignment align = TextAlignment::Left;
    TextVerticalAlignment vAlign = TextVerticalAlignment::Center;
    FontWeight weight = FontWeight::Normal;
    bool truncateWithEllipsis = false;
    FontStyle style = FontStyle::Normal;
    FontStretch stretch = FontStretch::Normal;
    bool underline = false;
    bool strikethrough = false;

    DrawTextOp(std::string t, Rect r, Color c, std::string fn = "微软雅黑", float sz = 13.0f,
               TextAlignment a = TextAlignment::Left, TextVerticalAlignment va = TextVerticalAlignment::Center,
               FontWeight w = FontWeight::Normal, bool trunc = false,
               FontStyle st = FontStyle::Normal, FontStretch str = FontStretch::Normal,
               bool u = false, bool s = false)
        : text(std::move(t)), rect(r), color(c), fontName(std::move(fn)), fontSize(sz),
          align(a), vAlign(va), weight(w), truncateWithEllipsis(trunc),
          style(st), stretch(str), underline(u), strikethrough(s) {}

    DrawOpType GetType() const override { return DrawOpType::DrawText; }
    void Execute(GraphicsContext& ctx) const override {
        ctx.DrawText(text, rect, color, fontName, fontSize, align, vAlign, weight, truncateWithEllipsis, style, stretch, underline, strikethrough);
    }
    Rect GetBounds() const override { return rect; }
};

class DrawTextRotatedOp final : public DrawOp {
public:
    std::string text;
    Point center;
    float degrees = 0.0f;
    Color color;
    std::string fontName = "Segoe UI";
    float fontSize = 11.0f;
    FontWeight weight = FontWeight::Normal;

    DrawTextRotatedOp(std::string t, Point c, float deg, Color col, std::string fn = "Segoe UI", float sz = 11.0f, FontWeight w = FontWeight::Normal)
        : text(std::move(t)), center(c), degrees(deg), color(col), fontName(std::move(fn)), fontSize(sz), weight(w) {}

    DrawOpType GetType() const override { return DrawOpType::DrawTextRotated; }
    void Execute(GraphicsContext& ctx) const override {
        ctx.DrawTextRotated(text, center, degrees, color, fontName, fontSize, weight);
    }
};

class PushClipOp final : public DrawOp {
public:
    Rect rect;
    explicit PushClipOp(Rect r) : rect(r) {}
    DrawOpType GetType() const override { return DrawOpType::PushClip; }
    void Execute(GraphicsContext& ctx) const override { ctx.PushClip(rect); }
    Rect GetBounds() const override { return rect; }
};

class PushRoundedClipOp final : public DrawOp {
public:
    Rect rect;
    float radius = 0.0f;
    PushRoundedClipOp(Rect r, float rad) : rect(r), radius(rad) {}
    DrawOpType GetType() const override { return DrawOpType::PushRoundedClip; }
    void Execute(GraphicsContext& ctx) const override { ctx.PushRoundedClip(rect, radius); }
    Rect GetBounds() const override { return rect; }
};

class PushEllipseClipOp final : public DrawOp {
public:
    Point center;
    float radiusX = 0.0f;
    float radiusY = 0.0f;
    PushEllipseClipOp(Point c, float rx, float ry) : center(c), radiusX(rx), radiusY(ry) {}
    DrawOpType GetType() const override { return DrawOpType::PushEllipseClip; }
    void Execute(GraphicsContext& ctx) const override { ctx.PushEllipseClip(center, radiusX, radiusY); }
    Rect GetBounds() const override { return Rect(center.x - radiusX, center.y - radiusY, radiusX * 2.0f, radiusY * 2.0f); }
};

class PopClipOp final : public DrawOp {
public:
    DrawOpType GetType() const override { return DrawOpType::PopClip; }
    void Execute(GraphicsContext& ctx) const override { ctx.PopClip(); }
};

class PushOpacityOp final : public DrawOp {
public:
    float opacity = 1.0f;
    explicit PushOpacityOp(float op) : opacity(op) {}
    DrawOpType GetType() const override { return DrawOpType::PushOpacity; }
    void Execute(GraphicsContext& ctx) const override { ctx.PushOpacity(opacity); }
};

class PopOpacityOp final : public DrawOp {
public:
    DrawOpType GetType() const override { return DrawOpType::PopOpacity; }
    void Execute(GraphicsContext& ctx) const override { ctx.PopOpacity(); }
};

class PushTransformOp final : public DrawOp {
public:
    Matrix3x2 transform;
    explicit PushTransformOp(const Matrix3x2& m) : transform(m) {}
    DrawOpType GetType() const override { return DrawOpType::PushTransform; }
    void Execute(GraphicsContext& ctx) const override { ctx.PushTransform(transform); }
};

class PushTranslationOp final : public DrawOp {
public:
    float dx = 0.0f;
    float dy = 0.0f;
    PushTranslationOp(float x, float y) : dx(x), dy(y) {}
    DrawOpType GetType() const override { return DrawOpType::PushTranslation; }
    void Execute(GraphicsContext& ctx) const override { ctx.PushTranslation(dx, dy); }
};

class PopTransformOp final : public DrawOp {
public:
    DrawOpType GetType() const override { return DrawOpType::PopTransform; }
    void Execute(GraphicsContext& ctx) const override { ctx.PopTransform(); }
};

class PushPopupRevealOp final : public DrawOp {
public:
    Rect bounds;
    float progress = 1.0f;
    Point origin;
    PushPopupRevealOp(Rect b, float p, Point orig) : bounds(b), progress(p), origin(orig) {}
    DrawOpType GetType() const override { return DrawOpType::PushPopupReveal; }
    void Execute(GraphicsContext& ctx) const override { ctx.PushPopupReveal(bounds, progress, origin); }
    Rect GetBounds() const override { return bounds; }
};

class PopPopupRevealOp final : public DrawOp {
public:
    DrawOpType GetType() const override { return DrawOpType::PopPopupReveal; }
    void Execute(GraphicsContext& ctx) const override { ctx.PopPopupReveal(); }
};

class CustomDrawOp final : public DrawOp {
public:
    std::function<void(GraphicsContext&)> callback;
    Rect bounds;

    CustomDrawOp(std::function<void(GraphicsContext&)> cb, Rect b = Rect())
        : callback(std::move(cb)), bounds(b) {}
    DrawOpType GetType() const override { return DrawOpType::Custom; }
    void Execute(GraphicsContext& ctx) const override {
        if (callback) callback(ctx);
    }
    Rect GetBounds() const override { return bounds; }
};

} // namespace CUI
