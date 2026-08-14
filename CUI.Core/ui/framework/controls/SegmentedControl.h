#pragma once
#include "Control.h"
#include "../animation/AnimationSystem.h"
#include <string>
#include <vector>

namespace CUI {

/**
 * @brief 分段选择控件（SegmentedControl）。
 * 一组横向排列的互斥单选卡片组（类似 iOS 的 Segmented Control）。
 * 选中某一项时，背景滑块（Pill）会平滑地滑动到对应的卡片上。
 */
class SegmentedControl : public Control {
public:
    SegmentedControl();
    virtual ~SegmentedControl() = default;

    virtual const char* GetClassName() const override { return "SegmentedControl"; } // 获取类名
    virtual Value GetProperty(PropertyId id) const override; // 反射获取属性值
    virtual bool HasProperty(PropertyId id) const override; // 检查是否存在对应属性
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; } // 获取悬浮交互鼠标样式

    virtual Size Measure(Size availableSize) override; // 计算各分段项目文本在单行横排下的累加总尺寸大小
    virtual void Arrange(Rect finalRect) override; // 排列排列分段栏几何位置
    virtual void OnRender(GraphicsContext& ctx) override; // 绘制圆角胶囊底槽、平滑位移的选中高亮滑块、以及分段标签文字
    virtual void OnMouseDown(Point pt) override; // 鼠标按下，高亮滑过指示索引项
    virtual void OnMouseUp(Point pt) override; // 鼠标松开，提交选中该分段卡片并派发事件
    virtual void OnMouseMove(Point pt) override; // 鼠标移动，更新 Hover 高亮卡片项
    virtual void OnMouseLeave() override; // 鼠标离去重置 Hover 状态
    virtual bool OnKeyDown(int vkCode) override; // 响应键盘左右方向键切换选中项
    virtual bool AcceptsTabFocus() const override { return true; } // 支持键盘 Tab 导航聚焦
    virtual bool OnAnimationTick() override; // 驱动高亮背景滑块 Pill 在选项间的位移和宽度形变动画
    virtual bool HasSelfAnimation() const override; // 检查背景滑块是否仍在滑动过程中

    void SetProperty(PropertyId id, const Value& val) override; // 反射设定属性值

    void AddItem(const std::string& item); // 插入一个新的文本分段卡片项
    void ClearItems(); // 清空所有的分段项目
    void SetItems(const std::string& itemsCsv); // 通过逗号分隔符 (CSV) 批量快速导入并更新分段项目
    const std::vector<std::string>& GetItems() const { return m_items; } // 读取分段选项队列

    PropertyRef<int, PropertyId::SelectedIndex> SelectedIndex; // 被选中分段卡片的索引双向绑定属性代理

    int GetSelectedIndex() const { return m_selectedIndex; } // 获取当前选中项的行号索引
    void SetSelectedIndex(int index); // 设置当前选中项的行号索引并引发滑动

    std::string GetSelectedItem() const; // 读取当前选中的分段项目文本

    Event<SegmentedControl*, int, const std::string&>& OnSelectionChanged() { return m_onSelectionChangedEvent; } // 分段更改时的事件发布中心

private:
    Rect SegmentRect(int index) const; // 计算并返回第 index 个分段选项在控件局部坐标系下的包络大小
    int HitTestIndex(Point pt) const; // 碰撞测试定位坐标落入第几个分段选项上
    void SyncPill(bool snap); // 强制刷新并同步高亮 Pill 滑块的目标平移位置（参数为是否直接瞬移）
    float MeasureContentWidth() const; // 计算单行文字卡片全部并排的理想测量总宽

    std::vector<std::string> m_items;                                   // 分段项目选项队列
    int m_selectedIndex = -1;                                           // 被选中分段项的行号
    int m_hoverIndex = -1;                                              // 鼠标指针正 Hover 的分段项行号
    int m_pressedIndex = -1;                                            // 鼠标左键按下捕获的分段项行号
    AnimatedScalar m_pillX{ 0.0f };                                     // 背景选中卡片滑块的当前 X 轴绝对坐标过渡动画
    AnimatedScalar m_pillW{ 0.0f };                                     // 背景选中卡片滑块的当前渲染宽度过渡动画
    Event<SegmentedControl*, int, const std::string&> m_onSelectionChangedEvent; // 分段选中更改事件对象
};

} // namespace CUI
