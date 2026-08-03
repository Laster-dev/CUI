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

static bool IsLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int GetDaysInMonth(int year, int month) {
    static const int days[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month == 2 && IsLeapYear(year)) return 29;
    if (month >= 1 && month <= 12) return days[month];
    return 31;
}

static int GetFirstDayOfWeek(int year, int month) {
    tm time_in = {};
    time_in.tm_year = year - 1900;
    time_in.tm_mon = month - 1;
    time_in.tm_mday = 1;
    mktime(&time_in);
    return time_in.tm_wday; // 0 = Sunday, 1 = Monday, ...
}

void DatePicker::SetDate(int y, int m, int d) {
    m_year = y;
    m_month = std::clamp(m, 1, 12);
    int daysInMonth = GetDaysInMonth(m_year, m_month);
    m_day = std::clamp(d, 1, daysInMonth);
    SetProperty("dateStr", Value(GetFormattedDate()));
    m_onDateChangedEvent.Invoke(this, m_year, m_month, m_day);
}

UIElement* DatePicker::OnHitTestOverlay(float x, float y) {
    float progress = UIElement::AreAnimationsEnabled() ? m_popupAnim.Current() : (m_isPopupOpen ? 1.0f : 0.0f);
    if (progress <= 0.5f) return nullptr;
    float popW = 240.0f;
    float popH = 240.0f;
    Rect popRect(m_bounds.x, m_bounds.y + m_bounds.height + 4.0f, popW, popH);
    if (popRect.Contains(x, y)) {
        return this;
    }
    return nullptr;
}

bool DatePicker::OnAnimationTick() {
    float dt = UIElement::GetAnimationDeltaSeconds();
    m_popupAnim.SetTarget(m_isPopupOpen ? 1.0f : 0.0f);
    return m_popupAnim.Tick(dt, AnimationSpec{ 0.55f, 0.01f });
}

bool DatePicker::HasSelfAnimation() const {
    return std::abs(m_popupAnim.Target() - m_popupAnim.Current()) > 0.001f;
}

