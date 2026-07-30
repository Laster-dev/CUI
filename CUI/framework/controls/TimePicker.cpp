#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "TimePicker.h"
#include <sstream>
#include <iomanip>
#include <ctime>

namespace CUI {

TimePicker::TimePicker() {
    time_t t = time(nullptr);
    tm tmVal = {};
    localtime_s(&tmVal, &t);
    m_hour = tmVal.tm_hour;
    m_minute = tmVal.tm_min;

    SetProperty("background", Value(D2D1::ColorF(0x2D / 255.0f, 0x2D / 255.0f, 0x2D / 255.0f, 1.0f)));
    SetProperty("borderBrush", Value(D2D1::ColorF(0x3E / 255.0f, 0x3E / 255.0f, 0x42 / 255.0f, 1.0f)));
    SetProperty("borderThickness", Value(1.0f));
    SetProperty("color", Value(D2D1::ColorF(0xCC / 255.0f, 0xCC / 255.0f, 0xCC / 255.0f, 1.0f)));
    SetProperty("cornerRadius", Value(4.0f));
    SetProperty("width", Value(140.0f));
    SetProperty("height", Value(30.0f));
}

std::vector<PropertyMeta> TimePicker::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "timeStr", "当前时间 (Time)", "时间配置", "string" });
    return metas;
}

Size TimePicker::Measure(Size availableSize) {
    float expW = GetProperty("width").AsFloat(140.0f);
    float expH = GetProperty("height").AsFloat(30.0f);
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

std::string TimePicker::GetFormattedTime() const {
    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << m_hour << ":" << std::setw(2) << std::setfill('0') << m_minute;
    return ss.str();
}

void TimePicker::SetTime(int h, int m) {
    m_hour = std::clamp(h, 0, 23);
    m_minute = std::clamp(m, 0, 59);
    SetProperty("timeStr", Value(GetFormattedTime()));
    m_onTimeChangedEvent.Invoke(this, m_hour, m_minute);
}

void TimePicker::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);

    if (m_isPopupOpen) {
        float popW = 180.0f;
        float popH = 160.0f;
        Rect popRect(m_bounds.x, m_bounds.y + m_bounds.height + 4.0f, popW, popH);

        if (popRect.Contains(pt.x, pt.y)) {
            // Clicked inside time picker selector
            float colW = popW * 0.5f;
            float headerH = 24.0f;
            float listY = popRect.y + headerH;
            float itemH = 26.0f;

            if (pt.y >= listY) {
                int clickedRow = static_cast<int>((pt.y - listY) / itemH);
                if (pt.x < popRect.x + colW) {
                    // Hour column clicked
                    int newHour = (m_hour - 2 + clickedRow + 24) % 24;
                    SetTime(newHour, m_minute);
                } else {
                    // Minute column clicked
                    int newMin = (m_minute / 15 + clickedRow) % 4 * 15;
                    SetTime(m_hour, newMin);
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

void TimePicker::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    std::string text = "🕒 " + GetFormattedTime();
    float fontSize = GetProperty("fontSize").AsFloat(13.0f);
    std::string fontFamily = GetProperty("fontFamily").AsString("Segoe UI");
    D2D1_COLOR_F textColor = GetProperty("color").AsColor(D2D1::ColorF(0xCC / 255.0f, 0xCC / 255.0f, 0xCC / 255.0f, 1.0f));

    Rect textRect(m_bounds.x + 8.0f, m_bounds.y, m_bounds.width - 16.0f, m_bounds.height);
    ctx.DrawText(text, textRect, textColor, fontFamily, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

void TimePicker::OnRenderOverlay(GraphicsContext& ctx) {
    if (!m_isPopupOpen) return;

    float popW = 180.0f;
    float popH = 160.0f;
    Rect popRect(m_bounds.x, m_bounds.y + m_bounds.height + 4.0f, popW, popH);

    D2D1_COLOR_F bg = D2D1::ColorF(0x25 / 255.0f, 0x25 / 255.0f, 0x26 / 255.0f, 1.0f);
    D2D1_COLOR_F border = D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f);
    D2D1_COLOR_F textCol = D2D1::ColorF(0xCC / 255.0f, 0xCC / 255.0f, 0xCC / 255.0f, 1.0f);
    D2D1_COLOR_F selBg = D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f);

    // Draw PopUp Container
    ctx.FillRoundedRect(popRect, 6.0f, bg);
    ctx.DrawRoundedRect(popRect, 6.0f, border, 1.5f);

    // Column Headers (小时 / 分钟)
    float colW = popW * 0.5f;
    Rect h1(popRect.x, popRect.y + 4.0f, colW, 20.0f);
    Rect h2(popRect.x + colW, popRect.y + 4.0f, colW, 20.0f);
    ctx.DrawText("小时 (Hour)", h1, D2D1::ColorF(0x56 / 255.0f, 0x9C / 255.0f, 0xD6 / 255.0f), "Segoe UI", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
    ctx.DrawText("分钟 (Min)", h2, D2D1::ColorF(0x56 / 255.0f, 0x9C / 255.0f, 0xD6 / 255.0f), "Segoe UI", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);

    ctx.DrawLine(Point(popRect.x + colW, popRect.y + 4.0f), Point(popRect.x + colW, popRect.y + popH - 4.0f), D2D1::ColorF(0x3E / 255.0f, 0x3E / 255.0f, 0x42 / 255.0f));

    // Render Hour Options
    float listY = popRect.y + 24.0f;
    float itemH = 26.0f;
    for (int i = 0; i < 5; ++i) {
        int hrVal = (m_hour - 2 + i + 24) % 24;
        Rect cellH(popRect.x + 4.0f, listY + i * itemH, colW - 8.0f, itemH - 2.0f);
        if (hrVal == m_hour) {
            ctx.FillRoundedRect(cellH, 3.0f, selBg);
            ctx.DrawText(std::to_string(hrVal) + " 时", cellH, D2D1::ColorF(1.0f, 1.0f, 1.0f), "Segoe UI", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
        } else {
            ctx.DrawText(std::to_string(hrVal) + " 时", cellH, textCol, "Segoe UI", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }

    // Render Minute Options (00, 15, 30, 45)
    int mins[] = { 0, 15, 30, 45 };
    for (int i = 0; i < 4; ++i) {
        Rect cellM(popRect.x + colW + 4.0f, listY + i * itemH, colW - 8.0f, itemH - 2.0f);
        if (mins[i] == m_minute) {
            ctx.FillRoundedRect(cellM, 3.0f, selBg);
            ctx.DrawText(std::to_string(mins[i]) + " 分", cellM, D2D1::ColorF(1.0f, 1.0f, 1.0f), "Segoe UI", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
        } else {
            ctx.DrawText(std::to_string(mins[i]) + " 分", cellM, textCol, "Segoe UI", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }
}

} // namespace CUI
