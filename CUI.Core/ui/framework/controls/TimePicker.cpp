#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "TimePicker.h"
#include "../style/ThemeManager.h"
#include "../window/PopupPlacement.h"
#include "../window/Dpi.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <windows.h>

namespace CUI {

namespace {
constexpr float kPopupWidth = 220.0f;
constexpr float kPopupHeight = 244.0f;
constexpr float kColumnGap = 8.0f;
constexpr float kHeaderHeight = 32.0f;
constexpr float kWheelTopPadding = 10.0f;
constexpr float kWheelBottomPadding = 14.0f;
constexpr float kItemHeight = 30.0f;
constexpr int kVisibleRadius = 3;

int WrapIndex(int value, int count) {
    if (count <= 0) return 0;
    value %= count;
    if (value < 0) value += count;
    return value;
}

float WrapPosition(float value, int count) {
    if (count <= 0) return 0.0f;
    float wrapped = std::fmod(value, static_cast<float>(count));
    if (wrapped < 0.0f) wrapped += static_cast<float>(count);
    return wrapped;
}
}

TimePicker::TimePicker() {
    const ThemeTokens& tokens = ThemeManager::Instance().GetTokens();
    time_t t = time(nullptr);
    tm tmVal = {};
    localtime_s(&tmVal, &t);
    m_hour = tmVal.tm_hour;
    m_minute = tmVal.tm_min;

    SetBackgroundToken(ThemeTokenId::InputBackground);
    SetBorderToken(ThemeTokenId::InputBorder);
    SetColorToken(ThemeTokenId::TextPrimary);
    SetBackground(tokens.inputBackground);
    SetBorderBrush(tokens.inputBorder);
    SetBorderThickness(1.0f);
    SetColor(tokens.textPrimary);
    SetCornerRadius(4.0f);
    SetWidth(140.0f);
    SetHeight(30.0f);

    m_hourPosition = static_cast<float>(m_hour);
    m_minutePosition = static_cast<float>(m_minute);
    m_hourTarget = m_hourPosition;
    m_minuteTarget = m_minutePosition;
}

Value TimePicker::GetProperty(PropertyId id) const {
    if (id == PropertyId::TimeStr) return Value(GetFormattedTime());
    return UIElement::GetProperty(id);
}

bool TimePicker::HasProperty(PropertyId id) const {
    return id == PropertyId::TimeStr || UIElement::HasProperty(id);
}

void TimePicker::SetProperty(PropertyId id, const Value& val) {
    if (id == PropertyId::TimeStr) {
        int h = 0, m = 0;
        if (sscanf_s(val.AsString().c_str(), "%d:%d", &h, &m) == 2) {
            SetTime(h, m);
        }
        return;
    }
    UIElement::SetProperty(id, val);
}

Size TimePicker::Measure(Size availableSize) {
    float expW = GetWidth(); if (expW < 0) expW = 140.0f;
    float expH = GetHeight(); if (expH < 0) expH = 30.0f;
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

std::string TimePicker::GetFormattedTime() const {
    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << m_hour
       << ":"
       << std::setw(2) << std::setfill('0') << m_minute;
    return ss.str();
}

void TimePicker::SetTime(int h, int m) {
    int nextHour = std::clamp(h, 0, 23);
    int nextMinute = std::clamp(m, 0, 59);
    bool changed = (nextHour != m_hour) || (nextMinute != m_minute);

    m_hour = nextHour;
    m_minute = nextMinute;
    NotifyFieldChanged(PropertyId::TimeStr, Value(GetFormattedTime()));
    if (changed) {
        m_onTimeChangedEvent.Invoke(this, m_hour, m_minute);
    }
}

Rect TimePicker::GetPopupRect() const {
    return PlacePopupNearAnchor(m_bounds, kPopupWidth, kPopupHeight, GetPopupViewportOrDefault(), 4.0f);
}

Rect TimePicker::GetWheelRect(int column) const {
    Rect popup = GetPopupRect();
    float innerX = popup.x + 12.0f;
    float innerW = popup.width - 24.0f;
    float columnW = (innerW - kColumnGap) * 0.5f;
    float x = innerX + ((column == 1) ? (columnW + kColumnGap) : 0.0f);
    float y = popup.y + kHeaderHeight + kWheelTopPadding;
    float h = popup.height - kHeaderHeight - kWheelTopPadding - kWheelBottomPadding;
    return Rect(x, y, columnW, h);
}

Rect TimePicker::GetSelectionRect(int column) const {
    Rect wheel = GetWheelRect(column);
    float y = wheel.y + (wheel.height - kItemHeight) * 0.5f;
    return Rect(wheel.x, y, wheel.width, kItemHeight);
}

int TimePicker::HitTestColumn(float x, float y) const {
    if (!m_isPopupOpen) return -1;
    for (int column = 0; column < 2; ++column) {
        if (GetWheelRect(column).Contains(x, y)) {
            return column;
        }
    }
    return -1;
}

void TimePicker::NudgeColumn(int column, int delta) {
    if (column == 0) {
        m_hourTarget = WrapPosition(m_hourTarget + static_cast<float>(delta), 24);
        SetTime(WrapIndex(static_cast<int>(std::round(m_hourTarget)), 24), m_minute);
    } else if (column == 1) {
        m_minuteTarget = WrapPosition(m_minuteTarget + static_cast<float>(delta), 60);
        SetTime(m_hour, WrapIndex(static_cast<int>(std::round(m_minuteTarget)), 60));
    }
    RequestAnimationTicks();
    MarkRenderContentDirty();
}

void TimePicker::SnapTargetsToSelection() {
    m_hourTarget = static_cast<float>(m_hour);
    m_minuteTarget = static_cast<float>(m_minute);
}

void TimePicker::ApplyAnimatedSelection() {
    int hourSelection = WrapIndex(static_cast<int>(std::round(m_hourTarget)), 24);
    int minuteSelection = WrapIndex(static_cast<int>(std::round(m_minuteTarget)), 60);
    SetTime(hourSelection, minuteSelection);
}

UIElement* TimePicker::OnHitTestOverlay(float x, float y) {
    if (!m_isPopupOpen) return nullptr;
    return GetPopupRect().Contains(x, y) ? this : nullptr;
}

void TimePicker::OnMouseWheel(float delta) {
    if (!m_isPopupOpen) return;

    float logicalX = 0.0f;
    float logicalY = 0.0f;
    POINT screenPt{};
    HWND hwnd = nullptr;
    if (GetCursorPos(&screenPt)) {
        hwnd = WindowFromPoint(screenPt);
    }
    if (!hwnd || !TryGetCursorClientLogical(hwnd, logicalX, logicalY)) {
        return;
    }

    int column = HitTestColumn(logicalX, logicalY);
    // Wheel over popup header/footer: still nudge the nearer column.
    if (column < 0 && GetPopupRect().Contains(logicalX, logicalY)) {
        Rect hourWheel = GetWheelRect(0);
        Rect minuteWheel = GetWheelRect(1);
        const float midX = (hourWheel.x + hourWheel.width + minuteWheel.x) * 0.5f;
        column = (logicalX < midX) ? 0 : 1;
    }
    if (column < 0) return;

    NudgeColumn(column, (delta > 0.0f) ? -1 : 1);
}

void TimePicker::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);

