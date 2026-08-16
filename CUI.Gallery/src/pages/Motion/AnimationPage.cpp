#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/animation/AnimationService.h"
#include "framework/style/ThemeManager.h"
#include <format>
#include <vector>

using namespace CUI;
using namespace CUI::DSL;
using CUI::DSL::Fluent::Button;

namespace Gallery {

namespace {

// 动画属性测试卡片控件
class AnimatedBoxControl : public UIElement {
public:
    AnimatedBoxControl() {
        m_widthScalar.Reset(160.0f);
        m_heightScalar.Reset(60.0f);
        m_radiusScalar.Reset(8.0f);
        m_opacityScalar.Reset(1.0f);
        m_offsetScalar.Reset(0.0f);
    }

    void ToggleExpanded() {
        m_expanded = !m_expanded;
        if (m_expanded) {
            m_widthScalar.SetTarget(360.0f);
            m_heightScalar.SetTarget(100.0f);
            m_radiusScalar.SetTarget(24.0f);
            m_opacityScalar.SetTarget(0.95f);
            m_offsetScalar.SetTarget(40.0f);
        } else {
            m_widthScalar.Reset(m_widthScalar.Current());
            m_heightScalar.Reset(m_heightScalar.Current());
            m_radiusScalar.Reset(m_radiusScalar.Current());
            m_opacityScalar.Reset(m_opacityScalar.Current());
            m_offsetScalar.Reset(m_offsetScalar.Current());

            m_widthScalar.SetTarget(160.0f);
            m_heightScalar.SetTarget(60.0f);
            m_radiusScalar.SetTarget(8.0f);
            m_opacityScalar.SetTarget(1.0f);
            m_offsetScalar.SetTarget(0.0f);
        }
        RequestAnimationTicks();
    }

    bool HasSelfAnimation() const override {
        return m_widthScalar.IsAnimating()
            || m_heightScalar.IsAnimating()
            || m_radiusScalar.IsAnimating()
            || m_opacityScalar.IsAnimating()
            || m_offsetScalar.IsAnimating();
    }

    bool OnAnimationTick() override {
        const float dt = GetAnimationDeltaSeconds();
        AnimationSpec spec{ 0.22f, 0.01f, 0.35f, EasingType::EaseOutCubic };
        const bool wMoving = m_widthScalar.Tick(dt, spec);
        const bool hMoving = m_heightScalar.Tick(dt, spec);
        const bool rMoving = m_radiusScalar.Tick(dt, spec);
        const bool oMoving = m_opacityScalar.Tick(dt, spec);
        const bool offMoving = m_offsetScalar.Tick(dt, spec);

        SetComposeOpacity(m_opacityScalar.Current());
        SetComposeOffset(m_offsetScalar.Current(), 0.0f);
        SetCornerRadius(m_radiusScalar.Current());
        InvalidateMeasure();
        MarkRenderContentDirty();

        return wMoving || hMoving || rMoving || oMoving || offMoving;
    }

    Size Measure(Size availableSize) override {
        (void)availableSize;
        return Size(m_widthScalar.Current(), m_heightScalar.Current());
    }

    void OnRender(GraphicsContext& ctx) override {
        const Rect rc = GetBounds();
        D2D1_COLOR_F bg = ThemeManager::Instance().GetColor(ThemeTokenId::AccentColor);
        bg.a = m_opacityScalar.Current();
        ctx.FillRoundedRect(rc, m_radiusScalar.Current(), bg);

        D2D1_COLOR_F border = ThemeManager::Instance().GetColor(ThemeTokenId::CardBorder);
        ctx.DrawRoundedRect(rc, m_radiusScalar.Current(), border, 1.5f);

        std::string txt = m_expanded ? "展开态：W360 H100 R24" : "初始态：W160 H60 R8";
        ctx.DrawText(
            txt,
            rc,
            D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f),
            "微软雅黑",
            13.0f,
            DWRITE_TEXT_ALIGNMENT_CENTER,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

private:
    bool m_expanded = false;
    AnimatedScalar m_widthScalar;
    AnimatedScalar m_heightScalar;
    AnimatedScalar m_radiusScalar;
    AnimatedScalar m_opacityScalar;
    AnimatedScalar m_offsetScalar;
};

// 弹簧物理振荡小球演示控件
class SpringBallControl : public UIElement {
public:
    SpringBallControl() {
        SetHeight(100.0f);
        m_currentX = 20.0f;
        m_targetX = 20.0f;
        m_velocity = 0.0f;
    }

