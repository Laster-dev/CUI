#pragma once
#include "Control.h"
#include "Button.h"
#include "../animation/AnimationSystem.h"
#include "../input/Command.h"
#include <memory>
#include <string>

namespace CUI {

enum class InfoBarSeverity {
    Informational,
    Success,
    Warning,
    Error
};

class InfoBar : public Control {
public:
    InfoBar();
    virtual ~InfoBar() = default;

    virtual const char* GetClassName() const override { return "InfoBar"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void SetProperty(PropertyId id, const Value& val) override;

    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual void OnThemeChanged() override;

    void SetTitle(const std::string& title);
    const std::string& GetTitle() const { return m_title; }
    void SetMessage(const std::string& message);
    const std::string& GetMessage() const { return m_message; }

    void SetSeverity(InfoBarSeverity severity);
    InfoBarSeverity GetSeverity() const { return m_severity; }

    void SetIsOpen(bool open);
    bool GetIsOpen() const { return m_isOpen; }

    void SetIsClosable(bool closable);
    bool GetIsClosable() const { return m_isClosable; }

    void SetActionText(const std::string& text);
    const std::string& GetActionText() const { return m_actionText; }
    void SetActionCommand(std::shared_ptr<Command> command);
    std::shared_ptr<Command> GetActionCommand() const { return m_actionCommand; }

    Event<>& OnAction() { return m_onAction; }
    Event<>& OnClosed() { return m_onClosed; }

private:
    struct Palette {
        D2D1_COLOR_F accent;
        D2D1_COLOR_F fill;
        D2D1_COLOR_F border;
    };

    void EnsureChrome();
    void SyncChrome();
    void LayoutChrome();
    void ApplyOpenState(bool open, bool animate);
    void CloseFromUser();
    void InvokeAction();
    Palette Colors() const;
    void DrawSeverityIcon(GraphicsContext& ctx, const Rect& slot, D2D1_COLOR_F color) const;
    Size MeasureWrapped(const std::string& text, float fontSize, float maxWidth,
                        DWRITE_FONT_WEIGHT weight) const;
    void DrawWrapped(GraphicsContext& ctx, const std::string& text, const Rect& rect,
                     float fontSize, D2D1_COLOR_F color, DWRITE_FONT_WEIGHT weight) const;
    float ContentHeight(float width) const;

    std::string m_title;
    std::string m_message;
    std::string m_actionText;
    InfoBarSeverity m_severity = InfoBarSeverity::Informational;
    bool m_isOpen = true;
    bool m_isClosable = true;
    float m_contentHeight = 52.0f;
    float m_lastMeasureWidth = 0.0f;

    std::shared_ptr<Button> m_actionBtn;
    std::shared_ptr<Button> m_closeBtn;
    std::shared_ptr<Command> m_actionCommand;
    Event<> m_onAction;
    Event<> m_onClosed;
    AnimatedScalar m_openAnim{ 1.0f };
};

} // namespace CUI