    if (!m_isPopupOpen) {
        if (m_bounds.Contains(pt.x, pt.y)) {
            SetPopupOpen(true);
            SnapTargetsToSelection();
            m_hourPosition = m_hourTarget;
            m_minutePosition = m_minuteTarget;
        }
        return;
    }

    Rect popup = GetPopupRect();
    if (!popup.Contains(pt.x, pt.y)) {
        SetPopupOpen(false);
        return;
    }

    Rect doneRect(popup.x + 12.0f, popup.y + popup.height - 36.0f, popup.width - 24.0f, 30.0f);
    if (doneRect.Contains(pt.x, pt.y)) {
        SetPopupOpen(false);
        return;
    }

    int column = HitTestColumn(pt.x, pt.y);
    if (column >= 0) {
        Rect wheel = GetWheelRect(column);
        float centerY = wheel.y + wheel.height * 0.5f;
        int step = static_cast<int>(std::round((pt.y - centerY) / kItemHeight));
        if (step != 0) {
            NudgeColumn(column, step);
        }
        return;
    }
}

bool TimePicker::OnAnimationTick() {
    bool base = UIElement::OnAnimationTick();
    if (!UIElement::AreAnimationsEnabled()) {
        m_hourPosition = WrapPosition(m_hourTarget, 24);
        m_minutePosition = WrapPosition(m_minuteTarget, 60);
        ApplyAnimatedSelection();
        m_lastAnimTime = std::chrono::steady_clock::time_point{};
        return base;
    }

    const auto now = std::chrono::steady_clock::now();
    float deltaSeconds = 1.0f / 60.0f;
    if (m_lastAnimTime.time_since_epoch().count() != 0) {
        deltaSeconds = std::chrono::duration<float>(now - m_lastAnimTime).count();
        deltaSeconds = std::clamp(deltaSeconds, 1.0f / 240.0f, 0.05f);
    }
    m_lastAnimTime = now;

    auto animateAxis = [deltaSeconds](float& current, float target, int count) {
        float diff = target - current;
        float halfRange = static_cast<float>(count) * 0.5f;
        if (diff > halfRange) diff -= static_cast<float>(count);
        if (diff < -halfRange) diff += static_cast<float>(count);

        if (std::abs(diff) <= 0.001f) {
            current = WrapPosition(target, count);
            return false;
        }

        float factor = 1.0f - std::exp(-14.0f * deltaSeconds);
        current = WrapPosition(current + diff * factor, count);
        return true;
    };

    bool hourAnimating = animateAxis(m_hourPosition, m_hourTarget, 24);
    bool minuteAnimating = animateAxis(m_minutePosition, m_minuteTarget, 60);

    m_popupAnim.SetTarget(m_isPopupOpen ? 1.0f : 0.0f);
    bool popupAnimating = m_popupAnim.Tick(deltaSeconds, PopupReveal::kSpec);

    ApplyAnimatedSelection();
    const bool animating = base || hourAnimating || minuteAnimating || popupAnimating;
    if (animating) {
        MarkRenderRectDirty(m_bounds.Inflate(4.0f));
        if (m_isPopupOpen || m_popupAnim.Current() > 0.001f) {
            MarkRenderRectDirty(GetPopupBounds().Inflate(6.0f));
        }
        RequestAnimationTicks();
    }
    return animating;
}

