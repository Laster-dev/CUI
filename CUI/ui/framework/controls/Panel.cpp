#include "Panel.h"
#include <sstream>

namespace CUI {

Panel::Panel() {
    SetProperty("background", Value(D2D1::ColorF(0, 0, 0, 0)));
}

StackPanel::StackPanel() {
    SetProperty("orientation", Value("Vertical"));
    SetProperty("gap", Value(0.0f));
}

StackPanel::StackPanel(Orientation orientation) : StackPanel() {
    SetProperty("orientation", Value(orientation == Orientation::Horizontal ? "Horizontal" : "Vertical"));
}

Canvas::Canvas() {
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

WrapPanel::WrapPanel() {
    SetProperty("orientation", Value("Horizontal"));
    SetProperty("itemWidth", Value(-1.0f));
    SetProperty("itemHeight", Value(-1.0f));
}

WrapPanel::WrapPanel(Orientation orientation) : WrapPanel() {
    SetProperty("orientation", Value(orientation == Orientation::Horizontal ? "Horizontal" : "Vertical"));
}

DockPanel::DockPanel() {
    SetProperty("lastChildFill", Value(true));
}

UniformGrid::UniformGrid() {
    SetProperty("rows", Value(0));
    SetProperty("columns", Value(0));
}

UniformGrid::UniformGrid(int rows, int cols) : UniformGrid() {
    SetProperty("rows", Value(rows));
    SetProperty("columns", Value(cols));
}

} // namespace CUI
