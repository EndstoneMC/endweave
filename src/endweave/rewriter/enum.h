#pragma once

#include "endweave/protocol/context.h"

#include <algorithm>
#include <array>
#include <bedrock/protocol/detail/reflect.hpp>
#include <bedrock/protocol/enum.hpp>
#include <concepts>
#include <cstddef>
#include <map>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace bp = bedrock::protocol;

namespace endweave {

/** What a value of `E` the destination cannot encode is sent as, or none to drop the packet. */
template <class E>
struct EnumPolicy;

template <class E>
concept HasEnumPolicy = requires(E value) {
    { EnumPolicy<E>::apply(value) } -> std::same_as<std::optional<E>>;
};

namespace detail {

template <class E>
concept ReflectedEnum = std::is_enum_v<E> && requires { bp::enum_count<E>(); };

template <class E, int V>
concept NameCoded = requires(E value) { bp::enum_name_<V>(value); };

/** Whether protocol `V` numbers `value`, and names it where `V` codes `E` by name. */
template <int V, class E>
constexpr bool encodes(E value)
{
    if constexpr (NameCoded<E, V>) {
        return bp::enum_contains_<V>(value) &&
               (bp::enum_names_<V, E>().empty() || bp::enum_name_<V>(value).has_value());
    }
    else {
        return bp::enum_contains_<V>(value);
    }
}

/** Whether `From` can send a value of `E` that `To` cannot encode. */
template <class E, int From, int To>
consteval bool loses()
{
    if constexpr (!ReflectedEnum<E>) {
        return false;
    }
    else {
        return std::ranges::any_of(bp::enum_values<E>(), [](E value) {
            return encodes<From>(value) && !encodes<To>(value);
        });
    }
}

/** Fails the build where `E` loses a value from `From` to `To` that no policy sends as one `To` encodes. */
template <class E, int From, int To>
consteval bool carriesEnum()
{
    if constexpr (loses<E, From, To>()) {
        static_assert(HasEnumPolicy<E>,
                      "endweave: the destination protocol cannot encode a value of this enum that the source "
                      "protocol can send. Give the enum an EnumPolicy in protocols/enums.h.");
        if constexpr (HasEnumPolicy<E>) {
            static_assert(std::ranges::all_of(bp::enum_values<E>(),
                                              [](E value) {
                                                  if (!encodes<From>(value) || encodes<To>(value)) {
                                                      return true;
                                                  }
                                                  const auto sent = EnumPolicy<E>::apply(value);
                                                  return !sent || encodes<To>(*sent);
                                              }),
                          "endweave: this enum's EnumPolicy sends a value the destination protocol cannot encode "
                          "either.");
        }
    }
    return true;
}

template <class T, class Test>
consteval bool holdsEnum();

/** Whether `T` holds, at any depth, an enum `Test::holds` is true for. */
template <class T, class Test>
struct HoldsEnum {
    static consteval bool get()
    {
        if constexpr (std::is_enum_v<T>) {
            return Test::template holds<T>();
        }
        else if constexpr (bp::Reflected<T>) {
            return []<std::size_t... I>(std::index_sequence<I...>) {
                return (holdsEnum<std::remove_cvref_t<decltype(bp::field_get<I>(std::declval<T &>()))>, Test>() || ...);
            }(std::make_index_sequence<bp::field_count<T>()>{});
        }
        else {
            return false;
        }
    }
};

template <class T, class Test>
struct HoldsEnum<std::optional<T>, Test> {
    static consteval bool get()
    {
        return holdsEnum<T, Test>();
    }
};

template <class T, class A, class Test>
struct HoldsEnum<std::vector<T, A>, Test> {
    static consteval bool get()
    {
        return holdsEnum<T, Test>();
    }
};

template <class T, std::size_t N, class Test>
struct HoldsEnum<std::array<T, N>, Test> {
    static consteval bool get()
    {
        return holdsEnum<T, Test>();
    }
};

template <class K, class V, class C, class A, class Test>
struct HoldsEnum<std::map<K, V, C, A>, Test> {
    static consteval bool get()
    {
        return holdsEnum<K, Test>() || holdsEnum<V, Test>();
    }
};

template <class... T, class Test>
struct HoldsEnum<std::variant<T...>, Test> {
    static consteval bool get()
    {
        return (holdsEnum<T, Test>() || ...);
    }
};

template <class T, class Test>
consteval bool holdsEnum()
{
    return HoldsEnum<T, Test>::get();
}

template <class E>
struct Is {
    template <class U>
    static consteval bool holds()
    {
        return std::is_same_v<U, E>;
    }
};

template <int From, int To>
struct Lost {
    template <class E>
    static consteval bool holds()
    {
        return loses<E, From, To>();
    }
};

/** An enum both packets hold that `carriesEnum` fails the build on. */
template <class ToPacket, int From, int To>
struct Uncarried {
    template <class E>
    static consteval bool holds()
    {
        if constexpr (holdsEnum<ToPacket, Is<E>>()) {
            return !carriesEnum<E, From, To>();
        }
        else {
            return false;
        }
    }
};

} // namespace detail

/** Whether `From` and `To` give some value of `E` different numbers, or one of them none. */
template <class E, int From, int To>
inline constexpr bool numbers_apart_v = std::ranges::any_of(bp::enum_values<E>(), [](E value) {
    return bp::enum_integer_<From>(value) != bp::enum_integer_<To>(value);
});

/** Whether every enum both packets hold carries, or has a policy for, each value `From` can send to `To`. */
template <class FromPacket, class ToPacket, int From, int To>
inline constexpr bool carries_enums_v = !detail::holdsEnum<FromPacket, detail::Uncarried<ToPacket, From, To>>();

/** Sends each enum value in a packet that `To` cannot encode as its policy says. */
template <int From, int To>
struct EnumRewriter {
    /** Whether `T` holds an enum that loses a value from `From` to `To`. */
    template <class T>
    static constexpr bool rewrites_v = detail::holdsEnum<T, detail::Lost<From, To>>();

