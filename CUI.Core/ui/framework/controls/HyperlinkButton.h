#pragma once
#include "Control.h"

namespace CUI {

/**
 * @brief 超链接按钮。
 * 用于承载网页链接跳转或触发应用内页面导航，视觉上呈现为带有下划线和特定悬停色的文本样式按钮。
 */
class HyperlinkButton : public Control {
public:
    HyperlinkButton();
    explicit HyperlinkButton(const std::string& text, const std::string& uri = "");
    virtual ~HyperlinkButton() = default;

    virtual const char* GetClassName() const override { return "HyperlinkButton"; } // 获取类名
    virtual Value GetProperty(PropertyId id) const override; // 反射获取属性
    virtual bool HasProperty(PropertyId id) const override; // 检查是否存在对应属性
    void SetProperty(PropertyId id, const Value& val) override; // 反射设定属性
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; } // 获取交互鼠标样式

    virtual Size Measure(Size availableSize) override; // 测量文字所占物理大小尺寸
    virtual void OnRender(GraphicsContext& ctx) override; // 绘制超链接下划线以及前景字元
    virtual bool OnKeyDown(int vkCode) override; // 响应键盘空格与回车点击行为
    virtual bool AcceptsTabFocus() const override { return true; } // 支持键盘 Tab 导航获焦

    const std::string& GetNavigateUri() const { return m_navigateUri; } // 获取超链接的目标网址或内部导航标签
    void SetNavigateUri(const std::string& uri) { // 设置要跳转的目标网址
        m_navigateUri = uri;
        MarkRenderContentDirty();
    }

private:
    std::string m_navigateUri; // 超链接目标跳转 URL 字符串
};

} // namespace CUI
