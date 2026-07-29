#include "Layout.h"
#include "../controls/UIElement.h"
#include <algorithm>

namespace CUI {

Size LayoutEngine::MeasureElement(UIElement* element, Size availableSize) {
    if (!element) return Size(0, 0);

    Thickness margin = element->GetProperty("margin").AsThickness(Thickness(0));
    Thickness padding = element->GetProperty("padding").AsThickness(Thickness(0));

    float explicitWidth = element->GetProperty("width").AsFloat(-1.0f);
    float explicitHeight = element->GetProperty("height").AsFloat(-1.0f);
    float minWidth = element->GetProperty("minWidth").AsFloat(0.0f);
    float minHeight = element->GetProperty("minHeight").AsFloat(0.0f);

    Size contentAvailable(
        availableSize.width - margin.left - margin.right - padding.left - padding.right,
        availableSize.height - margin.top - margin.bottom - padding.top - padding.bottom
    );
    if (contentAvailable.width < 0) contentAvailable.width = 0;
    if (contentAvailable.height < 0) contentAvailable.height = 0;

    Size contentSize(0, 0);

    // If element has children, measure children as a flex panel
    const auto& children = element->GetChildren();
    if (!children.empty()) {
        std::string orientStr = element->GetProperty("orientation").AsString("Vertical");
        Orientation orient = (orientStr == "Horizontal" || orientStr == "Row") ? Orientation::Horizontal : Orientation::Vertical;
        float gap = element->GetProperty("gap").AsFloat(0.0f);

        contentSize = MeasureFlexPanel(element, contentAvailable, orient, Alignment::Start, gap);
    }

    float finalW = (explicitWidth >= 0.0f) ? explicitWidth : contentSize.width + padding.left + padding.right;
    float finalH = (explicitHeight >= 0.0f) ? explicitHeight : contentSize.height + padding.top + padding.bottom;

    finalW = (std::max)(finalW, minWidth);
    finalH = (std::max)(finalH, minHeight);

    return Size(finalW + margin.left + margin.right, finalH + margin.top + margin.bottom);
}

void LayoutEngine::ArrangeElement(UIElement* element, Rect finalRect) {
    if (!element) return;

    const auto& children = element->GetChildren();
    if (children.empty()) return;

    Thickness margin = element->GetProperty("margin").AsThickness(Thickness(0));
    Thickness padding = element->GetProperty("padding").AsThickness(Thickness(0));

    Rect contentRect(
        finalRect.x + margin.left + padding.left,
        finalRect.y + margin.top + padding.top,
        finalRect.width - margin.left - margin.right - padding.left - padding.right,
        finalRect.height - margin.top - margin.bottom - padding.top - padding.bottom
    );
    if (contentRect.width < 0) contentRect.width = 0;
    if (contentRect.height < 0) contentRect.height = 0;

    std::string orientStr = element->GetProperty("orientation").AsString("Vertical");
    Orientation orient = (orientStr == "Horizontal" || orientStr == "Row") ? Orientation::Horizontal : Orientation::Vertical;
    float gap = element->GetProperty("gap").AsFloat(0.0f);

    ArrangeFlexPanel(element, contentRect, orient, Alignment::Start, gap);
}

Size LayoutEngine::MeasureFlexPanel(UIElement* panel, Size availableSize, Orientation orientation, Alignment align, float gap) {
    float totalMain = 0.0f;
    float maxCross = 0.0f;

    const auto& children = panel->GetChildren();
    bool first = true;
    for (auto& child : children) {
        std::string vis = child->GetProperty("visibility").AsString("Visible");
        if (vis == "Collapsed") continue;

        Size childDesired = child->Measure(availableSize);

        if (!first) {
            totalMain += gap;
        }
        first = false;

        if (orientation == Orientation::Horizontal) {
            totalMain += childDesired.width;
            maxCross = (std::max)(maxCross, childDesired.height);
        } else {
            totalMain += childDesired.height;
            maxCross = (std::max)(maxCross, childDesired.width);
        }
    }

    if (orientation == Orientation::Horizontal) {
        return Size(totalMain, maxCross);
    } else {
        return Size(maxCross, totalMain);
    }
}

void LayoutEngine::ArrangeFlexPanel(UIElement* panel, Rect finalRect, Orientation orientation, Alignment align, float gap) {
    const auto& children = panel->GetChildren();
    if (children.empty()) return;

    float totalFlexGrow = 0.0f;
    float usedMain = 0.0f;
    int visibleCount = 0;

    for (auto& child : children) {
        std::string vis = child->GetProperty("visibility").AsString("Visible");
        if (vis == "Collapsed") continue;

        visibleCount++;
        Size dSize = child->GetDesiredSize();

        float flex = child->GetProperty("flexGrow").AsFloat(0.0f);
        totalFlexGrow += flex;

        if (orientation == Orientation::Horizontal) {
            usedMain += dSize.width;
        } else {
            usedMain += dSize.height;
        }
    }

    if (visibleCount > 1) {
        usedMain += gap * (visibleCount - 1);
    }

    float availableMain = (orientation == Orientation::Horizontal) ? finalRect.width : finalRect.height;
    float extraMain = (std::max)(0.0f, availableMain - usedMain);

    float currentMain = (orientation == Orientation::Horizontal) ? finalRect.x : finalRect.y;
    float crossStart = (orientation == Orientation::Horizontal) ? finalRect.y : finalRect.x;
    float crossSize = (orientation == Orientation::Horizontal) ? finalRect.height : finalRect.width;

    for (auto& child : children) {
        std::string vis = child->GetProperty("visibility").AsString("Visible");
        if (vis == "Collapsed") continue;

        Size dSize = child->GetDesiredSize();
        float flex = child->GetProperty("flexGrow").AsFloat(0.0f);

        float childMainSize = (orientation == Orientation::Horizontal) ? dSize.width : dSize.height;
        if (totalFlexGrow > 0.0f && flex > 0.0f) {
            childMainSize += extraMain * (flex / totalFlexGrow);
        }

        std::string childAlign = child->GetProperty("align").AsString("Stretch");
        float childCrossSize = (orientation == Orientation::Horizontal) ? dSize.height : dSize.width;
        if (childAlign == "Stretch" || childCrossSize == 0.0f) {
            childCrossSize = crossSize;
        }

        Rect childRect;
        if (orientation == Orientation::Horizontal) {
            childRect = Rect(currentMain, crossStart, childMainSize, childCrossSize);
            currentMain += childMainSize + gap;
        } else {
            childRect = Rect(crossStart, currentMain, childCrossSize, childMainSize);
            currentMain += childMainSize + gap;
        }

        child->Arrange(childRect);
    }
}

} // namespace CUI
