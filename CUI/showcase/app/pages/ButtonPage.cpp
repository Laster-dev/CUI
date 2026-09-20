#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "PageRegistry.h"
#include "framework/core/Widgets.h"
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
                element->ApplyBackgroundToken(id);
                element->ApplyBackground(ThemeManager::Instance().GetColor(tokenName));
    } else if (tokenProp == "theme.borderToken") {
                element->ApplyBorderToken(id);
                element->ApplyBorderBrush(ThemeManager::Instance().GetColor(tokenName));
    } else if (tokenProp == "theme.hoverBackgroundToken") {
        element->ApplyHoverBackgroundToken(id);
    } else if (tokenProp == "theme.pressedBackgroundToken") {
        element->ApplyPressedBackgroundToken(id);
    } else if (tokenProp == "theme.colorToken") {
        element->ApplyColorToken(id);
    } else if (tokenProp == "theme.focusedBorderToken") {
        element->ApplyFocusedBorderToken(id);
    }
    return element;
}

template<typename T>
void WireFlyout(T& ctrl, Window* window, const std::shared_ptr<UIElement>& log, const char* kind) {
    ctrl.AddItem("新建");
    ctrl.AddItem("打开");
    ctrl.AddItem("删除");
    if constexpr (requires { ctrl.OnItemChosen(); }) {
        ctrl.OnItemChosen().Connect([window, log, kind](auto*, int, const std::string& text) {
            std::static_pointer_cast<TextBlock>(log)->ApplyText(std::string("[") + kind + "] 菜单：" + text);
            Toast::Show(window->GetRootElement().get(), kind, text.c_str(), ToastCorner::BottomRight, 1800);
        });
    }
}
} // namespace

ShowcasePage BuildButtonPage(const ShowcaseContext& ctx) {
    auto log = CreateShowcaseText("[就绪] 点击目标按钮或按钮族触发事件…", 12.0f, "#B5CEA8", false, "Consolas");
    auto target = ElevatedButton("交互测试按钮")
        .Background(Rgb(0x007ACC))
        .HoverBackground(Rgb(0x0098FF))
        .PressedBackground(Rgb(0x005A9E))
        .FontSize(14)
        .Padding(16, 8, 16, 8)
        .CornerRadius(4)
        .ToolTip("框架 ApplyToolTip：任意页可复用，支持换行与延迟显示/隐藏。")
        .OnClick([window = ctx.windowRef, log](UIElement*) {
            std::static_pointer_cast<TextBlock>(log)->ApplyText("[事件] OnClick 已触发，按钮交互链路正常。");
            Toast::Show(window->GetRootElement().get(), "Button", "按钮点击触发 OnClick 事件！", ToastCorner::BottomRight, 2200);
        })
        .Build();

    CUI::Widgets::Ref toggleState =std::static_pointer_cast<TextBlock>(
        CreateShowcaseText("未选中", 12.0f, "textSecondary", false));
    CUI::Widgets::Ref toggle = Widgets::ToggleButton("Bold")
        .Icon("B")
        .Shared();
    toggle->OnToggled().Connect([toggleState, log](ToggleButton*, bool on) {
        toggleState.Text(on ? "已选中" : "未选中");
        std::static_pointer_cast<TextBlock>(log)->ApplyText(on ? "[ToggleButton] 锁定选中" : "[ToggleButton] 取消选中");
    });

    auto splitBuilder = Widgets::SplitButton("保存");
    splitBuilder.OnClick([window = ctx.windowRef, log](UIElement*) {
        std::static_pointer_cast<TextBlock>(log)->ApplyText("[SplitButton] 主区 Click = 保存");
        Toast::Show(window->GetRootElement().get(), "SplitButton", "主区：保存", ToastCorner::BottomRight, 1800);
    });
    auto split = splitBuilder.Shared();
    WireFlyout(*split, ctx.windowRef, log, "SplitButton");

    auto dropBuilder = Widgets::DropDownButton("更多");
    auto drop = dropBuilder.Shared();
    WireFlyout(*drop, ctx.windowRef, log, "DropDownButton");
    drop->OnItemChosen().Connect([log](DropDownButton*, int, const std::string& text) {
        std::static_pointer_cast<TextBlock>(log)->ApplyText("[DropDownButton] 选中 " + text);
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
        demo) };
}
