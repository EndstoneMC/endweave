#pragma once

#include <concepts>
#include <cstddef>
#include <map>
#include <optional>
#include <tuple>
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
 * Binds an aggregate's fields as a tuple of references.
 *
 * Structured bindings need the field count as a literal, so this is a ladder rather than a
 * loop. It is generated; extend the bound if a wider type turns up.
 */
template <class T>
auto asTuple(T &value)
{
    constexpr std::size_t arity = kArity<std::remove_cvref_t<T>>;
    static_assert(arity >= 1 && arity <= 64, "aggregate is outside the supported field-count range");
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
    else if constexpr (arity == 13) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12);
    }
    else if constexpr (arity == 14) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13);
    }
    else if constexpr (arity == 15) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14);
    }
    else if constexpr (arity == 16) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15);
    }
    else if constexpr (arity == 17) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16);
    }
    else if constexpr (arity == 18) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17);
    }
    else if constexpr (arity == 19) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18);
    }
    else if constexpr (arity == 20) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19);
    }
    else if constexpr (arity == 21) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20);
    }
    else if constexpr (arity == 22) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21] =
            value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21);
    }
    else if constexpr (arity == 23) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21,
               f22] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22);
    }
    else if constexpr (arity == 24) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23);
    }
    else if constexpr (arity == 25) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24);
    }
    else if constexpr (arity == 26) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25);
    }
    else if constexpr (arity == 27) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26);
    }
    else if constexpr (arity == 28) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27);
    }
    else if constexpr (arity == 29) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28);
    }
    else if constexpr (arity == 30) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29);
    }
    else if constexpr (arity == 31) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30);
    }
    else if constexpr (arity == 32) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31);
    }
    else if constexpr (arity == 33) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32);
    }
    else if constexpr (arity == 34) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33);
    }
    else if constexpr (arity == 35) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34);
    }
    else if constexpr (arity == 36) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35);
    }
    else if constexpr (arity == 37) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36);
    }
    else if constexpr (arity == 38) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37);
    }
    else if constexpr (arity == 39) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38);
    }
    else if constexpr (arity == 40) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39);
    }
    else if constexpr (arity == 41) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39,
                        f40);
    }
    else if constexpr (arity == 42) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39,
                        f40, f41);
    }
    else if constexpr (arity == 43) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42] =
            value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39,
                        f40, f41, f42);
    }
    else if constexpr (arity == 44) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42,
               f43] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39,
                        f40, f41, f42, f43);
    }
    else if constexpr (arity == 45) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43,
               f44] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39,
                        f40, f41, f42, f43, f44);
    }
    else if constexpr (arity == 46) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43,
               f44, f45] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39,
                        f40, f41, f42, f43, f44, f45);
    }
    else if constexpr (arity == 47) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43,
               f44, f45, f46] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39,
                        f40, f41, f42, f43, f44, f45, f46);
    }
    else if constexpr (arity == 48) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43,
               f44, f45, f46, f47] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39,
                        f40, f41, f42, f43, f44, f45, f46, f47);
    }
    else if constexpr (arity == 49) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43,
               f44, f45, f46, f47, f48] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39,
                        f40, f41, f42, f43, f44, f45, f46, f47, f48);
    }
    else if constexpr (arity == 50) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43,
               f44, f45, f46, f47, f48, f49] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39,
                        f40, f41, f42, f43, f44, f45, f46, f47, f48, f49);
    }
    else if constexpr (arity == 51) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43,
               f44, f45, f46, f47, f48, f49, f50] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39,
                        f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50);
    }
    else if constexpr (arity == 52) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43,
               f44, f45, f46, f47, f48, f49, f50, f51] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39,
                        f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51);
    }
    else if constexpr (arity == 53) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43,
               f44, f45, f46, f47, f48, f49, f50, f51, f52] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39,
                        f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52);
    }
    else if constexpr (arity == 54) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43,
               f44, f45, f46, f47, f48, f49, f50, f51, f52, f53] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39,
                        f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53);
    }
    else if constexpr (arity == 55) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43,
               f44, f45, f46, f47, f48, f49, f50, f51, f52, f53, f54] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39,
                        f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53, f54);
    }
    else if constexpr (arity == 56) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43,
               f44, f45, f46, f47, f48, f49, f50, f51, f52, f53, f54, f55] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39,
                        f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53, f54, f55);
    }
    else if constexpr (arity == 57) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43,
               f44, f45, f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39,
                        f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56);
    }
    else if constexpr (arity == 58) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43,
               f44, f45, f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39,
                        f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57);
    }
    else if constexpr (arity == 59) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43,
               f44, f45, f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39,
                        f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58);
    }
    else if constexpr (arity == 60) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43,
               f44, f45, f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39,
                        f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58,
                        f59);
    }
    else if constexpr (arity == 61) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43,
               f44, f45, f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39,
                        f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58,
                        f59, f60);
    }
    else if constexpr (arity == 62) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43,
               f44, f45, f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60, f61] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39,
                        f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58,
                        f59, f60, f61);
    }
    else if constexpr (arity == 63) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43,
               f44, f45, f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60, f61, f62] = value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39,
                        f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58,
                        f59, f60, f61, f62);
    }
    else if constexpr (arity == 64) {
        auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
               f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43,
               f44, f45, f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60, f61, f62, f63] =
            value;
        return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20,
                        f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39,
                        f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58,
                        f59, f60, f61, f62, f63);
    }
}

/**
 * Copies field `FromIndex` of one tuple onto field `ToIndex` of another, converting as it goes.
 */
template <std::size_t ToIndex, std::size_t FromIndex, class Out, class In>
void copyField(Out &out, const In &in)
{
    using Target = std::remove_cvref_t<decltype(std::get<ToIndex>(out))>;
    std::get<ToIndex>(out) = convert<Target>(std::get<FromIndex>(in));
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
            (detail::copyField<I, I>(fields, source), ...);
        }(std::make_index_sequence<detail::kArity<From>>{});
        return out;
    }
};

/**
 * Copies a shape that only gained or lost a run of fields, leaving the new ones untouched.
 *
 * The fields before the gap line up one for one, and so do the fields after it, so only the
 * gap itself has to be spelled out by the caller. Use it for a version that appended or
 * inserted fields -- not for one that reordered or retyped them, which stays hand-written.
 *
 * @note Both sides are still copied positionally through convert(), so a field whose type
 * changed underneath recurses, and a mismatched pair fails to compile.
 *
 * @param in The source-version value.
 * @return The target-version value, with the gap left default-constructed.
 */
template <std::size_t At, std::size_t Count, class To, class From>
To convertAcrossGap(const From &in)
{
    constexpr std::size_t to_arity = detail::kArity<To>;
    constexpr std::size_t from_arity = detail::kArity<From>;
    static_assert(to_arity + Count == from_arity || from_arity + Count == to_arity,
                  "the gap does not account for the difference in field counts");
    constexpr bool widening = to_arity > from_arity;
    constexpr std::size_t shared = (widening ? from_arity : to_arity);
    static_assert(At <= shared, "the gap starts past the end of the shared fields");

    To out{};
    auto fields = detail::asTuple(out);
    const auto source = detail::asTuple(in);
    // Everything before the gap lines up.
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        (detail::copyField<I, I>(fields, source), ...);
    }(std::make_index_sequence<At>{});
    // Everything after it is shifted by the width of the gap, on whichever side is wider.
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        (detail::copyField<At + I + (widening ? Count : 0), At + I + (widening ? 0 : Count)>(fields, source), ...);
    }(std::make_index_sequence<shared - At>{});
    return out;
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
