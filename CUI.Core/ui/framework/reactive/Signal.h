#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace CUI {
namespace Reactive {

class SignalNodeBase;

/**
 * @brief 响应式依赖监听者接口（Subscriber）。
 * 任何依赖 Signal 的节点（如 Computed 或 Effect）必须实现该接口。
 */
class ISubscriber {
public:
    virtual ~ISubscriber() = default;

    /// 当所依赖的任意 Signal 值发生变化时触发
    virtual void OnDependencyChanged() = 0;

    /// 建立双向依赖映射
    virtual void AddDependency(SignalNodeBase* source) = 0;
};

/**
 * @brief 响应式上下文管理器（线程局部单例）。
 * 管理当前的监听者栈（用于自动依赖收集）与批处理事务（Batching）。
 */
class ReactiveContext {
public:
    static void PushSubscriber(ISubscriber* sub) {
        GetSubscriberStack().push_back(sub);
    }

    static void PopSubscriber() {
        auto& stack = GetSubscriberStack();
        if (!stack.empty()) {
            stack.pop_back();
        }
    }

    static ISubscriber* CurrentSubscriber() {
        const auto& stack = GetSubscriberStack();
        return stack.empty() ? nullptr : stack.back();
    }

    static void ScheduleNotification(ISubscriber* sub) {
        if (!sub) return;
        if (GetBatchDepth() > 0) {
            auto& pending = GetPendingSubscribers();
            if (std::find(pending.begin(), pending.end(), sub) == pending.end()) {
                pending.push_back(sub);
            }
        } else {
            sub->OnDependencyChanged();
        }
    }

    template<typename Func>
    static void Batch(Func&& fn) {
        ++GetBatchDepth();
        try {
            fn();
        } catch (...) {
            --GetBatchDepth();
            throw;
        }
        --GetBatchDepth();

        if (GetBatchDepth() == 0) {
            auto pending = std::move(GetPendingSubscribers());
            GetPendingSubscribers().clear();
            for (ISubscriber* sub : pending) {
                if (sub) {
                    sub->OnDependencyChanged();
                }
            }
        }
    }

private:
    static std::vector<ISubscriber*>& GetSubscriberStack() {
        thread_local std::vector<ISubscriber*> s_stack;
        return s_stack;
    }

    static int& GetBatchDepth() {
        thread_local int s_batchDepth = 0;
        return s_batchDepth;
    }

    static std::vector<ISubscriber*>& GetPendingSubscribers() {
        thread_local std::vector<ISubscriber*> s_pending;
        return s_pending;
    }
};

/**
 * @brief 信号节点抽象基类。维护对此信号建立依赖的所有下游订阅者。
 */
class SignalNodeBase {
public:
    virtual ~SignalNodeBase() {
        // 节点析构时通知所有下游解除依赖
        m_subscribers.clear();
    }

    void AddSubscriber(ISubscriber* sub) {
        if (!sub) return;
        if (std::find(m_subscribers.begin(), m_subscribers.end(), sub) == m_subscribers.end()) {
            m_subscribers.push_back(sub);
        }
    }

    void RemoveSubscriber(ISubscriber* sub) {
        m_subscribers.erase(
            std::remove(m_subscribers.begin(), m_subscribers.end(), sub),
            m_subscribers.end()
        );
    }

    void NotifySubscribers() {
        // 拷贝订阅者副本，防止回调内修改依赖链导致迭代器失效
        std::vector<ISubscriber*> subs = m_subscribers;
        for (ISubscriber* sub : subs) {
            ReactiveContext::ScheduleNotification(sub);
        }
    }

protected:
    void TrackDependency() {
        if (ISubscriber* current = ReactiveContext::CurrentSubscriber()) {
            AddSubscriber(current);
            current->AddDependency(this);
        }
    }

private:
    std::vector<ISubscriber*> m_subscribers;
};

namespace detail {

template<typename T>
concept EqualityComparable = requires(const T& a, const T& b) {
    { a == b } -> std::convertible_to<bool>;
};

template<typename T>
bool ValueEquals(const T& a, const T& b) {
    if constexpr (EqualityComparable<T>) {
        return a == b;
    } else {
        return false;
    }
}

template<typename T>
class SignalNode final : public SignalNodeBase {
public:
    explicit SignalNode(T initialValue) : m_value(std::move(initialValue)) {}

    const T& Get() {
        TrackDependency();
        return m_value;
    }

    const T& Peek() const {
        return m_value;
    }

    bool Set(T newValue) {
        if (ValueEquals(m_value, newValue)) {
            return false;
        }
        m_value = std::move(newValue);
        NotifySubscribers();
        return true;
    }

private:
    T m_value;
};

template<typename T>
class ComputedNode final : public SignalNodeBase, public ISubscriber, public std::enable_shared_from_this<ComputedNode<T>> {
public:
    explicit ComputedNode(std::function<T()> compute)
        : m_compute(std::move(compute)), m_dirty(true) {}

    virtual ~ComputedNode() override {
        ClearDependencies();
    }

    const T& Get() {
        if (m_dirty) {
            Recompute();
        }
        TrackDependency();
        return m_cachedValue;
    }

    const T& Peek() const {
        return m_cachedValue;
    }

    virtual void OnDependencyChanged() override {
        if (!m_dirty) {
            m_dirty = true;
            // 向上通知所有依赖该计算值的下游订阅者进行级联失效
            NotifySubscribers();
        }
    }

    virtual void AddDependency(SignalNodeBase* source) override {
        if (source && std::find(m_dependencies.begin(), m_dependencies.end(), source) == m_dependencies.end()) {
            m_dependencies.push_back(source);
        }
    }

private:
    void ClearDependencies() {
        for (SignalNodeBase* dep : m_dependencies) {
            if (dep) {
                dep->RemoveSubscriber(this);
            }
        }
        m_dependencies.clear();
    }

