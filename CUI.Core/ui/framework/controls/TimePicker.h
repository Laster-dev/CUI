#pragma once
#include "Control.h"
#include "../window/PopupHost.h"
#include <chrono>

namespace CUI {

class TimePicker : public Control, public IPopup {
public:
    TimePicker();
    virtual ~TimePicker() = default;

    virtual const char* GetClassName() const override { return "TimePicker"; }
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void SetProperty(PropertyId id, const Value& val) override;
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

    int GetHour() const { return m_hour; }
    int GetMinute() const { return m_minute; }
    void SetTime(int hour, int minute);
    std::string GetFormattedTime() const;

    Event<TimePicker*, int, int>& OnTimeChanged() { return m_onTimeChangedEvent; }

private:
    Rect GetPopupRect() const;
    Rect GetWheelRect(int column) const;
    Rect GetSelectionRect(int column) const;
    int HitTestColumn(float x, float y) const;
    void NudgeColumn(int column, int delta);
    void SnapTargetsToSelection();
    void ApplyAnimatedSelection();

    int m_hour = 14;
    int m_minute = 30;
    bool m_isPopupOpen = false;

    float m_hourPosition = 14.0f;
    float m_minutePosition = 30.0f;
    float m_hourTarget = 14.0f;
    float m_minuteTarget = 30.0f;
    AnimatedScalar m_popupAnim{};
    std::chrono::steady_clock::time_point m_lastAnimTime{};

    Event<TimePicker*, int, int> m_onTimeChangedEvent;
};

} // namespace CUI
