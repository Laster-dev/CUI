#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "DatePicker.h"
#include "../style/ThemeManager.h"
#include "../window/PopupPlacement.h"
#include <sstream>
#include <iomanip>

namespace CUI {

DatePicker::DatePicker() {
    SelectedDate.Initialize(*this);
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

Value DatePicker::GetProperty(PropertyId id) const {
    if (id == PropertyId::DateStr) return Value(GetFormattedDate());
    return UIElement::GetProperty(id);
}

bool DatePicker::HasProperty(PropertyId id) const {
    return id == PropertyId::DateStr || UIElement::HasProperty(id);
}

void DatePicker::SetProperty(PropertyId id, const Value& val) {
    if (id == PropertyId::DateStr) {
        int y = 0, m = 0, d = 0;
        if (sscanf_s(val.AsString().c_str(), "%d-%d-%d", &y, &m, &d) == 3) {
            SetDate(y, m, d);
        }
        return;
    }
    UIElement::SetProperty(id, val);
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
    if (progress <= 0.2f) return nullptr;
    Rect popRect = GetPopupBounds();
    if (popRect.Contains(x, y)) {
        return this;
    }
    return nullptr;
}

bool DatePicker::OnAnimationTick() {
    float dt = UIElement::GetAnimationDeltaSeconds();
    m_popupAnim.SetTarget(m_isPopupOpen ? 1.0f : 0.0f);
    bool animating = m_popupAnim.Tick(dt, PopupReveal::kSpec);
    if (m_scrollbarAutoHide.Tick(dt)) {
        animating = true;
    }
    if (animating) {
        MarkRenderRectDirty(m_bounds.Inflate(4.0f));
        if (m_isPopupOpen || m_popupAnim.Current() > 0.001f) {
            MarkRenderRectDirty(GetPopupBounds().Inflate(6.0f));
        }
        RequestAnimationTicks();
    }
    return animating;
}

bool DatePicker::HasSelfAnimation() const {
    return std::abs(m_popupAnim.Target() - m_popupAnim.Current()) > 0.001f
        || m_scrollbarAutoHide.NeedsTicks();
}

void DatePicker::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);

