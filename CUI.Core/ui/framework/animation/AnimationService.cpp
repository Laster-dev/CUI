#include "AnimationService.h"
#include "../controls/UIElement.h"
#include "../animation/FrameScheduler.h"
#include <algorithm>
#include <cmath>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace CUI {

namespace {

constexpr float kPi = 3.14159265358979323846f;

float EaseInBackImpl(float t) {
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.0f;
    return c3 * t * t * t - c1 * t * t;
}

float EaseOutBackImpl(float t) {
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.0f;
    const float inv = t - 1.0f;
    return 1.0f + c3 * inv * inv * inv + c1 * inv * inv;
}

float EaseOutBounceImpl(float t) {
    constexpr float n1 = 7.5625f;
    constexpr float d1 = 2.75f;
    if (t < 1.0f / d1) {
        return n1 * t * t;
    } else if (t < 2.0f / d1) {
        t -= 1.5f / d1;
        return n1 * t * t + 0.75f;
    } else if (t < 2.5f / d1) {
        t -= 2.25f / d1;
        return n1 * t * t + 0.9375f;
    } else {
        t -= 2.625f / d1;
        return n1 * t * t + 0.984375f;
    }
}

} // namespace

AnimationService& AnimationService::Instance() {
    static AnimationService s_instance;
    return s_instance;
}

AnimationService::AnimationService() {
    // 检查 Windows 系统的“在 Windows 中显示动画”无障碍全局偏好设定
    BOOL clientAnimations = TRUE;
    if (SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &clientAnimations, 0)) {
        m_animationsEnabled = (clientAnimations != FALSE);
    }
}

bool AnimationService::AreAnimationsEnabled() const {
    return m_animationsEnabled;
}

void AnimationService::SetAnimationsEnabled(bool enabled) {
    m_animationsEnabled = enabled;
}

void AnimationService::WakeFrame() {
    if (m_frameWakeCallback) {
        m_frameWakeCallback();
    }
    if (FrameScheduler* sched = FrameScheduler::Current()) {
        sched->ScheduleFrame();
    }
}