bool TimePicker::HasSelfAnimation() const {
    auto axisAnimating = [](float current, float target, int count) {
        float diff = target - current;
        float halfRange = static_cast<float>(count) * 0.5f;
        if (diff > halfRange) diff -= static_cast<float>(count);
        if (diff < -halfRange) diff += static_cast<float>(count);
        return std::abs(diff) > 0.001f;
    };

    return Control::HasSelfAnimation()
        || (std::abs(m_popupAnim.Target() - m_popupAnim.Current()) > 0.01f)
        || (UIElement::AreAnimationsEnabled() && axisAnimating(m_hourPosition, m_hourTarget, 24))
        || (UIElement::AreAnimationsEnabled() && axisAnimating(m_minutePosition, m_minuteTarget, 60));
}

void TimePicker::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    std::string text = "🕒 " + GetFormattedTime();
    float fontSize = GetFontSize();
    std::string fontFamily = GetFontFamily();
    D2D1_COLOR_F textColor = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextPrimary);

    Rect textRect(m_bounds.x + 10.0f, m_bounds.y, m_bounds.width - 34.0f, m_bounds.height);
    ctx.DrawText(text, textRect, textColor, fontFamily, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    ctx.DrawChevron(
        Rect(m_bounds.x + m_bounds.width - 22.0f, m_bounds.y, 16.0f, m_bounds.height),
        ThemeManager::Instance().GetTokens().textMuted,
        m_isPopupOpen ? GraphicsContext::ChevronDirection::Up : GraphicsContext::ChevronDirection::Down,
        1.7f
    );
}

