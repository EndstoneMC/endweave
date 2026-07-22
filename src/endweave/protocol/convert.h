#pragma once

#include <concepts>
#include <cstddef>
#include <map>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace endweave {

/**
 * Converts a value from one protocol version's shape to another's.
 *
 * The primary template walks an aggregate field by field, so a type whose field list did not
 * change between the two versions needs no converter at all -- it only differs by the nested
 * types it carries, and those recurse. A type that really was reshaped specialises this, and
 * the specialisation must be declared before the first use.
 *
 * @note The walk is positional and static_asserts that the two shapes have the same number of
 * fields, so a version that added, removed or reordered a field fails to compile rather than
 * silently losing it. That is the point: only a same-shape copy is safe to derive.
 */
template <class To, class From>
struct Conversion;

/**
 * Converts a value to the shape the target version expects.
 *
 * @param in The source-version value.
 * @return The target-version value.
 */
template <class To, class From>
To convert(const From &in)
{
    if constexpr (std::same_as<To, From>) {
        return in;
    }
    else {
        return Conversion<To, From>::apply(in);
    }
}

namespace detail {

/**
 * Stands in for any field while probing an aggregate's arity. It refuses to convert to the
 * aggregate itself, so a one-field probe cannot be mistaken for a copy construction.
 */
template <class T>
struct AnyField {
    template <class U>
        requires(!std::same_as<std::remove_cvref_t<U>, T>)
    constexpr operator U() const;
};

template <class T, class... Probes>
constexpr std::size_t countFields()
{
    if constexpr (requires { T{Probes{}..., AnyField<T>{}}; }) {
        return countFields<T, Probes..., AnyField<T>>();
    }
    else {
        return sizeof...(Probes);
    }
}

/**
 * How many fields an aggregate has.
 */
template <class T>
inline constexpr std::size_t kArity = countFields<T>();

/**
 * Assigns every field of an already-destructured pair of aggregates.
 */
template <class... Out, class... In>
void convertFields(std::tuple<Out &...> out, std::tuple<const In &...> in)
{
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        ((std::get<I>(out) = convert<std::remove_cvref_t<decltype(std::get<I>(out))>>(std::get<I>(in))), ...);
    }(std::index_sequence_for<Out...>{});
}

} // namespace detail

