#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "CommandBar.h"
#include "../style/ThemeManager.h"
#include "../window/Window.h"
#include <algorithm>
#include <windows.h>

#ifndef VK_ESCAPE
#define VK_ESCAPE 0x1B
#endif
#ifndef VK_DOWN
#define VK_DOWN 0x28
#endif

namespace CUI {
namespace {

constexpr const char* kSvgMore =
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M66.488889 211.781818h891.022222c28.198788 0 50.980202-22.238384 "
    "50.980202-49.648485 0-27.397172-22.768485-49.648485-50.980202-49.648485H66.488889C38.341818 "
    "112.484848 15.508687 134.723232 15.508687 162.133333s22.833131 49.648485 50.980202 49.648485z "
    "m891.009293 248.242424H66.488889C38.277172 460.024242 15.508687 482.262626 15.508687 "
    "509.672727s22.768485 49.648485 50.980202 49.648485h891.022222c28.198788 0 50.980202-22.238384 "
    "50.980202-49.648485-0.012929-27.410101-22.923636-49.648485-50.993131-49.648485z m0 351.63798H66.488889"
    "c-28.134141 0-50.980202 22.238384-50.980202 49.648485s22.833131 49.648485 50.980202 49.648485h891.022222"
    "c28.198788 0 50.980202-22.238384 50.980202-49.648485-0.012929-27.397172-22.781414-49.648485-50.993131-49.648485z\"/>"
    "</svg>";

D2D1_COLOR_F Mix(D2D1_COLOR_F a, D2D1_COLOR_F b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return D2D1::ColorF(
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t);
}

} // namespace

CommandBar::CommandBar() {
    SetWidth(-1.0f);
    SetHeight(kBarH);
    SetAlign(Alignment::Stretch);
    SetClipToBounds(true);
    SetCornerRadius(6.0f);
    SetBorderThickness(1.0f);
    SetBackgroundToken(ThemeTokenId::CardBackground);
    SetBorderToken(ThemeTokenId::CardBorder);
    SetColorToken(ThemeTokenId::TextPrimary);
    SetKeyboardNavigationMode(KeyboardNavigationMode::Cycle);
    EnsureOverflowChrome();
}

CommandBar::~CommandBar() {
    if (m_overflowMenu) {
        m_overflowMenu->Hide();
    }
}

Value CommandBar::GetProperty(PropertyId id) const {
    if (id == PropertyId::LabelPosition) {
        return Value(m_labelPosition == CommandBarLabelPosition::Collapsed ? "Collapsed" : "Right");
    }
    return UIElement::GetProperty(id);
}

bool CommandBar::HasProperty(PropertyId id) const {
    return id == PropertyId::LabelPosition || UIElement::HasProperty(id);
}

void CommandBar::SetProperty(PropertyId id, const Value& val) {
    if (id == PropertyId::LabelPosition) {
        SetLabelPosition(val.AsString() == "Collapsed"
            ? CommandBarLabelPosition::Collapsed
            : CommandBarLabelPosition::Right);
        return;
    }
    UIElement::SetProperty(id, val);
}

void CommandBar::EnsureOverflowChrome() {
    if (!m_overflowBtn) {
        m_overflowBtn = std::make_shared<Button>();
        m_overflowBtn->SetText("");
        m_overflowBtn->SetIcon(kSvgMore);
        m_overflowBtn->SetToolTip("更多");
        m_overflowBtn->SetWidth(kIconBtn);
        m_overflowBtn->SetHeight(kBtnH);
        m_overflowBtn->SetFontSize(14.0f);
        m_overflowBtn->SetPadding(Thickness(6.0f));
        m_overflowBtn->SetCornerRadius(4.0f);
        m_overflowBtn->SetBorderThickness(0.0f);
        m_overflowBtn->SetBackgroundToken(ThemeTokenId::Unset);
        m_overflowBtn->SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
        m_overflowBtn->SetPressedBackgroundToken(ThemeTokenId::PressedBackground);
        m_overflowBtn->SetBackground(D2D1::ColorF(0, 0, 0, 0));
        m_overflowBtn->SetColorToken(ThemeTokenId::TextPrimary);
        m_overflowBtn->OnClick().Connect([this](UIElement*) {
            SetOverflowOpen(!IsOverflowOpen());
        });
        AddChild(m_overflowBtn);
    }
    if (!m_overflowMenu) {
        m_overflowMenu = std::make_shared<ContextMenu>();
        m_overflowMenu->SetClosedCallback([this]() {
            if (m_overflowBtn) {
                m_overflowBtn->MarkRenderRectDirty(m_overflowBtn->GetBounds());
            }
        });
    }
}

void CommandBar::StyleItemButton(Button& btn, const Item& item, bool checked) const {
    btn.SetHeight(kBtnH);
    btn.SetFontSize(12.0f);
    btn.SetCornerRadius(4.0f);
    btn.SetPadding(Thickness(
        m_labelPosition == CommandBarLabelPosition::Right && !item.label.empty() ? 8.0f : 6.0f,
        4.0f,
        m_labelPosition == CommandBarLabelPosition::Right && !item.label.empty() ? 10.0f : 6.0f,
        4.0f));
    if (checked) {
        btn.SetBackgroundToken(ThemeTokenId::AccentColor);
        btn.SetHoverBackgroundToken(ThemeTokenId::AccentColor);
        btn.SetPressedBackgroundToken(ThemeTokenId::AccentColor);
        btn.SetColorToken(ThemeTokenId::AccentForeground);
        btn.SetBackground(ThemeManager::Instance().GetColor(ThemeTokenId::AccentColor));
        btn.SetBorderThickness(0.0f);
    } else {
        btn.SetBackgroundToken(ThemeTokenId::Unset);
        btn.SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
        btn.SetPressedBackgroundToken(ThemeTokenId::PressedBackground);
        btn.SetColorToken(ThemeTokenId::TextPrimary);
        btn.SetBackground(D2D1::ColorF(0, 0, 0, 0));
        btn.SetBorderThickness(0.0f);
    }
}

void CommandBar::ApplyLabelChrome(Item& item) {
    if (!item.button) {
        return;
    }
    const bool showLabel = m_labelPosition == CommandBarLabelPosition::Right && !item.label.empty();
    item.button->SetIcon(item.icon);
    item.button->SetText(showLabel ? item.label : std::string());
    item.button->SetToolTip(item.label);
    if (!showLabel) {
        item.button->SetWidth(kIconBtn);
    } else {
        item.button->SetWidth(-1.0f);
    }
    bool checked = false;
    if (auto* toggle = dynamic_cast<ToggleButton*>(item.button.get())) {
        checked = toggle->IsChecked();
    }
    StyleItemButton(*item.button, item, checked);
}

std::shared_ptr<Button> CommandBar::AddButton(
    const std::string& label,
    const std::string& icon,
    std::shared_ptr<Command> command)
{
    Item item;
    item.kind = ItemKind::Button;
    item.label = label;
    item.icon = icon;
    item.button = std::make_shared<Button>();
    if (command) {
        if (command->GetLabel().empty()) {
            command->SetLabel(label);
        }
        item.button->SetCommand(std::move(command));
    }
    ApplyLabelChrome(item);
    AddChild(item.button);
    auto btn = item.button;
    m_items.push_back(std::move(item));
    InvalidateMeasure();
    MarkRenderRectDirty(m_bounds);
    return btn;
}

std::shared_ptr<ToggleButton> CommandBar::AddToggle(
    const std::string& label,
    const std::string& icon,
    std::shared_ptr<Command> command)
{
    Item item;
    item.kind = ItemKind::Toggle;
    item.label = label;
    item.icon = icon;
    auto toggle = std::make_shared<ToggleButton>("");
    item.button = toggle;
    if (command) {
        if (command->GetLabel().empty()) {
            command->SetLabel(label);
        }
        toggle->SetCommand(std::move(command));
    }
    toggle->OnToggled().Connect([this, raw = toggle.get()](ToggleButton*, bool) {
        for (auto& it : m_items) {
            if (it.button.get() == raw) {
                ApplyLabelChrome(it);
                break;
            }
        }
        m_overflowKeys.clear();
        RebuildOverflowMenu();
        MarkRenderRectDirty(m_bounds);
    });
    ApplyLabelChrome(item);
    AddChild(item.button);
    m_items.push_back(std::move(item));
    InvalidateMeasure();
    MarkRenderRectDirty(m_bounds);
    return toggle;
}

void CommandBar::AddSeparator() {
    Item item;
    item.kind = ItemKind::Separator;
    m_items.push_back(std::move(item));
    InvalidateMeasure();
    MarkRenderRectDirty(m_bounds);
}

std::shared_ptr<Button> CommandBar::AddSecondary(
    const std::string& label,
    const std::string& icon,
    std::shared_ptr<Command> command)
{
    Item item;
    item.kind = ItemKind::Button;
    item.secondary = true;
    item.label = label;
    item.icon = icon;
    item.button = std::make_shared<Button>();
    if (command) {
        if (command->GetLabel().empty()) {
            command->SetLabel(label);
        }
        item.button->SetCommand(std::move(command));
    }
    ApplyLabelChrome(item);
    item.button->SetVisibility(Visibility::Collapsed);
    AddChild(item.button);
    auto btn = item.button;
    m_items.push_back(std::move(item));
    m_overflowKeys.clear();
    InvalidateMeasure();
    MarkRenderRectDirty(m_bounds);
    return btn;
}

void CommandBar::AddSecondarySeparator() {
    Item item;
    item.kind = ItemKind::Separator;
    item.secondary = true;
    m_items.push_back(std::move(item));
    m_overflowKeys.clear();
    InvalidateMeasure();
    MarkRenderRectDirty(m_bounds);
}

void CommandBar::Clear() {
    SetOverflowOpen(false);
    for (auto& item : m_items) {
        if (item.button) {
            RemoveChild(item.button);
        }
    }
    m_items.clear();
    m_overflowKeys.clear();
    m_overflowCount = 0;
    InvalidateMeasure();
    MarkRenderRectDirty(m_bounds);
}

void CommandBar::SetLabelPosition(CommandBarLabelPosition position) {
    if (m_labelPosition == position) {
        return;
    }
    m_labelPosition = position;
    for (auto& item : m_items) {
        if (!item.secondary) {
            ApplyLabelChrome(item);
        }
    }
    m_overflowKeys.clear();
    InvalidateMeasure();
    MarkRenderRectDirty(m_bounds);
}

int CommandBar::GetPrimaryCount() const {
    int n = 0;
    for (const auto& item : m_items) {
        if (!item.secondary) {
            ++n;
        }
    }
    return n;
}

bool CommandBar::IsOverflowOpen() const {
    return m_overflowMenu && m_overflowMenu->IsOpen();
}

float CommandBar::MeasureItemWidth(Item& item) const {
    if (item.kind == ItemKind::Separator) {
        return kSepW;
    }
    if (!item.button) {
        return kIconBtn;
    }
    const Size sz = item.button->Measure(Size(400.0f, kBtnH));
    return (std::max)(kIconBtn, sz.width);
}

Size CommandBar::Measure(Size availableSize) {
    float w = GetWidth();
    if (w < 0.0f) {
        w = (availableSize.width > 0.0f && availableSize.width < 1e6f)
            ? availableSize.width
            : 480.0f;
    }
    float h = GetHeight();
    if (h < 0.0f) {
        h = kBarH;
    }
    m_desiredSize = Size(w, h);
    return m_desiredSize;
}

void CommandBar::Arrange(Rect finalRect) {
    const Thickness margin = GetMargin();
    Rect arranged(
        finalRect.x + margin.left,
        finalRect.y + margin.top,
        (std::max)(0.0f, finalRect.width - margin.left - margin.right),
        (std::max)(0.0f, finalRect.height - margin.top - margin.bottom));
    if (GetWidth() >= 0.0f) {
        arranged.width = (std::min)(arranged.width, GetWidth());
    }
    if (GetHeight() >= 0.0f) {
        arranged.height = (std::min)(arranged.height, GetHeight());
    }
    SetBounds(arranged);
    LayoutChrome();
    m_arrangeDirty = false;
}

void CommandBar::LayoutChrome() {
    EnsureOverflowChrome();
    const float innerH = (std::max)(kBtnH, m_bounds.height - kPad * 2.0f);
    const float y = m_bounds.y + (m_bounds.height - kBtnH) * 0.5f;
    (void)innerH;

    bool anySecondary = false;
    for (const auto& item : m_items) {
        if (item.secondary) {
            anySecondary = true;
            break;
        }
    }

    std::vector<float> widths(m_items.size(), 0.0f);
    float primaryTotal = 0.0f;
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (m_items[i].secondary) {
            continue;
        }
        widths[i] = MeasureItemWidth(m_items[i]);
        if (i + 1 < m_items.size() && !m_items[i + 1].secondary) {
            primaryTotal += widths[i] + kGap;
        } else {
            primaryTotal += widths[i];
        }
    }

