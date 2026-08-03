#pragma once

#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace endweave {

/** @see ViaVersion ValueTransformer. */
template <class T>
struct Transformer;

template <class T>
auto transform(T &&from)
{
    return Transformer<std::remove_cvref_t<T>>::transform(std::move(from));
}

template <class T>
struct Transformer<std::optional<T>> {
    static auto transform(std::optional<T> &&from)
    {
        std::optional<decltype(endweave::transform(std::declval<T>()))> to;
        if (from.has_value()) {
            to = endweave::transform(from.value());
        }
        return to;
    }
};

template <class T>
struct Transformer<std::vector<T>> {
    static auto transform(std::vector<T> &&from)
    {
        std::vector<decltype(endweave::transform(std::declval<T>()))> to;
        to.reserve(from.size());
        for (auto &item : from) {
            to.push_back(endweave::transform(item));
        }
        return to;
    }
};

} // namespace endweave
