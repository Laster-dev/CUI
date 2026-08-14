#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Layout.h"
#include "../controls/UIElement.h"
#include "../controls/Panel.h"
#include <algorithm>
#include <sstream>
#include <cmath>

namespace CUI {

GridLength GridLength::Parse(const std::string& str) {
    if (str == "Auto" || str == "auto") {
        return GridLength::Auto();
    }
    if (!str.empty() && str.back() == '*') {
        std::string weightStr = str.substr(0, str.length() - 1);
        float weight = weightStr.empty() ? 1.0f : static_cast<float>(atof(weightStr.c_str()));
        return GridLength::Star(weight);
    }
    return GridLength::Pixel(static_cast<float>(atof(str.c_str())));
}

Size LayoutEngine::MeasureElement(UIElement* element, Size availableSize) {
    if (!element) return Size(0, 0);

    Thickness margin = element->GetMargin();
    Thickness padding = element->GetPadding();

    float explicitWidth = element->GetWidth();
    float explicitHeight = element->GetHeight();
    float minWidth = element->GetMinWidth();
    float minHeight = element->GetMinHeight();

    Size contentAvailable(
        availableSize.width - margin.left - margin.right - padding.left - padding.right,
        availableSize.height - margin.top - margin.bottom - padding.top - padding.bottom
    );
    if (contentAvailable.width < 0) contentAvailable.width = 0;
    if (contentAvailable.height < 0) contentAvailable.height = 0;

    Size contentSize(0, 0);

    if (auto* panel = dynamic_cast<Panel*>(element)) {
        contentSize = panel->MeasureOverride(contentAvailable);
    } else if (!element->GetChildren().empty()) {
        Orientation orient = element->GetOrientation();
        float gap = element->GetGap();
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

    // finalRect is already the element's arranged bounds (Margin applied by UIElement::Arrange).
    // Only inset Padding for child/content layout.
    Thickness padding = element->GetPadding();

    Rect contentRect(
        finalRect.x + padding.left,
        finalRect.y + padding.top,
        finalRect.width - padding.left - padding.right,
        finalRect.height - padding.top - padding.bottom
    );
    if (contentRect.width < 0) contentRect.width = 0;
    if (contentRect.height < 0) contentRect.height = 0;

    if (auto* panel = dynamic_cast<Panel*>(element)) {
        panel->ArrangeOverride(contentRect);
    } else if (!element->GetChildren().empty()) {
        Orientation orient = element->GetOrientation();
        float gap = element->GetGap();
        ArrangeFlexPanel(element, contentRect, orient, Alignment::Start, gap);
    }
}

Size LayoutEngine::MeasureFlexPanel(UIElement* panel, Size availableSize, Orientation orientation, Alignment align, float gap) {
    float totalMain = 0.0f;
    float maxCross = 0.0f;

    const auto& children = panel->GetChildren();
    bool first = true;
    for (auto& child : children) {
        if (child->GetVisibility() == Visibility::Collapsed) continue;

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

    std::vector<UIElement*> visibleChildren;
    std::vector<float> mainSizes;
    std::vector<float> flexFactors;
    std::vector<float> minMainSizes;
    float totalFlexGrow = 0.0f;
    float usedMain = 0.0f;

    for (auto& child : children) {
        if (child->GetVisibility() == Visibility::Collapsed) continue;

        Size dSize = child->GetDesiredSize();
        float flex = child->GetFlexGrow();
        float explicitMain = (orientation == Orientation::Horizontal)
            ? child->GetWidth()
            : child->GetHeight();
        float baseMain = explicitMain >= 0.0f
            ? explicitMain
            : ((orientation == Orientation::Horizontal) ? dSize.width : dSize.height);
        float minMain = (orientation == Orientation::Horizontal)
            ? child->GetMinWidth()
            : child->GetMinHeight();

        visibleChildren.push_back(child.get());
        mainSizes.push_back(std::max(baseMain, minMain));
        flexFactors.push_back(std::max(0.0f, flex));
        minMainSizes.push_back(std::max(0.0f, minMain));
        usedMain += mainSizes.back();
        totalFlexGrow += flexFactors.back();
    }

    if (visibleChildren.size() > 1) {
        usedMain += gap * static_cast<float>(visibleChildren.size() - 1);
    }

    float availableMain = (orientation == Orientation::Horizontal) ? finalRect.width : finalRect.height;
    float remaining = availableMain - usedMain;

    if (remaining > 0.0f && totalFlexGrow > 0.0f) {
        // Grow every flex item, including one with an explicit flex-basis.
        for (size_t i = 0; i < mainSizes.size(); ++i) {
            if (flexFactors[i] > 0.0f) {
                mainSizes[i] += remaining * (flexFactors[i] / totalFlexGrow);
            }
        }
    } else if (remaining < 0.0f) {
        float deficit = -remaining;

        auto shrinkToFit = [&](bool flexOnly) {
            for (int pass = 0; pass < 2 && deficit > 0.01f; ++pass) {
                float shrinkWeight = 0.0f;
                for (size_t i = 0; i < mainSizes.size(); ++i) {
                    if (mainSizes[i] <= minMainSizes[i]) continue;
                    if (flexOnly && flexFactors[i] <= 0.0f) continue;
                    shrinkWeight += flexOnly ? flexFactors[i] : (mainSizes[i] - minMainSizes[i]);
                }
                if (shrinkWeight <= 0.0f) break;

                float removed = 0.0f;
                for (size_t i = 0; i < mainSizes.size(); ++i) {
                    if (mainSizes[i] <= minMainSizes[i]) continue;
                    if (flexOnly && flexFactors[i] <= 0.0f) continue;
                    float weight = flexOnly ? flexFactors[i] : (mainSizes[i] - minMainSizes[i]);
                    float requested = deficit * (weight / shrinkWeight);
                    float actual = std::min(requested, mainSizes[i] - minMainSizes[i]);
                    mainSizes[i] -= actual;
                    removed += actual;
                }
                if (removed <= 0.01f) break;
                deficit -= removed;
            }
        };

        shrinkToFit(true);
        shrinkToFit(false);
    }

    float currentMain = (orientation == Orientation::Horizontal) ? finalRect.x : finalRect.y;
    float crossStart = (orientation == Orientation::Horizontal) ? finalRect.y : finalRect.x;
    float crossSize = (orientation == Orientation::Horizontal) ? finalRect.height : finalRect.width;

    for (size_t i = 0; i < visibleChildren.size(); ++i) {
        UIElement* child = visibleChildren[i];
        Size dSize = child->GetDesiredSize();
        float expCross = (orientation == Orientation::Horizontal) ? child->GetHeight() : child->GetWidth();

        float childCrossSize = (orientation == Orientation::Horizontal) ? dSize.height : dSize.width;
        if (expCross >= 0.0f) {
            childCrossSize = expCross;
        }
        float childCrossPos = crossStart;

        Alignment crossAlign = Alignment::Stretch;
        if (orientation == Orientation::Horizontal) {
            Alignment av = child->GetAlignVertical();
            Alignment a = child->GetAlign();
            crossAlign = (av != Alignment::Stretch) ? av : a;
        } else {
            Alignment ah = child->GetAlignHorizontal();
            Alignment a = child->GetAlign();
            crossAlign = (ah != Alignment::Stretch) ? ah : a;
        }

        if (expCross >= 0.0f || crossAlign == Alignment::Center || crossAlign == Alignment::Start || crossAlign == Alignment::End) {
            if (crossAlign == Alignment::Center) {
                childCrossPos = crossStart + (crossSize - childCrossSize) / 2.0f;
            } else if (crossAlign == Alignment::End) {
                childCrossPos = crossStart + crossSize - childCrossSize;
            }
        } else if (crossAlign == Alignment::Stretch) {
            childCrossSize = crossSize;
        }

        Rect childRect;
        if (orientation == Orientation::Horizontal) {
            childRect = Rect(currentMain, childCrossPos, mainSizes[i], childCrossSize);
            currentMain += mainSizes[i] + gap;
        } else {
            childRect = Rect(childCrossPos, currentMain, childCrossSize, mainSizes[i]);
            currentMain += mainSizes[i] + gap;
        }

        child->Arrange(childRect);
    }
}

// ----------------------------------------------------------------------
// Canvas Layout Implementation
// ----------------------------------------------------------------------
Size LayoutEngine::MeasureCanvas(UIElement* panel, Size availableSize) {
    Size infiniteSize(100000.0f, 100000.0f);
    for (auto& child : panel->GetChildren()) {
        if (child->GetVisibility() == Visibility::Collapsed) continue;

        child->Measure(infiniteSize);
    }
    return Size(0.0f, 0.0f);
}

void LayoutEngine::ArrangeCanvas(UIElement* panel, Rect finalRect) {
    for (auto& child : panel->GetChildren()) {
        if (child->GetVisibility() == Visibility::Collapsed) continue;

        Size dSize = child->GetDesiredSize();
        float x = finalRect.x;
        float y = finalRect.y;

        float left = child->GetCanvasLeft();
        float top = child->GetCanvasTop();
        float right = child->GetCanvasRight();
        float bottom = child->GetCanvasBottom();

        if (left != UIElement::kAttachedUnset) {
            x = finalRect.x + left;
        } else if (right != UIElement::kAttachedUnset) {
            x = finalRect.x + finalRect.width - dSize.width - right;
        }

        if (top != UIElement::kAttachedUnset) {
            y = finalRect.y + top;
        } else if (bottom != UIElement::kAttachedUnset) {
            y = finalRect.y + finalRect.height - dSize.height - bottom;
        }

        child->Arrange(Rect(x, y, dSize.width, dSize.height));
    }
}

// ----------------------------------------------------------------------
// Grid Layout Implementation
// ----------------------------------------------------------------------
Size LayoutEngine::MeasureGrid(UIElement* panel, Size availableSize) {
    Grid* grid = dynamic_cast<Grid*>(panel);
    if (!grid) return Size(0, 0);

    auto& cols = grid->GetColumnDefinitions();
    auto& rows = grid->GetRowDefinitions();

    size_t colCount = cols.empty() ? 1 : cols.size();
    size_t rowCount = rows.empty() ? 1 : rows.size();

    std::vector<ColumnDefinition> localCols = cols.empty() ? std::vector<ColumnDefinition>{ ColumnDefinition() } : cols;
    std::vector<RowDefinition> localRows = rows.empty() ? std::vector<RowDefinition>{ RowDefinition() } : rows;

    // 1. Reset actual sizes
    for (auto& c : localCols) c.actualWidth = 0.0f;
    for (auto& r : localRows) r.actualHeight = 0.0f;

    // 2. Measure Pixel columns and rows
    float fixedWidth = 0.0f;
    float starWidthWeight = 0.0f;
    for (auto& c : localCols) {
        if (c.width.unitType == GridUnitType::Pixel) {
            c.actualWidth = c.width.value;
            fixedWidth += c.actualWidth;
        } else if (c.width.unitType == GridUnitType::Star) {
            starWidthWeight += c.width.value;
        }
    }

    float fixedHeight = 0.0f;
    float starHeightWeight = 0.0f;
    for (auto& r : localRows) {
        if (r.height.unitType == GridUnitType::Pixel) {
            r.actualHeight = r.height.value;
            fixedHeight += r.actualHeight;
        } else if (r.height.unitType == GridUnitType::Star) {
            starHeightWeight += r.height.value;
        }
    }

    // 3. Measure children to solve Auto / Star
    for (auto& child : grid->GetChildren()) {
        if (child->GetVisibility() == Visibility::Collapsed) continue;

        int cIdx = (std::min)(child->GetGridColumn(), (int)colCount - 1);
        int rIdx = (std::min)(child->GetGridRow(), (int)rowCount - 1);

        Size childAvail(availableSize.width, availableSize.height);
        if (localCols[cIdx].width.unitType == GridUnitType::Pixel) childAvail.width = localCols[cIdx].actualWidth;
        if (localRows[rIdx].height.unitType == GridUnitType::Pixel) childAvail.height = localRows[rIdx].actualHeight;

        Size dSize = child->Measure(childAvail);

        if (localCols[cIdx].width.unitType == GridUnitType::Auto) {
            localCols[cIdx].actualWidth = (std::max)(localCols[cIdx].actualWidth, dSize.width);
        }
        if (localRows[rIdx].height.unitType == GridUnitType::Auto) {
            localRows[rIdx].actualHeight = (std::max)(localRows[rIdx].actualHeight, dSize.height);
        }
    }

    // Calculate auto total
    float autoWidth = 0.0f;
    for (auto& c : localCols) {
        if (c.width.unitType == GridUnitType::Auto) autoWidth += c.actualWidth;
    }
    float autoHeight = 0.0f;
    for (auto& r : localRows) {
        if (r.height.unitType == GridUnitType::Auto) autoHeight += r.actualHeight;
    }

    // Solve Star sizes
    float availStarWidth = (std::max)(0.0f, availableSize.width - fixedWidth - autoWidth);
    for (auto& c : localCols) {
        if (c.width.unitType == GridUnitType::Star) {
            c.actualWidth = starWidthWeight > 0 ? (availStarWidth * (c.width.value / starWidthWeight)) : 0.0f;
        }
    }

    float availStarHeight = (std::max)(0.0f, availableSize.height - fixedHeight - autoHeight);
    for (auto& r : localRows) {
        if (r.height.unitType == GridUnitType::Star) {
            r.actualHeight = starHeightWeight > 0 ? (availStarHeight * (r.height.value / starHeightWeight)) : 0.0f;
        }
    }

    float totalW = 0.0f;
    for (auto& c : localCols) totalW += c.actualWidth;
    float totalH = 0.0f;
    for (auto& r : localRows) totalH += r.actualHeight;

    return Size(totalW, totalH);
}

void LayoutEngine::ArrangeGrid(UIElement* panel, Rect finalRect) {
    Grid* grid = dynamic_cast<Grid*>(panel);
    if (!grid) return;

    auto localCols = grid->GetColumnDefinitions();
    auto localRows = grid->GetRowDefinitions();

    if (localCols.empty()) localCols.push_back(ColumnDefinition());
    if (localRows.empty()) localRows.push_back(RowDefinition());

    size_t colCount = localCols.size();
    size_t rowCount = localRows.size();

    // Calculate Column Widths
    float fixedWidth = 0.0f, autoWidth = 0.0f, starWidthWeight = 0.0f;
    for (auto& c : localCols) {
        if (c.width.unitType == GridUnitType::Pixel) fixedWidth += c.width.value;
        else if (c.width.unitType == GridUnitType::Star) starWidthWeight += c.width.value;
    }

    // Re-measure auto columns from children
    for (auto& child : grid->GetChildren()) {
        if (child->GetVisibility() == Visibility::Collapsed) continue;
        int cIdx = (std::min)(child->GetGridColumn(), (int)colCount - 1);
        if (localCols[cIdx].width.unitType == GridUnitType::Auto) {
            localCols[cIdx].actualWidth = (std::max)(localCols[cIdx].actualWidth, child->GetDesiredSize().width);
        }
    }
    for (auto& c : localCols) {
        if (c.width.unitType == GridUnitType::Auto) autoWidth += c.actualWidth;
        else if (c.width.unitType == GridUnitType::Pixel) c.actualWidth = c.width.value;
    }

    float availStarWidth = (std::max)(0.0f, finalRect.width - fixedWidth - autoWidth);
    for (auto& c : localCols) {
        if (c.width.unitType == GridUnitType::Star) {
            c.actualWidth = starWidthWeight > 0 ? (availStarWidth * (c.width.value / starWidthWeight)) : 0.0f;
        }
    }

    // Calculate Row Heights
    float fixedHeight = 0.0f, autoHeight = 0.0f, starHeightWeight = 0.0f;
    for (auto& r : localRows) {
        if (r.height.unitType == GridUnitType::Pixel) fixedHeight += r.height.value;
        else if (r.height.unitType == GridUnitType::Star) starHeightWeight += r.height.value;
    }

    for (auto& child : grid->GetChildren()) {
        if (child->GetVisibility() == Visibility::Collapsed) continue;
        int rIdx = (std::min)(child->GetGridRow(), (int)rowCount - 1);
        if (localRows[rIdx].height.unitType == GridUnitType::Auto) {
            localRows[rIdx].actualHeight = (std::max)(localRows[rIdx].actualHeight, child->GetDesiredSize().height);
        }
    }
    for (auto& r : localRows) {
        if (r.height.unitType == GridUnitType::Auto) autoHeight += r.actualHeight;
        else if (r.height.unitType == GridUnitType::Pixel) r.actualHeight = r.height.value;
    }

    float availStarHeight = (std::max)(0.0f, finalRect.height - fixedHeight - autoHeight);
    for (auto& r : localRows) {
        if (r.height.unitType == GridUnitType::Star) {
            r.actualHeight = starHeightWeight > 0 ? (availStarHeight * (r.height.value / starHeightWeight)) : 0.0f;
        }
    }

    // Compute cell positions
    float curX = finalRect.x;
    for (auto& c : localCols) {
        c.position = curX;
        curX += c.actualWidth;
    }

    float curY = finalRect.y;
    for (auto& r : localRows) {
        r.position = curY;
        curY += r.actualHeight;
    }

    // Arrange Children
    for (auto& child : grid->GetChildren()) {
        if (child->GetVisibility() == Visibility::Collapsed) continue;

        int cIdx = (std::min)(child->GetGridColumn(), (int)colCount - 1);
        int rIdx = (std::min)(child->GetGridRow(), (int)rowCount - 1);
        int cSpan = (std::min)(child->GetGridColumnSpan(), (int)(colCount - cIdx));
        int rSpan = (std::min)(child->GetGridRowSpan(), (int)(rowCount - rIdx));

        float cellX = localCols[cIdx].position;
        float cellY = localRows[rIdx].position;
        float cellW = 0.0f;
        for (int i = 0; i < cSpan; ++i) cellW += localCols[cIdx + i].actualWidth;
        float cellH = 0.0f;
        for (int i = 0; i < rSpan; ++i) cellH += localRows[rIdx + i].actualHeight;

        child->Arrange(Rect(cellX, cellY, cellW, cellH));
    }
}

// ----------------------------------------------------------------------
// WrapPanel Layout Implementation
// ----------------------------------------------------------------------
Size LayoutEngine::MeasureWrapPanel(UIElement* panel, Size availableSize) {
    Orientation orientation = panel->GetOrientation();
    if (orientation != Orientation::Vertical) orientation = Orientation::Horizontal;

    float itemWidth = panel->GetItemWidth();
    float itemHeight = panel->GetItemHeight();

    float totalWidth = 0.0f;
    float totalHeight = 0.0f;
    float curLineMain = 0.0f;
    float curLineCross = 0.0f;

    float maxMain = (orientation == Orientation::Horizontal) ? availableSize.width : availableSize.height;

    for (auto& child : panel->GetChildren()) {
        if (child->GetVisibility() == Visibility::Collapsed) continue;

        Size dSize = child->Measure(availableSize);
        float w = (itemWidth > 0) ? itemWidth : dSize.width;
        float h = (itemHeight > 0) ? itemHeight : dSize.height;

        float mainSize = (orientation == Orientation::Horizontal) ? w : h;
        float crossSize = (orientation == Orientation::Horizontal) ? h : w;

        if (curLineMain + mainSize > maxMain && curLineMain > 0) {
            // Wrap to next line
            if (orientation == Orientation::Horizontal) {
                totalWidth = (std::max)(totalWidth, curLineMain);
                totalHeight += curLineCross;
            } else {
                totalHeight = (std::max)(totalHeight, curLineMain);
                totalWidth += curLineCross;
            }
            curLineMain = mainSize;
            curLineCross = crossSize;
        } else {
            curLineMain += mainSize;
            curLineCross = (std::max)(curLineCross, crossSize);
        }
    }

    if (orientation == Orientation::Horizontal) {
        totalWidth = (std::max)(totalWidth, curLineMain);
        totalHeight += curLineCross;
    } else {
        totalHeight = (std::max)(totalHeight, curLineMain);
        totalWidth += curLineCross;
    }

    return Size(totalWidth, totalHeight);
}

void LayoutEngine::ArrangeWrapPanel(UIElement* panel, Rect finalRect) {
    Orientation orientation = panel->GetOrientation();
    if (orientation != Orientation::Vertical) orientation = Orientation::Horizontal;

    float itemWidth = panel->GetItemWidth();
    float itemHeight = panel->GetItemHeight();

    float curMain = (orientation == Orientation::Horizontal) ? finalRect.x : finalRect.y;
    float curCross = (orientation == Orientation::Horizontal) ? finalRect.y : finalRect.x;

    float maxMain = (orientation == Orientation::Horizontal) ? finalRect.width : finalRect.height;
    float lineCrossSize = 0.0f;

    for (auto& child : panel->GetChildren()) {
        if (child->GetVisibility() == Visibility::Collapsed) continue;

        Size dSize = child->GetDesiredSize();
        float w = (itemWidth > 0) ? itemWidth : dSize.width;
        float h = (itemHeight > 0) ? itemHeight : dSize.height;

        float mainSize = (orientation == Orientation::Horizontal) ? w : h;
        float crossSize = (orientation == Orientation::Horizontal) ? h : w;

        float startMain = (orientation == Orientation::Horizontal) ? finalRect.x : finalRect.y;
        if (curMain + mainSize > startMain + maxMain && curMain > startMain) {
            curCross += lineCrossSize;
            curMain = startMain;
            lineCrossSize = crossSize;
        } else {
            lineCrossSize = (std::max)(lineCrossSize, crossSize);
        }

        if (orientation == Orientation::Horizontal) {
            child->Arrange(Rect(curMain, curCross, w, h));
        } else {
            child->Arrange(Rect(curCross, curMain, w, h));
        }

        curMain += mainSize;
    }
}

// ----------------------------------------------------------------------
// DockPanel Layout Implementation
// ----------------------------------------------------------------------
Size LayoutEngine::MeasureDockPanel(UIElement* panel, Size availableSize) {
    float usedWidth = 0.0f;
    float usedHeight = 0.0f;
    float maxChildWidth = 0.0f;
    float maxChildHeight = 0.0f;

    const auto& children = panel->GetChildren();
    bool lastChildFill = panel->GetLastChildFill();

    size_t visibleCount = 0;
    for (auto& c : children) {
        if (c->GetVisibility() != Visibility::Collapsed) visibleCount++;
    }

    size_t count = 0;
    for (auto& child : children) {
        if (child->GetVisibility() == Visibility::Collapsed) continue;
        count++;

        Size remaining(
            (std::max)(0.0f, availableSize.width - usedWidth),
            (std::max)(0.0f, availableSize.height - usedHeight)
        );

        Size dSize = child->Measure(remaining);

        Dock dock = child->GetDock();

        if (count == visibleCount && lastChildFill) {
            maxChildWidth = (std::max)(maxChildWidth, usedWidth + dSize.width);
            maxChildHeight = (std::max)(maxChildHeight, usedHeight + dSize.height);
        } else {
            if (dock == Dock::Left || dock == Dock::Right) {
                usedWidth += dSize.width;
                maxChildHeight = (std::max)(maxChildHeight, usedHeight + dSize.height);
            } else if (dock == Dock::Top || dock == Dock::Bottom) {
                usedHeight += dSize.height;
                maxChildWidth = (std::max)(maxChildWidth, usedWidth + dSize.width);
            }
        }
    }

    return Size((std::max)(usedWidth, maxChildWidth), (std::max)(usedHeight, maxChildHeight));
}

void LayoutEngine::ArrangeDockPanel(UIElement* panel, Rect finalRect) {
    float x = finalRect.x;
    float y = finalRect.y;
    float width = finalRect.width;
    float height = finalRect.height;

    const auto& children = panel->GetChildren();
    bool lastChildFill = panel->GetLastChildFill();

    size_t visibleCount = 0;
    for (auto& c : children) {
        if (c->GetVisibility() != Visibility::Collapsed) visibleCount++;
    }

    size_t count = 0;
    for (auto& child : children) {
        if (child->GetVisibility() == Visibility::Collapsed) continue;
        count++;

        Size dSize = child->GetDesiredSize();

        if (count == visibleCount && lastChildFill) {
            child->Arrange(Rect(x, y, (std::max)(0.0f, width), (std::max)(0.0f, height)));
            break;
        }

        Dock dock = child->GetDock();
        if (dock == Dock::Left) {
            float childW = (std::min)(width, dSize.width);
            child->Arrange(Rect(x, y, childW, height));
            x += childW;
            width -= childW;
        } else if (dock == Dock::Right) {
            float childW = (std::min)(width, dSize.width);
            child->Arrange(Rect(x + width - childW, y, childW, height));
            width -= childW;
        } else if (dock == Dock::Top) {
            float childH = (std::min)(height, dSize.height);
            child->Arrange(Rect(x, y, width, childH));
            y += childH;
            height -= childH;
        } else if (dock == Dock::Bottom) {
            float childH = (std::min)(height, dSize.height);
            child->Arrange(Rect(x, y + height - childH, width, childH));
            height -= childH;
        }
    }
}

// ----------------------------------------------------------------------
// UniformGrid Layout Implementation
// ----------------------------------------------------------------------
Size LayoutEngine::MeasureUniformGrid(UIElement* panel, Size availableSize) {
    const auto& children = panel->GetChildren();
    int visibleCount = 0;
    for (auto& c : children) {
        if (c->GetVisibility() != Visibility::Collapsed) visibleCount++;
    }
    if (visibleCount == 0) return Size(0, 0);

    int rows = panel->GetRows();
    int cols = panel->GetColumns();

    if (rows <= 0 && cols <= 0) {
        rows = static_cast<int>(std::ceil(std::sqrt(visibleCount)));
        cols = static_cast<int>(std::ceil((double)visibleCount / rows));
    } else if (rows <= 0) {
        rows = static_cast<int>(std::ceil((double)visibleCount / cols));
    } else if (cols <= 0) {
        cols = static_cast<int>(std::ceil((double)visibleCount / rows));
    }

    Size childAvail(availableSize.width / cols, availableSize.height / rows);
    float maxW = 0.0f, maxH = 0.0f;

    for (auto& child : children) {
        if (child->GetVisibility() == Visibility::Collapsed) continue;

        Size dSize = child->Measure(childAvail);
        maxW = (std::max)(maxW, dSize.width);
        maxH = (std::max)(maxH, dSize.height);
    }

    return Size(maxW * cols, maxH * rows);
}

void LayoutEngine::ArrangeUniformGrid(UIElement* panel, Rect finalRect) {
    const auto& children = panel->GetChildren();
    int visibleCount = 0;
    for (auto& c : children) {
        if (c->GetVisibility() != Visibility::Collapsed) visibleCount++;
    }
    if (visibleCount == 0) return;

    int rows = panel->GetRows();
    int cols = panel->GetColumns();

    if (rows <= 0 && cols <= 0) {
        rows = static_cast<int>(std::ceil(std::sqrt(visibleCount)));
        cols = static_cast<int>(std::ceil((double)visibleCount / rows));
    } else if (rows <= 0) {
        rows = static_cast<int>(std::ceil((double)visibleCount / cols));
    } else if (cols <= 0) {
        cols = static_cast<int>(std::ceil((double)visibleCount / rows));
    }

    float cellW = finalRect.width / cols;
    float cellH = finalRect.height / rows;

    int index = 0;
    for (auto& child : children) {
        if (child->GetVisibility() == Visibility::Collapsed) continue;

        int r = index / cols;
        int c = index % cols;

        float x = finalRect.x + c * cellW;
        float y = finalRect.y + r * cellH;

        child->Arrange(Rect(x, y, cellW, cellH));
        index++;
    }
}

} // namespace CUI
