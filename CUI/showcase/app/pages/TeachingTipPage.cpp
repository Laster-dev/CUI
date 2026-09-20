#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/TeachingTip.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/Toast.h"

using namespace CUI;
using namespace CUI::DSL;

namespace {
std::shared_ptr<Button> MakeTipButton(const std::string& text) {
    CUI::Widgets::Ref btn = CUI::Widgets::Button(text).Shared();
        btn.Width(132.0f);
        btn.Height(34.0f);
        btn.CornerRadius(4.0f);
    return btn;
}
} // namespace

ShowcasePage BuildTeachingTipPage(const ShowcaseContext& ctx) {
    CUI::Widgets::Ref shortTip =MakeTipButton("短提示");
        shortTip.ToolTip("保存");

    CUI::Widgets::Ref wrapTip =MakeTipButton("长文本换行");
        wrapTip.ToolTip(
        "这是框架 ApplyToolTip 的换行示例：超过最大宽度后自动折行，气泡带三角并在靠近视口边缘时翻转。");
        wrapTip.ToolTipMaxWidth(220.0f);

    CUI::Widgets::Ref timeoutTip =MakeTipButton("超时自动隐藏");
        timeoutTip.ToolTip("悬停约 0.45s 后出现，显示 3 秒后自动消失；移出还有短暂隐藏延迟。");
        timeoutTip.ToolTipAutoHideMs(3000);

    CUI::Widgets::Ref anchor =MakeTipButton("锚点目标");
        anchor.Width(160.0f);
        anchor.ToolTip("TeachingTip 会指向这个锚点。");

    CUI::Widgets::Ref tip = CUI::Widgets::TeachingTip().Shared();
        tip.Title("欢迎使用 TeachingTip");
        tip.Message("自绘气泡 + 三角指向锚点。标题、正文、关闭与操作都是热区，不是 Button 子控件。");
        tip.ActionText("知道了");
        tip.PreferredPlacement(BubblePlacement::Bottom);

    CUI::Widgets::Ref log =std::static_pointer_cast<TextBlock>(
        CreateShowcaseText("操作日志：就绪", 12.0f, "#B5CEA8", false, "Consolas"));

    tip->OnAction().Connect([window = ctx.windowRef, log]() {
                log.Text("[TeachingTip] 操作按钮");
        if (window) {
            Toast::Show(window->GetRootElement().get(), "TeachingTip", "操作按钮", ToastCorner::BottomRight, 1800);
        }
    });
    tip->OnClosed().Connect([log]() {
                log.Text("[TeachingTip] 已关闭");
    });

    auto openAt = [tip, anchor](BubblePlacement placement, bool modal) {
        return [tip, anchor, placement, modal](UIElement*) {
                        tip.IsModal(modal);
                        tip.PreferredPlacement(placement);
            if (modal) {
                                tip.Title("模态引导");
                                tip.Message("遮罩打开期间点击空白处、关闭或 Esc 均可关掉。场景在开关动画期间冻结。");
                                tip.ActionText("完成");
            } else {
                                tip.Title("欢迎使用 TeachingTip");
                                tip.Message("自绘气泡 + 三角指向锚点。标题、正文、关闭与操作都是热区，不是 Button 子控件。");
                                tip.ActionText("知道了");
            }
            tip->ShowAround(anchor.get());
        };
    };

    auto btnBottom = MakeTipButton("下方");
    btnBottom->OnClick().Connect(openAt(BubblePlacement::Bottom, false));
    auto btnTop = MakeTipButton("上方");
    btnTop->OnClick().Connect(openAt(BubblePlacement::Top, false));
    auto btnLeft = MakeTipButton("左侧");
    btnLeft->OnClick().Connect(openAt(BubblePlacement::Left, false));
    auto btnRight = MakeTipButton("右侧");
    btnRight->OnClick().Connect(openAt(BubblePlacement::Right, false));
    auto btnModal = MakeTipButton("模态遮罩");
    btnModal->OnClick().Connect(openAt(BubblePlacement::Top, true));
    auto btnClose = MakeTipButton("关闭");
    btnClose->OnClick().Connect([tip](UIElement*) { tip->Close(); });

    auto demo = Column(12).Children({
        CreateDemoSurface({
            CreateShowcaseText("1. Tooltip（UIElement::ApplyToolTip，任意页可复用）", 12.0f, "textSecondary", false),
            CreateShowcaseText("悬停延迟显示，移出延迟隐藏；长文本换行；超时示例 3 秒后消失。", 12.0f, "textMuted", false),
            Row(10).Children({ shortTip, wrapTip, timeoutTip }).Build(),
        }, 10.0f),
        CreateDemoSurface({
            CreateShowcaseText("2. TeachingTip 锚点四边", 12.0f, "textSecondary", false),
            Row(10).Children({ btnTop, btnBottom, btnLeft, btnRight }).Build(),
            Row(10).Children({ btnModal, btnClose, anchor }).Build(),
            log,
            tip,
        }, 10.0f),
    }).Build();

    return { "TeachingTip", CreatePage(
        "TeachingTip / Tooltip",
        "Tooltip 是框架能力（ApplyToolTip）；TeachingTip 是自绘 IPopup 气泡，可模态，指向锚点。",
        demo) };
}