    const float overflowW = kIconBtn;
    float budget = m_bounds.width - kPad * 2.0f;
    bool needOverflow = anySecondary;
    if (needOverflow) {
        budget -= overflowW + kGap;
    }
    if (!needOverflow && primaryTotal > budget) {
        needOverflow = true;
        budget = m_bounds.width - kPad * 2.0f - overflowW - kGap;
    }

    float used = 0.0f;
    int firstOverflow = -1;
    for (size_t i = 0; i < m_items.size(); ++i) {
        auto& item = m_items[i];
        if (item.secondary) {
            item.overflowed = true;
            continue;
        }
        const float next = used + (used > 0.0f ? kGap : 0.0f) + widths[i];
        if (needOverflow && next > budget + 0.5f) {
            item.overflowed = true;
            if (firstOverflow < 0) {
                firstOverflow = static_cast<int>(i);
            }
        } else {
            item.overflowed = false;
            used = next;
        }
    }
    if (firstOverflow >= 0) {
        for (size_t i = static_cast<size_t>(firstOverflow); i < m_items.size(); ++i) {
            if (!m_items[i].secondary) {
                m_items[i].overflowed = true;
            }
        }
        needOverflow = true;
    }

    float x = m_bounds.x + kPad;
    m_overflowCount = 0;
    for (size_t i = 0; i < m_items.size(); ++i) {
        auto& item = m_items[i];
        if (item.secondary || item.overflowed) {
            item.slot = Rect();
            if (item.button) {
                item.button->SetVisibility(Visibility::Collapsed);
                item.button->Arrange(Rect(m_bounds.x, m_bounds.y, 0, 0));
            }
            ++m_overflowCount;
            continue;
        }
        const float w = item.kind == ItemKind::Separator ? kSepW : widths[i];
        item.slot = Rect(x, y, w, kBtnH);
        if (item.button) {
            item.button->SetVisibility(Visibility::Visible);
            item.button->Arrange(item.slot);
        }
        x += w + kGap;
    }

