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
    m_minute += 15;
    if (m_minute >= 60) {
        m_minute = 0;
        m_hour = (m_hour + 1) % 24;
    }
    SetTime(m_hour, m_minute);
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

} // namespace CUI
