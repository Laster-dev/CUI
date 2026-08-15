#pragma once
#include "Control.h"
#include "Button.h"
#include "ScrollbarAutoHide.h"
#include "../text/markdown/MarkdownAst.h"
#include "../text/markdown/MarkdownLayout.h"
#include <memory>
#include <string>
#include <vector>

namespace CUI {

class MarkdownView : public Control {
public:
    MarkdownView();
    explicit MarkdownView(const std::string& markdown);
    virtual ~MarkdownView() = default;

    virtual const char* GetClassName() const override { return "MarkdownView"; }
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    virtual HCURSOR GetCursor() const override;
    bool AcceptsTabFocus() const override { return true; }

    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual bool ShouldClipToBounds() const override { return true; }
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual void OnMouseDblClick(Point pt) override;
    virtual void OnMouseWheel(float delta) override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual void OnThemeChanged() override;
    void SetProperty(PropertyId id, const Value& val) override;

    /**
     * @brief Markdown 视图原始文档字符串内容属性代理。
     */
    struct MarkdownViewMarkdownProperty {
        MarkdownView* owner;
        MarkdownViewMarkdownProperty& operator=(const std::string& md) { owner->SetMarkdown(md); return *this; }
        operator const std::string&() const { return owner->GetMarkdown(); }
        const std::string& Get() const { return owner->GetMarkdown(); }
    } Markdown{this};

    void SetMarkdown(const std::string& markdown);
    const std::string& GetMarkdown() const { return UIElement::GetText(); }

    void SetShowCodeLineNumbers(bool show);
    bool GetShowCodeLineNumbers() const { return m_showLineNumbers; }

    std::string GetSelectedText() const;
    void SelectAll();
    bool CopySelection();
    bool HasSelection() const { return m_selA != m_selB; }

    bool ScrollToHeading(const std::string& heading);

    Event<MarkdownView*, const std::string&>& OnLinkClicked() { return m_onLinkClicked; }

private:
    void EnsureLayout(GraphicsContext& ctx);
    void ClampScroll();
    void SyncCopyButtons();
    void LayoutCopyButtons();
    void CopyCodeBlock(size_t index);
    Point ToDoc(Point pt) const;
    int HitChar(Point docPt) const;
    const MdRun* HitRun(Point docPt) const;
    void SetSelection(int a, int b);
    Rect ScrollbarTrack() const;
    Rect ScrollbarThumb() const;
    void OpenLink(const std::string& href);

    std::vector<MdBlock> m_blocks;
    MdLayoutResult m_layout;
    bool m_layoutDirty = true;
    float m_layoutWidth = -1.0f;
    bool m_showLineNumbers = true;

    float m_scrollY = 0.0f;
    float m_targetScrollY = 0.0f;
    float m_maxScrollY = 0.0f;
    AnimatedScalar m_scrollYAnim{ 0.0f };
    bool m_draggingScrollbar = false;
    float m_dragStartY = 0.0f;
    float m_dragStartScroll = 0.0f;
    ScrollbarAutoHide m_scrollbarAutoHide;

    int m_selA = 0;
    int m_selB = 0;
    bool m_selecting = false;
    std::string m_hoverHref;
    Event<MarkdownView*, const std::string&> m_onLinkClicked;

    std::vector<std::shared_ptr<Button>> m_copyBtns;
    std::vector<std::string> m_copyTexts;
};

} // namespace CUI
