#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/Toast.h"
#include "framework/controls/ToggleButton.h"
#include "framework/controls/DropDownButton.h"
#include "framework/controls/SplitButton.h"
#include "framework/controls/TextBlock.h"
#include "framework/style/ThemeManager.h"
#include "framework/style/ThemeTokenId.h"

using namespace CUI;
using namespace CUI::DSL;

namespace {
template <typename T>
std::shared_ptr<T> BindThemeToken(const std::shared_ptr<T>& element, const std::string& tokenProp, const std::string& tokenName) {
    if (!element) {
        return element;
    }
    ThemeTokenId id = ThemeTokenIdFromName(tokenName);
    if (tokenProp == "theme.backgroundToken") {
        element->SetBackgroundToken(id);
        element->SetBackground(ThemeManager::Instance().GetColor(tokenName));
    } else if (tokenProp == "theme.borderToken") {
        element->SetBorderToken(id);
        element->SetBorderBrush(ThemeManager::Instance().GetColor(tokenName));
    } else if (tokenProp == "theme.hoverBackgroundToken") {
        element->SetHoverBackgroundToken(id);
    } else if (tokenProp == "theme.pressedBackgroundToken") {
        element->SetPressedBackgroundToken(id);
    } else if (tokenProp == "theme.colorToken") {
        element->SetColorToken(id);
    } else if (tokenProp == "theme.focusedBorderToken") {
        element->SetFocusedBorderToken(id);
    }
    return element;
}

void WireFlyout(DropDownButton& btn, Window* window, const std::shared_ptr<UIElement>& log, const char* kind) {
    btn.AddItem("新建", [window, log, kind]() {
        log->SetText(std::string("[") + kind + "] 菜单：新建");
        Toast::Show(window->GetRootElement().get(), kind, "新建", ToastCorner::BottomRight, 1800);
    });
    btn.AddItem("打开", [window, log, kind]() {
        log->SetText(std::string("[") + kind + "] 菜单：打开");
        Toast::Show(window->GetRootElement().get(), kind, "打开", ToastCorner::BottomRight, 1800);
    });
    btn.AddSeparator();
    btn.AddItem("删除", [window, log, kind]() {
        log->SetText(std::string("[") + kind + "] 菜单：删除");
        Toast::Show(window->GetRootElement().get(), kind, "删除", ToastCorner::BottomRight, 1800);
    });
}
} // namespace

ShowcasePage BuildButtonPage(const ShowcaseContext& ctx) {
    auto target = ElevatedButton("交互测试按钮").Background(Rgb(0x007ACC)).HoverBackground(Rgb(0x0098FF)).PressedBackground(Rgb(0x005A9E)).FontSize(14).Padding(16, 8, 16, 8).CornerRadius(4).ToolTip("框架 SetToolTip：任意页可复用，支持换行与延迟显示/隐藏。").Build();
    auto log = CreateShowcaseText("[就绪] 点击目标按钮或按钮族触发事件…", 12.0f, "#B5CEA8", false, "Consolas");
    target->OnClick().Connect([window = ctx.windowRef, log](UIElement*) {
        log->SetText("[事件] OnClick 已触发，按钮交互链路正常。");
        Toast::Show(window->GetRootElement().get(), "Button", "按钮点击触发 OnClick 事件！", ToastCorner::BottomRight, 2200);
    });

    auto toggle = std::make_shared<ToggleButton>("Bold");
    toggle->SetIcon("B");
    auto toggleState = std::static_pointer_cast<TextBlock>(
        CreateShowcaseText("未选中", 12.0f, "textSecondary", false));
    toggle->OnToggled().Connect([toggleState, log](ToggleButton*, bool on) {
        toggleState->SetText(on ? "已选中" : "未选中");
        log->SetText(on ? "[ToggleButton] 锁定选中" : "[ToggleButton] 取消选中");
    });

    auto split = std::make_shared<SplitButton>("保存");
    split->OnClick().Connect([window = ctx.windowRef, log](UIElement*) {
        log->SetText("[SplitButton] 主区 Click = 保存");
        Toast::Show(window->GetRootElement().get(), "SplitButton", "主区：保存", ToastCorner::BottomRight, 1800);
    });
    WireFlyout(*split, ctx.windowRef, log, "SplitButton");

    auto drop = std::make_shared<DropDownButton>("更多");
    WireFlyout(*drop, ctx.windowRef, log, "DropDownButton");
    drop->OnItemChosen().Connect([log](DropDownButton*, int, const std::string& text) {
        log->SetText("[DropDownButton] 选中 " + text);
    });

    auto family = Row(16).Children({
        Column(8).Children({
            CreateShowcaseText("ToggleButton", 12.0f, "textSecondary", true),
            toggle,
            toggleState,
            CreateShowcaseText("Space 切换选中", 11.0f, "textMuted", false),
        }).Build(),
        Column(8).Children({
            CreateShowcaseText("SplitButton", 12.0f, "textSecondary", true),
            split,
            CreateShowcaseText("主区 Click · 箭头下拉", 11.0f, "textMuted", false),
        }).Build(),
        Column(8).Children({
            CreateShowcaseText("DropDownButton", 12.0f, "textSecondary", true),
            drop,
            CreateShowcaseText("整钮打开 · Alt+↓ / Space", 11.0f, "textMuted", false),
        }).Build(),
    }).Build();

    auto logPanel = Column(4).Padding(10).CornerRadius(4).BorderToken(ThemeTokenId::CardBorder, 1).Children({
            CreateShowcaseText("事件日志 (Event Log)", 11.0f, "#4EC9B0", true),
            log
        }).Build();
    BindThemeToken(logPanel, "theme.backgroundToken", "cardBackground");
    BindThemeToken(logPanel, "theme.borderToken", "cardBorder");

    auto demo = Column(16).Children({
        CreateDemoSurface({ target }, 0.0f),
        CreateDemoSurface({
            CreateShowcaseText("按钮族（自绘，不拼 Button + ContextMenu）", 13.0f, "textPrimary", true),
            family,
        }, 12.0f),
        logPanel
    }).Build();

    return { "Button 按钮", CreatePage(
        "Button 按钮控件全属性交互控制台",
        "标准 Button + Toggle / Split / DropDown。Split 主区与箭头分命中；下拉为自绘 IPopup 菜单。",
        demo,
        CreatePropertyGrid(ctx, target), target) };
}
