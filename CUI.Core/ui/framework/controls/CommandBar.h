#pragma once
#include "Control.h"
#include "Button.h"
#include "ToggleButton.h"
#include "ContextMenu.h"
#include <memory>
#include <string>
#include <vector>

namespace CUI {

enum class CommandBarLabelPosition {
    Collapsed,
    Right
};

class CommandBar : public Control {
public:
    CommandBar();
    virtual ~CommandBar();

    virtual const char* GetClassName() const override { return "CommandBar"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;

    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual void OnNavigatedFrom() override;

    std::shared_ptr<Button> AddButton(
        const std::string& label,
        const std::string& icon = {},
        std::shared_ptr<Command> command = nullptr);
    std::shared_ptr<ToggleButton> AddToggle(
        const std::string& label,
        const std::string& icon = {},
        std::shared_ptr<Command> command = nullptr);
    void AddSeparator();

    std::shared_ptr<Button> AddSecondary(
        const std::string& label,
        const std::string& icon = {},
        std::shared_ptr<Command> command = nullptr);
    void AddSecondarySeparator();

    void Clear();

    void SetLabelPosition(CommandBarLabelPosition position);
    CommandBarLabelPosition GetLabelPosition() const { return m_labelPosition; }

    int GetPrimaryCount() const;
    int GetOverflowCount() const { return m_overflowCount; }
    bool IsOverflowOpen() const;

    Event<>& OnOverflowOpened() { return m_onOverflowOpened; }

private:
    enum class ItemKind : uint8_t { Button, Toggle, Separator };

    struct Item {
        ItemKind kind = ItemKind::Button;
        bool secondary = false;
        std::string label;
        std::string icon;
        std::shared_ptr<Button> button;
        Rect slot;
        bool overflowed = false;
    };

    void EnsureOverflowChrome();
    void StyleItemButton(Button& btn, const Item& item, bool checked) const;
    void ApplyLabelChrome(Item& item);
    void LayoutChrome();
    void RebuildOverflowMenu();
    void SetOverflowOpen(bool open);
    void DrawSeparator(GraphicsContext& ctx, const Rect& slot) const;
    float MeasureItemWidth(Item& item) const;
    static constexpr float kBarH = 40.0f;
    static constexpr float kBtnH = 32.0f;
    static constexpr float kIconBtn = 32.0f;
    static constexpr float kPad = 4.0f;
    static constexpr float kGap = 2.0f;
    static constexpr float kSepW = 9.0f;

    std::vector<Item> m_items;
    CommandBarLabelPosition m_labelPosition = CommandBarLabelPosition::Right;
    std::shared_ptr<Button> m_overflowBtn;
    std::shared_ptr<ContextMenu> m_overflowMenu;
    int m_overflowCount = 0;
    std::vector<int> m_overflowKeys;
    Event<> m_onOverflowOpened;
};

} // namespace CUI
