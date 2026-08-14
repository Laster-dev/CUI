#pragma once
#include "CheckBox.h"

namespace CUI {

/**
 * @brief 单选按钮（RadioButton）。
 * 继承自 CheckBox。通过组名（GroupName）实现同组内按钮的互斥选中交互。
 */
class RadioButton : public CheckBox {
public:
    RadioButton();
    explicit RadioButton(const std::string& text);
    virtual ~RadioButton() = default;

    virtual const char* GetClassName() const override { return "RadioButton"; } // 获取类名
    virtual Value GetProperty(PropertyId id) const override; // 反射获取属性值
    virtual bool HasProperty(PropertyId id) const override; // 检查是否存在对应属性
    void SetProperty(PropertyId id, const Value& val) override; // 反射设定属性值

    virtual void OnRender(GraphicsContext& ctx) override; // 绘制圆形单选框边框、背景及选中核心圆点
    virtual void OnMouseDown(Point pt) override; // 鼠标按下，执行点击动作并触发同组互斥
    virtual void OnMouseUp(Point pt) override; // 鼠标抬起释放交互
    virtual bool OnKeyDown(int vkCode) override; // 响应键盘空格与回车选中
    virtual bool OnAnimationTick() override; // 驱动选中核心圆点缩放淡入动画
    virtual bool HasSelfAnimation() const override; // 检查是否依然处于缩放动画中

    const std::string& GetGroupName() const { return m_groupName; } // 获取同组互斥组名
    void SetGroupName(const std::string& group) { // 设置互斥组名
        if (m_groupName == group) {
            return;
        }
        m_groupName = group;
        NotifyFieldChanged(PropertyId::GroupName, Value(group));
        MarkRenderContentDirty();
    }

private:
    void SetChecked(bool checked); // 强制修改选中状态
    void UncheckSiblingsInGroup(); // 将同一父级树下、相同组名的其它单选钮强制设为未选中

    std::string m_groupName{ "DefaultGroup" }; // 单选钮所属的互斥分组标识名
    AnimatedScalar m_selectionAnim{};         // 选中状态缩放渐变过渡动画
};

} // namespace CUI
