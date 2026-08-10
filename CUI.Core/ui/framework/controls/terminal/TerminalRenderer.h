#pragma once
#include "../../render/GraphicsContext.h"
#include "AnsiColors.h"
#include "SelectionService.h"
#include "Terminal.h"
#include "TerminalOptions.h"
#include <string>
#include <vector>

namespace CUI {
namespace Term {

// Paints the terminal grid through the CUI GraphicsContext.
// Text is emitted as batched runs of same-styled cells (the DirectWrite analogue
// of the WPF GlyphRun batching) so a full 80x24 frame costs a few dozen draws.
class TerminalRenderer {
public:
    explicit TerminalRenderer(const TerminalOptions& options);

    float CellWidth() const { return m_cellWidth; }
    float CellHeight() const { return m_cellHeight; }
    AnsiColors& Colors() { return m_colors; }
    const AnsiColors& Colors() const { return m_colors; }

    void SetDpi(float pixelsPerDip);
    void UpdateFont(const std::string& familyList, float size);
    void ApplyTheme(const TerminalTheme& theme);

    // Resolves cell metrics from the active DirectWrite font. Cheap after the
    // first call; re-runs when the font, size, or DPI changed.
    void EnsureMetrics(GraphicsContext& ctx);
    bool HasMetrics() const { return m_metricsValid; }

    void PaintBackground(GraphicsContext& ctx, const Rect& area);

    // Paints one buffer line; origin is the top-left of the row in control space.
    void PaintRow(GraphicsContext& ctx, BufferLine& line, int cols, float originX, float originY);

    void PaintOverlay(GraphicsContext& ctx, Terminal& terminal, bool cursorOn,
                      float originX, float originY, const std::wstring& imePreedit);

    // Full-frame paint; the control normally uses the dirty-row path instead.
    void Render(GraphicsContext& ctx, Terminal& terminal, bool cursorOn,
                const Rect& area, const std::wstring& imePreedit);

    static bool TryGetSelectionSpan(const SelectionModel& selection, int absRow, int cols,
                                    int& startCol, int& endCol);

private:
    struct GlyphRunBatch {
        bool active = false;
        std::string text;
        float originX = 0.0f;
        TermColor fg;
        uint32_t attrs = 0;
        int cells = 0;
    };

    void FlushBatch(GraphicsContext& ctx, GlyphRunBatch& batch, float originY);
    void AppendBatch(GraphicsContext& ctx, GlyphRunBatch& batch, float originY,
                     int codePoint, TermColor fg, uint32_t attrs, float x, int cellSpan);

    void DrawBg(GraphicsContext& ctx, int colStart, int colEnd, float originX, float y,
                float h, TermColor bg);
    void PaintSelectionText(GraphicsContext& ctx, Terminal& terminal, SelectionModel& selection,
                            int rows, int cols, float originX, float originY);
    void PaintSelectionGeometry(GraphicsContext& ctx, const std::vector<Rect>& rects, float radius,
                                TermColor color);

    void Measure(GraphicsContext& ctx);
    static std::string PickFontFamily(GraphicsContext& ctx, const std::string& familyList);
    float Snap(float v) const;

    std::string m_fontFamilyList;
    std::string m_resolvedFamily = "Consolas";
    float m_fontSize = 14.0f;
    float m_cellWidth = 8.0f;
    float m_cellHeight = 16.0f;
    float m_pixelsPerDip = 1.0f;
    bool m_metricsValid = false;
    AnsiColors m_colors;
};

} // namespace Term
} // namespace CUI
