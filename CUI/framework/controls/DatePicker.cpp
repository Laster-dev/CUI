#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "DatePicker.h"
#include <sstream>
#include <iomanip>

namespace CUI {

DatePicker::DatePicker() {
    time_t t = time(nullptr);
    tm tmVal = {};
    localtime_s(&tmVal, &t);
    m_year = tmVal.tm_year + 1900;
    m_month = tmVal.tm_mon + 1;
    m_day = tmVal.tm_mday;

    SetProperty("background", Value(D2D1::ColorF(0x2D / 255.0f, 0x2D / 255.0f, 0x2D / 255.0f, 1.0f)));
    SetProperty("borderBrush", Value(D2D1::ColorF(0x3E / 255.0f, 0x3E / 255.0f, 0x42 / 255.0f, 1.0f)));
    SetProperty("borderThickness", Value(1.0f));
    SetProperty("color", Value(D2D1::ColorF(0xCC / 255.0f, 0xCC / 255.0f, 0xCC / 255.0f, 1.0f)));
    SetProperty("cornerRadius", Value(4.0f));
    SetProperty("width", Value(160.0f));
    SetProperty("height", Value(30.0f));
}

std::vector<PropertyMeta> DatePicker::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "dateStr", "当前日期 (Date)", "日期配置", "string" });
    return metas;
}

Size DatePicker::Measure(Size availableSize) {
    float expW = GetProperty("width").AsFloat(160.0f);
    float expH = GetProperty("height").AsFloat(30.0f);
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

std::string DatePicker::GetFormattedDate() const {
    std::stringstream ss;
    ss << m_year << "-" << std::setw(2) << std::setfill('0') << m_month << "-" << std::setw(2) << std::setfill('0') << m_day;
    return ss.str();
}

void DatePicker::SetDate(int y, int m, int d) {
    m_year = y;
    m_month = std::clamp(m, 1, 12);
    m_day = std::clamp(d, 1, 31);
    SetProperty("dateStr", Value(GetFormattedDate()));
    m_onDateChangedEvent.Invoke(this, m_year, m_month, m_day);
}

void DatePicker::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);

    if (m_isPopupOpen) {
        // Calculate popup bounds
        float popW = 210.0f;
        float popH = 200.0f;
        Rect popRect(m_bounds.x, m_bounds.y + m_bounds.height + 4.0f, popW, popH);

        if (popRect.Contains(pt.x, pt.y)) {
            // Clicked inside calendar grid
            float headerH = 28.0f;
            float gridY = popRect.y + headerH + 20.0f;
            float cellW = popW / 7.0f;
            float cellH = 24.0f;

            if (pt.y >= gridY) {
                int col = static_cast<int>((pt.x - popRect.x) / cellW);
                int row = static_cast<int>((pt.y - gridY) / cellH);
                int clickedDay = row * 7 + col + 1;
                if (clickedDay >= 1 && clickedDay <= 31) {
                    SetDate(m_year, m_month, clickedDay);
                    m_isPopupOpen = false;
                    return;
                }
            }
        }
        m_isPopupOpen = false;
    } else {
        if (m_bounds.Contains(pt.x, pt.y)) {
            m_isPopupOpen = true;
        }
    }
}

void DatePicker::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    std::string text = "📅 " + GetFormattedDate();
    float fontSize = GetProperty("fontSize").AsFloat(13.0f);
    std::string fontFamily = GetProperty("fontFamily").AsString("Segoe UI");
    D2D1_COLOR_F textColor = GetProperty("color").AsColor(D2D1::ColorF(0xCC / 255.0f, 0xCC / 255.0f, 0xCC / 255.0f, 1.0f));

    Rect textRect(m_bounds.x + 8.0f, m_bounds.y, m_bounds.width - 16.0f, m_bounds.height);
    ctx.DrawText(text, textRect, textColor, fontFamily, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

void DatePicker::OnRenderOverlay(GraphicsContext& ctx) {
    if (!m_isPopupOpen) return;

    float popW = 210.0f;
    float popH = 200.0f;
    Rect popRect(m_bounds.x, m_bounds.y + m_bounds.height + 4.0f, popW, popH);

    D2D1_COLOR_F bg = D2D1::ColorF(0x25 / 255.0f, 0x25 / 255.0f, 0x26 / 255.0f, 1.0f);
    D2D1_COLOR_F border = D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f);
    D2D1_COLOR_F textCol = D2D1::ColorF(0xCC / 255.0f, 0xCC / 255.0f, 0xCC / 255.0f, 1.0f);
    D2D1_COLOR_F selBg = D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f);

    // Draw PopUp Drop Shadow & Background
    ctx.FillRoundedRect(popRect, 6.0f, bg);
    ctx.DrawRoundedRect(popRect, 6.0f, border, 1.5f);

    // Render Calendar Header (Year-Month)
    std::stringstream ssHeader;
    ssHeader << m_year << "年 " << m_month << "月";
    Rect headerRect(popRect.x, popRect.y + 4.0f, popW, 24.0f);
    ctx.DrawText(ssHeader.str(), headerRect, D2D1::ColorF(0x56 / 255.0f, 0x9C / 255.0f, 0xD6 / 255.0f), "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);

    // Render Day-of-Week headers (日 一 二 三 四 五 六)
    const char* weekNames[] = { "日", "一", "二", "三", "四", "五", "六" };
    float cellW = popW / 7.0f;
    float startY = popRect.y + 28.0f;
    for (int w = 0; w < 7; ++w) {
        Rect wRect(popRect.x + w * cellW, startY, cellW, 18.0f);
        ctx.DrawText(weekNames[w], wRect, D2D1::ColorF(0x88 / 255.0f, 0x88 / 255.0f, 0x88 / 255.0f), "Segoe UI", 10.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    // Render Days 1..31
    float gridY = startY + 20.0f;
    float cellH = 24.0f;
    for (int d = 1; d <= 31; ++d) {
        int idx = d - 1;
        int row = idx / 7;
        int col = idx % 7;
        Rect cellRect(popRect.x + col * cellW + 2.0f, gridY + row * cellH, cellW - 4.0f, cellH - 2.0f);

        if (d == m_day) {
            ctx.FillRoundedRect(cellRect, 3.0f, selBg);
            ctx.DrawText(std::to_string(d), cellRect, D2D1::ColorF(1.0f, 1.0f, 1.0f), "Segoe UI", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
        } else {
            ctx.DrawText(std::to_string(d), cellRect, textCol, "Segoe UI", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }
}

} // namespace CUI