    if (needOverflow && m_overflowBtn) {
        m_overflowBtn->SetVisibility(Visibility::Visible);
        const float ox = m_bounds.x + m_bounds.width - kPad - overflowW;
        m_overflowBtn->Arrange(Rect(ox, y, overflowW, kBtnH));
    } else if (m_overflowBtn) {
        m_overflowBtn->SetVisibility(Visibility::Collapsed);
        m_overflowBtn->Arrange(Rect(m_bounds.x, m_bounds.y, 0, 0));
        SetOverflowOpen(false);
    }

    RebuildOverflowMenu();
}

void CommandBar::RebuildOverflowMenu() {
    if (!m_overflowMenu) {
        return;
    }
    std::vector<int> keys;
    keys.reserve(m_items.size());
    for (size_t i = 0; i < m_items.size(); ++i) {
        const auto& item = m_items[i];
        if (item.secondary || item.overflowed) {
            const int kind = static_cast<int>(item.kind);
            const int checked = (item.kind == ItemKind::Toggle
                && item.button
                && static_cast<ToggleButton*>(item.button.get())->IsChecked()) ? 1 : 0;
            keys.push_back(static_cast<int>(i) * 8 + kind * 2 + checked);
        }
    }
    if (keys == m_overflowKeys) {
        return;
    }
    m_overflowKeys = std::move(keys);
    m_overflowMenu->ClearItems();

    bool pendingSep = false;
    bool anyItem = false;
    auto flushSep = [&]() {
        if (pendingSep && anyItem) {
            m_overflowMenu->AddSeparator();
        }
        pendingSep = false;
    };

    for (auto& item : m_items) {
        if (!item.secondary && !item.overflowed) {
            continue;
        }
        if (item.kind == ItemKind::Separator) {
            pendingSep = anyItem;
            continue;
        }
        flushSep();
        anyItem = true;
        auto* toggle = dynamic_cast<ToggleButton*>(item.button.get());
        std::shared_ptr<MenuItem> menuItem;
        if (toggle) {
            auto btn = item.button;
            menuItem = m_overflowMenu->AddItem(item.label, [toggle, btn]() {
                toggle->SetIsChecked(!toggle->IsChecked());
                if (btn) {
                    btn->ExecuteBoundCommand();
                }
            });
        } else if (item.button && item.button->GetCommand()) {
            menuItem = m_overflowMenu->AddItem(item.label, item.button->GetCommand());
        } else if (item.button) {
            auto btn = item.button;
            menuItem = m_overflowMenu->AddItem(item.label, [btn]() {
                btn->OnClick().Invoke(btn.get());
            });
        } else {
            menuItem = m_overflowMenu->AddItem(item.label);
        }
        if (toggle && menuItem) {
            menuItem->SetChecked(toggle->IsChecked());
        }
        if (menuItem && !item.icon.empty()) {
            menuItem->SetIcon(item.icon);
        }
    }
}

