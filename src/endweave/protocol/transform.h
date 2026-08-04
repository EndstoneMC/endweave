#pragma once

#include <map>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace endweave {

/** @see ViaVersion ValueTransformer. */
template <class T>
struct Transformer;

template <class T>
auto upgrade(T &&from)
{
    return Transformer<std::remove_cvref_t<T>>::upgrade(std::move(from));
}

template <class T>
auto downgrade(T &&from)
{
    return Transformer<std::remove_cvref_t<T>>::downgrade(std::move(from));
}

template <class T>
struct Transformer<std::optional<T>> {
    static auto upgrade(std::optional<T> &&from)
    {
        std::optional<decltype(endweave::upgrade(std::declval<T>()))> to;
        if (from.has_value()) {
            to = endweave::upgrade(from.value());
        }
        return to;
    }

    static auto downgrade(std::optional<T> &&from)
    {
        std::optional<decltype(endweave::downgrade(std::declval<T>()))> to;
        if (from.has_value()) {
            to = endweave::downgrade(from.value());
        }
        return to;
    }
};

template <class K, class V>
struct Transformer<std::map<K, V>> {
    static auto upgrade(std::map<K, V> &&from)
    {
        std::map<K, decltype(endweave::upgrade(std::declval<V>()))> to;
        for (auto &[key, value] : from) {
            to.emplace(key, endweave::upgrade(value));
        }
        return to;
    }

    static auto downgrade(std::map<K, V> &&from)
    {
        std::map<K, decltype(endweave::downgrade(std::declval<V>()))> to;
        for (auto &[key, value] : from) {
            to.emplace(key, endweave::downgrade(value));
        }
        return to;
    }
};

template <class T>
struct Transformer<std::vector<T>> {
    static auto upgrade(std::vector<T> &&from)
    {
        std::vector<decltype(endweave::upgrade(std::declval<T>()))> to;
        to.reserve(from.size());
        for (auto &item : from) {
            to.push_back(endweave::upgrade(item));
        }
        return to;
    }

    static auto downgrade(std::vector<T> &&from)
    {
        std::vector<decltype(endweave::downgrade(std::declval<T>()))> to;
        to.reserve(from.size());
        for (auto &item : from) {
            to.push_back(endweave::downgrade(item));
        }
        return to;
    }
};

} // namespace endweave