    void TriggerImpulse(float target) {
        m_targetX = target;
        RequestAnimationTicks();
    }

    void SetStiffness(float k) { m_stiffness = k; }
    void SetDamping(float d) { m_damping = d; }

    bool HasSelfAnimation() const override {
        return std::abs(m_currentX - m_targetX) > 0.001f || std::abs(m_velocity) > 0.001f;
    }

    bool OnAnimationTick() override {
        const float dt = GetAnimationDeltaSeconds();
        const bool moving = AnimationService::StepSpring(
            m_currentX, m_velocity, m_targetX, dt, m_stiffness, m_damping, 0.001f);
        MarkRenderContentDirty();
        return moving;
    }

    void OnRender(GraphicsContext& ctx) override {
        const Rect rc = GetBounds();
        // 绘制导轨背景
        const Rect trackRc(rc.x + 10.0f, rc.y + rc.height * 0.5f - 2.0f, rc.width - 20.0f, 4.0f);
        ctx.FillRoundedRect(trackRc, 2.0f, ThemeManager::Instance().GetColor(ThemeTokenId::InputBorder));

        // 绘制目标参考线
        const float targetWorldX = rc.x + m_targetX;
        ctx.DrawLine(
            Point(targetWorldX, rc.y + 10.0f),
            Point(targetWorldX, rc.y + rc.height - 10.0f),
            ThemeManager::Instance().GetColor(ThemeTokenId::AccentColor),
            1.5f);

        // 绘制振荡小球
        const float ballX = rc.x + m_currentX;
        const float ballY = rc.y + rc.height * 0.5f;
        const Rect ballRc(ballX - 13.0f, ballY - 13.0f, 26.0f, 26.0f);
        ctx.FillRoundedRect(ballRc, 13.0f, ThemeManager::Instance().GetColor(ThemeTokenId::AccentColor));
        ctx.DrawRoundedRect(ballRc, 13.0f, ThemeManager::Instance().GetColor(ThemeTokenId::TextPrimary), 1.5f);
    }

private:
    float m_currentX = 20.0f;
    float m_targetX = 20.0f;
    float m_velocity = 0.0f;
    float m_stiffness = 180.0f;
    float m_damping = 18.0f;
};

// 跑道指示器
class RaceTrackIndicator : public UIElement {
public:
    RaceTrackIndicator(EasingType type, ThemeTokenId token)
        : m_type(type), m_token(token) {
        SetHeight(28.0f);
    }

    void StartRace(float durationSec) {
        m_fromX = m_currentX;
        m_targetX = (m_fromX > 200.0f) ? 20.0f : 420.0f;
        m_elapsed = 0.0f;
        m_duration = durationSec;
        m_running = true;
        RequestAnimationTicks();
    }

    void ResetPosition() {
        m_fromX = 20.0f;
        m_currentX = 20.0f;
        m_targetX = 20.0f;
        m_elapsed = 0.0f;
        m_running = false;
        MarkRenderContentDirty();
    }

    bool HasSelfAnimation() const override {
        return m_running;
    }

    bool OnAnimationTick() override {
        if (!m_running) return false;
        const float dt = GetAnimationDeltaSeconds();
        m_elapsed += dt;
        m_running = AnimationService::StepDuration(
            m_currentX, m_fromX, m_targetX, m_elapsed, m_duration, m_type);
        MarkRenderContentDirty();
        return m_running;
    }

