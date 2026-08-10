#include "Panel.h"
#include <sstream>

namespace CUI {

Panel::Panel() {
    SetBackground(D2D1::ColorF(0, 0, 0, 0));
}

Size Panel::MeasureOverride(Size availableSize) {
    return LayoutEngine::MeasureFlexPanel(this, availableSize, GetOrientation(), Alignment::Start, GetGap());
}

void Panel::ArrangeOverride(Rect finalRect) {
    LayoutEngine::ArrangeFlexPanel(this, finalRect, GetOrientation(), Alignment::Start, GetGap());
}

StackPanel::StackPanel() {
    SetOrientation(Orientation::Vertical);
    SetGap(0.0f);
}

StackPanel::StackPanel(Orientation orientation) : StackPanel() {
    SetOrientation(orientation);
}

Size StackPanel::MeasureOverride(Size availableSize) {
    return LayoutEngine::MeasureFlexPanel(this, availableSize, GetOrientation(), Alignment::Start, GetGap());
}

void StackPanel::ArrangeOverride(Rect finalRect) {
    LayoutEngine::ArrangeFlexPanel(this, finalRect, GetOrientation(), Alignment::Start, GetGap());
}

Canvas::Canvas() {
}

Size Canvas::MeasureOverride(Size availableSize) {
    return LayoutEngine::MeasureCanvas(this, availableSize);
}

void Canvas::ArrangeOverride(Rect finalRect) {
    LayoutEngine::ArrangeCanvas(this, finalRect);
}

Grid::Grid() {
}

void Grid::SetColumnDefinitions(const std::string& colDefsStr) {
    m_columns.clear();
    std::stringstream ss(colDefsStr);
    std::string token;
    while (std::getline(ss, token, ',')) {
        if (token.empty()) continue;
        ColumnDefinition col;
        col.width = GridLength::Parse(token);
        m_columns.push_back(col);
    }
}

void Grid::SetRowDefinitions(const std::string& rowDefsStr) {
    m_rows.clear();
    std::stringstream ss(rowDefsStr);
    std::string token;
    while (std::getline(ss, token, ',')) {
        if (token.empty()) continue;
        RowDefinition row;
        row.height = GridLength::Parse(token);
        m_rows.push_back(row);
    }
}

Size Grid::MeasureOverride(Size availableSize) {
    return LayoutEngine::MeasureGrid(this, availableSize);
}

void Grid::ArrangeOverride(Rect finalRect) {
    LayoutEngine::ArrangeGrid(this, finalRect);
}

WrapPanel::WrapPanel() {
    SetOrientation(Orientation::Horizontal);
    SetItemWidth(-1.0f);
    SetItemHeight(-1.0f);
}

WrapPanel::WrapPanel(Orientation orientation) : WrapPanel() {
    SetOrientation(orientation);
}

Size WrapPanel::MeasureOverride(Size availableSize) {
    return LayoutEngine::MeasureWrapPanel(this, availableSize);
}

void WrapPanel::ArrangeOverride(Rect finalRect) {
    LayoutEngine::ArrangeWrapPanel(this, finalRect);
}

DockPanel::DockPanel() {
    SetLastChildFill(true);
}

Size DockPanel::MeasureOverride(Size availableSize) {
    return LayoutEngine::MeasureDockPanel(this, availableSize);
}

void DockPanel::ArrangeOverride(Rect finalRect) {
    LayoutEngine::ArrangeDockPanel(this, finalRect);
}

UniformGrid::UniformGrid() {
    SetRows(0);
    SetColumns(0);
}

UniformGrid::UniformGrid(int rows, int cols) : UniformGrid() {
    SetRows(rows);
    SetColumns(cols);
}

Size UniformGrid::MeasureOverride(Size availableSize) {
    return LayoutEngine::MeasureUniformGrid(this, availableSize);
}

void UniformGrid::ArrangeOverride(Rect finalRect) {
    LayoutEngine::ArrangeUniformGrid(this, finalRect);
}

} // namespace CUI
