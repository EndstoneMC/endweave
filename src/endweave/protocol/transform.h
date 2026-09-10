#pragma once

#include "endweave/protocol/context.h"

#include <bedrock/protocol/detail/reflect.hpp>
#include <concepts>
#include <cstddef>
#include <map>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace endweave {

/** @see ViaVersion ValueTransformer. */
template <class From, class To>
struct Transformer;

template <class From, class To>
concept Transformable = requires(Context<To> &ctx, From &&from) {
    { Transformer<std::remove_cvref_t<From>, To>::transform(ctx, std::forward<From>(from)) } -> std::same_as<void>;
};

/** Transforms `from` into `out`, sharing the caller's session and cancel flag. */
template <class To, class From, class Parent>
constexpr void transform_into(const Context<Parent> &ctx, From &&from, To &out)
{
    static_assert(Transformable<From, To>,
                  "endweave: no Transformer<From, To>::transform accepting this value category");
    if constexpr (Transformable<From, To>) {
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

namespace detail {

/** Moves member `I` of `from` into the same-named member of `to`, transforming it if the types
 * differ. Does nothing if `to` has no such member. */
template <std::size_t I, class From, class To, class Parent>
constexpr void assign_member(const Context<Parent> &ctx, From &from, To &to)
{
    constexpr std::size_t j = bedrock::protocol::field_index<To>(bedrock::protocol::field_name<I, From>());
    if constexpr (j != bedrock::protocol::field_npos) {
        auto &source = bedrock::protocol::field_get<I>(from);
        auto &target = bedrock::protocol::field_get<j>(to);
        using S = std::remove_cvref_t<decltype(source)>;
        using T = std::remove_cvref_t<decltype(target)>;
        if constexpr (std::is_same_v<S, T>) {
            target = std::move(source);
        }
        else if constexpr (Transformable<S, T>) {
            transform_into(ctx, std::move(source), target);
        }
    }
}

template <std::size_t J, class From, class To>
consteval bool member_filled()
{
    constexpr std::size_t i = bedrock::protocol::field_index<From>(bedrock::protocol::field_name<J, To>());
    if constexpr (i == bedrock::protocol::field_npos) {
        return false;
    }
    else {
        using S = std::remove_cvref_t<decltype(bedrock::protocol::field_get<i>(std::declval<From &>()))>;
        using T = std::remove_cvref_t<decltype(bedrock::protocol::field_get<J>(std::declval<To &>()))>;
        return std::is_same_v<S, T> || Transformable<S, T>;
    }
}

template <class From, class To>
consteval bool memberwise_complete()
{
    if constexpr (!bedrock::protocol::Reflected<From> || !bedrock::protocol::Reflected<To>) {
        return false;
    }
    // Matching members aren't enough: NormalTransactionData and InventoryMismatchData look alike.
    else if constexpr (bedrock::protocol::struct_name<From>() != bedrock::protocol::struct_name<To>()) {
        return false;
    }
    else {
        return []<std::size_t... J>(std::index_sequence<J...>) {
            return (member_filled<J, From, To>() && ...);
        }(std::make_index_sequence<bedrock::protocol::field_count<To>()>{});
    }
}

} // namespace detail

/** Whether every member of `To` can be filled from a same-named member of `From`. Members only
 * `From` has are ignored; a member only `To` has, or a rename, makes this false. */
template <class From, class To>
inline constexpr bool memberwise_complete_v = detail::memberwise_complete<From, To>();

/** Moves each member of `from` into the same-named member of `to`. */
template <class From, class To, class Parent>
constexpr void transform_members(const Context<Parent> &ctx, From &&from, To &to)
{
    using F = std::remove_cvref_t<From>;
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        (detail::assign_member<I>(ctx, from, to), ...);
    }(std::make_index_sequence<bedrock::protocol::field_count<F>()>{});
}

/** Copies member by member when the pair is memberwise complete. Any other pair needs a
 * hand-written Transformer. */
template <class From, class To>
    requires memberwise_complete_v<From, To>
struct Transformer<From, To> {
    static void transform(Context<To> &ctx, From &&from)
    {
        transform_members(ctx, std::move(from), ctx.out());
    }
};

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
        requires Transformable<Source, To>
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

/** Keeps the active alternative's index, since BDS writes it as the discriminant. Alternatives are
 * matched by index, never by shape. */
template <class... From, class... To>
    requires(sizeof...(From) == sizeof...(To))
struct Transformer<std::variant<From...>, std::variant<To...>> {
    static void transform(Context<std::variant<To...>> &ctx, std::variant<From...> &&from)
        requires(... && (std::is_same_v<From, To> || Transformable<From, To>))
    {
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            (void)((from.index() == I && (carry<I>(ctx, std::move(from)), true)) || ...);
        }(std::index_sequence_for<From...>{});
    }

private:
    template <std::size_t I>
    static void carry(Context<std::variant<To...>> &ctx, std::variant<From...> &&from)
    {
        using F = std::variant_alternative_t<I, std::variant<From...>>;
        using T = std::variant_alternative_t<I, std::variant<To...>>;
        auto &arm = ctx.out().template emplace<I>();
        if constexpr (std::is_same_v<F, T>) {
            arm = std::move(std::get<I>(from));
        }
        else {
            transform_into(ctx, std::move(std::get<I>(from)), arm);
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
