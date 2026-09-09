#include "endweave/protocols/v2168_to_v2192/resource_pack.h"

#include <utility>
#include <variant>

namespace endweave {

void Transformer<bp::ServerboundPackSettingChangePacket_<2168>, bp::ServerboundPackSettingChangePacket_<2192>>::
    transform(Context<bp::ServerboundPackSettingChangePacket_<2192>> &ctx,
              bp::ServerboundPackSettingChangePacket_<2168> &&from)
{
    auto &to = ctx.out();
    to.pack_id = from.pack_id;
    to.pack_setting_name = std::move(from.pack_setting_name);
    // ENDWEAVE: 2192 added a list of strings as a fourth setting shape. 2168 can never send one, so
    // the three arms it does have carry across as themselves and the fourth is simply never written.
    std::visit(
        [&to]<class Alt>(Alt &&value) {
            to.pack_setting_value = std::forward<Alt>(value);
        },
        std::move(from.pack_setting_value));
}

} // namespace endweave
