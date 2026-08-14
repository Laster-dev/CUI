#pragma once

#include "Observable.h"

#include <functional>
#include <memory>
#include <optional>
#include <utility>

namespace CUI {

template<typename TSource, typename TTarget>
class IValueConverter {
public:
    virtual ~IValueConverter() = default;
    virtual TTarget Convert(const TSource& value) const = 0;
    virtual std::optional<TSource> ConvertBack(const TTarget&) const { return std::nullopt; }
};

template<typename TSource, typename TTarget>
class FunctionValueConverter final : public IValueConverter<TSource, TTarget> {
public:
    using ConvertFunction = std::function<TTarget(const TSource&)>;
    using ConvertBackFunction = std::function<std::optional<TSource>(const TTarget&)>;

    FunctionValueConverter(ConvertFunction convert, ConvertBackFunction convertBack = {})
        : m_convert(std::move(convert)), m_convertBack(std::move(convertBack)) {}

    TTarget Convert(const TSource& value) const override { return m_convert(value); }
    std::optional<TSource> ConvertBack(const TTarget& value) const override {
        return m_convertBack ? m_convertBack(value) : std::nullopt;
    }

private:
    ConvertFunction m_convert;
    ConvertBackFunction m_convertBack;
};

template<typename TSource, typename TTarget>
std::shared_ptr<IValueConverter<TSource, TTarget>> MakeConverter(
    typename FunctionValueConverter<TSource, TTarget>::ConvertFunction convert,
    typename FunctionValueConverter<TSource, TTarget>::ConvertBackFunction convertBack = {}) {
    return std::make_shared<FunctionValueConverter<TSource, TTarget>>(
        std::move(convert), std::move(convertBack));
}

template<typename TTarget, typename TSource>
std::shared_ptr<ComputedObservable<TTarget>> ConvertObservable(
    const std::shared_ptr<Observable<TSource>>& source,
    const std::shared_ptr<IValueConverter<TSource, TTarget>>& converter) {
    if (!source || !converter) return nullptr;
    return MakeComputed<TTarget>([converter](const TSource& value) {
        return converter->Convert(value);
    }, source);
}

} // namespace CUI
