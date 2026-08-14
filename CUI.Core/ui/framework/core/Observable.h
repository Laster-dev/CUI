#pragma once

#include "Event.h"

#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace CUI {

template<typename T>
class Observable {
public:
    Observable() = default;
    explicit Observable(T value) : m_value(std::move(value)) {}

    const T& Get() const { return m_value; }

    void Set(T value) {
        m_value = std::move(value);
        m_changed.Invoke(m_value);
    }

    Observable& operator=(T value) {
        Set(std::move(value));
        return *this;
    }

    Event<const T&>& OnChanged() { return m_changed; }

private:
    T m_value{};
    Event<const T&> m_changed;
};

template<typename T>
class ComputedObservable final : public Observable<T> {
public:
    ~ComputedObservable() {
        for (const auto& dispose : m_disposers) {
            dispose();
        }
    }

    void AddDisposer(std::function<void()> dispose) {
        m_disposers.push_back(std::move(dispose));
    }

private:
    std::vector<std::function<void()>> m_disposers;
};

template<typename T, typename Compute, typename... Sources>
std::shared_ptr<ComputedObservable<T>> MakeComputed(Compute compute,
                                                     const std::shared_ptr<Observable<Sources>>&... sources) {
    auto result = std::make_shared<ComputedObservable<T>>();
    auto update = [weakResult = std::weak_ptr<ComputedObservable<T>>(result), compute, sources...]() mutable {
        if (auto target = weakResult.lock()) {
            target->Set(compute(sources->Get()...));
        }
    };

    auto connect = [&result, &update](const auto& source) {
        const EventId connection = source->OnChanged().Connect([update](const auto&) mutable {
            update();
        });
        result->AddDisposer([source, connection]() {
            source->OnChanged().Disconnect(connection);
        });
    };
    (connect(sources), ...);
    update();
    return result;
}

} // namespace CUI
