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

    virtual const char* GetClassName() const override { return "HyperlinkButton"; }
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void SetProperty(PropertyId id, const Value& val) override;
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual bool AcceptsTabFocus() const override { return true; }

    /**
     * @brief 获取超链接的目标网址或内部导航标签。
     */
    const std::string& GetNavigateUri() const { return m_navigateUri; }
    
    /**
     * @brief 设置要跳转的目标网址。
     */
    void SetNavigateUri(const std::string& uri) {
        m_navigateUri = uri;
        MarkRenderContentDirty();
    }

private:
    std::string m_navigateUri;
};

} // namespace CUI
