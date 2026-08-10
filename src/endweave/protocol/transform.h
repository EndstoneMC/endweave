#pragma once

#include "endweave/protocol/context.h"

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

template <class From, class To>
struct WireCompatible : std::false_type {};

template <class From, class To>
inline constexpr bool wire_equal_v = std::is_same_v<From, To> || WireCompatible<From, To>::value;

namespace detail {

template <class Source, class To>
concept TransformableFrom = requires(Context<To> &ctx, Source &&source) {
    {
        Transformer<std::remove_cvref_t<Source>, To>::transform(ctx, std::forward<Source>(source))
    } -> std::same_as<void>;
};

} // namespace detail

template <class From, class To>
concept Transformable = detail::TransformableFrom<From, To>;

/** Fills `out`, through a context of its own over the caller's connection and cancel. */
template <class To, class From, class Parent>
constexpr void transform_into(const Context<Parent> &ctx, From &&from, To &out)
{
    static_assert(detail::TransformableFrom<From, To>,
                  "endweave: no Transformer<From, To>::transform accepting this value category");
    if constexpr (detail::TransformableFrom<From, To>) {
        Context<To> child = ctx.with(out);
        Transformer<std::remove_cvref_t<From>, To>::transform(child, std::forward<From>(from));
    }
}

template <class To, class From, class Parent>
[[nodiscard]] constexpr To transform_to(const Context<Parent> &ctx, From &&from)
{
    To out;
    transform_into(ctx, std::forward<From>(from), out);
    return out;
}

template <class Source, class Parent>
class TransformProxy {
public:
    using From = std::remove_cvref_t<Source>;

    constexpr TransformProxy(const Context<Parent> &ctx,
                             Source &&source) noexcept(std::is_nothrow_constructible_v<Source, Source &&>)
        : ctx_(ctx), source_(static_cast<Source &&>(source))
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
        return transform_to<To>(ctx_, static_cast<Source &&>(source_));
    }

private:
    const Context<Parent> &ctx_;
    Source source_;
};

template <class Parent, class From>
[[nodiscard]] constexpr auto transform(const Context<Parent> &ctx, From &&from) noexcept(
    std::is_nothrow_constructible_v<TransformProxy<From, Parent>, const Context<Parent> &, From &&>)
{
    return TransformProxy<From, Parent>(ctx, std::forward<From>(from));
}

template <class From, class To>
struct Transformer<std::optional<From>, std::optional<To>> {
    static void transform(Context<std::optional<To>> &ctx, std::optional<From> &&from)
        requires Transformable<From, To>
    {
        if (from.has_value()) {
            ctx.out() = transform_to<To>(ctx, std::move(from).value());
        }
    }
};

template <class From, class To>
struct Transformer<std::vector<From>, std::vector<To>> {
    static void transform(Context<std::vector<To>> &ctx, std::vector<From> &&from)
        requires Transformable<From, To>
    {
        auto &to = ctx.out();
        to.reserve(from.size());
        for (auto &item : from) {
            transform_into(ctx, std::move(item), to.emplace_back());
        }
    }
};

template <class K, class From, class To>
struct Transformer<std::map<K, From>, std::map<K, To>> {
    static void transform(Context<std::map<K, To>> &ctx, std::map<K, From> &&from)
        requires Transformable<From, To>
    {
        for (auto &[key, value] : from) {
            transform_into(ctx, std::move(value), ctx.out()[key]);
        }
    }
};

} // namespace endweave