void DatePicker::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);

    if (m_isPopupOpen) {
        float popW = 240.0f;
        float popH = 240.0f;
        Rect popRect(m_bounds.x, m_bounds.y + m_bounds.height + 4.0f, popW, popH);

        if (popRect.Contains(pt.x, pt.y)) {
            // Header Buttons: Prev / Year / Month / Next
            Rect btnPrev(popRect.x + 4.0f, popRect.y + 6.0f, 22.0f, 22.0f);
            Rect btnNext(popRect.x + popW - 26.0f, popRect.y + 6.0f, 22.0f, 22.0f);
            float midW = (popW - 56.0f) * 0.5f;
            Rect yrRect(popRect.x + 28.0f, popRect.y + 6.0f, midW, 22.0f);
            Rect moRect(popRect.x + 28.0f + midW, popRect.y + 6.0f, midW, 22.0f);

            // Click Year button -> YearGrid mode directly
            if (yrRect.Contains(pt.x, pt.y)) {
                m_viewMode = DatePickerViewMode::YearGrid;
                m_viewStartYear = (m_year / 12) * 12;
                return;
            }
            // Click Month button -> MonthGrid mode directly
            if (moRect.Contains(pt.x, pt.y)) {
                m_viewMode = DatePickerViewMode::MonthGrid;
                return;
            }

            // Arrow Prev/Next behavior dependent on ViewMode
            if (btnPrev.Contains(pt.x, pt.y)) {
                if (m_viewMode == DatePickerViewMode::DayGrid) {
                    int newM = m_month - 1;
                    int newY = m_year;
                    if (newM < 1) { newM = 12; newY--; }
                    SetDate(newY, newM, m_day);
                } else if (m_viewMode == DatePickerViewMode::MonthGrid) {
                    SetDate(m_year - 1, m_month, m_day);
                } else {
                    m_viewStartYear -= 12;
                }
                return;
            }

            if (btnNext.Contains(pt.x, pt.y)) {
                if (m_viewMode == DatePickerViewMode::DayGrid) {
                    int newM = m_month + 1;
                    int newY = m_year;
                    if (newM > 12) { newM = 1; newY++; }
                    SetDate(newY, newM, m_day);
                } else if (m_viewMode == DatePickerViewMode::MonthGrid) {
                    SetDate(m_year + 1, m_month, m_day);
                } else {
                    m_viewStartYear += 12;
                }
                return;
            }

            // Content Grid Click Handling
            float bodyY = popRect.y + 36.0f;
            if (m_viewMode == DatePickerViewMode::DayGrid) {
                float gridY = bodyY + 20.0f;
                float cellW = popW / 7.0f;
                float cellH = 26.0f;
                if (pt.y >= gridY) {
                    int col = static_cast<int>((pt.x - popRect.x) / cellW);
                    int row = static_cast<int>((pt.y - gridY) / cellH);
                    int firstWday = GetFirstDayOfWeek(m_year, m_month);
                    int clickedIdx = row * 7 + col;
                    int clickedDay = clickedIdx - firstWday + 1;
                    int maxDays = GetDaysInMonth(m_year, m_month);
                    if (clickedDay >= 1 && clickedDay <= maxDays) {
                        SetDate(m_year, m_month, clickedDay);
                        m_isPopupOpen = false;
                        return;
                    }
                }
            } else if (m_viewMode == DatePickerViewMode::MonthGrid) {
                // 3x4 Month Grid
                float cellW = popW / 3.0f;
                float cellH = (popH - 44.0f) / 4.0f;
                int col = static_cast<int>((pt.x - popRect.x) / cellW);
                int row = static_cast<int>((pt.y - bodyY) / cellH);
                int selectedMonth = row * 3 + col + 1;
                if (selectedMonth >= 1 && selectedMonth <= 12) {
                    SetDate(m_year, selectedMonth, m_day);
                    m_viewMode = DatePickerViewMode::DayGrid;
                    return;
                }
            } else if (m_viewMode == DatePickerViewMode::YearGrid) {
                // 3x4 Year Grid (12 years per page)
                float cellW = popW / 3.0f;
                float cellH = (popH - 44.0f) / 4.0f;
                int col = static_cast<int>((pt.x - popRect.x) / cellW);
                int row = static_cast<int>((pt.y - bodyY) / cellH);
                int selectedYear = m_viewStartYear + row * 3 + col;
                SetDate(selectedYear, m_month, m_day);
                m_viewMode = DatePickerViewMode::MonthGrid;
                return;
            }
            return;
        }
        m_isPopupOpen = false;
    } else {
        if (m_bounds.Contains(pt.x, pt.y)) {
            m_isPopupOpen = true;
            m_viewMode = DatePickerViewMode::DayGrid;
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
    float progress = UIElement::AreAnimationsEnabled() ? m_popupAnim.Current() : (m_isPopupOpen ? 1.0f : 0.0f);
    if (progress <= 0.001f) return;

    float popW = 240.0f;
    float popH = 240.0f;
    float currentH = (m_isPopupOpen && progress >= 0.98f) ? popH : (popH * progress);
    Rect popRect(m_bounds.x, m_bounds.y + m_bounds.height + 4.0f, popW, popH);
    Rect clipRect(m_bounds.x, m_bounds.y + m_bounds.height + 4.0f, popW, currentH);

    ctx.PushClip(clipRect);

    D2D1_COLOR_F bg = D2D1::ColorF(0x25 / 255.0f, 0x25 / 255.0f, 0x26 / 255.0f, 1.0f);
    D2D1_COLOR_F border = D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f);
    D2D1_COLOR_F textCol = D2D1::ColorF(0xCC / 255.0f, 0xCC / 255.0f, 0xCC / 255.0f, 1.0f);
    D2D1_COLOR_F btnCol = D2D1::ColorF(0x56 / 255.0f, 0x9C / 255.0f, 0xD6 / 255.0f, 1.0f);
    D2D1_COLOR_F selBg = D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f);

    ctx.FillRoundedRect(popRect, 6.0f, bg);
    ctx.DrawRoundedRect(popRect, 6.0f, border, 1.5f);

    // Header Controls (Prev Arrow / Year Button / Month Button / Next Arrow)
    Rect btnPrev(popRect.x + 4.0f, popRect.y + 6.0f, 22.0f, 22.0f);
    Rect btnNext(popRect.x + popW - 26.0f, popRect.y + 6.0f, 22.0f, 22.0f);
    float midW = (popW - 56.0f) * 0.5f;
    Rect yrRect(popRect.x + 28.0f, popRect.y + 6.0f, midW, 22.0f);
    Rect moRect(popRect.x + 28.0f + midW, popRect.y + 6.0f, midW, 22.0f);

    ctx.DrawText("‹", btnPrev, btnCol, "Segoe UI", 16.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    ctx.DrawText("›", btnNext, btnCol, "Segoe UI", 16.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    if (m_viewMode == DatePickerViewMode::DayGrid) {
        ctx.DrawText(std::to_string(m_year) + "年 ▼", yrRect, D2D1::ColorF(1.0f, 1.0f, 1.0f), "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
        ctx.DrawText(std::to_string(m_month) + "月 ▼", moRect, D2D1::ColorF(1.0f, 1.0f, 1.0f), "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
    } else if (m_viewMode == DatePickerViewMode::MonthGrid) {
        Rect fullTitle(popRect.x + 28.0f, popRect.y + 6.0f, popW - 56.0f, 22.0f);
        ctx.DrawText(std::to_string(m_year) + "年 (选择月份)", fullTitle, D2D1::ColorF(1.0f, 1.0f, 1.0f), "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
    } else {
        Rect fullTitle(popRect.x + 28.0f, popRect.y + 6.0f, popW - 56.0f, 22.0f);
        ctx.DrawText(std::to_string(m_viewStartYear) + " - " + std::to_string(m_viewStartYear + 11) + "年", fullTitle, D2D1::ColorF(1.0f, 1.0f, 1.0f), "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
    }

    ctx.DrawLine(Point(popRect.x + 6.0f, popRect.y + 32.0f), Point(popRect.x + popW - 6.0f, popRect.y + 32.0f), D2D1::ColorF(0x3E / 255.0f, 0x3E / 255.0f, 0x42 / 255.0f));

    float bodyY = popRect.y + 36.0f;

    if (m_viewMode == DatePickerViewMode::DayGrid) {
        // Week Headers
        const char* weekNames[] = { "日", "一", "二", "三", "四", "五", "六" };
        float cellW = popW / 7.0f;
        for (int w = 0; w < 7; ++w) {
            Rect wRect(popRect.x + w * cellW, bodyY, cellW, 18.0f);
            ctx.DrawText(weekNames[w], wRect, D2D1::ColorF(0x88 / 255.0f, 0x88 / 255.0f, 0x88 / 255.0f), "Segoe UI", 10.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        // Days Grid
        float gridY = bodyY + 20.0f;
        float cellH = 26.0f;
        int firstWday = GetFirstDayOfWeek(m_year, m_month);
        int daysInMonth = GetDaysInMonth(m_year, m_month);

        for (int d = 1; d <= daysInMonth; ++d) {
            int idx = firstWday + (d - 1);
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
    } else if (m_viewMode == DatePickerViewMode::MonthGrid) {
        // Render 12 Month Cards Grid (3x4)
        const char* monthNames[] = { "1月", "2月", "3月", "4月", "5月", "6月", "7月", "8月", "9月", "10月", "11月", "12月" };
        float cellW = popW / 3.0f;
        float cellH = (popH - 44.0f) / 4.0f;

        for (int m = 1; m <= 12; ++m) {
            int idx = m - 1;
            int row = idx / 3;
            int col = idx % 3;
            Rect cellRect(popRect.x + col * cellW + 4.0f, bodyY + row * cellH + 2.0f, cellW - 8.0f, cellH - 4.0f);

            if (m == m_month) {
                ctx.FillRoundedRect(cellRect, 4.0f, selBg);
                ctx.DrawText(monthNames[idx], cellRect, D2D1::ColorF(1.0f, 1.0f, 1.0f), "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
            } else {
                ctx.FillRoundedRect(cellRect, 4.0f, D2D1::ColorF(0x2D / 255.0f, 0x2D / 255.0f, 0x30 / 255.0f));
                ctx.DrawText(monthNames[idx], cellRect, textCol, "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            }
        }
    } else if (m_viewMode == DatePickerViewMode::YearGrid) {
        // Render 12 Year Cards Grid (3x4)
        float cellW = popW / 3.0f;
        float cellH = (popH - 44.0f) / 4.0f;

        for (int i = 0; i < 12; ++i) {
            int yr = m_viewStartYear + i;
            int row = i / 3;
            int col = i % 3;
            Rect cellRect(popRect.x + col * cellW + 4.0f, bodyY + row * cellH + 2.0f, cellW - 8.0f, cellH - 4.0f);

            if (yr == m_year) {
                ctx.FillRoundedRect(cellRect, 4.0f, selBg);
                ctx.DrawText(std::to_string(yr), cellRect, D2D1::ColorF(1.0f, 1.0f, 1.0f), "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
            } else {
                ctx.FillRoundedRect(cellRect, 4.0f, D2D1::ColorF(0x2D / 255.0f, 0x2D / 255.0f, 0x30 / 255.0f));
                ctx.DrawText(std::to_string(yr), cellRect, textCol, "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            }
        }
    }

    ctx.PopClip();
}

} // namespace CUI
