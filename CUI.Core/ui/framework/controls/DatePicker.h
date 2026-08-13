#pragma once
#include "Control.h"
#include "ScrollbarAutoHide.h"
#include "../window/PopupHost.h"
#include <ctime>

namespace CUI {

enum class DatePickerViewMode {
    DayGrid,
    MonthGrid,
    YearGrid
};

class DatePicker : public Control, public IPopup {
public:
    DatePicker();
    virtual ~DatePicker() = default;

    virtual const char* GetClassName() const override { return "DatePicker"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnRenderOverlay(GraphicsContext& ctx) override;
    virtual UIElement* OnHitTestOverlay(float x, float y) override;
    virtual bool NeedsOverlayHitTest() const override { return true; }
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseWheel(float delta) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    // IPopup
    virtual bool IsPopupOpen() const override { return m_isPopupOpen; }
    virtual Rect GetPopupBounds() const override;
    virtual bool HitDismissExempt(float x, float y) const override;
    virtual UIElement* HitTestPopup(float x, float y) override { return OnHitTestOverlay(x, y); }
    virtual void RenderPopup(GraphicsContext& ctx) override;
    virtual void OnLightDismiss() override { SetPopupOpen(false); }

    void SetPopupOpen(bool open);

    int GetYear() const { return m_year; }
    int GetMonth() const { return m_month; }
    int GetDay() const { return m_day; }
    void SetDate(int year, int month, int day);
    std::string GetFormattedDate() const;

    Event<DatePicker*, int, int, int>& OnDateChanged() { return m_onDateChangedEvent; }

private:
    int m_year = 2026;
    int m_month = 7;
    int m_day = 30;
    int m_viewStartYear = 2020;
    DatePickerViewMode m_viewMode = DatePickerViewMode::DayGrid;
    bool m_isPopupOpen = false;
    // When popup content exceeds visible height, allow browsing via internal scrolling.
    float m_scrollOffset = 0.0f; // layout/DIP coords; shifts the grid area upward
    ScrollbarAutoHide m_scrollbarAutoHide;
    AnimatedScalar m_popupAnim{};
    Event<DatePicker*, int, int, int> m_onDateChangedEvent;
};

} // namespace CUI