template <class To, class From>
struct Conversion {
    static To apply(const From &in)
    {
        static_assert(detail::kArity<To> == detail::kArity<From>,
                      "the two versions of this type have different field counts, so it was "
                      "reshaped and needs its own Conversion specialisation");
        constexpr std::size_t arity = detail::kArity<From>;
        To out{};
        // One arm per field count. Structured bindings need the arity as a literal, so the
        // ladder is written out; extend it when a wider same-shape type turns up.
        if constexpr (arity == 1) {
            auto &[o0] = out;
            const auto &[i0] = in;
            detail::convertFields(std::tie(o0), std::tie(i0));
        }
        else if constexpr (arity == 2) {
            auto &[o0, o1] = out;
            const auto &[i0, i1] = in;
            detail::convertFields(std::tie(o0, o1), std::tie(i0, i1));
        }
        else if constexpr (arity == 3) {
            auto &[o0, o1, o2] = out;
            const auto &[i0, i1, i2] = in;
            detail::convertFields(std::tie(o0, o1, o2), std::tie(i0, i1, i2));
        }
        else if constexpr (arity == 4) {
            auto &[o0, o1, o2, o3] = out;
            const auto &[i0, i1, i2, i3] = in;
            detail::convertFields(std::tie(o0, o1, o2, o3), std::tie(i0, i1, i2, i3));
        }
        else if constexpr (arity == 5) {
            auto &[o0, o1, o2, o3, o4] = out;
            const auto &[i0, i1, i2, i3, i4] = in;
            detail::convertFields(std::tie(o0, o1, o2, o3, o4), std::tie(i0, i1, i2, i3, i4));
        }
        else if constexpr (arity == 6) {
            auto &[o0, o1, o2, o3, o4, o5] = out;
            const auto &[i0, i1, i2, i3, i4, i5] = in;
            detail::convertFields(std::tie(o0, o1, o2, o3, o4, o5), std::tie(i0, i1, i2, i3, i4, i5));
        }
        else if constexpr (arity == 7) {
            auto &[o0, o1, o2, o3, o4, o5, o6] = out;
            const auto &[i0, i1, i2, i3, i4, i5, i6] = in;
            detail::convertFields(std::tie(o0, o1, o2, o3, o4, o5, o6), std::tie(i0, i1, i2, i3, i4, i5, i6));
        }
        else if constexpr (arity == 8) {
            auto &[o0, o1, o2, o3, o4, o5, o6, o7] = out;
            const auto &[i0, i1, i2, i3, i4, i5, i6, i7] = in;
            detail::convertFields(std::tie(o0, o1, o2, o3, o4, o5, o6, o7), std::tie(i0, i1, i2, i3, i4, i5, i6, i7));
        }
        else if constexpr (arity == 9) {
            auto &[o0, o1, o2, o3, o4, o5, o6, o7, o8] = out;
            const auto &[i0, i1, i2, i3, i4, i5, i6, i7, i8] = in;
            detail::convertFields(std::tie(o0, o1, o2, o3, o4, o5, o6, o7, o8),
                                  std::tie(i0, i1, i2, i3, i4, i5, i6, i7, i8));
        }
        else if constexpr (arity == 10) {
            auto &[o0, o1, o2, o3, o4, o5, o6, o7, o8, o9] = out;
            const auto &[i0, i1, i2, i3, i4, i5, i6, i7, i8, i9] = in;
            detail::convertFields(std::tie(o0, o1, o2, o3, o4, o5, o6, o7, o8, o9),
                                  std::tie(i0, i1, i2, i3, i4, i5, i6, i7, i8, i9));
        }
        else if constexpr (arity == 11) {
            auto &[o0, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10] = out;
            const auto &[i0, i1, i2, i3, i4, i5, i6, i7, i8, i9, i10] = in;
            detail::convertFields(std::tie(o0, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10),
                                  std::tie(i0, i1, i2, i3, i4, i5, i6, i7, i8, i9, i10));
        }
        else if constexpr (arity == 12) {
            auto &[o0, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11] = out;
            const auto &[i0, i1, i2, i3, i4, i5, i6, i7, i8, i9, i10, i11] = in;
            detail::convertFields(std::tie(o0, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11),
                                  std::tie(i0, i1, i2, i3, i4, i5, i6, i7, i8, i9, i10, i11));
        }
        else {
            static_assert(arity <= 12, "extend the field-count ladder");
        }
        return out;
    }
};

/**
 * An absent value stays absent; a present one converts.
 */
template <class To, class From>
struct Conversion<std::optional<To>, std::optional<From>> {
    static std::optional<To> apply(const std::optional<From> &in)
    {
        if (!in.has_value()) {
            return std::nullopt;
        }
        return convert<To>(*in);
    }
};

template <class To, class From>
struct Conversion<std::vector<To>, std::vector<From>> {
    static std::vector<To> apply(const std::vector<From> &in)
    {
        std::vector<To> out;
        out.reserve(in.size());
        for (const From &element : in) {
            out.push_back(convert<To>(element));
        }
        return out;
    }
};

template <class Key, class To, class From>
struct Conversion<std::map<Key, To>, std::map<Key, From>> {
    static std::map<Key, To> apply(const std::map<Key, From> &in)
    {
        std::map<Key, To> out;
        for (const auto &[key, value] : in) {
            out.emplace(key, convert<To>(value));
        }
        return out;
    }
};

/**
 * Converts a variant case by case, holding the index steady.
 *
 * @note Only sound while the two versions number their cases the same way. A variant that gained
 * a case at the front, or reordered one, must be converted by hand.
 */
template <class... To, class... From>
    requires(sizeof...(To) == sizeof...(From))
struct Conversion<std::variant<To...>, std::variant<From...>> {
    static std::variant<To...> apply(const std::variant<From...> &in)
    {
        return [&]<std::size_t... I>(std::index_sequence<I...>) -> std::variant<To...> {
            std::variant<To...> out;
            (void)((in.index() == I
                        ? (out = convert<std::variant_alternative_t<I, std::variant<To...>>>(std::get<I>(in)), true)
                        : false) ||
                   ...);
            return out;
        }(std::index_sequence_for<From...>{});
    }
};

} // namespace endweave
