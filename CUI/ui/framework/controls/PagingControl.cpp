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

    auto styleBtn = [](std::shared_ptr<Button> btn, bool active) {
        if (!btn) return;
        if (active) {
            btn->SetProperty("background", Value(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f)));
            btn->SetProperty("hoverBackground", Value(D2D1::ColorF(0x1C / 255.0f, 0x97 / 255.0f, 0xEA / 255.0f, 1.0f)));
            btn->SetProperty("color", Value(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f)));
            btn->SetProperty("borderBrush", Value(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f)));
        } else {
            btn->SetProperty("background", Value(D2D1::ColorF(0x2D / 255.0f, 0x2D / 255.0f, 0x2D / 255.0f, 1.0f)));
            btn->SetProperty("hoverBackground", Value(D2D1::ColorF(0x3E / 255.0f, 0x3E / 255.0f, 0x42 / 255.0f, 1.0f)));
            btn->SetProperty("color", Value(D2D1::ColorF(0xCC / 255.0f, 0xCC / 255.0f, 0xCC / 255.0f, 1.0f)));
            btn->SetProperty("borderBrush", Value(D2D1::ColorF(0x3E / 255.0f, 0x3E / 255.0f, 0x42 / 255.0f, 1.0f)));
        }
        btn->SetProperty("borderThickness", Value(1.0f));
        btn->SetProperty("cornerRadius", Value(4.0f));
        btn->SetProperty("fontSize", Value(12.0f));
        btn->SetProperty("padding", Value(Thickness(0)));
    };

    styleBtn(m_btnPrev, false);
    styleBtn(m_btnPage1, true);
    styleBtn(m_btnPage2, false);
    styleBtn(m_btnPage3, false);
    styleBtn(m_btnNext, false);

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

    m_btnPage1->OnClick().Connect([this](UIElement*) { SetCurrentPage(m_page1Val); });
    m_btnPage2->OnClick().Connect([this](UIElement*) { SetCurrentPage(m_page2Val); });
    m_btnPage3->OnClick().Connect([this](UIElement*) { SetCurrentPage(m_page3Val); });

    UpdatePageButtons();
}

void PagingControl::UpdatePageButtons() {
    int current = GetCurrentPage();
    int total = GetTotalPages();

    if (total <= 3) {
        m_page1Val = 1;
        m_page2Val = 2;
        m_page3Val = 3;
    } else {
        if (current <= 2) {
            m_page1Val = 1;
            m_page2Val = 2;
            m_page3Val = 3;
        } else if (current >= total - 1) {
            m_page1Val = total - 2;
            m_page2Val = total - 1;
            m_page3Val = total;
        } else {
            m_page1Val = current - 1;
            m_page2Val = current;
            m_page3Val = current + 1;
        }
    }

    auto updateBtn = [current, total](std::shared_ptr<Button> btn, int val) {
        if (!btn) return;
        if (val > total) {
            btn->SetProperty("visibility", Value("Collapsed"));
            return;
        }
        btn->SetProperty("visibility", Value("Visible"));
        btn->SetText(std::to_string(val));
        bool active = (current == val);
        btn->SetProperty("background", Value(D2D1::ColorF(0x2D / 255.0f, 0x2D / 255.0f, 0x2D / 255.0f, 1.0f)));
        btn->SetProperty("color", Value(active ? D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f) : D2D1::ColorF(0xAA / 255.0f, 0xAA / 255.0f, 0xAA / 255.0f, 1.0f)));
        btn->SetProperty("borderBrush", Value(active ? D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 0.8f) : D2D1::ColorF(0x3E / 255.0f, 0x3E / 255.0f, 0x42 / 255.0f, 1.0f)));
        btn->SetProperty("borderThickness", Value(1.0f));
    };

    updateBtn(m_btnPage1, m_page1Val);
    updateBtn(m_btnPage2, m_page2Val);
    updateBtn(m_btnPage3, m_page3Val);
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
    float startX = finalRect.x + (finalRect.width - (5 * btnW + 4 * gap + 70.0f)) * 0.5f;

    m_btnPrev->Arrange(Rect(startX + 0 * (btnW + gap), finalRect.y, btnW, finalRect.height));
    m_btnPage1->Arrange(Rect(startX + 1 * (btnW + gap), finalRect.y, btnW, finalRect.height));
    m_btnPage2->Arrange(Rect(startX + 2 * (btnW + gap), finalRect.y, btnW, finalRect.height));
    m_btnPage3->Arrange(Rect(startX + 3 * (btnW + gap), finalRect.y, btnW, finalRect.height));
    m_btnNext->Arrange(Rect(startX + 4 * (btnW + gap), finalRect.y, btnW, finalRect.height));
}

void PagingControl::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    float btnW = 32.0f;
    float gap = 6.0f;
    float startX = m_bounds.x + (m_bounds.width - (5 * btnW + 4 * gap + 70.0f)) * 0.5f;
    
    // Draw sliding active page indicator bar under page buttons
    int current = GetCurrentPage();
    int total = GetTotalPages();
    float targetIndex = 1.0f;
    if (current == m_page1Val) targetIndex = 1.0f;
    else if (current == m_page2Val) targetIndex = 2.0f;
    else if (current == m_page3Val) targetIndex = 3.0f;

    float activeIndex = UIElement::AreAnimationsEnabled() ? m_pageIndicatorAnim.Current() : targetIndex;
    float pillX = startX + activeIndex * (btnW + gap) + 4.0f;
    float pillY = m_bounds.y + m_bounds.height - 2.0f;
    Rect pillRect(pillX, pillY, btnW - 8.0f, 2.0f);
    ctx.FillRoundedRect(pillRect, 1.0f, D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f));

    // Draw total pages text on right end
    Rect infoRect(startX + 5 * (btnW + gap) + 4.0f, m_bounds.y, 65.0f, m_bounds.height);
    std::string info = "共 " + std::to_string(total) + " 页";
    ctx.DrawText(info, infoRect, D2D1::ColorF(0x88 / 255.0f, 0x88 / 255.0f, 0x88 / 255.0f), "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

bool PagingControl::OnAnimationTick() {
    bool base = Control::OnAnimationTick();
    float dt = UIElement::GetAnimationDeltaSeconds();
    
    int current = GetCurrentPage();
    float targetIndex = 1.0f;
    if (current == m_page1Val) targetIndex = 1.0f;
    else if (current == m_page2Val) targetIndex = 2.0f;
    else if (current == m_page3Val) targetIndex = 3.0f;

    m_pageIndicatorAnim.SetTarget(targetIndex);
    bool indicatorAnim = m_pageIndicatorAnim.Tick(dt, AnimationSpec{ 0.55f, 0.01f });
    return base || indicatorAnim;
}

bool PagingControl::HasSelfAnimation() const {
    return Control::HasSelfAnimation() ||
           std::abs(m_pageIndicatorAnim.Target() - m_pageIndicatorAnim.Current()) > 0.01f;
}

void PagingControl::SetCurrentPage(int page) {
    int total = GetTotalPages();
    page = std::clamp(page, 1, total);
    if (GetCurrentPage() != page) {
        SetProperty("currentPage", Value(page));
        UpdatePageButtons();
        m_onPageChangedEvent.Invoke(this, page);
        MarkRenderContentDirty();
    }
}

void PagingControl::SetTotalPages(int total) {
    int validTotal = (std::max)(1, total);
    SetProperty("totalPages", Value(validTotal));
    if (GetCurrentPage() > validTotal) {
        SetCurrentPage(validTotal);
    } else {
        UpdatePageButtons();
    }
}

} // namespace CUI
