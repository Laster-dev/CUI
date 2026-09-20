#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "Gallery.h"
#include "framework/core/Widgets.h"
#include <format>

using namespace CUI;
using namespace CUI::DSL;
using CUI::DSL::Fluent::Button;

namespace Gallery {

namespace {

class AnimatedPopupBox : public UIElement {
public:
    AnimatedPopupBox() {
        m_opacityScalar.Reset(0.0f);
        m_offsetScalar.Reset(-16.0f);
                this->SetHeight(70.0f);
    }

    void TriggerReveal(float durationSec, EasingType easing, float startOffset) {
        m_opacityScalar.Reset(0.0f);
        m_offsetScalar.Reset(-startOffset);
 
        m_offsetScalar.SetTarget(0.0f);
        m_duration = durationSec;
        m_easing = easing;
        m_running = true;
        RequestAnimationTicks();
    }

    bool HasSelfAnimation() const override {
        return m_running;
    }

    bool OnAnimationTick() override {
        if (!m_running) return false;
        const float dt = GetAnimationDeltaSeconds();
        AnimationSpec spec{ 0.20f, 0.005f, m_duration, m_easing };
        const bool opMoving = m_opacityScalar.Tick(dt, spec);
        const bool offMoving = m_offsetScalar.Tick(dt, spec);

        SetComposeOpacity(m_opacityScalar.Current());
        SetComposeOffset(0.0f, m_offsetScalar.Current());
        MarkRenderContentDirty();

        m_running = opMoving || offMoving;
        return m_running;
    }

