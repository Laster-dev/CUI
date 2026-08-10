#pragma once
#include "Control.h"
#include "Button.h"

namespace CUI {

class PagingControl : public Control {
public:
    PagingControl();
    virtual ~PagingControl() = default;

    virtual const char* GetClassName() const override { return "PagingControl"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;

    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    int GetCurrentPage() const { return m_currentPage; }
    void SetCurrentPage(int page);

    int GetTotalPages() const { return m_totalPages; }
    void SetTotalPages(int total);

    Event<PagingControl*, int>& OnPageChanged() { return m_onPageChangedEvent; }

private:
    void UpdatePageButtons();

    int m_currentPage = 1;
    int m_totalPages = 10;
    std::shared_ptr<Button> m_btnPrev;
    std::shared_ptr<Button> m_btnNext;
    std::shared_ptr<Button> m_btnPage1;
    std::shared_ptr<Button> m_btnPage2;
    std::shared_ptr<Button> m_btnPage3;
    int m_page1Val = 1;
    int m_page2Val = 2;
    int m_page3Val = 3;
    AnimatedScalar m_pageIndicatorAnim{};
    Event<PagingControl*, int> m_onPageChangedEvent;
};

} // namespace CUI
