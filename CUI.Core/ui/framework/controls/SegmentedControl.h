#pragma once
#include "Control.h"
#include "../animation/AnimationSystem.h"
#include <string>
#include <vector>

namespace CUI {

// Mutually exclusive segments in one bar (iOS Segmented Control / WinUI Segmented).
// Same selection model as ComboBox, without a dropdown.
class SegmentedControl : public Control {
public:
    SegmentedControl();
    virtual ~SegmentedControl() = default;

    virtual const char* GetClassName() const override { return "SegmentedControl"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual bool AcceptsTabFocus() const override { return true; }
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    void SetProperty(PropertyId id, const Value& val) override;

    void AddItem(const std::string& item);
    void ClearItems();
    void SetItems(const std::string& itemsCsv);
    const std::vector<std::string>& GetItems() const { return m_items; }

    int GetSelectedIndex() const { return m_selectedIndex; }
    void SetSelectedIndex(int index);

    std::string GetSelectedItem() const;

    Event<SegmentedControl*, int, const std::string&>& OnSelectionChanged() { return m_onSelectionChangedEvent; }

private:
    Rect SegmentRect(int index) const;
    int HitTestIndex(Point pt) const;
    void SyncPill(bool snap);
    float MeasureContentWidth() const;

    std::vector<std::string> m_items;
    int m_selectedIndex = -1;
    int m_hoverIndex = -1;
    int m_pressedIndex = -1;
    AnimatedScalar m_pillX{ 0.0f };
    AnimatedScalar m_pillW{ 0.0f };
    Event<SegmentedControl*, int, const std::string&> m_onSelectionChangedEvent;
};

} // namespace CUI
