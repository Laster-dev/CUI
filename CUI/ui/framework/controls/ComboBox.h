#pragma once
#include "Control.h"
#include <vector>
#include <string>

namespace CUI {

class ComboBox : public Control {
public:
    ComboBox();
    virtual ~ComboBox() = default;

    virtual const char* GetClassName() const override { return "ComboBox"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnRenderOverlay(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnBlur() override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual void CollectAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override;

    virtual bool ShouldClipToBounds() const override { return !m_isDropDownOpen; }
    virtual UIElement* HitTest(float x, float y) override;
    virtual UIElement* HitTestOverlay(float x, float y) override;

    void AddItem(const std::string& item);
    void ClearItems();
    const std::vector<std::string>& GetItems() const { return m_items; }

    int GetSelectedIndex() const { return m_selectedIndex; }
    void SetSelectedIndex(int index);

    std::string GetSelectedItem() const;

    Event<ComboBox*, int, const std::string&>& OnSelectionChanged() { return m_onSelectionChangedEvent; }

private:
    std::vector<std::string> m_items;
    int m_selectedIndex = -1;
    bool m_isDropDownOpen = false;
    int m_hoveredIndex = -1;
    AnimatedScalar m_popupAnim{};
    AnimatedScalar m_arrowAnim{};

    Event<ComboBox*, int, const std::string&> m_onSelectionChangedEvent;
};

} // namespace CUI
