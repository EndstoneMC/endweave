#pragma once

#include "endweave/protocol/context.h"

#include <bedrock/reflect.hpp>
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

namespace detail {

/** Where `To` carries a member named like `From`'s member `I`, moves it across --
 * directly when the two are one type, through their Transformer when they are not.
 * A member the other side does not name is left alone, so a hand-written body can
 * call this for the bulk and then write only what differs. */
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
    // Shape alone does not make two types the same type: NormalTransactionData and
    // InventoryMismatchData both hold one `transaction` and mean different things, and BDS
    // puts them in one variant. Only a type meeting itself across snapshots copies member
    // for member.
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

/** Whether every member the destination carries is named by the source and can reach
 * it. The test is deliberately one-sided. A member the destination drops asks nothing
 * of anyone -- the field is gone and its contents have nowhere to go. A member the
 * destination gained does: something has to say what it holds, and the answer is
 * rarely the default. A rename reads as both at once, so it fails here too. */
template <class From, class To>
inline constexpr bool memberwise_complete_v = detail::memberwise_complete<From, To>();

/** Moves every member `To` names alike across. The pack expands, so this costs the
 * member assignments and nothing else. */
template <class From, class To, class Parent>
constexpr void transform_members(const Context<Parent> &ctx, From &&from, To &to)
{
    using F = std::remove_cvref_t<From>;
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        (detail::assign_member<I>(ctx, from, to), ...);
    }(std::make_index_sequence<bedrock::protocol::field_count<F>()>{});
}

/** Two snapshots that name the same members need nothing written by hand: only the
 * serialisers differ, which the pair of types already carries. A pair that is not
 * memberwise-complete falls back to the undefined primary, so it stays a build
 * error until someone writes what the difference means. */
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

/** An alternative keeps its index across the hop: that index is the discriminant BDS
 * writes, so the arm a packet arrived on is the arm it leaves on. Two arms of one
 * variant can hold the same members -- NormalTransactionData and InventoryMismatchData
 * both hold a `transaction` -- and matching them by shape would swap them silently. */
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
