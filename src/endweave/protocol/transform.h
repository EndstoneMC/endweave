#pragma once

#include <concepts>
#include <map>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace endweave {

/** @see ViaVersion ValueTransformer. */
template <class From, class To>
struct Transformer;

namespace detail {

template <class Source, class To>
concept TransformableFrom = requires(Source &&source) {
    { Transformer<std::remove_cvref_t<Source>, To>::transform(std::forward<Source>(source)) } -> std::same_as<To>;
};

} // namespace detail

template <class From, class To>
concept TransformableFromLvalue = detail::TransformableFrom<const From &, To>;

template <class From, class To>
concept Transformable = detail::TransformableFrom<From, To>;

template <class To, class From>
[[nodiscard]] constexpr To transform_to(From &&from)
{
    static_assert(detail::TransformableFrom<From, To>,
                  "endweave: no Transformer<From, To>::transform accepting this value category");
    if constexpr (detail::TransformableFrom<From, To>) {
        return Transformer<std::remove_cvref_t<From>, To>::transform(std::forward<From>(from));
    }
}

template <class Source>
class TransformProxy {
public:
    using From = std::remove_cvref_t<Source>;

    explicit constexpr TransformProxy(Source &&source) noexcept(std::is_nothrow_constructible_v<Source, Source &&>)
        : source_(static_cast<Source &&>(source))
    {
    }

    TransformProxy(const TransformProxy &) = delete;
    TransformProxy(TransformProxy &&) = delete;
    TransformProxy &operator=(const TransformProxy &) = delete;
    TransformProxy &operator=(TransformProxy &&) = delete;

    template <class To>
        requires detail::TransformableFrom<Source, To>
    [[nodiscard]] constexpr operator To() &&
    {
        return transform_to<To>(static_cast<Source &&>(source_));
    }

private:
    Source source_;
};

template <class From>
[[nodiscard]] constexpr auto transform(From &&from) noexcept(
    std::is_nothrow_constructible_v<TransformProxy<From>, From &&>)
{
    return TransformProxy<From>(std::forward<From>(from));
}

template <class From, class To>
struct Transformer<std::optional<From>, std::optional<To>> {
    static std::optional<To> transform(const std::optional<From> &from)
        requires TransformableFromLvalue<From, To>
    {
        std::optional<To> to;
        if (from.has_value()) {
            to = transform_to<To>(from.value());
        }
        return to;
    }

    static std::optional<To> transform(std::optional<From> &&from)
        requires Transformable<From, To>
    {
        std::optional<To> to;
        if (from.has_value()) {
            to = transform_to<To>(std::move(from).value());
        }
        return to;
    }
};

template <class From, class To>
struct Transformer<std::vector<From>, std::vector<To>> {
    static std::vector<To> transform(const std::vector<From> &from)
        requires TransformableFromLvalue<From, To>
    {
        std::vector<To> to;
        to.reserve(from.size());
        for (const auto &item : from) {
            to.push_back(transform_to<To>(item));
        }
        return to;
    }

    static std::vector<To> transform(std::vector<From> &&from)
        requires Transformable<From, To>
    {
        std::vector<To> to;
        to.reserve(from.size());
        for (auto &item : from) {
            to.push_back(transform_to<To>(std::move(item)));
        }
        return to;
    }
};

template <class K, class From, class To>
struct Transformer<std::map<K, From>, std::map<K, To>> {
    static std::map<K, To> transform(const std::map<K, From> &from)
        requires TransformableFromLvalue<From, To>
    {
        std::map<K, To> to;
        for (const auto &[key, value] : from) {
            to.emplace(key, transform_to<To>(value));
        }
        return to;
    }

    static std::map<K, To> transform(std::map<K, From> &&from)
        requires Transformable<From, To>
    {
        std::map<K, To> to;
        for (auto &[key, value] : from) {
            to.emplace(key, transform_to<To>(std::move(value)));
        }
        return to;
    }
};

} // namespace endweave
