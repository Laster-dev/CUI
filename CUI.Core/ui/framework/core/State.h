#pragma once

#include "Observable.h"

#include <memory>
#include <utility>

namespace CUI {

template<typename T>
class State final {
public:
    State() : State(T{}) {}
    State(T value) : m_observable(std::make_shared<Observable<T>>(std::move(value))) {}

    const T& Get() const { return m_observable->Get(); }
    void Set(T value) const { m_observable->Set(std::move(value)); }

    State& operator=(T value) {
        Set(std::move(value));
        return *this;
    }

    const State& operator=(T value) const {
        Set(std::move(value));
        return *this;
    }

    operator const T&() const { return Get(); }

    Event<const T&>& OnChanged() const { return m_observable->OnChanged(); }
    const std::shared_ptr<Observable<T>>& Share() const { return m_observable; }

private:
    std::shared_ptr<Observable<T>> m_observable;
};

template<typename T>
State(T) -> State<T>;

template<typename T, typename Compute, typename... Sources>
std::shared_ptr<ComputedObservable<T>> MakeComputed(Compute compute, const State<Sources>&... sources) {
    return MakeComputed<T>(std::move(compute), sources.Share()...);
}

} // namespace CUI
