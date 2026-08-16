#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <vector>

namespace CUI {

class UIElement;

/**
 * @brief 动画缓动插值函数类型。
 */
enum class EasingType {
    Linear,         // 线性匀速
    EaseInQuad,     // 二次加速缓入
    EaseOutQuad,    // 二次减速缓出
    EaseInOutQuad,  // 二次对称缓入缓出
    EaseInCubic,    // 三次强力加速缓入
    EaseOutCubic,   // 三次强力减速缓出 (Fluent UI / CSS 推荐标准)
    EaseInOutCubic, // 三次强力对称缓入缓出
    EaseInExpo,     // 指数极速加速缓入
    EaseOutExpo,    // 指数极速减速缓出
    EaseInOutExpo,  // 指数极速对称缓入缓出
    EaseInBack,     // 回弹蓄力缓入
    EaseOutBack,    // 过冲回弹缓出
    EaseInOutBack,  // 双向回弹缓入缓出
    EaseOutBounce,  // 物理弹跳跌落缓出
    Spring          // 弹簧阻尼震荡模型
};

/**
 * @brief 动画动态配置参数规范。
 */
struct AnimationSpec {
    float responseAt60Hz = 0.20f;
    float epsilon = 0.01f;
    // 0 = 无固定总时长（经典指数逼近平滑收敛）。>0 为固定总时长（秒），超时自动吸附到目标值。
    float maxDurationSeconds = 0.0f;
    EasingType easing = EasingType::EaseOutCubic;
};

/**
 * @brief 弹出层显隐动效预设规范（WinUI 3 / Fluent 风格 220ms 缓出）。
 */
struct PopupReveal {
    static constexpr AnimationSpec kSpec{ 0.20f, 0.005f, 0.22f, EasingType::EaseOutCubic };
};

/**
 * @brief 全局独立动画服务（AnimationService）。
 * 统一管理所有控件的动画算法、缓动计算、时钟调度与全局动画策略。
 * 所有控件在变为可视（Visible）时自动注册，不可视时自动注销，统一向本服务请求算法步进与调度。
 */
class AnimationService {
public:
    using clock = std::chrono::steady_clock;

    static AnimationService& Instance();

    // ==========================================
    // 1. 全局动画策略与开关
    // ==========================================
    bool AreAnimationsEnabled() const;
    void SetAnimationsEnabled(bool enabled);

    float GetDeltaSeconds() const { return m_deltaSeconds; }
    void SetDeltaSeconds(float dt) { m_deltaSeconds = std::clamp(dt, 1.0f / 240.0f, 0.05f); }

    // ==========================================
    // 2. 动画数学算法库与步进器 (Animation Math & Easing Suite)
    // ==========================================
    static float Ease(EasingType type, float t);
    static float BlendFactor(float responseAt60Hz, float dtSeconds);

    static bool Step(float& current, float target, float dtSeconds, const AnimationSpec& spec);
    static bool Step(float& current, float target, float dtSeconds, float responseAt60Hz, float epsilon = 0.01f);
    static bool StepDuration(float& current, float from, float target, float elapsed, float durationSeconds, EasingType easing = EasingType::EaseOutCubic);
    static bool StepSpring(float& current, float& velocity, float target, float dtSeconds, float stiffness = 180.0f, float damping = 24.0f, float epsilon = 0.001f);

    // ==========================================
    // 3. 控件自动注册与集中调度 (Central Dispatch & Auto-Lifecycle)
    // ==========================================
    void RegisterElement(UIElement* element);
    void UnregisterElement(UIElement* element);

    void RequestAnimationTicks(UIElement* element);
    void CancelAnimationTicks(UIElement* element);

    bool IsRegistered(const UIElement* element) const;
    bool IsAnimating(const UIElement* element) const;

    bool HasAnimating() const { return !m_animatingElements.empty(); }

    // 驱动所有当前活跃的动画控件进行 OnAnimationTick，返回是否仍有控件在动画中
    bool Tick(clock::time_point now);

    // 唤醒帧调度回调注册（由 Window 系统接入）
    void SetFrameWakeCallback(std::function<void()> callback) { m_frameWakeCallback = std::move(callback); }
    void WakeFrame();

private:
    AnimationService();
    ~AnimationService() = default;

    AnimationService(const AnimationService&) = delete;
    AnimationService& operator=(const AnimationService&) = delete;

    bool m_animationsEnabled = true;
    float m_deltaSeconds = 1.0f / 60.0f;
    clock::time_point m_lastFrameTime{};
    bool m_hasLastFrameTime = false;

    std::vector<UIElement*> m_registeredElements; // 所有当前处于 Visible 可视树中的控件
    std::vector<UIElement*> m_animatingElements;  // 正在请求逐帧时钟 Tick 的控件
    std::function<void()> m_frameWakeCallback;    // 帧唤醒回调
};

} // namespace CUI