void CommandBar::SetOverflowOpen(bool open) {
    EnsureOverflowChrome();
    if (!m_overflowMenu) {
        return;
    }
    if (open) {
        if (m_overflowBtn && m_overflowBtn->GetVisibility() != Visibility::Visible) {
            return;
        }
        RebuildOverflowMenu();
        const Rect btn = m_overflowBtn ? m_overflowBtn->GetBounds() : m_bounds;
        m_overflowMenu->ShowAt(btn.x + btn.width - 180.0f, btn.y + btn.height);
        UIElement* curr = this;
        while (curr) {
            curr->SetContextMenu(m_overflowMenu);
            curr = curr->GetParent();
        }
        m_onOverflowOpened.Invoke();
    } else if (m_overflowMenu->IsOpen()) {
        m_overflowMenu->Hide();
    }
}

void CommandBar::DrawSeparator(GraphicsContext& ctx, const Rect& slot) const {
    if (slot.IsEmpty()) {
        return;
    }
    const ThemeTokens& tokens = ThemeManager::Instance().GetTokens();
    const float x = slot.x + slot.width * 0.5f;
    ctx.DrawLine(
        Point(x, slot.y + 6.0f),
        Point(x, slot.y + slot.height - 6.0f),
        Mix(tokens.cardBorder, tokens.textMuted, 0.25f),
        1.0f);
}

