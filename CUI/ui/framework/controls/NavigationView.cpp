#include "NavigationView.h"
#include <algorithm>

namespace CUI {

NavigationView::NavigationView() {
    SetProperty("background", Value(D2D1::ColorF(0x1E / 255.0f, 0x1E / 255.0f, 0x1E / 255.0f, 1.0f)));
    SetProperty("paneBackground", Value(D2D1::ColorF(0x25 / 255.0f, 0x25 / 255.0f, 0x26 / 255.0f, 1.0f)));
    SetProperty("indicatorColor", Value(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f)));

    m_btnBack = std::make_shared<Button>("←");
    m_btnBack->SetProperty("width", Value(32.0f));
    m_btnBack->SetProperty("height", Value(32.0f));
    m_btnBack->SetProperty("background", Value(D2D1::ColorF(0x2D / 255.0f, 0x2D / 255.0f, 0x2D / 255.0f, 1.0f)));
    m_btnBack->SetProperty("cornerRadius", Value(16.0f));
    m_btnBack->SetProperty("fontSize", Value(14.0f));
    AddChild(m_btnBack);

    m_btnTogglePane = std::make_shared<Button>("☰");
    m_btnTogglePane->SetProperty("width", Value(32.0f));
    m_btnTogglePane->SetProperty("height", Value(32.0f));
    m_btnTogglePane->SetProperty("background", Value(D2D1::ColorF(0x2D / 255.0f, 0x2D / 255.0f, 0x2D / 255.0f, 1.0f)));
    m_btnTogglePane->SetProperty("cornerRadius", Value(4.0f));
    m_btnTogglePane->SetProperty("fontSize", Value(14.0f));
    AddChild(m_btnTogglePane);

    m_btnBack->OnClick().Connect([this](UIElement*) {
        m_backRequestedEvent.Invoke(this, true);
    });

    m_btnTogglePane->OnClick().Connect([this](UIElement*) {
        TogglePane();
    });
}

void NavigationView::AddItem(const std::string& tag, const std::string& content, const std::string& icon, std::shared_ptr<UIElement> page) {
    NavigationViewItemData item;
    item.tag = tag;
    item.content = content;
    item.icon = icon;
    item.page = page;

    if (page) {
        AddChild(page);
        if (m_items.size() != m_selectedIndex) {
            page->SetProperty("visibility", Value("Collapsed"));
        }
    }

    m_items.push_back(item);
    MarkRenderContentDirty();
}

void NavigationView::SetPaneDisplayMode(NavigationViewPaneDisplayMode mode) {
    if (m_paneDisplayMode != mode) {
        m_paneDisplayMode = mode;
        UpdateLayoutRects();
        MarkRenderContentDirty();
    }
}

void NavigationView::SetPaneOpen(bool open) {
    SetPaneDisplayMode(open ? NavigationViewPaneDisplayMode::Expanded : NavigationViewPaneDisplayMode::Compact);
}

void NavigationView::TogglePane() {
    SetPaneOpen(!IsPaneOpen());
}

void NavigationView::SelectItemByTag(const std::string& tag) {
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (m_items[i].tag == tag) {
            m_selectedIndex = static_cast<int>(i);
            m_indicatorAnim.SetTarget(static_cast<float>(m_selectedIndex));
            
            for (size_t k = 0; k < m_items.size(); ++k) {
                if (m_items[k].page) {
                    m_items[k].page->SetProperty("visibility", Value(k == i ? "Visible" : "Collapsed"));
                }
            }

            if (m_autoCollapse) {
                SetPaneOpen(false);
            }

            m_selectionChangedEvent.Invoke(this, m_selectedIndex);
            MarkRenderContentDirty();
            break;
        }
    }
}

Size NavigationView::Measure(Size availableSize) {
    m_desiredSize = availableSize;
    return m_desiredSize;
}

void NavigationView::Arrange(Rect finalRect) {
    SetBounds(finalRect);
    UpdateLayoutRects();
}

