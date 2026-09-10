#include "resource_pack.h"

#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace ew = endweave;

namespace endweave {

void Transformer<bp::ServerboundPackSettingChangePacket_<2192>, bp::ServerboundPackSettingChangePacket_<2168>>::
    transform(Context<bp::ServerboundPackSettingChangePacket_<2168>> &ctx,
              bp::ServerboundPackSettingChangePacket_<2192> &&from)
{
    auto &to = ctx.out();
    to.pack_id = from.pack_id;
    to.pack_setting_name = std::move(from.pack_setting_name);
    // ENDWEAVE: 2192 added a list of strings as a fourth setting shape, and 2168 has no arm to put it
    // in. The setting is the whole of the packet, so a change carrying one is dropped rather than
    // delivered as some other value.
    std::visit(
        [&ctx, &to]<class Alt>(Alt &&value) {
            if constexpr (std::is_same_v<std::remove_cvref_t<Alt>, std::vector<std::string>>) {
                ctx.cancel();
            }
            else {
                to.pack_setting_value = std::forward<Alt>(value);
            }
        },
        std::move(from.pack_setting_value));
}

} // namespace endweave