void TimePicker::SetPopupOpen(bool open) {
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

Rect TimePicker::GetPopupBounds() const {
    return GetPopupRect();
}

bool TimePicker::HitDismissExempt(float x, float y) const {
    if (m_bounds.Contains(x, y)) return true;
    return GetPopupRect().Contains(x, y);
}

void TimePicker::OnRenderOverlay(GraphicsContext& ctx) {
    if (PopupHost::Current() && m_isPopupOpen) return;
    RenderPopup(ctx);
}

void TimePicker::RenderPopup(GraphicsContext& ctx) {
    float progress = UIElement::AreAnimationsEnabled() ? m_popupAnim.Current() : (m_isPopupOpen ? 1.0f : 0.0f);
    if (progress <= 0.001f) return;

    Rect popup = GetPopupRect();
    ctx.PushPopupReveal(popup, progress, Point(popup.x + popup.width * 0.5f, popup.y));

    D2D1_COLOR_F popupBg = ThemeManager::Instance().GetTokens().cardBackground;
    D2D1_COLOR_F border = ThemeManager::Instance().GetTokens().cardBorder;
    D2D1_COLOR_F textPrimary = ThemeManager::Instance().GetTokens().textPrimary;
    D2D1_COLOR_F textSecondary = ThemeManager::Instance().GetTokens().textMuted;
    D2D1_COLOR_F headerAccent = ThemeManager::Instance().GetTokens().accentColor;
    D2D1_COLOR_F overlayFill = ThemeManager::Instance().GetTokens().inputBackground;
    D2D1_COLOR_F overlayBorder = ThemeManager::Instance().GetTokens().inputBorder;
    D2D1_COLOR_F divider = ThemeManager::Instance().GetTokens().cardBorder;

    ctx.FillRoundedRect(popup, 6.0f, popupBg);
    ctx.DrawRoundedRect(popup, 6.0f, border, 1.5f);

    ctx.DrawText("时", Rect(popup.x + 12.0f, popup.y + 6.0f, (popup.width - 34.0f) * 0.5f, 20.0f),
        headerAccent, "微软雅黑", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
    ctx.DrawText("分", Rect(popup.x + popup.width * 0.5f + 5.0f, popup.y + 6.0f, (popup.width - 34.0f) * 0.5f, 20.0f),
        headerAccent, "微软雅黑", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);

    ctx.DrawLine(
        Point(popup.x + 6.0f, popup.y + kHeaderHeight),
        Point(popup.x + popup.width - 6.0f, popup.y + kHeaderHeight),
        divider
    );

    ctx.DrawLine(
        Point(popup.x + popup.width * 0.5f, popup.y + kHeaderHeight + 8.0f),
        Point(popup.x + popup.width * 0.5f, popup.y + popup.height - 44.0f),
        divider
    );

    for (int column = 0; column < 2; ++column) {
        Rect wheel = GetWheelRect(column);
        Rect selection = GetSelectionRect(column);
        float position = (column == 0) ? m_hourPosition : m_minutePosition;
        int count = (column == 0) ? 24 : 60;

        ctx.PushClip(wheel);
        ctx.FillRoundedRect(selection, 4.0f, overlayFill);
        ctx.DrawRoundedRect(selection, 4.0f, overlayBorder, 1.0f);

        float centerY = wheel.y + wheel.height * 0.5f;
        int centerIndex = static_cast<int>(std::round(position));

        for (int offset = -kVisibleRadius; offset <= kVisibleRadius; ++offset) {
            int itemIndex = centerIndex + offset;
            int wrapped = WrapIndex(itemIndex, count);
            float distance = static_cast<float>(itemIndex) - position;
            float y = centerY + distance * kItemHeight;

            Rect itemRect(wheel.x, y - kItemHeight * 0.5f, wheel.width, kItemHeight);
            float absDistance = std::abs(distance);
            float emphasis = std::clamp(1.0f - absDistance / static_cast<float>(kVisibleRadius + 1), 0.15f, 1.0f);
            float alpha = 0.18f + emphasis * 0.82f;
            float fontSize = 12.0f + emphasis * 2.5f;
            D2D1_COLOR_F color = (absDistance < 0.5f)
                ? D2D1::ColorF(textPrimary.r, textPrimary.g, textPrimary.b, 1.0f)
                : D2D1::ColorF(textSecondary.r, textSecondary.g, textSecondary.b, alpha);

            std::stringstream ss;
            ss << std::setw(2) << std::setfill('0') << wrapped;
            ctx.DrawText(
                ss.str(),
                itemRect,
                color,
                "微软雅黑",
                fontSize,
                DWRITE_TEXT_ALIGNMENT_CENTER,
                DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
                (absDistance < 0.5f) ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL
            );
        }

        ctx.FillRect(Rect(wheel.x, wheel.y, wheel.width, 18.0f), D2D1::ColorF(popupBg.r, popupBg.g, popupBg.b, 0.88f));
        ctx.FillRect(Rect(wheel.x, wheel.y + wheel.height - 18.0f, wheel.width, 18.0f), D2D1::ColorF(popupBg.r, popupBg.g, popupBg.b, 0.88f));
        ctx.PopClip();
    }

    Rect footerLine(popup.x + 12.0f, popup.y + popup.height - 40.0f, popup.width - 24.0f, 1.0f);
    ctx.FillRect(footerLine, divider);

    Rect doneRect(popup.x + 12.0f, popup.y + popup.height - 32.0f, popup.width - 24.0f, 20.0f);
    ctx.DrawText("确定", doneRect, textPrimary, "微软雅黑", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);

    ctx.PopPopupReveal();
}

} // namespace CUI