void CommandBar::OnRender(GraphicsContext& ctx) {
    const ThemeTokens& tokens = ThemeManager::Instance().GetTokens();
    ctx.FillRoundedRect(m_bounds, GetCornerRadius(), tokens.cardBackground);
    ctx.DrawRoundedRect(m_bounds, GetCornerRadius(), tokens.cardBorder, 1.0f);
    for (const auto& item : m_items) {
        if (item.kind == ItemKind::Separator && !item.overflowed && !item.secondary) {
            DrawSeparator(ctx, item.slot);
        }
    }
}

bool CommandBar::OnKeyDown(int vkCode) {
    if (vkCode == VK_ESCAPE && IsOverflowOpen()) {
        SetOverflowOpen(false);
        return true;
    }
    if ((vkCode == VK_DOWN || vkCode == VK_SPACE || vkCode == VK_RETURN)
        && m_overflowBtn
        && m_overflowBtn->IsFocused()
        && m_overflowBtn->GetVisibility() == Visibility::Visible) {
        SetOverflowOpen(true);
        if (m_overflowMenu) {
            m_overflowMenu->HighlightFirst();
        }
        return true;
    }
    return false;
}

void CommandBar::OnNavigatedFrom() {
    SetOverflowOpen(false);
}

} // namespace CUI
