#pragma once
#include "Control.h"
#include "FileBrowserHelper.h"
#include "../window/PopupHost.h"
#include <string>
#include <vector>

namespace CUI {

class FilePicker : public Control, public IPopup {
public:
    FilePicker();
    virtual ~FilePicker() = default;

    virtual const char* GetClassName() const override { return "FilePicker"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual HCURSOR GetCursor() const override;
    bool AcceptsTabFocus() const override { return true; }

    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnRenderOverlay(GraphicsContext& ctx) override;
    virtual UIElement* OnHitTestOverlay(float x, float y) override;
    virtual bool NeedsOverlayHitTest() const override { return true; }
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    virtual bool IsPopupOpen() const override { return m_isPopupOpen; }
    virtual Rect GetPopupBounds() const override;
    virtual bool HitDismissExempt(float x, float y) const override;
    virtual UIElement* HitTestPopup(float x, float y) override { return OnHitTestOverlay(x, y); }
    virtual void RenderPopup(GraphicsContext& ctx) override;
    virtual void OnLightDismiss() override { SetPopupOpen(false); }
    // Overlay-composed TreeView / BreadcrumbBar — re-arm ticks if attach raced.
    virtual void CollectPopupOwnedElements(std::vector<UIElement*>& out) const override;

    void SetPopupOpen(bool open);

    const std::string& GetPath() const { return GetText(); }
    void SetPath(const std::string& path);

    const std::string& GetDialogTitle() const { return m_dialogTitle; }
    void SetDialogTitle(const std::string& title) { m_dialogTitle = title; }

    void SetFilter(const std::string& name, const std::string& spec);
    void AddFilter(const std::string& name, const std::string& spec);
    void ClearFilters();

    Event<FilePicker*, const std::string&>& OnPathChanged() { return m_onPathChangedEvent; }

private:
    enum class HitPart : uint8_t { None, Path, Browse };

    Rect PathRect() const;
    Rect BrowseRect() const;
    HitPart HitTestPart(Point pt) const;
    void MarkPickerDirty();
    void UpdateHover(Point pt);
    bool HandleBrowserClick(Point pt);
    void SetFilterDropDownOpen(bool open);
    void SyncBrowserChrome();
    float PopupProgress() const;

    std::string m_dialogTitle{ "选择文件" };
    std::vector<std::pair<std::string, std::string>> m_filters;
    HitPart m_hover = HitPart::None;
    HitPart m_pressed = HitPart::None;

    bool m_isPopupOpen = false;
    AnimatedScalar m_popupAnim{};
    FileBrowserSession m_browser;
    FileBrowserBreadcrumbHost m_breadcrumbHost;
    FileBrowserTreeHost m_treeHost;
    bool m_hoverUp = false;
    bool m_hoverFilter = false;
    bool m_hoverCancel = false;
    bool m_hoverConfirm = false;
    bool m_filterDropDownOpen = false;
    int m_hoverFilterItem = -1;

    Event<FilePicker*, const std::string&> m_onPathChangedEvent;
};

} // namespace CUI