float AnimationService::Ease(EasingType type, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    switch (type) {
    case EasingType::Linear:
        return t;

    case EasingType::EaseInQuad:
        return t * t;

    case EasingType::EaseOutQuad:
        return 1.0f - (1.0f - t) * (1.0f - t);

    case EasingType::EaseInOutQuad:
        return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;

    case EasingType::EaseInCubic:
        return t * t * t;

    case EasingType::EaseOutCubic: {
        const float inv = 1.0f - t;
        return 1.0f - inv * inv * inv;
    }

    case EasingType::EaseInOutCubic:
        return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;

    case EasingType::EaseInExpo:
        return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * t - 10.0f);

    case EasingType::EaseOutExpo:
        return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);

    case EasingType::EaseInOutExpo:
        if (t == 0.0f) return 0.0f;
        if (t == 1.0f) return 1.0f;
        return t < 0.5f ? std::pow(2.0f, 20.0f * t - 10.0f) / 2.0f : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;

    case EasingType::EaseInBack:
        return EaseInBackImpl(t);

    case EasingType::EaseOutBack:
        return EaseOutBackImpl(t);

    case EasingType::EaseInOutBack: {
        constexpr float c1 = 1.70158f;
        constexpr float c2 = c1 * 1.525f;
        return t < 0.5f
            ? (std::pow(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f
            : (std::pow(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f;
    }

    case EasingType::EaseOutBounce:
        return EaseOutBounceImpl(t);

    case EasingType::Spring: {
        // 欠阻尼弹簧衰减解析模型
        return 1.0f - std::exp(-6.0f * t) * std::cos(10.0f * t);
    }

    default:
        return t;
    }
}

float AnimationService::BlendFactor(float responseAt60Hz, float dtSeconds) {
    responseAt60Hz = std::clamp(responseAt60Hz, 0.0f, 0.999f);
    dtSeconds = std::clamp(dtSeconds, 1.0f / 240.0f, 0.050f);
    const float oneMinus = 1.0f - responseAt60Hz;
    const float k = -60.0f * std::log((std::max)(oneMinus, 1.0e-4f));
    return 1.0f - std::exp(-k * dtSeconds);
}

bool AnimationService::Step(float& current, float target, float dtSeconds, const AnimationSpec& spec) {
    if (!Instance().AreAnimationsEnabled()) {
        const bool changed = std::abs(target - current) > 0.0001f;
        current = target;
        return changed;
    }

    return Step(current, target, dtSeconds, spec.responseAt60Hz, spec.epsilon);
}

bool AnimationService::Step(float& current, float target, float dtSeconds, float responseAt60Hz, float epsilon) {
    if (!Instance().AreAnimationsEnabled()) {
        const bool changed = std::abs(target - current) > 0.0001f;
        current = target;
        return changed;
    }

    const float previous = current;
    const float delta = target - current;
    if (std::abs(delta) <= epsilon) {
        current = target;
        return std::abs(current - previous) > 0.0001f;
    }

    current += delta * BlendFactor(responseAt60Hz, dtSeconds);
    if (std::abs(target - current) <= epsilon * 1.5f) {
        current = target;
    }
    return std::abs(current - previous) > 0.0001f;
}

bool AnimationService::StepDuration(
    float& current, float from, float target, float elapsed, float durationSeconds, EasingType easing) {
    if (!Instance().AreAnimationsEnabled() || durationSeconds <= 0.0001f) {
        current = target;
        return false;
    }

    const float t = std::clamp(elapsed / durationSeconds, 0.0f, 1.0f);
    const float factor = Ease(easing, t);
    current = from + (target - from) * factor;

    if (t >= 1.0f) {
        current = target;
        return false;
    }
    return true;
}

bool AnimationService::StepSpring(
    float& current, float& velocity, float target, float dtSeconds, float stiffness, float damping, float epsilon) {
    if (!Instance().AreAnimationsEnabled()) {
        current = target;
        velocity = 0.0f;
        return false;
    }

    dtSeconds = std::clamp(dtSeconds, 1.0f / 240.0f, 0.05f);
    const float displacement = current - target;
    const float springForce = -stiffness * displacement;
    const float dampingForce = -damping * velocity;
    const float acceleration = springForce + dampingForce;

    velocity += acceleration * dtSeconds;
    current += velocity * dtSeconds;

    if (std::abs(current - target) <= epsilon && std::abs(velocity) <= epsilon) {
        current = target;
        velocity = 0.0f;
        return false;
    }
    return true;
}

void AnimationService::RegisterElement(UIElement* element) {
    if (!element) return;
    if (std::find(m_registeredElements.begin(), m_registeredElements.end(), element) == m_registeredElements.end()) {
        m_registeredElements.push_back(element);
    }
}

void AnimationService::UnregisterElement(UIElement* element) {
    if (!element) return;
    m_registeredElements.erase(
        std::remove(m_registeredElements.begin(), m_registeredElements.end(), element),
        m_registeredElements.end());
    m_animatingElements.erase(
        std::remove(m_animatingElements.begin(), m_animatingElements.end(), element),
        m_animatingElements.end());
}

void AnimationService::RequestAnimationTicks(UIElement* element) {
    if (!element) return;
    RegisterElement(element);
    if (std::find(m_animatingElements.begin(), m_animatingElements.end(), element) == m_animatingElements.end()) {
        m_animatingElements.push_back(element);
    }
    WakeFrame();
}

void AnimationService::CancelAnimationTicks(UIElement* element) {
    if (!element) return;
    m_animatingElements.erase(
        std::remove(m_animatingElements.begin(), m_animatingElements.end(), element),
        m_animatingElements.end());
}

bool AnimationService::IsRegistered(const UIElement* element) const {
    if (!element) return false;
    return std::find(m_registeredElements.begin(), m_registeredElements.end(), element) != m_registeredElements.end();
}

bool AnimationService::IsAnimating(const UIElement* element) const {
    if (!element) return false;
    return std::find(m_animatingElements.begin(), m_animatingElements.end(), element) != m_animatingElements.end();
}

bool AnimationService::Tick(clock::time_point now) {
    if (!m_hasLastFrameTime) {
        m_deltaSeconds = 1.0f / 60.0f;
        m_lastFrameTime = now;
        m_hasLastFrameTime = true;
    } else {
        m_deltaSeconds = std::chrono::duration<float>(now - m_lastFrameTime).count();
        m_deltaSeconds = std::clamp(m_deltaSeconds, 1.0f / 240.0f, 0.05f);
        m_lastFrameTime = now;
    }

    UIElement::SetAnimationDeltaSeconds(m_deltaSeconds);

    if (m_animatingElements.empty()) {
        return false;
    }

    // 拍下快照进行安全迭代
    std::vector<UIElement*> animSnapshot = m_animatingElements;
    std::vector<UIElement*> stillAnimating;

    for (UIElement* el : animSnapshot) {
        if (!el) continue;
        // 如果控件已不可视或已销毁，自动停止
        if (el->GetVisibility() != Visibility::Visible) {
            continue;
        }
        const bool keepGoing = el->OnAnimationTick();
        if (keepGoing) {
            stillAnimating.push_back(el);
        }
    }

    m_animatingElements = std::move(stillAnimating);
    return !m_animatingElements.empty();
}

} // namespace CUI
