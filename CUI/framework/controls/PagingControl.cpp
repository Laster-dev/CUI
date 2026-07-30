#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "PagingControl.h"
#include <algorithm>

namespace CUI {

PagingControl::PagingControl() {
    SetProperty("currentPage", Value(1));
    SetProperty("totalPages", Value(10));
    SetProperty("width", Value(260.0f));
    SetProperty("height", Value(30.0f));

    m_btnPrev = std::make_shared<Button>("<");
    m_btnNext = std::make_shared<Button>(">");
    m_btnPage1 = std::make_shared<Button>("1");
    m_btnPage2 = std::make_shared<Button>("2");
    m_btnPage3 = std::make_shared<Button>("3");

    AddChild(m_btnPrev);
    AddChild(m_btnPage1);
    AddChild(m_btnPage2);
    AddChild(m_btnPage3);
    AddChild(m_btnNext);

    m_btnPrev->OnClick().Connect([this](UIElement*) {
        SetCurrentPage(GetCurrentPage() - 1);
    });

    m_btnNext->OnClick().Connect([this](UIElement*) {
        SetCurrentPage(GetCurrentPage() + 1);
    });

    m_btnPage1->OnClick().Connect([this](UIElement*) { SetCurrentPage(1); });
    m_btnPage2->OnClick().Connect([this](UIElement*) { SetCurrentPage(2); });
    m_btnPage3->OnClick().Connect([this](UIElement*) { SetCurrentPage(3); });
}

std::vector<PropertyMeta> PagingControl::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "currentPage", "当前页 (CurrentPage)", "分页配置", "number" });
    metas.push_back({ "totalPages", "总页数 (TotalPages)", "分页配置", "number" });
    return metas;
}

Size PagingControl::Measure(Size availableSize) {
    float expW = GetProperty("width").AsFloat(260.0f);
    float expH = GetProperty("height").AsFloat(30.0f);
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

void PagingControl::Arrange(Rect finalRect) {
    m_bounds = finalRect;
    float btnW = 32.0f;
    float gap = 6.0f;
    float startX = finalRect.x + (finalRect.width - (5 * btnW + 4 * gap)) * 0.5f;

    m_btnPrev->Arrange(Rect(startX + 0 * (btnW + gap), finalRect.y, btnW, finalRect.height));
    m_btnPage1->Arrange(Rect(startX + 1 * (btnW + gap), finalRect.y, btnW, finalRect.height));
    m_btnPage2->Arrange(Rect(startX + 2 * (btnW + gap), finalRect.y, btnW, finalRect.height));
    m_btnPage3->Arrange(Rect(startX + 3 * (btnW + gap), finalRect.y, btnW, finalRect.height));
    m_btnNext->Arrange(Rect(startX + 4 * (btnW + gap), finalRect.y, btnW, finalRect.height));
}

void PagingControl::SetCurrentPage(int page) {
    int total = GetTotalPages();
    page = std::clamp(page, 1, total);
    if (GetCurrentPage() != page) {
        SetProperty("currentPage", Value(page));
        m_onPageChangedEvent.Invoke(this, page);
    }
}

void PagingControl::SetTotalPages(int total) {
    SetProperty("totalPages", Value((std::max)(1, total)));
}

} // namespace CUI