    void OnRender(GraphicsContext& ctx) override {
        const Rect rc = GetBounds();
        const Rect trackRc(rc.x + 10.0f, rc.y + rc.height * 0.5f - 2.0f, rc.width - 20.0f, 4.0f);
        ctx.FillRoundedRect(trackRc, 2.0f, ThemeManager::Instance().GetColor(ThemeTokenId::InputBorder));

        const float blockW = 24.0f;
        const float blockH = 18.0f;
        const Rect blockRc(
            rc.x + m_currentX - blockW * 0.5f,
            rc.y + rc.height * 0.5f - blockH * 0.5f,
            blockW,
            blockH);
        ctx.FillRoundedRect(blockRc, 4.0f, ThemeManager::Instance().GetColor(m_token));
        ctx.DrawRoundedRect(blockRc, 4.0f, ThemeManager::Instance().GetColor(ThemeTokenId::CardBorder), 1.0f);
    }

private:
    EasingType m_type;
    ThemeTokenId m_token;
    float m_fromX = 20.0f;
    float m_currentX = 20.0f;
    float m_targetX = 20.0f;
    float m_elapsed = 0.0f;
    float m_duration = 0.8f;
    bool m_running = false;
};

} // namespace

Element BuildAnimationPage() {
    // 1. 全局动画开关
    const bool initialEnabled = AnimationService::Instance().AreAnimationsEnabled();
    auto toggleAnim = ToggleSwitchTile("启用全局动效 (Animations Enabled)", initialEnabled);
    auto statusGlobal = MakeStatus(initialEnabled ? "当前状态：全局动画已开启（流畅过渡）" : "当前状态：全局动画已禁用（即刻吸附）");

    toggleAnim->OnToggled().Connect([statusGlobal](ToggleSwitch*, bool on) {
        AnimationService::Instance().SetAnimationsEnabled(on);
        statusGlobal->Text = on ? "当前状态：全局动画已开启（流畅过渡）" : "当前状态：全局动画已禁用（即刻吸附）";
    });

    // 2. 多轨道缓动函数竞速
    std::vector<std::pair<std::string, EasingType>> easings = {
        { "Linear (匀速线性)", EasingType::Linear },
        { "EaseInQuad (二次加速)", EasingType::EaseInQuad },
        { "EaseOutQuad (二次减速)", EasingType::EaseOutQuad },
        { "EaseInOutQuad (对称二次)", EasingType::EaseInOutQuad },
        { "EaseOutCubic (Fluent 标准)", EasingType::EaseOutCubic },
        { "EaseOutExpo (极速缓出)", EasingType::EaseOutExpo },
        { "EaseOutBack (过冲回弹)", EasingType::EaseOutBack },
        { "EaseOutBounce (物理弹跳)", EasingType::EaseOutBounce },
        { "Spring (欠阻尼弹簧)", EasingType::Spring },
    };

    std::vector<std::shared_ptr<RaceTrackIndicator>> tracks;
    auto tracksColumn = Column(8.0f);

    ThemeTokenId tokens[] = {
        ThemeTokenId::AccentColor,
        ThemeTokenId::TextPrimary,
        ThemeTokenId::FocusedBorder,
        ThemeTokenId::SelectedBackground,
        ThemeTokenId::DangerColor,
        ThemeTokenId::AccentForeground,
        ThemeTokenId::HoverBackground,
        ThemeTokenId::AccentColor,
        ThemeTokenId::TextPrimary
    };

    int colorIdx = 0;
    for (const auto& item : easings) {
        auto ind = std::make_shared<RaceTrackIndicator>(item.second, tokens[colorIdx % 9]);
        ind->Width = 460.0f;
        tracks.push_back(ind);

        auto lbl = MakeLabel(item.first, 12.0f, ThemeTokenId::TextPrimary, false);
        lbl->Width = 170.0f;

        tracksColumn->AddChild(Row(8, { lbl, ind }).Build());
        colorIdx++;
    }

    auto btnPlayAll = Button("全曲线同时竞速 (Play All)")
        .OnClick([tracks](UIElement*) {
            for (auto& t : tracks) {
                t->StartRace(0.9f);
            }
        });

    auto btnReset = Button("重置位置 (Reset)")
        .BackgroundToken(ThemeTokenId::CardBackground)
        .ColorToken(ThemeTokenId::TextPrimary)
        .BorderToken(ThemeTokenId::CardBorder)
        .BorderThickness(1.0f)
        .OnClick([tracks](UIElement*) {
            for (auto& t : tracks) {
                t->ResetPosition();
            }
        });

    // 3. 复合属性隐式平滑过渡
    auto animatedBox = std::make_shared<AnimatedBoxControl>();
    auto btnTriggerBox = Button("触发多属性平滑变形")
        .OnClick([animatedBox](UIElement*) {
            animatedBox->ToggleExpanded();
        });

    // 4. 物理弹簧振荡器
    auto springBall = std::make_shared<SpringBallControl>();
    springBall->Width = 460.0f;

    auto btnSpringLeft = Button("移至左侧 (20px)")
        .BackgroundToken(ThemeTokenId::CardBackground)
        .ColorToken(ThemeTokenId::TextPrimary)
        .BorderToken(ThemeTokenId::CardBorder)
        .BorderThickness(1.0f)
        .OnClick([springBall](UIElement*) { springBall->TriggerImpulse(20.0f); });

    auto btnSpringMid = Button("冲击中位 (230px)")
        .OnClick([springBall](UIElement*) { springBall->TriggerImpulse(230.0f); });

    auto btnSpringRight = Button("移至右侧 (420px)")
        .BackgroundToken(ThemeTokenId::CardBackground)
        .ColorToken(ThemeTokenId::TextPrimary)
        .BorderToken(ThemeTokenId::CardBorder)
        .BorderThickness(1.0f)
        .OnClick([springBall](UIElement*) { springBall->TriggerImpulse(420.0f); });

    auto sliderStiffness = SliderWidget(180.0f, 40.0f, 400.0f);
    sliderStiffness->Width = 180.0f;

    auto sliderDamping = SliderWidget(18.0f, 4.0f, 50.0f);
    sliderDamping->Width = 180.0f;

    auto statusStiffness = MakeStatus(std::format("刚度 (Stiffness): {:.0f}", 180.0f));
    auto statusDamping = MakeStatus(std::format("阻尼 (Damping): {:.1f}", 18.0f));

    sliderStiffness->OnValueChanged().Connect([springBall, statusStiffness](Slider*, float k) {
        springBall->SetStiffness(k);
        statusStiffness->Text = std::format("刚度 (Stiffness): {:.0f}", k);
    });

    sliderDamping->OnValueChanged().Connect([springBall, statusDamping](Slider*, float d) {
        springBall->SetDamping(d);
        statusDamping->Text = std::format("阻尼 (Damping): {:.1f}", d);
    });

    SamplePageSpec spec;
    spec.title = "Implicit Animations (隐式动效 & 缓动算法)";
    spec.subtitle = "基于全局中央动画服务（AnimationService）的集中时钟驱动、全套现代缓动曲线与欠阻尼物理弹簧振荡算法展示。";
    spec.sections = {
        {
            "全局动画策略控制",
            "通过 AnimationService::Instance().SetAnimationsEnabled 控制全局动效开关。关闭时将立即可视吸附，兼顾无障碍设定与节能场景。",
            Column(12, { toggleAnim, statusGlobal }),
        },
        {
            "多曲线缓动函数竞速全景 (Easing Curves Race)",
            "点击“全曲线同时竞速”，观察不同数学插值算法在相同时间周期内产生的加速度分布、过冲与回弹特征。",
            Column(12, {
                Row(12, { btnPlayAll, btnReset }),
                tracksColumn,
            }),
        },
        {
            "复合属性隐式平滑过渡 (Multi-Property Transitions)",
            "由 AnimatedScalar 与 AnimationService::Step 联合驱动。点击按钮观察尺寸（W/H）、圆角（CornerRadius）、透明度（Opacity）和横向位移（Offset）的协同平滑变形。",
            Column(14, {
                btnTriggerBox,
                animatedBox,
            }),
        },
        {
            "物理弹簧振荡模拟器 (Spring Oscillator)",
            "基于欠阻尼二阶物理振荡动力学模型 (StepSpring)。可自由调节刚度 (Stiffness) 与阻尼系数 (Damping)，实时观察自然的物理弹性回弹与收敛效果。",
            Column(14, {
                Row(12, { btnSpringLeft, btnSpringMid, btnSpringRight }),
                springBall,
                Row(20, {
                    Column(6, { statusStiffness, sliderStiffness }),
                    Column(6, { statusDamping, sliderDamping }),
                }),
            }),
        },
    };
    spec.source =
        "// 1. 全局动画开关与状态控制\n"
        "AnimationService::Instance().SetAnimationsEnabled(true);\n\n"
        "// 2. 标量动画过渡 (AnimatedScalar)\n"
        "AnimatedScalar widthScalar(160.0f);\n"
        "widthScalar.SetTarget(360.0f);\n"
        "AnimationSpec spec{ 0.22f, 0.01f, 0.35f, EasingType::EaseOutCubic };\n"
        "widthScalar.Tick(dt, spec);\n\n"
        "// 3. 欠阻尼物理弹簧步进 (StepSpring)\n"
        "float current = 20.0f, velocity = 0.0f, target = 230.0f;\n"
        "AnimationService::StepSpring(current, velocity, target, dt, 180.0f, 18.0f);\n";

    return BuildSamplePage(spec);
}

} // namespace Gallery
