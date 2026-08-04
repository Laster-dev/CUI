#include "TerminalRenderer.h"
#include "UnicodeWidth.h"
#include <algorithm>
#include <cmath>

namespace CUI {
namespace Term {

namespace {
const int kMaxRunCells = 128;

std::vector<std::string> SplitFamilies(const std::string& list) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : list) {
        if (c == ',') {
            parts.push_back(current);
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    parts.push_back(current);

    std::vector<std::string> result;
    for (auto& part : parts) {
        size_t start = 0;
        size_t end = part.size();
        while (start < end && (part[start] == ' ' || part[start] == '\t')) start++;
        while (end > start && (part[end - 1] == ' ' || part[end - 1] == '\t')) end--;
        if (end > start) {
            result.push_back(part.substr(start, end - start));
        }
    }
    return result;
}

TermColor Dim(TermColor c) {
    return TermColor::FromArgb(c.a,
                               static_cast<uint8_t>(c.r / 2),
                               static_cast<uint8_t>(c.g / 2),
                               static_cast<uint8_t>(c.b / 2));
}

Point PointAlong(Point from, Point to, float distance) {
    const float vx = to.x - from.x;
    const float vy = to.y - from.y;
    const float len = std::sqrt(vx * vx + vy * vy);
    if (len < 1e-6f) {
        return from;
    }
    const float t = std::clamp(distance / len, 0.0f, 1.0f);
    return Point(from.x + vx * t, from.y + vy * t);
}

float Distance(Point a, Point b) {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
}
}

TerminalRenderer::TerminalRenderer(const TerminalOptions& options)
    : m_fontFamilyList(options.FontFamily)
    , m_fontSize(options.FontSize)
    , m_colors(options.Theme) {
}

void TerminalRenderer::SetDpi(float pixelsPerDip) {
    if (pixelsPerDip <= 0.0f) {
        pixelsPerDip = 1.0f;
    }
    if (std::fabs(m_pixelsPerDip - pixelsPerDip) < 0.001f) {
        return;
    }
    m_pixelsPerDip = pixelsPerDip;
    m_metricsValid = false;
}

void TerminalRenderer::UpdateFont(const std::string& familyList, float size) {
    m_fontFamilyList = familyList;
    m_fontSize = size;
    m_resolvedFamily.clear();
    m_metricsValid = false;
}

void TerminalRenderer::ApplyTheme(const TerminalTheme& theme) {
    m_colors.SetTheme(theme);
}

void TerminalRenderer::EnsureMetrics(GraphicsContext& ctx) {
    if (m_metricsValid) {
        return;
    }
    Measure(ctx);
}

float TerminalRenderer::Snap(float v) const {
    return std::round(v * m_pixelsPerDip) / m_pixelsPerDip;
}

std::string TerminalRenderer::PickFontFamily(GraphicsContext& ctx, const std::string& familyList) {
    (void)ctx;
    const std::vector<std::string> families = SplitFamilies(familyList);
    auto factory = GraphicsContext::GetSharedWriteFactory();
    if (factory) {
        ComPtr<IDWriteFontCollection> collection;
        if (SUCCEEDED(factory->GetSystemFontCollection(&collection, FALSE)) && collection) {
            for (const auto& family : families) {
                const std::wstring wide = Utf16FromUtf8(family);
                UINT32 index = 0;
                BOOL exists = FALSE;
                if (SUCCEEDED(collection->FindFamilyName(wide.c_str(), &index, &exists)) && exists) {
                    return family;
                }
            }
        }
    }
    return families.empty() ? std::string("Consolas") : families.back();
}

void TerminalRenderer::Measure(GraphicsContext& ctx) {
    if (m_resolvedFamily.empty()) {
        m_resolvedFamily = PickFontFamily(ctx, m_fontFamilyList);
    }

    // Measuring a long run and dividing keeps the per-cell advance exact, so a
    // batched DrawText run lands on the same grid the backgrounds use.
    const int sampleCount = 64;
    const std::string sample(static_cast<size_t>(sampleCount), 'W');
    const Size runSize = ctx.MeasureText(sample, m_resolvedFamily, m_fontSize);
    const Size lineSize = ctx.MeasureText("Wgy", m_resolvedFamily, m_fontSize);

    if (runSize.width > 0.0f) {
        m_cellWidth = runSize.width / static_cast<float>(sampleCount);
    } else {
        m_cellWidth = m_fontSize * 0.6f;
    }

    float height = (std::max)(lineSize.height, runSize.height);
    if (height <= 0.0f) {
        height = m_fontSize * 1.35f;
    }
    m_cellHeight = std::ceil(height * m_pixelsPerDip) / m_pixelsPerDip;

    m_cellWidth = (std::max)(1.0f, m_cellWidth);
    m_cellHeight = (std::max)(1.0f, m_cellHeight);
    m_metricsValid = m_cellWidth > 0.0f && m_cellHeight > 0.0f && runSize.width > 0.0f;
}

void TerminalRenderer::PaintBackground(GraphicsContext& ctx, const Rect& area) {
    Rect r = area;
    r.width = (std::max)(1.0f, r.width);
    r.height = (std::max)(1.0f, r.height);
    ctx.FillRect(r, m_colors.DefaultBackground().ToD2D());
}

void TerminalRenderer::DrawBg(GraphicsContext& ctx, int colStart, int colEnd, float originX,
                              float y, float h, TermColor bg) {
    if (bg == m_colors.DefaultBackground()) {
        return;
    }
    const float x0 = originX + colStart * m_cellWidth;
    const float x1 = originX + colEnd * m_cellWidth;
    ctx.FillRect(Rect(x0, y, x1 - x0, h), bg.ToD2D());
}

void TerminalRenderer::FlushBatch(GraphicsContext& ctx, GlyphRunBatch& batch, float originY) {
    if (!batch.active || batch.text.empty() || batch.cells <= 0) {
        batch.active = false;
        batch.text.clear();
        batch.cells = 0;
        return;
    }

    const DWRITE_FONT_WEIGHT weight = (batch.attrs & CellData::AttrBold) != 0
        ? DWRITE_FONT_WEIGHT_BOLD
        : DWRITE_FONT_WEIGHT_NORMAL;

    // A tiny horizontal slack keeps DirectWrite from clipping the final glyph of
    // the run when its ink extends past the advance (italic-ish faces, powerline).
    const Rect rect(batch.originX, originY,
                    batch.cells * m_cellWidth + m_cellWidth * 0.5f,
                    m_cellHeight);
    ctx.DrawText(batch.text, rect, batch.fg.ToD2D(), m_resolvedFamily, m_fontSize,
                 DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, weight);

    batch.active = false;
    batch.text.clear();
    batch.cells = 0;
}

void TerminalRenderer::AppendBatch(GraphicsContext& ctx, GlyphRunBatch& batch, float originY,
                                   int codePoint, TermColor fg, uint32_t attrs, float x, int cellSpan) {
    const uint32_t styleAttrs = attrs & CellData::AttrBold;

    if (cellSpan != 1) {
        // Wide / fallback glyphs get their own centered draw so the grid stays put.
        FlushBatch(ctx, batch, originY);
        const DWRITE_FONT_WEIGHT weight = (styleAttrs & CellData::AttrBold) != 0
            ? DWRITE_FONT_WEIGHT_BOLD
            : DWRITE_FONT_WEIGHT_NORMAL;
        std::string utf8;
        AppendUtf8CodePoint(utf8, codePoint);
        ctx.DrawText(utf8, Rect(x, originY, cellSpan * m_cellWidth, m_cellHeight),
                     fg.ToD2D(), m_resolvedFamily, m_fontSize,
                     DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, weight);
        return;
    }

    if (batch.active && (batch.fg != fg || batch.attrs != styleAttrs || batch.cells >= kMaxRunCells)) {
        FlushBatch(ctx, batch, originY);
    }

    if (!batch.active) {
        batch.active = true;
        batch.fg = fg;
        batch.attrs = styleAttrs;
        batch.originX = x;
        batch.text.clear();
        batch.cells = 0;
    }

    AppendUtf8CodePoint(batch.text, codePoint);
    batch.cells += 1;
}

void TerminalRenderer::PaintRow(GraphicsContext& ctx, BufferLine& line, int cols,
                                float originX, float originY) {
    const float rowH = (std::max)(1.0f, m_cellHeight);

    // Background runs first so glyphs are never overdrawn.
    int bgStart = -1;
    TermColor bgColor;
    for (int col = 0; col <= cols; ++col) {
        bool hasBg = false;
        TermColor nextBg;
        if (col < cols && col < line.Length()) {
            CellData& cell = line[col];
            // Width==0 trailers still carry the lead cell's Fg/Bg, which keeps CJK
            // backgrounds continuous.
            TermColor bg = m_colors.Resolve(cell.Bg, false);
            TermColor fg = m_colors.Resolve(cell.Fg, true);
            if ((cell.Attrs & CellData::AttrInverse) != 0) {
                std::swap(fg, bg);
            }
            nextBg = bg;
            hasBg = true;
        }

        if (hasBg) {
            if (bgStart < 0) {
                bgStart = col;
                bgColor = nextBg;
            } else if (nextBg != bgColor) {
                DrawBg(ctx, bgStart, col, originX, originY, rowH, bgColor);
                bgStart = col;
                bgColor = nextBg;
            }
        } else if (bgStart >= 0) {
            DrawBg(ctx, bgStart, col, originX, originY, rowH, bgColor);
            bgStart = -1;
        }
    }

    GlyphRunBatch batch;
    for (int col = 0; col < cols && col < line.Length(); ++col) {
        CellData& cell = line[col];
        if (cell.GetWidth() == 0) {
            continue;
        }

        TermColor fg = m_colors.Resolve(cell.Fg, true);
        TermColor bg = m_colors.Resolve(cell.Bg, false);
        if ((cell.Attrs & CellData::AttrInverse) != 0) {
            std::swap(fg, bg);
        }
        if ((cell.Attrs & CellData::AttrInvisible) != 0) {
            fg = bg;
        }
        if ((cell.Attrs & CellData::AttrDim) != 0) {
            fg = Dim(fg);
        }

        const int cells = (std::max)(1, cell.GetWidth());
        const float x = originX + col * m_cellWidth;
        const float width = cells * m_cellWidth;
        const int ch = cell.GetCodePoint();

        if (ch != 0 && ch != ' ') {
            AppendBatch(ctx, batch, originY, ch, fg, cell.Attrs, x, cells);
        } else {
            FlushBatch(ctx, batch, originY);
        }

        if ((cell.Attrs & CellData::AttrUnderline) != 0 || cell.LinkId != 0) {
            FlushBatch(ctx, batch, originY);
            const TermColor penColor = cell.LinkId != 0
                ? TermColor::FromRgb(0x4F, 0xC1, 0xFF)
                : fg;
            const float uy = Snap(originY + rowH - 1.0f);
            ctx.DrawLine(Point(x, uy), Point(x + width, uy), penColor.ToD2D(), 1.0f);
        }

        if ((cell.Attrs & CellData::AttrStrikethrough) != 0) {
            FlushBatch(ctx, batch, originY);
            const float uy = Snap(originY + rowH * 0.5f);
            ctx.DrawLine(Point(x, uy), Point(x + width, uy), fg.ToD2D(), 1.0f);
        }
    }

    FlushBatch(ctx, batch, originY);
}

bool TerminalRenderer::TryGetSelectionSpan(const SelectionModel& selection, int absRow, int cols,
                                           int& startCol, int& endCol) {
    startCol = 0;
    endCol = -1;
    if (!selection.HasSelection) {
        return false;
    }

    int selStartCol = 0, selStartRow = 0, selEndCol = 0, selEndRow = 0;
    selection.GetOrdered(selStartCol, selStartRow, selEndCol, selEndRow);
    if (absRow < selStartRow || absRow > selEndRow) {
        return false;
    }

    if (selStartRow == selEndRow) {
        startCol = selStartCol;
        endCol = selEndCol;
    } else if (absRow == selStartRow) {
        startCol = selStartCol;
        endCol = cols - 1;
    } else if (absRow == selEndRow) {
        startCol = 0;
        endCol = selEndCol;
    } else {
        startCol = 0;
        endCol = cols - 1;
    }

    startCol = std::clamp(startCol, 0, (std::max)(0, cols - 1));
    endCol = std::clamp(endCol, 0, (std::max)(0, cols - 1));
    return endCol >= startCol;
}

void TerminalRenderer::PaintSelectionGeometry(GraphicsContext& ctx, const std::vector<Rect>& rects,
                                              float radius, TermColor color) {
    if (rects.empty()) {
        return;
    }

    if (rects.size() == 1) {
        const Rect& r = rects[0];
        const float rr = (std::min)(radius, (std::min)(r.width, r.height) * 0.5f);
        ctx.FillRoundedRect(r, rr, color.ToD2D());
        return;
    }

    ID2D1DeviceContext* device = ctx.GetD2DContext();
    if (device == nullptr) {
        for (const auto& r : rects) {
            ctx.FillRect(r, color.ToD2D());
        }
        return;
    }

    // Build the merged multi-row outline, then round every corner - matches the
    // StreamGeometry path used by the WPF renderer.
    const size_t n = rects.size();
    std::vector<Point> pts;
    pts.reserve(n * 4);

    pts.push_back(Point(rects[0].x, rects[0].y));
    pts.push_back(Point(rects[0].x + rects[0].width, rects[0].y));

    for (size_t i = 0; i < n; ++i) {
        const Rect& cur = rects[i];
        const float curRight = cur.x + cur.width;
        const float curBottom = cur.y + cur.height;
        if (i + 1 < n) {
            const Rect& next = rects[i + 1];
            const float nextRight = next.x + next.width;
            if (std::fabs(curRight - nextRight) > 0.01f) {
                pts.push_back(Point(curRight, curBottom));
                pts.push_back(Point(nextRight, curBottom));
            }
        } else {
            pts.push_back(Point(curRight, curBottom));
        }
    }

    pts.push_back(Point(rects[n - 1].x, rects[n - 1].y + rects[n - 1].height));

    for (size_t k = n; k-- > 0;) {
        const Rect& cur = rects[k];
        if (k > 0) {
            const Rect& prev = rects[k - 1];
            if (std::fabs(cur.x - prev.x) > 0.01f) {
                pts.push_back(Point(cur.x, cur.y));
                pts.push_back(Point(prev.x, cur.y));
            }
        } else {
            pts.push_back(Point(cur.x, cur.y));
        }
    }

    for (size_t i = pts.size(); i-- > 1;) {
        if (Distance(pts[i], pts[i - 1]) < 0.1f) {
            pts.erase(pts.begin() + static_cast<ptrdiff_t>(i));
        }
    }
    if (pts.size() > 1 && Distance(pts.back(), pts.front()) < 0.1f) {
        pts.pop_back();
    }

    if (pts.size() < 3) {
        for (const auto& r : rects) {
            ctx.FillRect(r, color.ToD2D());
        }
        return;
    }

    ComPtr<ID2D1Factory> factory;
    device->GetFactory(factory.GetAddressOf());
    if (!factory) {
        for (const auto& r : rects) {
            ctx.FillRect(r, color.ToD2D());
        }
        return;
    }

    ComPtr<ID2D1PathGeometry> geometry;
    if (FAILED(factory->CreatePathGeometry(geometry.GetAddressOf())) || !geometry) {
        for (const auto& r : rects) {
            ctx.FillRect(r, color.ToD2D());
        }
        return;
    }

    ComPtr<ID2D1GeometrySink> sink;
    if (FAILED(geometry->Open(sink.GetAddressOf())) || !sink) {
        for (const auto& r : rects) {
            ctx.FillRect(r, color.ToD2D());
        }
        return;
    }

    const size_t count = pts.size();
    std::vector<float> radii(count, 0.0f);
    for (size_t i = 0; i < count; ++i) {
        const Point& prev = pts[(i + count - 1) % count];
        const Point& cur = pts[i];
        const Point& next = pts[(i + 1) % count];
        radii[i] = (std::min)(radius, (std::min)(Distance(prev, cur), Distance(cur, next)) * 0.5f);
    }

    auto before = [&](size_t i) {
        const Point& prev = pts[(i + count - 1) % count];
        const Point& cur = pts[i];
        return PointAlong(prev, cur, Distance(prev, cur) - radii[i]);
    };
    auto after = [&](size_t i) {
        const Point& cur = pts[i];
        const Point& next = pts[(i + 1) % count];
        return PointAlong(cur, next, radii[i]);
    };

    sink->SetFillMode(D2D1_FILL_MODE_WINDING);
    const Point start = before(0);
    sink->BeginFigure(D2D1::Point2F(start.x, start.y), D2D1_FIGURE_BEGIN_FILLED);

    for (size_t i = 0; i < count; ++i) {
        if (i > 0) {
            const Point p = before(i);
            sink->AddLine(D2D1::Point2F(p.x, p.y));
        }

        const Point& prev = pts[(i + count - 1) % count];
        const Point& cur = pts[i];
        const Point& next = pts[(i + 1) % count];
        const float r = radii[i];

        if (r < 0.25f) {
            sink->AddLine(D2D1::Point2F(cur.x, cur.y));
            continue;
        }

        const float v1x = cur.x - prev.x;
        const float v1y = cur.y - prev.y;
        const float v2x = next.x - cur.x;
        const float v2y = next.y - cur.y;
        const float cross = v1x * v2y - v1y * v2x;

        const Point arcEnd = after(i);
        D2D1_ARC_SEGMENT arc = {};
        arc.point = D2D1::Point2F(arcEnd.x, arcEnd.y);
        arc.size = D2D1::SizeF(r, r);
        arc.rotationAngle = 0.0f;
        arc.sweepDirection = cross < 0.0f
            ? D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE
            : D2D1_SWEEP_DIRECTION_CLOCKWISE;
        arc.arcSize = D2D1_ARC_SIZE_SMALL;
        sink->AddArc(arc);
    }

    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    sink->Close();

    ID2D1SolidColorBrush* brush = ctx.GetResources().GetSolidBrush(color.ToD2D());
    if (brush) {
        device->FillGeometry(geometry.Get(), brush);
    }
}

void TerminalRenderer::PaintSelectionText(GraphicsContext& ctx, Terminal& terminal,
                                          SelectionModel& selection, int rows, int cols,
                                          float originX, float originY) {
    TerminalBuffer& buf = terminal.Buffers().Active();
    const TermColor fg = m_colors.SelectionForeground();

    for (int row = 0; row < rows; ++row) {
        const int absRow = buf.BaseY() - buf.YDisp + row;
        int startCol = 0;
        int endCol = -1;
        if (!TryGetSelectionSpan(selection, absRow, cols, startCol, endCol)) {
            continue;
        }

        BufferLine& line = buf.GetViewportLine(row);
        const float rowY = originY + row * m_cellHeight;
        GlyphRunBatch batch;

        for (int col = startCol; col <= endCol && col < line.Length(); ++col) {
            CellData& cell = line[col];
            if (cell.GetWidth() == 0) {
                continue;
            }

            const int cells = (std::max)(1, cell.GetWidth());
            const float x = originX + col * m_cellWidth;
            const int ch = cell.GetCodePoint();

            if (ch != 0 && ch != ' ') {
                AppendBatch(ctx, batch, rowY, ch, fg, cell.Attrs, x, cells);
            } else {
                FlushBatch(ctx, batch, rowY);
            }
        }

        FlushBatch(ctx, batch, rowY);
    }
}

void TerminalRenderer::PaintOverlay(GraphicsContext& ctx, Terminal& terminal, bool cursorOn,
                                    float originX, float originY, const std::wstring& imePreedit) {
    TerminalBuffer& buf = terminal.Buffers().Active();
    const int cols = terminal.Cols();
    const int rows = terminal.Rows();
    SelectionModel& selection = terminal.Selection().Model();
    // Do not Normalize() here — swapping Start/End during drag drops the
    // selection anchor and breaks reverse (bottom-right → top-left) selection.

    if (selection.HasSelection) {
        std::vector<Rect> rects;
        for (int row = 0; row < rows; ++row) {
            const int absRow = buf.BaseY() - buf.YDisp + row;
            int startCol = 0;
            int endCol = -1;
            if (!TryGetSelectionSpan(selection, absRow, cols, startCol, endCol)) {
                continue;
            }

            const float y0 = originY + row * m_cellHeight;
            const float y1 = originY + (row + 1) * m_cellHeight;
            const float x0 = originX + startCol * m_cellWidth;
            const float x1 = originX + (endCol + 1) * m_cellWidth;
            rects.push_back(Rect(x0, y0, (std::max)(1.0f, x1 - x0), (std::max)(1.0f, y1 - y0)));
        }

        if (!rects.empty()) {
            const float radius = (std::max)(3.0f, (std::min)(6.0f, m_cellHeight * 0.28f));
            PaintSelectionGeometry(ctx, rects, radius, m_colors.SelectionBackground());
            PaintSelectionText(ctx, terminal, selection, rows, cols, originX, originY);
        }
    }

    if (cursorOn && terminal.Input().CursorVisible() && buf.YDisp == 0) {
        const float cx = originX + buf.CursorX * m_cellWidth;
        const float cy0 = originY + buf.CursorY * m_cellHeight;
        const TermColor c = m_colors.CursorColor();
        const TermColor cursorColor = TermColor::FromArgb(0xA0, c.r, c.g, c.b);
        const int style = terminal.Input().CursorStyle();
        if (style == 1) { // underline
            const float h = (std::max)(2.0f, Snap(m_cellHeight * 0.12f));
            ctx.FillRect(Rect(cx, cy0 + m_cellHeight - h, m_cellWidth, h), cursorColor.ToD2D());
        } else if (style == 2) { // bar
            ctx.FillRect(Rect(cx, cy0, (std::max)(2.0f, Snap(m_cellWidth * 0.15f)), m_cellHeight),
                         cursorColor.ToD2D());
        } else { // block, drawn as the WPF renderer does (thin bar highlight)
            ctx.FillRect(Rect(cx, cy0, (std::max)(2.0f, Snap(m_cellWidth * 0.15f)), m_cellHeight),
                         cursorColor.ToD2D());
        }
    }

    if (!imePreedit.empty() && buf.YDisp == 0) {
        const float x = originX + buf.CursorX * m_cellWidth;
        const float y0 = originY + buf.CursorY * m_cellHeight;
        const std::string utf8 = Utf8FromUtf16(imePreedit);
        const Size measured = ctx.MeasureText(utf8, m_resolvedFamily, m_fontSize);
        const float w = (std::max)(measured.width, m_cellWidth);
        ctx.FillRect(Rect(x, y0, w, m_cellHeight),
                     TermColor::FromArgb(0x60, 0x80, 0x80, 0xFF).ToD2D());
        ctx.DrawText(utf8, Rect(x, y0, w, m_cellHeight), m_colors.DefaultForeground().ToD2D(),
                     m_resolvedFamily, m_fontSize,
                     DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
}

void TerminalRenderer::Render(GraphicsContext& ctx, Terminal& terminal, bool cursorOn,
                              const Rect& area, const std::wstring& imePreedit) {
    EnsureMetrics(ctx);
    PaintBackground(ctx, area);

    TerminalBuffer& buf = terminal.Buffers().Active();
    const int cols = terminal.Cols();
    const int rows = terminal.Rows();

    for (int row = 0; row < rows; ++row) {
        BufferLine& line = buf.GetViewportLine(row);
        PaintRow(ctx, line, cols, area.x, area.y + row * m_cellHeight);
        line.SetIsDirty(false);
    }
    PaintOverlay(ctx, terminal, cursorOn, area.x, area.y, imePreedit);
}

} // namespace Term
} // namespace CUI
