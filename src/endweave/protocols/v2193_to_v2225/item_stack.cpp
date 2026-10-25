#include "item_stack.h"

#include <cstddef>
#include <type_traits>
#include <utility>
#include <variant>

namespace ew = endweave;

namespace endweave {
namespace {

using FromAction = decltype(bp::ItemStackRequestCereal_<2193>::RequestData::actions)::value_type;
using ToAction = decltype(bp::ItemStackRequestCereal_<2225>::RequestData::actions)::value_type;

/** 2225 inserted CraftDiscPressActionData here, ahead of the two deprecated crafts. */
constexpr std::size_t kCraftDiscPress = 16;
static_assert(std::is_same_v<std::variant_alternative_t<kCraftDiscPress, ToAction>,
                             bp::ItemStackRequestCereal_<2225>::CraftDiscPressActionData>);

template <std::size_t I, class Parent>
void carry(const Context<Parent> &ctx, FromAction &&from, ToAction &to)
{
    constexpr std::size_t j = I < kCraftDiscPress ? I : I + 1;
    ew::transform_into(ctx, std::move(std::get<I>(from)), to.template emplace<j>());
}

} // namespace

void Transformer<bp::ItemStackRequestCereal_<2193>::RequestData, bp::ItemStackRequestCereal_<2225>::RequestData>::
    transform(Context<bp::ItemStackRequestCereal_<2225>::RequestData> &ctx,
              bp::ItemStackRequestCereal_<2193>::RequestData &&from)
{
    auto &to = ctx.out();
    to.client_request_id = from.client_request_id;
    to.actions.reserve(from.actions.size());
    for (auto &action : from.actions) {
        auto &arm = to.actions.emplace_back();
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            (void)((action.index() == I && (carry<I>(ctx, std::move(action), arm), true)) || ...);
        }(std::make_index_sequence<std::variant_size_v<FromAction>>{});
    }
    to.strings_to_filter = std::move(from.strings_to_filter);
    to.strings_to_filter_origin = from.strings_to_filter_origin;
}

} // namespace endweave