    template <class T, class Parent>
    static void rewriteEnums(const Context<Parent> &ctx, T &value)
    {
        if constexpr (!rewrites_v<T>) {
            return;
        }
        else if constexpr (std::is_enum_v<T>) {
            if (const auto sent = send(value)) {
                value = *sent;
            }
            else {
                ctx.cancel();
            }
        }
        else if constexpr (bp::Reflected<T>) {
            [&]<std::size_t... I>(std::index_sequence<I...>) {
                (rewriteEnums(ctx, bp::field_get<I>(value)), ...);
            }(std::make_index_sequence<bp::field_count<T>()>{});
        }
        else if constexpr (requires { value.index(); }) {
            std::visit(
                [&](auto &alternative) {
                    rewriteEnums(ctx, alternative);
                },
                value);
        }
        else if constexpr (requires { value.has_value(); }) {
            if (value.has_value()) {
                rewriteEnums(ctx, *value);
            }
        }
        else if constexpr (requires { typename T::key_type; }) {
            T kept;
            for (auto &[key, mapped] : value) {
                rewriteEnums(ctx, mapped);
                const auto sent = send(key);
                if (!sent) {
                    ctx.cancel();
                    return;
                }
                // ENDWEAVE: a key sent as a fallback that the map already holds keeps the entry that
                // held it first.
                kept.emplace(*sent, std::move(mapped));
            }
            value = std::move(kept);
        }
        else {
            for (auto &element : value) {
                rewriteEnums(ctx, element);
            }
        }
    }

private:
    /** `value` as `To` can encode it, or none to drop the packet. */
    template <class E>
    static std::optional<E> send(E value)
    {
        if constexpr (!detail::loses<E, From, To>()) {
            return value;
        }
        else {
            return detail::encodes<To>(value) ? std::optional{value} : EnumPolicy<E>::apply(value);
        }
    }
};

} // namespace endweave
