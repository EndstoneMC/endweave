#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <map>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-inline"
#endif

namespace endweave {

/**
 * Converts a value from one protocol version's shape to another's.
 *
 * The primary template walks an aggregate field by field, so a type whose field list did not
 * change between the two versions needs no converter at all -- it only differs by the nested
 * types it carries, and those recurse. A type whose field list *did* change specialises this,
 * usually over a name-keyed copy (see ENDWEAVE_DEFINE_FIELD_COPY), and the specialisation must
 * be declared before the first use.
 *
 * @note The primary walk is positional and static_asserts that the two shapes have the same
 * number of fields, so a version that added, removed or reordered a field fails to compile
 * rather than silently losing it.
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
    // Only ever named in an unevaluated requires-expression, so it needs no definition; the
    // pragma keeps Clang from warning about the one it cannot see.
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
 * Binds an aggregate's fields as a tuple of references.
 *
 * Structured bindings need the field count as a literal, so this is a ladder rather than a
 * loop. Only same-shape types come through here; a reshaped one is copied by name instead, so
 * the bound stays small.
 */
template <class T>
auto asTuple(T &value)
{
    constexpr std::size_t arity = kArity<std::remove_cvref_t<T>>;
    static_assert(arity >= 1 && arity <= 12, "aggregate is outside the positional range -- give it a name-keyed copy");
    if constexpr (arity == 1) {
        auto &[f0] = value;
        return std::tie(f0);
    }
    else if constexpr (arity == 2) {
        auto &[f0, f1] = value;
        return std::tie(f0, f1);
    }
    else if constexpr (arity == 3) {
        auto &[f0, f1, f2] = value;
        return std::tie(f0, f1, f2);
    }
    else if constexpr (arity == 4) {
        auto &[f0, f1, f2, f3] = value;
        return std::tie(f0, f1, f2, f3);
    }
    else if constexpr (arity == 5) {
        auto &[f0, f1, f2, f3, f4] = value;
        return std::tie(f0, f1, f2, f3, f4);
    }
    else if constexpr (arity == 6) {
        auto &[f0, f1, f2, f3, f4, f5] = value;
        return std::tie(f0, f1, f2, f3, f4, f5);
    }
    else if constexpr (arity == 7) {
        auto &[f0, f1, f2, f3, f4, f5, f6] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6);
    }
    else if constexpr (arity == 8) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7);
    }
    else if constexpr (arity == 9) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8);
    }
    else if constexpr (arity == 10) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9);
    }
    else if constexpr (arity == 11) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10);
    }
    else if constexpr (arity == 12) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11);
    }
}

} // namespace detail

template <class To, class From>
struct Conversion {
    static To apply(const From &in)
    {
        static_assert(detail::kArity<To> == detail::kArity<From>,
                      "the two versions of this type have different field counts, so it was "
                      "reshaped and needs its own Conversion specialisation");
        To out{};
        auto fields = detail::asTuple(out);
        const auto source = detail::asTuple(in);
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            ((std::get<I>(fields) = convert<std::remove_cvref_t<decltype(std::get<I>(fields))>>(std::get<I>(source))),
             ...);
        }(std::make_index_sequence<detail::kArity<From>>{});
        return out;
    }
};

/**
 * Copies one field when both versions have it under that name, converting as it goes.
 *
 * A field only one version has is skipped, leaving the target's default for the caller to fill
 * in or drop deliberately.
 */
#define ENDWEAVE_COPY_FIELD(field)                                                           \
    if constexpr (requires {                                                                 \
                      in.field;                                                              \
                      out.field;                                                             \
                  }) {                                                                       \
        out.field = ::endweave::convert<std::remove_cvref_t<decltype(out.field)>>(in.field); \
    }

#define ENDWEAVE_COUNT_FIELD(field) +1

/**
 * Rejects a name no version of the type actually has, so a typo in a field list is a compile
 * error rather than a field that quietly stops being copied.
 */
#define ENDWEAVE_ASSERT_FIELD(field)                                                \
    static_assert(                                                                  \
        requires(const From &f) { f.field; } || requires(const To &t) { t.field; }, \
        "neither version of this type has a field called " #field);

/**
 * Defines a name-keyed copy between two versions of a type whose field list changed.
 *
 * Field order is irrelevant, so this handles a reorder as well as a field coming or going --
 * which is why it is preferred over the positional walk for any reshaped type. The generated
 * function copies every name both versions share; the caller then sets whatever only the
 * target has, and simply does not mention whatever only the source had.
 *
 * The list must name every field of both versions. Its length is checked against the two field
 * counts and every name is checked to exist on at least one side, so an omission or a typo is a
 * compile error rather than a field that quietly stops copying.
 *
 * @note That check assumes one version's fields are a subset of the other's, which is true of
 * every packet modelled so far. A version where each side gained a field of its own would trip
 * it, and should -- it wants looking at rather than counting.
 */
#define ENDWEAVE_DEFINE_FIELD_COPY(name, fields)                                                      \
    template <class To, class From>                                                                   \
    To name(const From &in)                                                                           \
    {                                                                                                 \
        static_assert((0 fields(ENDWEAVE_COUNT_FIELD)) ==                                             \
                          std::max(::endweave::detail::kArity<To>, ::endweave::detail::kArity<From>), \
                      "the field list does not name every field of both versions of " #name);         \
        fields(ENDWEAVE_ASSERT_FIELD) To out{};                                                       \
        fields(ENDWEAVE_COPY_FIELD) return out;                                                       \
    }

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
 * @note Only sound while the two versions number their cases the same way. A variant that
 * gained a case at the front, or reordered one, must be converted by hand.
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

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
