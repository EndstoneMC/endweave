#include "item_stack.h"

#include <cstddef>
#include <type_traits>
#include <utility>
#include <variant>

namespace endweave {
namespace {

/** Where 2225 put CraftDiscPressActionData, ahead of the two deprecated crafts. */
constexpr std::size_t kCraftDiscPress = 16;

template <std::size_t I, class FromAction, class ToAction>
void carry(FromAction &&from, ToAction &to)
{
    if constexpr (I != kCraftDiscPress) {
        constexpr std::size_t j = I < kCraftDiscPress ? I : I - 1;
        to.template emplace<j>(std::move(std::get<I>(from)));
    }
}

/** Both forms of a request hold the same actions. */
template <class To, class From>
void carryRequest(Context<To> &ctx, From &&from)
{
    using FromAction = typename decltype(from.actions)::value_type;
    static_assert(std::is_same_v<std::variant_alternative_t<kCraftDiscPress, FromAction>,
                                 bp::ItemStackRequestCereal_<2225>::CraftDiscPressActionData>);
    auto &to = ctx.out();
    to.client_request_id = from.client_request_id;
    to.actions.reserve(from.actions.size());
    for (auto &action : from.actions) {
        if (action.index() == kCraftDiscPress) {
            // ENDWEAVE: 2193 has no disc press to craft at, so a request that uses one is dropped.
            ctx.cancel();
            return;
        }
        auto &arm = to.actions.emplace_back();
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            (void)((action.index() == I && (carry<I>(std::move(action), arm), true)) || ...);
        }(std::make_index_sequence<std::variant_size_v<FromAction>>{});
    }
    to.strings_to_filter = std::move(from.strings_to_filter);
    to.strings_to_filter_origin = from.strings_to_filter_origin;
}

} // namespace

void Transformer<bp::ItemStackRequestCereal_<2225>::RequestData, bp::ItemStackRequestCereal_<2193>::RequestData>::
    transform(Context<bp::ItemStackRequestCereal_<2193>::RequestData> &ctx,
              bp::ItemStackRequestCereal_<2225>::RequestData &&from)
{
    carryRequest(ctx, std::move(from));
}

void Transformer<bp::ItemStackRequestPacketData_<2225>::RequestData,
                 bp::ItemStackRequestPacketData_<2193>::RequestData>::
    transform(Context<bp::ItemStackRequestPacketData_<2193>::RequestData> &ctx,
              bp::ItemStackRequestPacketData_<2225>::RequestData &&from)
{
    carryRequest(ctx, std::move(from));
}

} // namespace endweave
