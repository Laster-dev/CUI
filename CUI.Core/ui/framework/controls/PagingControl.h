#pragma once
#include "Control.h"
#include <cstdint>
#include <vector>

namespace CUI {

class PagingControl : public Control {
public:
    PagingControl();
    virtual ~PagingControl() = default;

    virtual const char* GetClassName() const override { return "PagingControl"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void SetProperty(PropertyId id, const Value& val) override;
    virtual HCURSOR GetCursor() const override;

    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual void OnMouseWheel(float delta) override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    int GetCurrentPage() const { return m_currentPage; }
    void SetCurrentPage(int page);

    int GetTotalPages() const { return m_totalPages; }
    void SetTotalPages(int total);

    Event<PagingControl*, int>& OnPageChanged() { return m_onPageChangedEvent; }

private:
    enum class ItemKind : uint8_t { Page, Ellipsis };
    enum class HitTarget : int8_t { None = -1, Prev = -2, Next = -3 };

    struct PageItem {
        ItemKind kind = ItemKind::Page;
        int page = 0;
        Rect bounds;
    };

    static constexpr float kHeight = 32.0f;
    static constexpr float kNavSize = 28.0f;
    static constexpr float kGap = 4.0f;
    static constexpr float kPagePadH = 8.0f;
    static constexpr float kCorner = 6.0f;

    void RebuildPageList();
    void UpdateLayout();
    float MeasureContentWidth() const;
    float PageCellWidth(int page) const;
    HitTarget HitTestTarget(Point pt) const;
    bool IsNavEnabled(bool prev) const;
    void ActivateTarget(HitTarget target);
    void SyncSelectionPillTarget(bool immediate);
    Rect SelectionPillTarget() const;
    void MarkPagingDirty();

    int m_currentPage = 1;
    int m_totalPages = 10;
    std::vector<PageItem> m_pageItems;
    Rect m_prevRect;
    Rect m_nextRect;
    Rect m_infoRect;
    float m_contentWidth = 0.0f;

    HitTarget m_hover = HitTarget::None;
    HitTarget m_pressed = HitTarget::None;
    AnimatedScalar m_pillX{ 0.0f };
    AnimatedScalar m_pillW{ 0.0f };
    Event<PagingControl*, int> m_onPageChangedEvent;
};

} // namespace CUI
