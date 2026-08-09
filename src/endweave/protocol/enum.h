#pragma once

#include <array>
#include <bedrock/enum.hpp>
#include <cstddef>

namespace endweave {

template <class To, To Fallback, class From>
To byName(From from)
{
    // ENDWEAVE: matching 600-odd names against 600-odd names is a constant, but folding it at
    // compile time costs seconds a translation unit and a raised -fconstexpr-steps, so it is
    // folded once on first use instead.
    static const auto table = [] {
        std::array<To, bedrock::protocol::enum_count<From>()> built{};
        for (std::size_t i = 0; i < built.size(); ++i) {
            built[i] = bedrock::protocol::enum_cast<To>(bedrock::protocol::enum_names<From>()[i]).value_or(Fallback);
        }
        return built;
    }();

    const auto index = bedrock::protocol::enum_index(from);
    if (!index) {
        return Fallback;
    }
    return table[index.value()];
}

} // namespace endweave
