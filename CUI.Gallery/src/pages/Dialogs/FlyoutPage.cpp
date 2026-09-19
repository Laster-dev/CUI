#include "Gallery.h"
using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildFlyoutPage() {
    auto status = MakeStatus("点击按钮弹出浮出层，此处显示操作提示。");

    // ── 1. 简单文本浮出层 ────────────────────────────────────────────────
    auto btnSimple = Button("简单文本浮出层")
        .OnClick([](UIElement* src) {
            auto flyout = FlyoutWidget();
            flyout.Placement(FlyoutPlacement::Bottom);
            
            auto content = Column(8, {
            MakeLabel("这是一个 Flyout 浮出层", 13.0f, ThemeTokenId::TextPrimary, true),
            MakeLabel("点击此区域外的任意位置即可关闭。", 12.0f, ThemeTokenId::TextSecondary),
            });
            CUI::DSL::Borrow(flyout).Content(content);
            CUI::DSL::Borrow(src).AddChild(flyout);
            flyout->ShowAt(src);
            });

    // ── 2. 多方向放置 ────────────────────────────────────────────────────
    auto btnTop    = Button("↑ Top");
    auto btnBottom = Button("↓ Bottom");
    auto btnLeft   = Button("← Left");
    auto btnRight  = Button("→ Right");

    CUI::DSL::Borrow(btnTop).Width(90.0f);
    CUI::DSL::Borrow(btnBottom).Width(90.0f);
    CUI::DSL::Borrow(btnLeft).Width(90.0f);
    CUI::DSL::Borrow(btnRight).Width(90.0f);

    auto makePlacementFlyout = [](UIElement* src, FlyoutPlacement p, const std::string& label) {
        auto flyout = FlyoutWidget();
        flyout.Placement(p);
        auto content = Column(6, {
            MakeLabel(label, 12.0f, ThemeTokenId::TextPrimary),
            MakeLabel("Placement 演示内容。", 12.0f, ThemeTokenId::TextSecondary),
        });
        CUI::DSL::Borrow(flyout).Content(content);
        CUI::DSL::Borrow(src).AddChild(flyout);
        flyout->ShowAt(src);
    };

    btnTop->OnClick().Connect([makePlacementFlyout](UIElement* src) {
        makePlacementFlyout(src, FlyoutPlacement::Top, "放置于上方（Top）");
    });
    btnBottom->OnClick().Connect([makePlacementFlyout](UIElement* src) {
        makePlacementFlyout(src, FlyoutPlacement::Bottom, "放置于下方（Bottom）");
    });
    btnLeft->OnClick().Connect([makePlacementFlyout](UIElement* src) {
        makePlacementFlyout(src, FlyoutPlacement::Left, "放置于左侧（Left）");
    });
    btnRight->OnClick().Connect([makePlacementFlyout](UIElement* src) {
        makePlacementFlyout(src, FlyoutPlacement::Right, "放置于右侧（Right）");
    });

    // ── 3. 带操作按钮的浮出层 ───────────────────────────────────────────
    auto btnAction = Button("带操作的浮出层");
    CUI::DSL::Borrow(btnAction).BackgroundToken(ThemeTokenId::CardBackground);
    CUI::DSL::Borrow(btnAction).ForegroundToken(ThemeTokenId::TextPrimary);
    CUI::DSL::Borrow(btnAction).BorderToken(ThemeTokenId::CardBorder);
    CUI::DSL::Borrow(btnAction).BorderThickness(1.0f);
    btnAction->OnClick().Connect([status](UIElement* src) {
        auto flyout = FlyoutWidget();
        flyout.Placement(FlyoutPlacement::Bottom);

        auto confirmBtn = Button("删除")
            .Background(Color::Hex("#C62828"))
            .HoverBackground(Color::Hex("#B71C1C"))
            .Foreground(Color::White)
            .Width(72.0f);

        auto cancelBtn = Button("取消");
        CUI::DSL::Borrow(cancelBtn).BackgroundToken(ThemeTokenId::CardBackground);
        CUI::DSL::Borrow(cancelBtn).ForegroundToken(ThemeTokenId::TextPrimary);
        CUI::DSL::Borrow(cancelBtn).BorderToken(ThemeTokenId::CardBorder);
        CUI::DSL::Borrow(cancelBtn).BorderThickness(1.0f);
        CUI::DSL::Borrow(cancelBtn).Width(72.0f);

        auto content = Column(12, {
            MakeLabel("确认删除？", 13.0f, ThemeTokenId::TextPrimary, true),
            MakeLabel("此操作将永久移除所选项目。", 12.0f, ThemeTokenId::TextSecondary),
            Row(8, {confirmBtn, cancelBtn }),
        });
        CUI::DSL::Borrow(flyout).Content(content);

        confirmBtn->OnClick().Connect([status, flyout](UIElement*) {
            status->Text = "已确认删除操作。";
            flyout->Hide();
        });
        cancelBtn->OnClick().Connect([status, flyout](UIElement*) {
            status->Text = "已取消删除操作。";
            flyout->Hide();
        });

        CUI::DSL::Borrow(src).AddChild(flyout);
        flyout->ShowAt(src);
    });

    // ── 4. 包含输入框的浮出层 ───────────────────────────────────────────
    auto btnInputFlyout = Button("输入浮出层");
    CUI::DSL::Borrow(btnInputFlyout).BackgroundToken(ThemeTokenId::CardBackground);
    CUI::DSL::Borrow(btnInputFlyout).ForegroundToken(ThemeTokenId::TextPrimary);
    CUI::DSL::Borrow(btnInputFlyout).BorderToken(ThemeTokenId::CardBorder);
    CUI::DSL::Borrow(btnInputFlyout).BorderThickness(1.0f);
    btnInputFlyout->OnClick().Connect([status](UIElement* src) {
        auto flyout = FlyoutWidget();
        flyout.Placement(FlyoutPlacement::Bottom);

        auto input = TextField()
            .Placeholder("输入新项目名称")
            .Width(200.0f);

        auto applyBtn = Button("应用")
            .Width(60.0f);

        auto content = Column(10, {
            MakeLabel("重命名项目", 13.0f, ThemeTokenId::TextPrimary, true),
            input,
            applyBtn,
        });
        CUI::DSL::Borrow(flyout).Content(content);

        applyBtn->OnClick().Connect([status, flyout, input](UIElement*) {
            std::string name = input->GetText();
            status->Text = "已应用名称：「" + (name.empty() ? "（空）" : name) + "」。";
            flyout->Hide();
        });

        CUI::DSL::Borrow(src).AddChild(flyout);
        flyout->ShowAt(src);
    });

    SamplePageSpec spec;
    spec.title    = "Flyout（浮出层）";
    spec.subtitle = "轻量弹出面板，附着于目标控件旁，点击外部区域自动收拢，不阻断背景操作。";
    spec.sections = {
        {
            "简单浮出层",
            "调用 ShowAt(target) 将 Flyout 显示在指定控件附近，.Placement() 控制停靠方向，默认为 Bottom。",
            Column(12, {
                btnSimple,
                status,
            }),
        },
        {
            "停靠方向",
            "支持 Top、Bottom、Left、Right 四个方向，框架会在边界受限时自动调整位置。",
            Row(10, { btnTop, btnBottom, btnLeft, btnRight }),
        },
        {
            "带操作按钮 & 输入框",
            "Flyout 内容为任意 UIElement，可嵌入按钮、输入框等控件实现轻量交互，无需打开完整对话框。",
            Row(12, { btnAction, btnInputFlyout }),
        },
    };
    spec.source =
        "auto flyout = FlyoutWidget();\n"
        "flyout.Placement(FlyoutPlacement::Bottom);\n"
        "flyout.Content(/* 任意 UIElement */);\n"
        "parent->AddChild(flyout); // 先挂载到活动 UI 树\n"
        "flyout->ShowAt(targetElement);\n"
        "\n"
        "// 手动关闭\n"
        "flyout->Hide();\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery




