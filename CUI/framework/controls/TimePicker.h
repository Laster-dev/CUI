#pragma once
#include "Control.h"

namespace CUI {

class TimePicker : public Control {
public:
    TimePicker();
    virtual ~TimePicker() = default;

    virtual const char* GetClassName() const override { return "TimePicker"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;

    int GetHour() const { return m_hour; }
    int GetMinute() const { return m_minute; }
    void SetTime(int hour, int minute);
    std::string GetFormattedTime() const;

    Event<TimePicker*, int, int>& OnTimeChanged() { return m_onTimeChangedEvent; }

private:
    int m_hour = 14;
    int m_minute = 30;
    Event<TimePicker*, int, int> m_onTimeChangedEvent;
};

} // namespace CUI
