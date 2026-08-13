#pragma once
#include "Control.h"

namespace CUI {

class HyperlinkButton : public Control {
public:
    HyperlinkButton();
    explicit HyperlinkButton(const std::string& text, const std::string& uri = "");
    virtual ~HyperlinkButton() = default;

    virtual const char* GetClassName() const override { return "HyperlinkButton"; }
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual bool AcceptsTabFocus() const override { return true; }

    const std::string& GetNavigateUri() const { return m_navigateUri; }
    void SetNavigateUri(const std::string& uri) {
        m_navigateUri = uri;
        MarkRenderContentDirty();
    }

private:
    std::string m_navigateUri;
};

} // namespace CUI
