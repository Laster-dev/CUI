#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/Toast.h"
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
}

ShowcasePage BuildButtonPage(const ShowcaseContext& ctx) {
    auto target = ElevatedButton("交互测试按钮").Background("#007ACC").HoverBackground("#0098FF").PressedBackground("#005A9E").FontSize(14).Padding(16, 8, 16, 8).CornerRadius(4).Build();
    auto log = CreateShowcaseText("[就绪] 点击目标按钮触发 OnClick 点击事件...", 12.0f, "#B5CEA8", false, "Consolas");
    target->OnClick().Connect([window = ctx.windowRef, log](UIElement*) {
        log->SetText("[事件] OnClick 已触发，按钮交互链路正常。");
        Toast::Show(window->GetRootElement().get(), "Button", "按钮点击触发 OnClick 事件！", ToastCorner::BottomRight, 2200);
    });

    auto logPanel = Column(4).Padding(10).CornerRadius(4).Border(ThemeManager::Instance().GetColorHex("cardBorder"), 1).Children({
            CreateShowcaseText("事件日志 (Event Log)", 11.0f, "#4EC9B0", true),
            log
        }).Build();
    BindThemeToken(logPanel, "theme.backgroundToken", "cardBackground");
    BindThemeToken(logPanel, "theme.borderToken", "cardBorder");

    auto demo = Column(16).Children({
        CreateDemoSurface({ target }, 0.0f),
        logPanel
    }).Build();

    return { "Button 按钮", CreatePage(
        "Button 按钮控件全属性交互控制台",
        "由 PropertyGrid 自动化反射引擎进行 100% 精准双向绑定控制。",
        demo,
        CreatePropertyGrid(ctx, target), target) };
}
