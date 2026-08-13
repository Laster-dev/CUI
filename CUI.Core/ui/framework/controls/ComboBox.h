#pragma once
#include "Control.h"
#include "ScrollbarAutoHide.h"
#include "../window/PopupHost.h"
#include <vector>
#include <string>

namespace CUI {

class ComboBox : public Control, public IPopup {
public:
    ComboBox();
    virtual ~ComboBox() = default;

    virtual const char* GetClassName() const override { return "ComboBox"; }
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnRenderOverlay(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual bool AcceptsTabFocus() const override { return true; }
    virtual void OnBlur() override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual void CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override;
    virtual void CollectAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override;

    virtual bool ShouldClipToBounds() const override { return !m_isDropDownOpen; }
    virtual UIElement* HitTest(float x, float y) override;
    virtual UIElement* HitTestOverlay(float x, float y) override;
    virtual void OnMouseWheel(float delta) override;

    // IPopup
    virtual bool IsPopupOpen() const override { return m_isDropDownOpen; }
    virtual Rect GetPopupBounds() const override;
    virtual bool HitDismissExempt(float x, float y) const override;
    virtual UIElement* HitTestPopup(float x, float y) override { return HitTestOverlay(x, y); }
    virtual void RenderPopup(GraphicsContext& ctx) override;
    virtual void OnLightDismiss() override { SetDropDownOpen(false); }

    void SetProperty(PropertyId id, const Value& val) override;

    void AddItem(const std::string& item);
    void ClearItems();
    void SetItems(const std::string& itemsCsv);
    const std::vector<std::string>& GetItems() const { return m_items; }

    int GetSelectedIndex() const { return m_selectedIndex; }
    void SetSelectedIndex(int index);

    std::string GetSelectedItem() const;

    void SetDropDownOpen(bool open);
    bool IsDropDownOpen() const { return m_isDropDownOpen; }

    Event<ComboBox*, int, const std::string&>& OnSelectionChanged() { return m_onSelectionChangedEvent; }

private:
    std::vector<std::string> m_items;
    int m_selectedIndex = -1;
    bool m_isDropDownOpen = false;
    int m_hoveredIndex = -1;
    // When dropdown content exceeds the visible height (auto-clamped by placement),
    // keep an internal scroll offset so users can browse instead of only clipping.
    float m_scrollOffset = 0.0f;
    ScrollbarAutoHide m_scrollbarAutoHide;
    AnimatedScalar m_popupAnim{};
    AnimatedScalar m_arrowAnim{};

    Event<ComboBox*, int, const std::string&> m_onSelectionChangedEvent;
};

} // namespace CUI