    void OnRender(GraphicsContext& ctx) override {
        const Rect rc = GetBounds();
        D2D1_COLOR_F bg = ThemeManager::Instance().GetColor(ThemeTokenId::CardBackground);
        bg.a = m_opacityScalar.Current();
        ctx.FillRoundedRect(rc, 8.0f, bg);

        D2D1_COLOR_F border = ThemeManager::Instance().GetColor(ThemeTokenId::AccentColor);
        border.a = m_opacityScalar.Current();
        ctx.DrawRoundedRect(rc, 8.0f, border, 1.5f);

        ctx.DrawText(
            "✨ 正在以自定义缓出曲线展开渲染 (Reveal Active)",
            rc,
            ThemeManager::Instance().GetColor(ThemeTokenId::TextPrimary),
            "微软雅黑",
            13.0f,
            DWRITE_TEXT_ALIGNMENT_CENTER,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

private:
    AnimatedScalar m_opacityScalar;
    AnimatedScalar m_offsetScalar;
    float m_duration = 0.22f;
    EasingType m_easing = EasingType::EaseOutCubic;
    bool m_running = false;
};

} // namespace

Element BuildPopupRevealPage() {
    auto status = MakeStatus("点击下方按钮呼出各种浮动层，观察 Fluent 220ms 展开曲线与平滑淡入。");

    // 1. Flyout 气泡弹出层
    auto btnFlyout = Button("弹出 Flyout 气泡")
        .OnClick([status](UIElement* src) {
            CUI::Widgets::Ref flyout = Widgets::Flyout().Shared();
                        flyout.Placement(FlyoutPlacement::Bottom);
            auto content = Column(8, {
                MakeLabel("Fluent 220ms Flyout", 14.0f, ThemeTokenId::TextPrimary, true),
                MakeLabel("采用 EaseOutCubic 曲线展开，带有轻微 Y 轴滑入与阴影合成。", 12.0f, ThemeTokenId::TextSecondary, false),
            });
                        flyout.Content(content.Build());
                        src->AddChild(flyout);
            flyout->ShowAt(src);
            status->Text = "已呼出 Flyout 气泡层。";
        });

    // 2. TeachingTip 教学提示气泡
    auto btnTeachingTip = Button("弹出 TeachingTip 引导")
        .OnClick([status](UIElement* src) {
            CUI::Widgets::Ref tip = Widgets::TeachingTip().Shared();
                        tip.Title("智能引导提示 (TeachingTip)");
                        tip.Message("由 AnimationService 自动调度浮层生命周期。小三角气泡锚定于宿主四周展开。");
                        tip.ActionText("我知道了");
            tip->OnAction().Connect([status, tip]() {
                status->Text = "已点击【我知道了】，提示气泡已消退。";
                tip->Close();
            });
                        src->AddChild(tip);
            tip->ShowAround(src);
            status->Text = "已呼出 TeachingTip 教学提示。";
        });

    // 3. ContextMenu 级联菜单呼出（支持左键点击或右键点击）
    auto menu = std::make_shared<ContextMenu>();
    auto newSub = menu->AddSubMenu("新建 (New)");
    newSub->AddItem("文本文档 (.txt)", [status]() { status->Text = "已创建：文本文档.txt"; });
    newSub->AddItem("C++ 源代码 (.cpp)", [status]() { status->Text = "已创建：main.cpp"; });
    newSub->AddItem("JSON 配置文件 (.json)", [status]() { status->Text = "已创建：config.json"; });
    menu->AddItem("打开项目 (Open Project)", "Ctrl+O", [status]() { status->Text = "已执行：打开项目"; });
    menu->AddSeparator();
    menu->AddItem("系统偏好设定 (Preferences)", "Ctrl+,", [status]() { status->Text = "已执行：偏好设置"; });

    auto btnContextMenu = Button("呼出 ContextMenu 级联菜单")
        .BackgroundToken(ThemeTokenId::CardBackground)
        .ForegroundToken(ThemeTokenId::TextPrimary)
        .BorderToken(ThemeTokenId::CardBorder)
        .BorderThickness(1.0f)
        .OnClick([menu, status](UIElement* src) {
            if (Window* win = Window::Current()) {
                POINT pt{};
                ::GetCursorPos(&pt);
                ::ScreenToClient(win->GetHWND(), &pt);
                Point logicalPt = win->ClientPointToLogical(pt.x, pt.y);
                                win->SetActiveContextMenu(menu);
                menu->ShowAt(logicalPt.x, logicalPt.y);
                status->Text = "已呼出 ContextMenu 级联菜单。";
            }
        });

        btnContextMenu->SetContextMenu(menu);

    // 4. ContentDialog 模态对话框
    auto btnDialog = Button("弹出模态对话框 (ContentDialog)")
        .BackgroundToken(ThemeTokenId::CardBackground)
        .ForegroundToken(ThemeTokenId::TextPrimary)
        .BorderToken(ThemeTokenId::CardBorder)
        .BorderThickness(1.0f)
        .OnClick([status](UIElement* src) {
            CUI::Widgets::Ref dlg = Widgets::ContentDialog().Shared();
                        dlg.Title("模态弹窗过渡动效");
                        dlg.Message("观察背景半透明遮罩的平滑渐显，以及卡片从 95% 轻微缩放展开至 100% 的质感过渡。");
                        dlg.PrimaryButtonText("确定");
                        dlg.CloseButtonText("取消");
                        src->AddChild(dlg);
            dlg->Show([status, dlg](DialogResult r) {
                status->Text = (r == DialogResult::Primary) ? "对话框已确认 (Primary)" : "对话框已取消 (Close)";
            });
        });

    // 5. 下拉列表 ComboBox
    CUI::Widgets::Ref combo = Widgets::ComboBox()
        .Width(240.0f).Shared();
    combo->AddItem("Fluent 220ms 标准缓出");
    combo->AddItem("Spring 欠阻尼弹性展开");
    combo->AddItem("Linear 匀速展开");
    combo->AddItem("EaseOutBack 过冲展开");
        combo.SelectedIndex(0);

    // 6. 缓动参数微调模拟台
    auto animatedTestBox = std::make_shared<AnimatedPopupBox>();
        animatedTestBox->SetWidth(440.0f);

    CUI::Widgets::Ref sliderDuration = Widgets::Slider().Shared();
    sliderDuration.Minimum(80.0f);
    sliderDuration.Maximum(600.0f);
    sliderDuration.Value(220.0f);
        sliderDuration.Width(180.0f);

    CUI::Widgets::Ref sliderOffset = Widgets::Slider().Shared();
    sliderOffset.Minimum(0.0f);
    sliderOffset.Maximum(40.0f);
    sliderOffset.Value(16.0f);
        sliderOffset.Width(180.0f);

    auto statusDuration = MakeStatus(std::format("展开时长 (Duration): {:.0f} ms", 220.0f));
    auto statusOffset = MakeStatus(std::format("位移偏移量 (Slide Offset): {:.0f} px", 16.0f));

    sliderDuration->OnValueChanged().Connect([statusDuration](Slider*, float v) {
        statusDuration->Text = std::format("展开时长 (Duration): {:.0f} ms", v);
    });

    sliderOffset->OnValueChanged().Connect([statusOffset](Slider*, float v) {
        statusOffset->Text = std::format("位移偏移量 (Slide Offset): {:.0f} px", v);
    });

    std::shared_ptr<ComboBox> comboPtr;
    comboPtr = combo;

    auto btnTestCustom = Button("测试自定义展开动效")
        .OnClick([animatedTestBox, sliderDuration, sliderOffset, comboPtr](UIElement*) {
            const float dur = sliderDuration->GetValue() / 1000.0f;
            const float off = sliderOffset->GetValue();
            EasingType easing = EasingType::EaseOutCubic;
            switch (comboPtr->GetSelectedIndex()) {
            case 1: easing = EasingType::Spring; break;
            case 2: easing = EasingType::Linear; break;
            case 3: easing = EasingType::EaseOutBack; break;
            default: easing = EasingType::EaseOutCubic; break;
            }
            animatedTestBox->TriggerReveal(dur, easing, off);
        });

    SamplePageSpec spec;
    spec.title = "Popup Reveal (弹出层缓出动效)";
    spec.subtitle = "WinUI 3 / Fluent Design 规范下的 220ms 缓出曲线（EaseOutCubic）与弹出层生命周期调度展示。";
    spec.sections = {
        {
            "浮出层实机演示 (Interactive Popups)",
            "点击下方各控件，体验 Flyout 气泡、TeachingTip 引导、ContextMenu 级联菜单及 ContentDialog 模态对话框的平滑展开动效。",
            Column(12, {
                status,
                Row(12, { btnFlyout, btnTeachingTip, btnContextMenu, btnDialog }),
            }),
        },
        {
            "展开曲线参数实验室 (Curve Tuning Lab)",
            "自由调节展开总时长（Duration）、滑入偏移量（Slide Offset）及缓动曲线类型，点击测试按钮观察平滑效果。",
            Column(14, {
                Row(16, {
                    Column(6, { MakeLabel("展开缓动算法", 12.0f, ThemeTokenId::TextSecondary), comboPtr }),
                    Column(6, { statusDuration, sliderDuration }),
                    Column(6, { statusOffset, sliderOffset }),
                }),
                btnTestCustom,
                animatedTestBox,
            }),
        },
    };
    spec.source =
        "// WinUI 3 官方规范：220ms Cubic Ease-Out\n"
        "struct PopupReveal {\n"
        "    static constexpr AnimationSpec kSpec{ 0.20f, 0.005f, 0.22f, EasingType::EaseOutCubic };\n"
        "};\n\n"
        "// 浮层显示时自动登记在 AnimationService 中，展开后自动注销时钟\n"
        "flyout->ShowAt(targetElement);\n";

    return BuildSamplePage(spec);
}

} // namespace Gallery