void NavigationView::UpdateLayoutRects() {
    float targetW = (m_paneDisplayMode == NavigationViewPaneDisplayMode::Expanded) ? 200.0f : 48.0f;
    float paneW = UIElement::AreAnimationsEnabled() ? m_paneWidthAnim.Current() : targetW;

    m_btnBack->Arrange(Rect(m_bounds.x + 8.0f, m_bounds.y + 6.0f, 32.0f, 32.0f));
    m_btnTogglePane->Arrange(Rect(m_bounds.x + 8.0f, m_bounds.y + 44.0f, 32.0f, 32.0f));

    float currentY = m_bounds.y + 84.0f;

    for (size_t i = 0; i < m_items.size(); ++i) {
        float itemW = (std::max)(36.0f, paneW - 12.0f);
        m_items[i].bounds = Rect(m_bounds.x + 6.0f, currentY, itemW, 36.0f);
        currentY += 40.0f;
    }

    // Content Page Rect
    Rect contentRect(m_bounds.x + paneW, m_bounds.y, (std::max)(0.0f, m_bounds.width - paneW), m_bounds.height);

    for (size_t i = 0; i < m_items.size(); ++i) {
        if (m_items[i].page) {
            m_items[i].page->Measure(Size(contentRect.width, contentRect.height));
            m_items[i].page->Arrange(contentRect);
        }
    }
}

void NavigationView::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    float targetW = (m_paneDisplayMode == NavigationViewPaneDisplayMode::Expanded) ? 200.0f : 48.0f;
    float paneW = UIElement::AreAnimationsEnabled() ? m_paneWidthAnim.Current() : targetW;
    Rect paneRect(m_bounds.x, m_bounds.y, paneW, m_bounds.height);
    D2D1_COLOR_F paneBg = GetProperty("paneBackground").AsColor(D2D1::ColorF(0x25 / 255.0f, 0x25 / 255.0f, 0x26 / 255.0f, 1.0f));
    D2D1_COLOR_F indicatorColor = GetProperty("indicatorColor").AsColor(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f));

    ctx.FillRect(paneRect, paneBg);
    ctx.DrawLine(Point(m_bounds.x + paneW, m_bounds.y), Point(m_bounds.x + paneW, m_bounds.y + m_bounds.height), D2D1::ColorF(0x3E / 255.0f, 0x3E / 255.0f, 0x42 / 255.0f, 1.0f));

    // Header Title (if expanded)
    if (paneW > 80.0f) {
        Rect headerRect(m_bounds.x + 48.0f, m_bounds.y + 44.0f, paneW - 56.0f, 32.0f);
        ctx.PushClip(headerRect);
        ctx.DrawText(m_headerText, headerRect, D2D1::ColorF(0xEE / 255.0f, 0xEE / 255.0f, 0xEE / 255.0f), "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
        ctx.PopClip();
    }

    // Render Items
    for (size_t i = 0; i < m_items.size(); ++i) {
        Rect r = m_items[i].bounds;
        bool isSelected = (static_cast<int>(i) == m_selectedIndex);
        bool isHovered = (static_cast<int>(i) == m_hoveredIndex);

        if (isSelected) {
            ctx.FillRoundedRect(r, 4.0f, D2D1::ColorF(0x37 / 255.0f, 0x37 / 255.0f, 0x3D / 255.0f, 1.0f));
        } else if (isHovered) {
            ctx.FillRoundedRect(r, 4.0f, D2D1::ColorF(0x2D / 255.0f, 0x2D / 255.0f, 0x30 / 255.0f, 1.0f));
        }

        D2D1_COLOR_F textColor = isSelected ? D2D1::ColorF(1.0f, 1.0f, 1.0f) : D2D1::ColorF(0xCC / 255.0f, 0xCC / 255.0f, 0xCC / 255.0f);
        
        // Icon
        float iconX = r.x + 6.0f;
        if (!m_items[i].icon.empty()) {
            ctx.DrawText(m_items[i].icon, Rect(iconX, r.y, 24.0f, r.height), textColor, "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            iconX += 28.0f;
        }

        // Text (if expanded)
        if (paneW > 80.0f) {
            Rect textClipRect(iconX, r.y, (std::max)(0.0f, r.width - (iconX - r.x) - 4.0f), r.height);
            ctx.PushClip(textClipRect);
            ctx.DrawText(m_items[i].content, textClipRect, textColor, "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, isSelected ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL);
            ctx.PopClip();
        }
    }

    // Draw Active Indicator Pill smoothly sliding along items
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_items.size())) {
        float animPos = UIElement::AreAnimationsEnabled() ? m_indicatorAnim.Current() : static_cast<float>(m_selectedIndex);
        int baseIdx = std::clamp(static_cast<int>(std::floor(animPos)), 0, static_cast<int>(m_items.size()) - 1);
        int nextIdx = std::clamp(baseIdx + 1, 0, static_cast<int>(m_items.size()) - 1);
        float frac = animPos - baseIdx;

        Rect r1 = m_items[baseIdx].bounds;
        Rect r2 = m_items[nextIdx].bounds;

        float pillY = r1.y + (r2.y - r1.y) * frac;
        Rect pillRect(r1.x, pillY + 6.0f, 3.0f, r1.height - 12.0f);
        ctx.FillRoundedRect(pillRect, 1.5f, indicatorColor);
    }
}

void NavigationView::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);

    for (size_t i = 0; i < m_items.size(); ++i) {
        if (m_items[i].bounds.Contains(pt.x, pt.y)) {
            m_selectedIndex = static_cast<int>(i);
            m_indicatorAnim.SetTarget(static_cast<float>(m_selectedIndex));

            for (size_t k = 0; k < m_items.size(); ++k) {
                if (m_items[k].page) {
                    m_items[k].page->SetProperty("visibility", Value(k == i ? "Visible" : "Collapsed"));
                }
            }

            if (m_autoCollapse) {
                SetPaneOpen(false);
            }

            m_selectionChangedEvent.Invoke(this, m_selectedIndex);
            MarkRenderContentDirty();
            break;
        }
    }
}

void NavigationView::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);

    int oldHover = m_hoveredIndex;
    m_hoveredIndex = -1;

    for (size_t i = 0; i < m_items.size(); ++i) {
        if (m_items[i].bounds.Contains(pt.x, pt.y)) {
            m_hoveredIndex = static_cast<int>(i);
            break;
        }
    }

    if (oldHover != m_hoveredIndex) {
        MarkRenderContentDirty();
    }
}

void NavigationView::OnMouseLeave() {
    Control::OnMouseLeave();
    if (m_hoveredIndex != -1) {
        m_hoveredIndex = -1;
        MarkRenderContentDirty();
    }
}

bool NavigationView::OnAnimationTick() {
    bool base = Control::OnAnimationTick();
    float dt = UIElement::GetAnimationDeltaSeconds();
    m_indicatorAnim.SetTarget(static_cast<float>(m_selectedIndex));
    float targetWidth = (m_paneDisplayMode == NavigationViewPaneDisplayMode::Expanded) ? 200.0f : 48.0f;
    m_paneWidthAnim.SetTarget(targetWidth);

    bool indicatorAnim = m_indicatorAnim.Tick(dt, AnimationSpec{ 0.55f, 0.01f });
    bool widthAnim = m_paneWidthAnim.Tick(dt, AnimationSpec{ 0.55f, 0.01f });

    if (widthAnim) {
        UpdateLayoutRects();
    }
    return base || indicatorAnim || widthAnim;
}

bool NavigationView::HasSelfAnimation() const {
    return Control::HasSelfAnimation() ||
           std::abs(m_indicatorAnim.Target() - m_indicatorAnim.Current()) > 0.001f ||
           std::abs(m_paneWidthAnim.Target() - m_paneWidthAnim.Current()) > 0.001f;
}

} // namespace CUI