    if (m_isPopupOpen) {
        Rect popRect = GetPopupBounds();
        float popW = popRect.width;
        float popH = popRect.height;

        float progress = UIElement::AreAnimationsEnabled() ? m_popupAnim.Current() : 1.0f;

        if (progress > 0.2f && popRect.Contains(pt.x, pt.y)) {
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
                m_scrollOffset = 0.0f;
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
                m_scrollOffset = 0.0f;
                return;
            }
            if (yrRect.Contains(pt.x, pt.y)) {
                m_viewStartYear = (m_year / 12) * 12;
                m_viewMode = DatePickerViewMode::YearGrid;
                m_scrollOffset = 0.0f;
                return;
            }
            if (moRect.Contains(pt.x, pt.y)) {
                m_viewMode = DatePickerViewMode::MonthGrid;
                m_scrollOffset = 0.0f;
                return;
            }

            // Grid Cell Selection (supports internal scrolling)
            constexpr float kBodyYFromTop = 36.0f;
            constexpr float kPopHDesign = 240.0f;
            constexpr float kScrollBarW = 10.0f;
            constexpr float kScrollBarPad = 4.0f;

            float bodyY = popRect.y + kBodyYFromTop;

            // 1) Scrollbar click: update m_scrollOffset (without dismissing popup)
            const float visibleScrollH = popH - kBodyYFromTop;
            if (visibleScrollH > 0.0f) {
                float contentH = 0.0f;
                float cellH = 0.0f;
                if (m_viewMode == DatePickerViewMode::DayGrid) {
                    int firstWday = GetFirstDayOfWeek(m_year, m_month);
                    int daysInMonth = GetDaysInMonth(m_year, m_month);
                    const int idxLast = firstWday + daysInMonth - 1;
                    const int rows = (idxLast / 7) + 1;
                    constexpr float kCellH = 26.0f;
                    cellH = kCellH;
                    contentH = 20.0f + rows * kCellH;
                } else {
                    cellH = (kPopHDesign - 44.0f) / 4.0f;
                    contentH = 4.0f * cellH; // 3x4
                }

                const float maxScroll = (std::max)(0.0f, contentH - visibleScrollH);
                if (maxScroll > 0.001f) {
                    const float trackX = popRect.x + popW - kScrollBarW - kScrollBarPad;
                    const Rect trackRect(trackX, bodyY, kScrollBarW, visibleScrollH);
                    if (trackRect.Contains(pt.x, pt.y)) {
                        const float ratio = std::clamp((pt.y - bodyY) / visibleScrollH, 0.0f, 1.0f);
                        m_scrollOffset = ratio * maxScroll;
                        m_scrollbarAutoHide.NotifyActivity(this);
                        RequestAnimationTicks();
                        MarkRenderContentDirty();
                        return;
                    }
                }
            }

            // 2) Grid cell click (account m_scrollOffset by shifting Y)
            if (m_viewMode == DatePickerViewMode::DayGrid) {
                float gridY = bodyY + 20.0f - m_scrollOffset;
                float cellW = popW / 7.0f;
                constexpr float cellH = 26.0f;
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
                float cellW = popW / 3.0f;
                const float cellH = (kPopHDesign - 44.0f) / 4.0f;
                int col = static_cast<int>((pt.x - popRect.x) / cellW);
                int row = static_cast<int>((pt.y - bodyY + m_scrollOffset) / cellH);
                int selectedMonth = row * 3 + col + 1;
                if (selectedMonth >= 1 && selectedMonth <= 12) {
                    SetDate(m_year, selectedMonth, m_day);
                    m_viewMode = DatePickerViewMode::DayGrid;
                    m_scrollOffset = 0.0f;
                    return;
                }
            } else if (m_viewMode == DatePickerViewMode::YearGrid) {
                float cellW = popW / 3.0f;
                const float cellH = (kPopHDesign - 44.0f) / 4.0f;
                int col = static_cast<int>((pt.x - popRect.x) / cellW);
                int row = static_cast<int>((pt.y - bodyY + m_scrollOffset) / cellH);
                int selectedYear = m_viewStartYear + row * 3 + col;
                SetDate(selectedYear, m_month, m_day);
                m_viewMode = DatePickerViewMode::MonthGrid;
                m_scrollOffset = 0.0f;
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

void DatePicker::OnMouseWheel(float delta) {
    if (!m_isPopupOpen) return;
    float progress = UIElement::AreAnimationsEnabled() ? m_popupAnim.Current() : 1.0f;
    if (progress <= 0.01f) return;

    Rect popRect = GetPopupBounds();
    const float visibleH = popRect.height;
    if (visibleH <= 0.0f) return;

    // Scroll region starts at bodyY (fixed header area above).
    constexpr float kBodyYFromTop = 36.0f;
    const float visibleScrollH = visibleH - kBodyYFromTop;
    if (visibleScrollH <= 0.0f) return;

    constexpr float kPopHDesign = 240.0f;

    // Compute full scrollable content height (relative to bodyY).
    float contentH = 0.0f;
    if (m_viewMode == DatePickerViewMode::DayGrid) {
        int firstWday = GetFirstDayOfWeek(m_year, m_month);
        int daysInMonth = GetDaysInMonth(m_year, m_month);
        const int idxLast = firstWday + daysInMonth - 1;
        const int rows = (idxLast / 7) + 1; // up to 6
        constexpr float kCellH = 26.0f;
        contentH = 20.0f + static_cast<float>(rows) * kCellH; // bodyY->week header/grid gap + grid rows
    } else {
        const float cellH = (kPopHDesign - 44.0f) / 4.0f;
        contentH = 4.0f * cellH;
    }

    const float maxScroll = (std::max)(0.0f, contentH - visibleScrollH);
    if (maxScroll <= 0.001f) return;

    // delta > 0 means wheel up => show earlier rows => decrease scrollOffset.
    const float step = (m_viewMode == DatePickerViewMode::DayGrid) ? 26.0f : ((kPopHDesign - 44.0f) / 4.0f);
    float next = m_scrollOffset - delta * step;
    next = (std::clamp)(next, 0.0f, maxScroll);
    if (std::abs(next - m_scrollOffset) <= 0.001f) return;

    m_scrollOffset = next;
    m_scrollbarAutoHide.NotifyActivity(this);
    RequestAnimationTicks();
    MarkRenderContentDirty();
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
    if (open) {
        m_scrollOffset = 0.0f;
    }
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
    constexpr float popW = 240.0f;
    constexpr float popH = 240.0f;
    return PlacePopupNearAnchor(m_bounds, popW, popH, GetPopupViewportOrDefault(), 4.0f);
}

bool DatePicker::HitDismissExempt(float x, float y) const {
    if (m_bounds.Contains(x, y)) return true;
    return GetPopupBounds().Contains(x, y);
}

void DatePicker::RenderPopup(GraphicsContext& ctx) {
    float progress = UIElement::AreAnimationsEnabled() ? m_popupAnim.Current() : (m_isPopupOpen ? 1.0f : 0.0f);
    if (progress <= 0.001f) return;

    Rect popRect = GetPopupBounds();
    float popW = popRect.width;
    float popH = popRect.height;
    ctx.PushPopupReveal(popRect, progress, Point(popRect.x + popW * 0.5f, popRect.y));

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

    ctx.DrawText("‹", btnPrev, btnCol, "微软雅黑", 16.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    ctx.DrawText("›", btnNext, btnCol, "微软雅黑", 16.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    auto drawHeaderWithChevron = [&](const std::string& label, const Rect& rect) {
        const Size ts = ctx.MeasureText(label, "微软雅黑", 12.0f, DWRITE_FONT_WEIGHT_BOLD);
        const float chev = 10.0f;
        const float gap = 3.0f;
        const float total = ts.width + gap + chev;
        const float start = rect.x + (rect.width - total) * 0.5f;
        ctx.DrawText(
            label,
            Rect(start, rect.y, ts.width, rect.height),
            textCol,
            "微软雅黑",
            12.0f,
            DWRITE_TEXT_ALIGNMENT_LEADING,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
            DWRITE_FONT_WEIGHT_BOLD);
        ctx.DrawChevron(
            Rect(start + ts.width + gap, rect.y + (rect.height - chev) * 0.5f, chev, chev),
            textCol,
            GraphicsContext::ChevronDirection::Down,
            1.4f);
    };

    if (m_viewMode == DatePickerViewMode::DayGrid) {
        drawHeaderWithChevron(std::to_string(m_year) + "年", yrRect);
        drawHeaderWithChevron(std::to_string(m_month) + "月", moRect);
    } else if (m_viewMode == DatePickerViewMode::MonthGrid) {
        Rect fullTitle(popRect.x + 28.0f, popRect.y + 6.0f, popW - 56.0f, 22.0f);
        ctx.DrawText(std::to_string(m_year) + "年 (选择月份)", fullTitle, textCol, "微软雅黑", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
    } else {
        Rect fullTitle(popRect.x + 28.0f, popRect.y + 6.0f, popW - 56.0f, 22.0f);
        ctx.DrawText(std::to_string(m_viewStartYear) + " - " + std::to_string(m_viewStartYear + 11) + "年", fullTitle, textCol, "微软雅黑", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
    }

    ctx.DrawLine(Point(popRect.x + 6.0f, popRect.y + 32.0f), Point(popRect.x + popW - 6.0f, popRect.y + 32.0f), border);

    constexpr float kBodyYFromTop = 36.0f;
    constexpr float kPopHDesign = 240.0f;
    constexpr float kScrollBarW = 10.0f;
    constexpr float kScrollBarPad = 4.0f;

    const float bodyY = popRect.y + kBodyYFromTop;
    const float visibleScrollH = popH - kBodyYFromTop;

    float contentH = 0.0f;
    float dayCellH = 26.0f;
    float monthYearCellH = (kPopHDesign - 44.0f) / 4.0f;

    if (m_viewMode == DatePickerViewMode::DayGrid) {
        int firstWday = GetFirstDayOfWeek(m_year, m_month);
        int daysInMonth = GetDaysInMonth(m_year, m_month);
        const int idxLast = firstWday + daysInMonth - 1;
        const int rows = (idxLast / 7) + 1;
        contentH = 20.0f + static_cast<float>(rows) * dayCellH;
    } else {
        contentH = 4.0f * monthYearCellH;
    }

    float maxScroll = (std::max)(0.0f, contentH - visibleScrollH);
    if (maxScroll <= 0.001f) {
        m_scrollOffset = 0.0f;
    } else {
        m_scrollOffset = (std::clamp)(m_scrollOffset, 0.0f, maxScroll);
    }
    const float yShift = -m_scrollOffset;

    if (m_viewMode == DatePickerViewMode::DayGrid) {
        // Week Headers (scrollable)
        const char* weekNames[] = { "日", "一", "二", "三", "四", "五", "六" };
        float cellW = popW / 7.0f;
        for (int w = 0; w < 7; ++w) {
            Rect wRect(popRect.x + w * cellW, bodyY + yShift, cellW, 18.0f);
            ctx.DrawText(weekNames[w], wRect, textMutedCol, "微软雅黑", 10.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        // Days Grid
        float gridY = bodyY + 20.0f + yShift;
        int firstWday = GetFirstDayOfWeek(m_year, m_month);
        int daysInMonth = GetDaysInMonth(m_year, m_month);

        for (int d = 1; d <= daysInMonth; ++d) {
            int idx = firstWday + d - 1;
            int row = idx / 7;
            int col = idx % 7;
            Rect cellRect(popRect.x + col * cellW + 2.0f, gridY + row * dayCellH + 2.0f, cellW - 4.0f, dayCellH - 4.0f);

            bool isSelected = (d == m_day);
            if (isSelected) {
                ctx.FillRoundedRect(cellRect, 4.0f, selBg);
            }

            D2D1_COLOR_F dCol = isSelected ? ThemeManager::Instance().GetTokens().textPrimary : textCol;
            ctx.DrawText(std::to_string(d), cellRect, dCol, "微软雅黑", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, isSelected ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL);
        }
    } else if (m_viewMode == DatePickerViewMode::MonthGrid) {
        // 3x4 Month Grid (scrollable)
        float cellW = popW / 3.0f;
        float cellH = monthYearCellH;
        const char* monthNames[] = { "1月", "2月", "3月", "4月", "5月", "6月", "7月", "8月", "9月", "10月", "11月", "12月" };

        for (int m = 1; m <= 12; ++m) {
            int row = (m - 1) / 3;
            int col = (m - 1) % 3;
            Rect cellRect(popRect.x + col * cellW + 4.0f, bodyY + row * cellH + 4.0f + yShift, cellW - 8.0f, cellH - 8.0f);

            bool isSelected = (m == m_month);
            if (isSelected) {
                ctx.FillRoundedRect(cellRect, 4.0f, selBg);
            }

            D2D1_COLOR_F mCol = isSelected ? ThemeManager::Instance().GetTokens().textPrimary : textCol;
            ctx.DrawText(monthNames[m - 1], cellRect, mCol, "微软雅黑", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, isSelected ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL);
        }
    } else if (m_viewMode == DatePickerViewMode::YearGrid) {
        // 3x4 Year Grid (scrollable)
        float cellW = popW / 3.0f;
        float cellH = monthYearCellH;

        for (int i = 0; i < 12; ++i) {
            int yr = m_viewStartYear + i;
            int row = i / 3;
            int col = i % 3;
            Rect cellRect(popRect.x + col * cellW + 4.0f, bodyY + row * cellH + 4.0f + yShift, cellW - 8.0f, cellH - 8.0f);

            bool isSelected = (yr == m_year);
            if (isSelected) {
                ctx.FillRoundedRect(cellRect, 4.0f, selBg);
            }

            D2D1_COLOR_F yCol = isSelected ? ThemeManager::Instance().GetTokens().textPrimary : textCol;
            ctx.DrawText(std::to_string(yr), cellRect, yCol, "微软雅黑", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, isSelected ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL);
        }
    }

    // Scrollbar
    if (visibleScrollH > 12.0f && contentH > visibleScrollH + 0.001f && m_scrollbarAutoHide.IsDrawn()) {
        const float trackX = (std::min)(popRect.x + popW - kScrollBarPad, popRect.x + popW - kScrollBarW);
        const Rect trackRect(trackX, bodyY, kScrollBarW, visibleScrollH);
        const float vis = m_scrollbarAutoHide.Opacity();

        D2D1_COLOR_F trackColor = ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBorder);
        trackColor.a = 0.35f * vis;
        ctx.DrawRoundedRect(trackRect, 4.0f, trackColor, 1.0f);

        const float thumbH = (std::max)(16.0f, visibleScrollH * visibleScrollH / contentH);
        const float travel = (visibleScrollH - thumbH);
        const float thumbY = bodyY + ((maxScroll > 0.001f) ? (m_scrollOffset / maxScroll) * travel : 0.0f);

        Rect thumbRect(trackX + 2.0f, thumbY, kScrollBarW - 4.0f, thumbH);
        D2D1_COLOR_F thumbColor = ThemeManager::Instance().GetFlatColor(ThemeTokenId::AccentColor);
        thumbColor.a = 0.45f * vis;
        ctx.FillRoundedRect(thumbRect, 4.0f, thumbColor);
    }

    ctx.PopPopupReveal();
}

void DatePicker::OnRenderOverlay(GraphicsContext& ctx) {
    if (PopupHost::Current() && m_isPopupOpen) return;
    RenderPopup(ctx);
}

} // namespace CUI
