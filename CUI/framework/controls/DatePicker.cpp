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
    // Simple cycle for demo/picker toggle
    m_day++;
    if (m_day > 28) {
        m_day = 1;
        m_month++;
        if (m_month > 12) {
            m_month = 1;
            m_year++;
        }
    }
    SetDate(m_year, m_month, m_day);
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

} // namespace CUI
