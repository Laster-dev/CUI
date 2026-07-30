#pragma once
#include "Control.h"
#include <ctime>

namespace CUI {

class DatePicker : public Control {
public:
    DatePicker();
    virtual ~DatePicker() = default;

    virtual const char* GetClassName() const override { return "DatePicker"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnRenderOverlay(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;

    bool IsPopupOpen() const { return m_isPopupOpen; }
    void SetPopupOpen(bool open) { m_isPopupOpen = open; }

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
    bool m_isPopupOpen = false;
    Event<DatePicker*, int, int, int> m_onDateChangedEvent;
};

} // namespace CUI
