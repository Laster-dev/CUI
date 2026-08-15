#pragma once
#include "UIElement.h"
#include "../layout/Layout.h"

namespace CUI {

/**
 * @brief 所有容器布局面板的基类。
 * Panel 类继承自 UIElement，并在基类布局双管道中提供 MeasureOverride 和 ArrangeOverride 覆盖点。
 */
class Panel : public UIElement {
public:
    Panel();
    virtual ~Panel() = default;

    virtual const char* GetClassName() const override { return "Panel"; } // 获取类名

    virtual Size MeasureOverride(Size availableSize); // 由具体派生面板重写的子控件尺寸测算实现
    virtual void ArrangeOverride(Rect finalRect); // 由具体派生面板重写的子控件对齐与排列位置实现
};

/**
 * @brief 线性堆叠布局容器（StackPanel）。
 * 根据 Orientation (Horizontal / Vertical) 排列子元素，提供流畅的线形自适应大小堆叠。
 */
class StackPanel : public Panel {
public:
    StackPanel();
    explicit StackPanel(Orientation orientation);
    virtual ~StackPanel() = default;

    virtual const char* GetClassName() const override { return "StackPanel"; } // 获取类名

    Size MeasureOverride(Size availableSize) override; // 沿特定朝向线性累加测量所有子控件大小
    void ArrangeOverride(Rect finalRect) override; // 沿排布方向将各子控件按顺序紧凑排布
};

/**
 * @brief 自由画布绝对坐标定位容器（Canvas）。
 * 利用附加属性 CanvasLeft, CanvasTop 等绝对坐标放置子控件。
 */
class Canvas : public Panel {
public:
    Canvas();
    virtual ~Canvas() = default;

    virtual const char* GetClassName() const override { return "Canvas"; } // 获取类名
    bool UsesZIndexOrdering() const override { return true; }

    Size MeasureOverride(Size availableSize) override; // 不加限制地让子控件自行测量大小
    void ArrangeOverride(Rect finalRect) override; // 根据子控件的 CanvasLeft 等附加坐标属性进行物理定位放置
};

/**
 * @brief 网格行列划分容器（Grid）。
 * 类似于 XAML Grid。支持像素大小、*比例（Star）、自适应（Auto）三种网格规则。
 */
class Grid : public Panel {
public:
    Grid();
    virtual ~Grid() = default;

    virtual const char* GetClassName() const override { return "Grid"; } // 获取类名

    Size MeasureOverride(Size availableSize) override; // 分阶段分析行列限制并测算各个格子内子控件的最适期望大小
    void ArrangeOverride(Rect finalRect) override; // 将各子控件按其 GridRow, GridColumn 属性对齐塞入计算出的格子矩形

    void AddColumnDefinition(const ColumnDefinition& col) { m_columns.push_back(col); } // 增加一列网格定义
    void AddRowDefinition(const RowDefinition& row) { m_rows.push_back(row); } // 增加一行网格定义

    const std::vector<ColumnDefinition>& GetColumnDefinitions() const { return m_columns; } // 获取网格列定义明细集合
    std::vector<ColumnDefinition>& GetColumnDefinitions() { return m_columns; }

    const std::vector<RowDefinition>& GetRowDefinitions() const { return m_rows; } // 获取网格行定义明细集合
    std::vector<RowDefinition>& GetRowDefinitions() { return m_rows; }

    void SetColumnDefinitions(const std::string& colDefsStr); // 传入 CSV/比例字符串快速初始化并设定网格列规则
    void SetRowDefinitions(const std::string& rowDefsStr); // 传入 CSV/比例字符串快速初始化并设定网格行规则

private:
    std::vector<ColumnDefinition> m_columns; // 注册的所有网格列限制定义
    std::vector<RowDefinition> m_rows;       // 注册的所有网格行限制定义
};

/**
 * @brief 自动换行/换列流式排列面板（WrapPanel）。
 * 当子控件在单行或单列排列不下时，会自动移入下一行或下一列。
 */
class WrapPanel : public Panel {
public:
    WrapPanel();
    explicit WrapPanel(Orientation orientation);
    virtual ~WrapPanel() = default;

    virtual const char* GetClassName() const override { return "WrapPanel"; } // 获取类名

    Size MeasureOverride(Size availableSize) override; // 流式累加并计算溢出换行所需的总宽高尺寸
    void ArrangeOverride(Rect finalRect) override; // 将所有子项流式排列，并在空间满时进行换行/换列重新排列
};

/**
 * @brief 边缘停靠布局容器（DockPanel）。
 * 子元素通过 Dock 附加属性（Left, Top, Right, Bottom）紧贴父容器四边放置，最后一个填充剩余区。
 */
class DockPanel : public Panel {
public:
    DockPanel();
    virtual ~DockPanel() = default;

    virtual const char* GetClassName() const override { return "DockPanel"; } // 获取类名

    Size MeasureOverride(Size availableSize) override; // 依据子控件的 Dock 方位依次向内部缩进测算剩余空间大小
    void ArrangeOverride(Rect finalRect) override; // 将各子控件贴靠分配的边沿放置，并将最后一项拉伸塞满死角
};

/**
 * @brief 均等网格分块容器（UniformGrid）。
 * 所有格子均摊可用宽高并保持绝对均等。
 */
class UniformGrid : public Panel {
public:
    UniformGrid();
    UniformGrid(int rows, int cols);
    virtual ~UniformGrid() = default;

    virtual const char* GetClassName() const override { return "UniformGrid"; } // 获取类名

    Size MeasureOverride(Size availableSize) override; // 根据行列格数等分测算每个子控件分得的最佳期望尺寸
    void ArrangeOverride(Rect finalRect) override; // 将所有格子等分，并将子项整齐码入各个行列单元格中
};

} // namespace CUI
