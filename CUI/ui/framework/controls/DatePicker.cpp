#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "DatePicker.h"
#include "../style/ThemeManager.h"
#include <sstream>
#include <iomanip>

namespace CUI {

DatePicker::DatePicker() {
    const ThemeTokens& tokens = ThemeManager::Instance().GetTokens();
    time_t t = time(nullptr);
    tm tmVal = {};
    localtime_s(&tmVal, &t);
    m_year = tmVal.tm_year + 1900;
    m_month = tmVal.tm_mon + 1;
    m_day = tmVal.tm_mday;

    SetBackgroundToken(ThemeTokenId::InputBackground);
    SetBorderToken(ThemeTokenId::InputBorder);
    SetColorToken(ThemeTokenId::TextPrimary);
    SetBackground(tokens.inputBackground);
    SetBorderBrush(tokens.inputBorder);
    SetBorderThickness(1.0f);
    SetColor(tokens.textPrimary);
    SetCornerRadius(4.0f);
    SetWidth(160.0f);
    SetHeight(30.0f);
}

std::vector<PropertyMeta> DatePicker::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "dateStr", "当前日期 (Date)", "日期配置", "string" });
    return metas;
}

Size DatePicker::Measure(Size availableSize) {
    float expW = GetWidth(); if (expW < 0) expW = 160.0f;
    float expH = GetHeight(); if (expH < 0) expH = 30.0f;
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
    NotifyFieldChanged(PropertyId::DateStr, Value(GetFormattedDate()));
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
            // Header button clicks
            Rect btnPrev(popRect.x + 4.0f, popRect.y + 6.0f, 22.0f, 22.0f);
            Rect btnNext(popRect.x + popW - 26.0f, popRect.y + 6.0f, 22.0f, 22.0f);
            float midW = (popW - 56.0f) * 0.5f;
            Rect yrRect(popRect.x + 28.0f, popRect.y + 6.0f, midW, 22.0f);
            Rect moRect(popRect.x + 28.0f + midW, popRect.y + 6.0f, midW, 22.0f);

            if (btnPrev.Contains(pt.x, pt.y)) {
                if (m_viewMode == DatePickerViewMode::DayGrid) {
                    if (m_month == 1) { m_month = 12; m_year--; }
                    else { m_month--; }
                    SetDate(m_year, m_month, m_day);
                } else if (m_viewMode == DatePickerViewMode::MonthGrid) {
                    m_year--;
                } else if (m_viewMode == DatePickerViewMode::YearGrid) {
                    m_viewStartYear -= 12;
                }
                return;
            }
            if (btnNext.Contains(pt.x, pt.y)) {
                if (m_viewMode == DatePickerViewMode::DayGrid) {
                    if (m_month == 12) { m_month = 1; m_year++; }
                    else { m_month++; }
                    SetDate(m_year, m_month, m_day);
                } else if (m_viewMode == DatePickerViewMode::MonthGrid) {
                    m_year++;
                } else if (m_viewMode == DatePickerViewMode::YearGrid) {
                    m_viewStartYear += 12;
                }
                return;
            }
            if (yrRect.Contains(pt.x, pt.y)) {
                m_viewStartYear = (m_year / 12) * 12;
                m_viewMode = DatePickerViewMode::YearGrid;
                return;
            }
            if (moRect.Contains(pt.x, pt.y)) {
                m_viewMode = DatePickerViewMode::MonthGrid;
                return;
            }

            // Grid Cell Selection
            float bodyY = popRect.y + 36.0f;
            if (m_viewMode == DatePickerViewMode::DayGrid) {
                float gridY = bodyY + 20.0f;
                float cellW = popW / 7.0f;
                float cellH = 26.0f;
                int firstWday = GetFirstDayOfWeek(m_year, m_month);
                int daysInMonth = GetDaysInMonth(m_year, m_month);

                int col = static_cast<int>((pt.x - popRect.x) / cellW);
                int row = static_cast<int>((pt.y - gridY) / cellH);
                int cellIdx = row * 7 + col;
                int day = cellIdx - firstWday + 1;

                if (day >= 1 && day <= daysInMonth) {
                    SetDate(m_year, m_month, day);
                    SetPopupOpen(false);
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
        SetPopupOpen(false);
    } else {
        if (m_bounds.Contains(pt.x, pt.y)) {
            SetPopupOpen(true);
            m_viewMode = DatePickerViewMode::DayGrid;
        }
    }
}

void DatePicker::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    std::string text = "📅 " + GetFormattedDate();
    float fontSize = GetFontSize();
    std::string fontFamily = GetFontFamily();
    D2D1_COLOR_F textColor = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextPrimary);

    Rect textRect(m_bounds.x + 8.0f, m_bounds.y, m_bounds.width - 16.0f, m_bounds.height);
    ctx.DrawText(text, textRect, textColor, fontFamily, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

void DatePicker::SetPopupOpen(bool open) {
    if (m_isPopupOpen == open) return;
    m_isPopupOpen = open;
    if (PopupHost* host = PopupHost::Current()) {
        if (open) {
            host->Open(this);
        } else {
            host->Close(this);
        }
    }
    MarkRenderContentDirty();
}

Rect DatePicker::GetPopupBounds() const {
    return Rect(m_bounds.x, m_bounds.y + m_bounds.height + 4.0f, 240.0f, 240.0f);
}

bool DatePicker::HitDismissExempt(float x, float y) const {
    if (m_bounds.Contains(x, y)) return true;
    return GetPopupBounds().Contains(x, y);
}

void DatePicker::RenderPopup(GraphicsContext& ctx) {
    float progress = UIElement::AreAnimationsEnabled() ? m_popupAnim.Current() : (m_isPopupOpen ? 1.0f : 0.0f);
    if (progress <= 0.001f) return;

    float popW = 240.0f;
    float popH = 240.0f;
    float currentH = (m_isPopupOpen && progress >= 0.98f) ? popH : (popH * progress);
    Rect popRect(m_bounds.x, m_bounds.y + m_bounds.height + 4.0f, popW, popH);
    Rect clipRect(m_bounds.x, m_bounds.y + m_bounds.height + 4.0f, popW, currentH);

    ctx.PushClip(clipRect);

    D2D1_COLOR_F bg = ThemeManager::Instance().GetTokens().cardBackground;
    D2D1_COLOR_F border = ThemeManager::Instance().GetTokens().cardBorder;
    D2D1_COLOR_F textCol = ThemeManager::Instance().GetTokens().textPrimary;
    D2D1_COLOR_F textMutedCol = ThemeManager::Instance().GetTokens().textMuted;
    D2D1_COLOR_F btnCol = ThemeManager::Instance().GetTokens().accentColor;
    D2D1_COLOR_F selBg = ThemeManager::Instance().GetTokens().accentColor;

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
        ctx.DrawText(std::to_string(m_year) + "年 ▼", yrRect, textCol, "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
        ctx.DrawText(std::to_string(m_month) + "月 ▼", moRect, textCol, "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
    } else if (m_viewMode == DatePickerViewMode::MonthGrid) {
        Rect fullTitle(popRect.x + 28.0f, popRect.y + 6.0f, popW - 56.0f, 22.0f);
        ctx.DrawText(std::to_string(m_year) + "年 (选择月份)", fullTitle, textCol, "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
    } else {
        Rect fullTitle(popRect.x + 28.0f, popRect.y + 6.0f, popW - 56.0f, 22.0f);
        ctx.DrawText(std::to_string(m_viewStartYear) + " - " + std::to_string(m_viewStartYear + 11) + "年", fullTitle, textCol, "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
    }

    ctx.DrawLine(Point(popRect.x + 6.0f, popRect.y + 32.0f), Point(popRect.x + popW - 6.0f, popRect.y + 32.0f), border);

    float bodyY = popRect.y + 36.0f;

    if (m_viewMode == DatePickerViewMode::DayGrid) {
        // Week Headers
        const char* weekNames[] = { "日", "一", "二", "三", "四", "五", "六" };
        float cellW = popW / 7.0f;
        for (int w = 0; w < 7; ++w) {
            Rect wRect(popRect.x + w * cellW, bodyY, cellW, 18.0f);
            ctx.DrawText(weekNames[w], wRect, textMutedCol, "Segoe UI", 10.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        // Days Grid
        float gridY = bodyY + 20.0f;
        float cellH = 26.0f;
        int firstWday = GetFirstDayOfWeek(m_year, m_month);
        int daysInMonth = GetDaysInMonth(m_year, m_month);

        for (int d = 1; d <= daysInMonth; ++d) {
            int idx = firstWday + d - 1;
            int row = idx / 7;
            int col = idx % 7;
            Rect cellRect(popRect.x + col * cellW + 2.0f, gridY + row * cellH + 2.0f, cellW - 4.0f, cellH - 4.0f);

            bool isSelected = (d == m_day);
            if (isSelected) {
                ctx.FillRoundedRect(cellRect, 4.0f, selBg);
            }

            D2D1_COLOR_F dCol = isSelected ? ThemeManager::Instance().GetTokens().textPrimary : textCol;
            ctx.DrawText(std::to_string(d), cellRect, dCol, "Segoe UI", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, isSelected ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL);
        }
    } else if (m_viewMode == DatePickerViewMode::MonthGrid) {
        // 3x4 Month Grid
        float cellW = popW / 3.0f;
        float cellH = (popH - 44.0f) / 4.0f;
        const char* monthNames[] = { "1月", "2月", "3月", "4月", "5月", "6月", "7月", "8月", "9月", "10月", "11月", "12月" };

        for (int m = 1; m <= 12; ++m) {
            int row = (m - 1) / 3;
            int col = (m - 1) % 3;
            Rect cellRect(popRect.x + col * cellW + 4.0f, bodyY + row * cellH + 4.0f, cellW - 8.0f, cellH - 8.0f);

            bool isSelected = (m == m_month);
            if (isSelected) {
                ctx.FillRoundedRect(cellRect, 4.0f, selBg);
            }

            D2D1_COLOR_F mCol = isSelected ? ThemeManager::Instance().GetTokens().textPrimary : textCol;
            ctx.DrawText(monthNames[m - 1], cellRect, mCol, "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, isSelected ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL);
        }
    } else if (m_viewMode == DatePickerViewMode::YearGrid) {
        // 3x4 Year Grid (12 years)
        float cellW = popW / 3.0f;
        float cellH = (popH - 44.0f) / 4.0f;

        for (int i = 0; i < 12; ++i) {
            int yr = m_viewStartYear + i;
            int row = i / 3;
            int col = i % 3;
            Rect cellRect(popRect.x + col * cellW + 4.0f, bodyY + row * cellH + 4.0f, cellW - 8.0f, cellH - 8.0f);

            bool isSelected = (yr == m_year);
            if (isSelected) {
                ctx.FillRoundedRect(cellRect, 4.0f, selBg);
            }

            D2D1_COLOR_F yCol = isSelected ? ThemeManager::Instance().GetTokens().textPrimary : textCol;
            ctx.DrawText(std::to_string(yr), cellRect, yCol, "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, isSelected ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL);
        }
    }

    ctx.PopClip();
}

void DatePicker::OnRenderOverlay(GraphicsContext& ctx) {
    if (PopupHost::Current() && m_isPopupOpen) return;
    RenderPopup(ctx);
}

} // namespace CUI
