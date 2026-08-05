#pragma once
#include "UIElement.h"
#include "../layout/Layout.h"

namespace CUI {

class Panel : public UIElement {
public:
    Panel();
    virtual ~Panel() = default;

    virtual const char* GetClassName() const override { return "Panel"; }

    // Content measure/arrange (margin/padding/explicit size handled by LayoutEngine).
    virtual Size MeasureOverride(Size availableSize);
    virtual void ArrangeOverride(Rect finalRect);
};

class StackPanel : public Panel {
public:
    StackPanel();
    explicit StackPanel(Orientation orientation);
    virtual ~StackPanel() = default;

    virtual const char* GetClassName() const override { return "StackPanel"; }

    Size MeasureOverride(Size availableSize) override;
    void ArrangeOverride(Rect finalRect) override;
};

class Canvas : public Panel {
public:
    Canvas();
    virtual ~Canvas() = default;

    virtual const char* GetClassName() const override { return "Canvas"; }

    Size MeasureOverride(Size availableSize) override;
    void ArrangeOverride(Rect finalRect) override;
};

class Grid : public Panel {
public:
    Grid();
    virtual ~Grid() = default;

    virtual const char* GetClassName() const override { return "Grid"; }

    Size MeasureOverride(Size availableSize) override;
    void ArrangeOverride(Rect finalRect) override;

    void AddColumnDefinition(const ColumnDefinition& col) { m_columns.push_back(col); }
    void AddRowDefinition(const RowDefinition& row) { m_rows.push_back(row); }

    const std::vector<ColumnDefinition>& GetColumnDefinitions() const { return m_columns; }
    std::vector<ColumnDefinition>& GetColumnDefinitions() { return m_columns; }

    const std::vector<RowDefinition>& GetRowDefinitions() const { return m_rows; }
    std::vector<RowDefinition>& GetRowDefinitions() { return m_rows; }

    void SetColumnDefinitions(const std::string& colDefsStr);
    void SetRowDefinitions(const std::string& rowDefsStr);

private:
    std::vector<ColumnDefinition> m_columns;
    std::vector<RowDefinition> m_rows;
};

class WrapPanel : public Panel {
public:
    WrapPanel();
    explicit WrapPanel(Orientation orientation);
    virtual ~WrapPanel() = default;

    virtual const char* GetClassName() const override { return "WrapPanel"; }

    Size MeasureOverride(Size availableSize) override;
    void ArrangeOverride(Rect finalRect) override;
};

class DockPanel : public Panel {
public:
    DockPanel();
    virtual ~DockPanel() = default;

    virtual const char* GetClassName() const override { return "DockPanel"; }

    Size MeasureOverride(Size availableSize) override;
    void ArrangeOverride(Rect finalRect) override;
};

class UniformGrid : public Panel {
public:
    UniformGrid();
    UniformGrid(int rows, int cols);
    virtual ~UniformGrid() = default;

    virtual const char* GetClassName() const override { return "UniformGrid"; }

    Size MeasureOverride(Size availableSize) override;
    void ArrangeOverride(Rect finalRect) override;
};

} // namespace CUI