    void Recompute() {
        ClearDependencies();
        ReactiveContext::PushSubscriber(this);
        try {
            m_cachedValue = m_compute();
        } catch (...) {
            ReactiveContext::PopSubscriber();
            throw;
        }
        ReactiveContext::PopSubscriber();
        m_dirty = false;
    }

    std::function<T()> m_compute;
    T m_cachedValue{};
    bool m_dirty = true;
    std::vector<SignalNodeBase*> m_dependencies;
};

class EffectNode final : public ISubscriber, public std::enable_shared_from_this<EffectNode> {
public:
    explicit EffectNode(std::function<void()> effect)
        : m_effect(std::move(effect)), m_disposed(false), m_running(false) {}

    virtual ~EffectNode() override {
        Dispose();
    }

    void Run() {
        if (m_disposed || m_running || !m_effect) {
            return;
        }
        m_running = true;
        ClearDependencies();
        ReactiveContext::PushSubscriber(this);
        try {
            m_effect();
        } catch (...) {
            ReactiveContext::PopSubscriber();
            m_running = false;
            throw;
        }
        ReactiveContext::PopSubscriber();
        m_running = false;
    }

    void Dispose() {
        if (m_disposed) return;
        m_disposed = true;
        ClearDependencies();
        m_effect = nullptr;
    }

    virtual void OnDependencyChanged() override {
        if (!m_disposed) {
            Run();
        }
    }

    virtual void AddDependency(SignalNodeBase* source) override {
        if (source && std::find(m_dependencies.begin(), m_dependencies.end(), source) == m_dependencies.end()) {
            m_dependencies.push_back(source);
        }
    }

private:
    void ClearDependencies() {
        for (SignalNodeBase* dep : m_dependencies) {
            if (dep) {
                dep->RemoveSubscriber(this);
            }
        }
        m_dependencies.clear();
    }

    std::function<void()> m_effect;
    bool m_disposed = false;
    bool m_running = false;
    std::vector<SignalNodeBase*> m_dependencies;
};

} // namespace detail

/**
 * @brief 细粒度响应式原子状态（Signal）。
 * 支持值语义轻量传递与自动依赖收集。读取时自动向当前计算上下文（Computed/Effect）建立订阅；
 * 写入时仅触发精确下游节点的增量重算。
 */
template<typename T>
class Signal {
public:
    Signal() : m_node(std::make_shared<detail::SignalNode<T>>(T{})) {}
    explicit Signal(T initialValue) : m_node(std::make_shared<detail::SignalNode<T>>(std::move(initialValue))) {}

    /// 读取当前值并自动注册依赖
    const T& Get() const { return m_node->Get(); }

    /// 读取当前值（语法糖）
    const T& operator()() const { return Get(); }

    /// 读取当前值但不建立响应式依赖追踪
    const T& Peek() const { return m_node->Peek(); }

    /// 写入新值并触发所有下游订阅者更新
    bool Set(T value) const { return m_node->Set(std::move(value)); }

    /// 赋值运算符语法糖
    Signal& operator=(T value) {
        Set(std::move(value));
        return *this;
    }

    const Signal& operator=(T value) const {
        Set(std::move(value));
        return *this;
    }

    /// 隐式类型转换提取当前值
    operator const T&() const { return Get(); }

    /// 原地更新值并触发变更
    template<typename Func>
    void Update(Func&& fn) const {
        T val = m_node->Peek();
        fn(val);
        Set(std::move(val));
    }

    bool operator==(const Signal& other) const { return m_node == other.m_node; }
    bool operator!=(const Signal& other) const { return m_node != other.m_node; }

private:
    std::shared_ptr<detail::SignalNode<T>> m_node;
};

template<typename T>
Signal(T) -> Signal<T>;

/**
 * @brief 衍生计算值（Computed）。
 * 自动收集求值表达式内访问的所有 Signal 依赖，采用惰性求值与结果缓存机制。
 */
template<typename T>
class Computed {
public:
    explicit Computed(std::function<T()> compute)
        : m_node(std::make_shared<detail::ComputedNode<T>>(std::move(compute))) {}

    const T& Get() const { return m_node->Get(); }
    const T& operator()() const { return Get(); }
    const T& Peek() const { return m_node->Peek(); }
    operator const T&() const { return Get(); }

private:
    std::shared_ptr<detail::ComputedNode<T>> m_node;
};

template<typename Func>
Computed(Func) -> Computed<std::invoke_result_t<Func>>;

/**
 * @brief 副作用执行器（Effect）。
 * 在创建时自动执行一次并收集依赖，后续当依赖的任何 Signal 变动时自动重新运行。
 */
class Effect {
public:
    Effect() = default;
    explicit Effect(std::function<void()> action)
        : m_node(std::make_shared<detail::EffectNode>(std::move(action))) {
        if (m_node) {
            m_node->Run();
        }
    }

    void Dispose() {
        if (m_node) {
            m_node->Dispose();
            m_node.reset();
        }
    }

    bool IsActive() const { return m_node != nullptr; }

private:
    std::shared_ptr<detail::EffectNode> m_node;
};

/**
 * @brief 批处理事务包装器（Batch）。
 * 将函数内所有的 Signal 写操作合并为一次通知，避免中间过程产生重复重绘。
 */
template<typename Func>
inline void Batch(Func&& fn) {
    ReactiveContext::Batch(std::forward<Func>(fn));
}

} // namespace Reactive

// 导出至 CUI 顶级命名空间以方便用户直接使用
using Reactive::Signal;
using Reactive::Computed;
using Reactive::Effect;
using Reactive::Batch;

} // namespace CUI
