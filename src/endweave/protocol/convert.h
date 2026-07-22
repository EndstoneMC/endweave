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
 * usually over a name-keyed copy (see fieldList), and the specialisation must
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
 * Names one field, for a copy that matches fields by name rather than by position.
 *
 * An accessor is a generic callable returning a reference to the field it names, written so the
 * name appears in its return type as well as its body:
 *
 *     [](auto &v) -> decltype((v.boss_id)) { return v.boss_id; }
 *
 * The trailing return type is what makes it usable here: it puts the field lookup in the
 * immediate context, so asking whether a version has the field is a substitution failure rather
 * than a hard error.
 */
template <class... Accessors>
class FieldList {
public:
    constexpr explicit FieldList(Accessors... accessors) : accessors_(accessors...) {}

    /**
     * Copies every field both versions have, leaving the rest of the target default.
     *
     * Field order is irrelevant, so this handles a reorder as well as a field coming or going.
     * The caller sets whatever only the target has, and simply does not mention whatever only
     * the source had.
     *
     * @param in The source-version value.
     * @return The target-version value.
     */
    template <class To, class From>
    To copy(const From &in) const
    {
        static_assert(sizeof...(Accessors) == std::max(detail::kArity<To>, detail::kArity<From>),
                      "the field list does not name every field of both versions of this type");
        To out{};
        std::apply(
            [&](const Accessors &...field) {
                (copyOne(out, in, field), ...);
            },
            accessors_);
        return out;
    }

private:
    template <class To, class From, class Accessor>
    static void copyOne(To &out, const From &in, const Accessor &field)
    {
        static_assert(
            requires { field(in); } || requires { field(out); },
            "this accessor names a field neither version of the type has");
        if constexpr (requires {
                          field(in);
                          field(out);
                      }) {
            field(out) = convert<std::remove_cvref_t<decltype(field(out))>>(field(in));
        }
    }

    std::tuple<Accessors...> accessors_;
};

/**
 * Builds the field list for a type whose field list changed between versions.
 *
 * One list serves both directions: which way the copy runs is decided by the target type.
 *
 * @note The list must name every field of both versions. Its length is checked against the two
 * field counts and every accessor is checked to name a real field, so an omission or a typo is
 * a compile error rather than a field that quietly stops copying. The length check assumes one
 * version's fields are a subset of the other's, which holds for every packet modelled so far; a
 * version where each side gained a field of its own would trip it, and should.
 */
template <class... Accessors>
constexpr auto fieldList(Accessors... accessors)
{
    return FieldList<Accessors...>(accessors...);
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
